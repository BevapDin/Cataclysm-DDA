#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "json.h"
#include "input.h"
#include "game.h"
#include "options.h"
#include "output.h"
#include "crafting.h"
#include "inventory.h"
#include "item_factory.h"
#include "catacharset.h"
#include <queue>
#include "crafting_inventory_t.h"
#include "helper.h"

std::vector<craft_cat> craft_cat_list;
std::vector<std::string> recipe_names;
recipe_map recipes;
std::map<std::string,quality> qualities;
std::map<std::string, std::queue<std::pair<recipe*, int> > > recipe_booksets;

recipe the_many_recipe;
void multiply(const recipe &in, recipe &r, int factor);

void draw_recipe_tabs(WINDOW *w, craft_cat tab,bool filtered=false);

void load_recipe_category(JsonObject &jsobj)
{
    craft_cat_list.push_back(jsobj.get_string("id"));
}

bool recipe_sorter(const recipe * const &a, const recipe * const &b) {
    if(a->difficulty != b->difficulty) {
        return a->difficulty < b->difficulty;
    }
    /*
    if(a->time != b->time) {
        return a->time < b->time;
    }
    */
    return a->ident < b->ident;
}

void load_recipe(JsonObject &jsobj)
{
    JsonArray jsarr;

    // required
    std::string result = jsobj.get_string("result");
    std::string category = jsobj.get_string("category");
    int difficulty = jsobj.get_int("difficulty");
    int time = jsobj.get_int("time");
    bool autolearn = jsobj.get_bool("autolearn");
    // optional
    bool reversible = jsobj.get_bool("reversible", false);
    std::string skill_used = jsobj.get_string("skill_used", "");
    std::string id_suffix = jsobj.get_string("id_suffix", "");
    int learn_by_disassembly = jsobj.get_int("decomp_learn", -1);

    std::map<std::string, int> requires_skills;
    jsarr = jsobj.get_array("skills_required");
    if (jsarr.size() > 0) {
        // could be a single requirement, or multiple
        try {
            // try to parse as single requirement
            requires_skills[jsarr.get_string(0)] = jsarr.get_int(1);
        } catch (std::string e) {
            // get_string or get_int failed, so assume array of arrays
            while (jsarr.has_more()) {
                JsonArray ja = jsarr.next_array();
                requires_skills[ja.get_string(0)] = ja.get_int(1);
            }
        }
    }

    std::string rec_name = result + id_suffix;

    for (std::vector<std::string>::iterator name_iter = recipe_names.begin();
         name_iter != recipe_names.end();
         ++name_iter)
    {
        if ((*name_iter) == rec_name) {
            throw jsobj.line_number() + ": Recipe name collision (set a unique value for the id_suffix field to fix): " + rec_name;
        }
    }

    recipe_names.push_back(rec_name);
    int id = recipe_names.size();

    recipe* rec = new recipe(rec_name, id, result, category, skill_used,
                             requires_skills, difficulty, time, reversible,
                             autolearn, learn_by_disassembly);


    if(jsobj.has_member("count")) {
        rec->count = jsobj.get_int("count");
    }
    if(jsobj.has_member("count_range")) {
        rec->count_range = jsobj.get_int("count_range");
    }
    if(jsobj.has_member("noise") && jsobj.has_member("noise_string")) {
        rec->noise = jsobj.get_int("noise");
        rec->noise_string = jsobj.get_string("noise_string");
    }


    jsarr = jsobj.get_array("components");
    while (jsarr.has_more()) {
        std::vector<component> component_choices;
        JsonArray ja = jsarr.next_array();
        while (ja.has_more()) {
            JsonArray comp = ja.next_array();
            std::string name = comp.get_string(0);
            int quant = comp.get_int(1);
            component_choices.push_back(component(name, quant));
        }
        rec->components.push_back(component_choices);
    }

    jsarr = jsobj.get_array("qualities");
    while(jsarr.has_more()){
        JsonObject quality_data = jsarr.next_object();
        std::string ident = quality_data.get_string("id");
        int level = quality_data.get_int("level", 1);
        int amount = quality_data.get_int("amount", 1);
        rec->qualities.push_back(quality_requirement(ident, level, amount));
    }
    for(std::vector<quality_requirement>::const_iterator a = rec->qualities.begin(); a != rec->qualities.end(); ++a) {
        std::ostringstream buffer;
        buffer << "func:" << a->id << ":" << a->level;
        if(buffer.str() == "func:CUT:1") { buffer.str(std::string("func:blade")); }
        std::vector<component> tool_choices;
        tool_choices.push_back(component(buffer.str(), -1));
        rec->tools.push_back(tool_choices);
    }
    rec->qualities.clear();

    jsarr = jsobj.get_array("tools");
    while (jsarr.has_more()) {
        std::vector<component> tool_choices;
        JsonArray ja = jsarr.next_array();
        while (ja.has_more()) {
            JsonArray comp = ja.next_array();
            std::string name = comp.get_string(0);
            int quant = comp.get_int(1);
            tool_choices.push_back(component(name, quant));
        }
        rec->tools.push_back(tool_choices);
    }

    jsarr = jsobj.get_array("book_learn");
    while (jsarr.has_more()) {
        JsonArray ja = jsarr.next_array();
        std::string book_name = ja.get_string(0);
        int book_level = ja.get_int(1);
        std::pair<recipe*, int> temp_pair(rec, book_level);
        if (recipe_booksets.find(book_name) == recipe_booksets.end()){
            recipe_booksets[book_name] = std::queue<std::pair<recipe*, int> >();
        }
        recipe_booksets[book_name].push(temp_pair);
    }

    recipes[category].push_back(rec);
    std::sort(recipes[category].begin(), recipes[category].end(), recipe_sorter);
}

void finalize_recipes()
{
    for (std::map<std::string, std::queue<std::pair<recipe*, int> > >::iterator book_ref_it = recipe_booksets.begin();
         book_ref_it != recipe_booksets.end(); ++book_ref_it){
        if (!book_ref_it->second.empty() && item_controller->find_template(book_ref_it->first)->is_book()){
            it_book *book_def = dynamic_cast<it_book*>(item_controller->find_template(book_ref_it->first));
            while (!book_ref_it->second.empty()){
                std::pair<recipe*, int> rec_pair = book_ref_it->second.front();
                book_ref_it->second.pop();
                book_def->recipes[rec_pair.first] = rec_pair.second;
            }
        }
    }
    the_many_recipe.ident = "the_many_recipe";
    recipe_names.push_back(the_many_recipe.ident);
    the_many_recipe.id = recipe_names.size();
    the_many_recipe.autolearn = false;
}

void load_quality(JsonObject &jo)
{
    quality qual;
    qual.id = jo.get_string("id");
    qual.name = _(jo.get_string("name").c_str());
    qualities[qual.id] = qual;
}

bool game::crafting_allowed()
{
    if (u.morale_level() < MIN_MORALE_CRAFT)
    { // See morale.h
        add_msg(_("Your morale is too low to craft..."));
        return false;
    }

    return true;
}

void game::recraft()
{
 if(u.lastrecipe == NULL)
 {
  popup(_("Craft something first"));
 }
 else if (making_would_work(u.lastrecipe))
 {
  make_craft(u.lastrecipe);
 }
}

// See crafting_inventory_t.cpp
extern const std::string &name(const itype_id &type);

//TODO clean up this function to give better status messages (e.g., "no fire available")
bool game::making_would_work(recipe *making)
{
    if (!crafting_allowed()) {
        return false;
    }

    crafting_inventory_t crafting_inv(this, &u);
    if(can_make(making, crafting_inv)) {
        if (item_controller->find_template((making->result))->phase == LIQUID) {
            if (u.has_watertight_container() ||
                u.has_matching_liquid(item_controller->find_template(making->result)->id)) {
                return true;
            }
            else
            {
                popup(_("You don't have anything to store that liquid in!"));
            }
        }
        else
        {
            return true;
        }
    }
    else
    {
        std::ostringstream buffer;
        buffer << _("You can no longer make ") << ::name(making->result) << "\n";
        ::list_missing_ones(buffer, *making);
        popup_top(buffer.str().c_str());
        draw();
    }

    return false;
}
bool game::can_make(recipe *r, crafting_inventory_t &crafting_inv)
{
    bool RET_VAL = true;
    if(!u.knows_recipe(r))
    {
        return false;
    }
    // under the assumption that all comp and tool's array contains
    // all the required stuffs at the start of the array

    // check all tool_quality requirements
    // this is an alternate method of checking for tools by using the tools qualities instead of the specific tool
    // You can specify the amount of tools with this quality required, but it does not work for consumed charges.
    std::vector<quality_requirement> &qualities = r->qualities;
    std::vector<quality_requirement>::iterator quality_iter = qualities.begin();
    while(quality_iter != qualities.end()){
        std::string id = quality_iter->id;
        int amount = quality_iter->count;
        int level = quality_iter->level;
        if(crafting_inv.has_items_with_quality(id, level, amount)){
            quality_iter->available = true;
        } else {
            quality_iter->available = false;
            RET_VAL = false;
        }
        ++quality_iter;
    }

    // check all tools and check all components
    return RET_VAL && crafting_inv.has_all_requirements(*r);
//    return check_enough_materials(r, crafting_inv) && RET_VAL;
}

bool game::check_enough_materials(recipe *r, crafting_inventory_t &crafting_inv)
{
    bool RET_VAL = true;
    std::vector<std::vector<component> > &components = r->components;
    std::vector<std::vector<component> >::iterator comp_set_it = components.begin();
    while (comp_set_it != components.end())
    {
        std::vector<component> &set_of_components = *comp_set_it;
        std::vector<component>::iterator comp_it = set_of_components.begin();
        bool atleast_one_available = false;
        while (comp_it != set_of_components.end())
        {
            component &comp = *comp_it;
            if (comp.available == 1)
            {
                bool have_enough_in_set = true;
                std::vector<std::vector<component> > &tools = r->tools;
                std::vector<std::vector<component> >::iterator tool_set_it = tools.begin();
                while (tool_set_it != tools.end())
                {
                    bool have_enough = false;
                    std::vector<component> &set_of_tools = *tool_set_it;
                    std::vector<component>::iterator tool_it = set_of_tools.begin();
                    while(tool_it != set_of_tools.end())
                    {
                        component &tool = *tool_it;
                        if (tool.available == 1)
                        {
                            if (comp.type == tool.type)
                            {
                                bool count_by_charges = item_controller->find_template(comp.type)->count_by_charges();
                                if (count_by_charges)
                                {
                                    int req = comp.count;
                                    if (tool.count > 0)
                                    {
                                        req += tool.count;
                                    } else
                                    {
                                        ++req;
                                    }
                                    if (crafting_inv.has_charges(comp.type, req))
                                    {
                                        have_enough = true;
                                    }
                                } else {
                                    int req = comp.count + 1;
                                    if (crafting_inv.has_amount(comp.type, req))
                                    {
                                        have_enough = true;
                                    }
                                }
                            }
                            else
                            {
                                have_enough = true;
                            }
                        }
                        ++tool_it;
                    }
                    have_enough_in_set = have_enough_in_set && have_enough;
                    ++tool_set_it;
                }
                if (!have_enough_in_set)
                // This component can't be used with any tools from one of the sets of tools
                // which means it's availability should be set to 0 (in inventory, but
                // not enough for both tool and components).
                {
                    comp.available = 0;
                }
            }
            //Flag that at least one of the components in the set is available
            if (comp.available == 1)
            {
                atleast_one_available = true;
            }
            ++comp_it;
        }

        if (!atleast_one_available)
        // this set doesn't have any components available, so the recipe can't be crafted
        {
            RET_VAL = false;
        }
        ++comp_set_it;
    }


    std::vector<std::vector<component> > &tools = r->tools;
    std::vector<std::vector<component> >::iterator tool_set_it = tools.begin();
    while (tool_set_it != tools.end())
    {
        std::vector<component> &set_of_tools = *tool_set_it;
        std::vector<component>::iterator tool_it = set_of_tools.begin();
        bool atleast_one_available = false;
        while (tool_it != set_of_tools.end())
        {
            component &tool = *tool_it;
            if (tool.available == 1)
            {
                bool have_enough_in_set = true;
                std::vector<std::vector<component> > &components = r->components;
                std::vector<std::vector<component> >::iterator comp_set_it = components.begin();
                while (comp_set_it != components.end())
                {
                    bool have_enough = false, conflict = false;
                    std::vector<component> &set_of_components = *comp_set_it;
                    std::vector<component>::iterator comp_it = set_of_components.begin();
                    while(comp_it != set_of_components.end())
                    {
                        component &comp = *comp_it;
                        if (tool.type == comp.type)
                        {
                            if (tool.count > 0)
                            {
                                int req = comp.count + tool.count;
                                if (!crafting_inv.has_charges(comp.type, req))
                                {
                                    conflict = true;
                                    have_enough = have_enough || false;
                                }
                            } else
                            {
                                int req = comp.count + 1;
                                if (!crafting_inv.has_amount(comp.type, req))
                                {
                                    conflict = true;
                                    have_enough = have_enough || false;
                                }
                            }
                        } else if (comp.available == 1)
                        {
                            have_enough = true;
                        }
                        ++comp_it;
                    }
                    if (conflict)
                    {
                        have_enough_in_set = have_enough_in_set && have_enough;
                    }
                    ++comp_set_it;
                }
                if (!have_enough_in_set)
                    // This tool can't be used with any components from one of the sets of components
                    // which means it's availability should be set to 0 (in inventory, but
                    // not enough for both tool and components).
                {
                    tool.available = 0;
                }
            }
            //Flag that at least one of the tools in the set is available
            if (tool.available == 1)
            {
                atleast_one_available = true;
            }
            ++tool_it;
        }

        if (!atleast_one_available)
            // this set doesn't have any tools available, so the recipe can't be crafted
        {
            RET_VAL = false;
        }
        ++tool_set_it;
    }

    return RET_VAL;
}

void game::craft()
{
    recipe *rec = select_crafting_recipe();
    if (!crafting_allowed())
    {
        return;
    }
    if (rec)
    {
        make_craft(rec);
    }
}

void game::long_craft()
{
    if (!crafting_allowed())
    {
        return;
    }

    recipe *rec = select_crafting_recipe();
    if (rec)
    {
        make_all_craft(rec);
    }
}

craft_cat game::next_craft_cat(craft_cat cat)
{
    for (std::vector<craft_cat>::iterator iter = craft_cat_list.begin();
         iter != craft_cat_list.end();
         ++iter)
    {
        if ((*iter) == cat)
        {
            return *(++iter);
        }
    }
    return NULL;
}

craft_cat game::prev_craft_cat(craft_cat cat)
{
    for (std::vector<craft_cat>::iterator iter = craft_cat_list.begin();
         iter != craft_cat_list.end();
         ++iter)
    {
        if ((*iter) == cat)
        {
            return *(--iter);
        }
    }
    return NULL;
}

recipe* game::select_crafting_recipe()
{
    const int headHeight = 3;
    const int tailHeight = 4;
    const int dataLines = TERMY - headHeight - tailHeight;
    const int dataHalfLines = dataLines / 2;
    const int dataHeight = TERMY - headHeight;

    WINDOW *w_head = newwin(headHeight, FULL_SCREEN_WIDTH, 0, (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0);
    WINDOW *w_data = newwin(dataHeight, FULL_SCREEN_WIDTH, headHeight, (TERMX  > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0);


    craft_cat tab = "CC_WEAPON";
    std::vector<recipe*> current;
    std::vector<bool> available;
    item tmp;
    int line = 0, xpos, ypos;
    bool redraw = true;
    bool keepline = false;
    bool done = false;
    recipe *chosen = NULL;
    InputEvent input;

    crafting_inventory_t crafting_inv(this, &u);
    std::string filterstring = "";
    do {
        if (redraw)
        { // When we switch tabs, redraw the header
            redraw = false;
            if ( ! keepline ) {
                line = 0;
            } else {
                keepline = false;
            }
            draw_recipe_tabs(w_head, tab, (filterstring == "")?false:true);
            current.clear();
            available.clear();
            // Set current to all recipes in the current tab; available are possible to make
            pick_recipes(current, available, tab,filterstring);
        }

        // Clear the screen of recipe data, and draw it anew
        werase(w_data);
        if (filterstring != "") {
            mvwprintz(w_data, dataLines+1, 5, c_white, _("[?/E]: Describe, [F]ind , [R]eset"));
        } else {
            mvwprintz(w_data, dataLines+1, 5, c_white, _("[?/E]: Describe, [F]ind"));
        }
        mvwprintz(w_data, dataLines+2, 5, c_white, _("Press <ENTER> to attempt to craft object."));

        // Draw borders
        for (int i = 1; i < FULL_SCREEN_WIDTH-1; ++i) { // _
            mvwputch(w_data, dataHeight-1, i, c_ltgray, LINE_OXOX);
        }
        for (int i = 1; i < dataHeight-1; ++i) { // |
            mvwputch(w_data, i, 0, c_ltgray, LINE_XOXO);
            mvwputch(w_data, i, FULL_SCREEN_WIDTH-1, c_ltgray, LINE_XOXO);
        }
        mvwputch(w_data, dataHeight-1,  0, c_ltgray, LINE_XXOO); // _|
        mvwputch(w_data, dataHeight-1, FULL_SCREEN_WIDTH-1, c_ltgray, LINE_XOOX); // |_

        int recmin = 0, recmax = current.size();
        if (recmax > dataLines) {
            if (line <= recmin + dataHalfLines) {
                for (int i = recmin; i < recmin + dataLines; ++i) {
                    mvwprintz(w_data, i - recmin, 2, c_dkgray, ""); // Clear the line
                    if (i == line) {
                        mvwprintz(w_data, i - recmin, 2, (available[i] ? h_white : h_dkgray),
                        item_controller->find_template(current[i]->result)->name.c_str());
                    } else {
                        mvwprintz(w_data, i - recmin, 2, (available[i] ? c_white : c_dkgray),
                        item_controller->find_template(current[i]->result)->name.c_str());
                    }
                }
            } else if (line >= recmax - dataHalfLines) {
                for (int i = recmax - dataLines; i < recmax; ++i) {
                    mvwprintz(w_data, dataLines + i - recmax, 2, c_ltgray, ""); // Clear the line
                    if (i == line) {
                        mvwprintz(w_data, dataLines + i - recmax, 2, (available[i] ? h_white : h_dkgray),
                        item_controller->find_template(current[i]->result)->name.c_str());
                    } else {
                        mvwprintz(w_data, dataLines + i - recmax, 2, (available[i] ? c_white : c_dkgray),
                        item_controller->find_template(current[i]->result)->name.c_str());
                    }
                }
            } else {
                for (int i = line - dataHalfLines; i < line - dataHalfLines + dataLines; ++i) {
                    mvwprintz(w_data, dataHalfLines + i - line, 2, c_ltgray, ""); // Clear the line
                    if (i == line) {
                        mvwprintz(w_data, dataHalfLines + i - line, 2, (available[i] ? h_white : h_dkgray),
                        item_controller->find_template(current[i]->result)->name.c_str());
                    } else {
                        mvwprintz(w_data, dataHalfLines + i - line, 2, (available[i] ? c_white : c_dkgray),
                        item_controller->find_template(current[i]->result)->name.c_str());
                    }
                }
            }
        } else {
            for (int i = 0; i < current.size() && i < dataHeight+1; ++i) {
                if (i == line) {
                    mvwprintz(w_data, i, 2, (available[i] ? h_white : h_dkgray),
                    item_controller->find_template(current[i]->result)->name.c_str());
                } else {
                    mvwprintz(w_data, i, 2, (available[i] ? c_white : c_dkgray),
                    item_controller->find_template(current[i]->result)->name.c_str());
                }
            }
        }
        if (!current.empty())
        {
            nc_color col = (available[line] ? c_white : c_dkgray);
            mvwprintz(w_data, 0, 30, col, _("Skills used: %s"),
                (current[line]->skill_used == NULL ? "N/A" :
                current[line]->skill_used->name().c_str()));

            mvwprintz(w_data, 1, 30, col, _("Required skills: %s"),
                (current[line]->required_skills_string().c_str()));
            mvwprintz(w_data, 2, 30, col, _("Difficulty: %d"), current[line]->difficulty);
            if (current[line]->skill_used == NULL)
            {
                mvwprintz(w_data, 3, 30, col, _("Your skill level: N/A"));
            }
            else
            {
                mvwprintz(w_data, 3, 30, col, _("Your skill level: %d"),
                // Macs don't seem to like passing this as a class, so force it to int
                (int)u.skillLevel(current[line]->skill_used));
            }
            if (current[line]->time >= 60 * 1000)
            {
                if (current[line]->time % 60000 == 0) {
                    mvwprintz(w_data, 4, 30, col, _("Time to complete: %d hours"),
                    int(current[line]->time / 60000));
                } else {
                    mvwprintz(w_data, 4, 30, col, _("Time to complete: %d hours %d minutes"),
                    int(current[line]->time / 60000), int((current[line]->time % 60000) / 1000));
                }
            }
            else if (current[line]->time >= 1000)
            {
                mvwprintz(w_data, 4, 30, col, _("Time to complete: %d minutes"),
                int(current[line]->time / 1000));
            }
            else
            {
                mvwprintz(w_data, 4, 30, col, _("Time to complete: %d turns"),
                int(current[line]->time / 100));
            }
            mvwprintz(w_data, 5, 30, col, _("Tools required:"));
            if (current[line]->tools.size() == 0 && current[line]->qualities.size() == 0)
            {
                mvwputch(w_data, 6, 30, col, '>');
                mvwprintz(w_data, 6, 32, c_green, _("NONE"));
                ypos = 6;
            }
            else
            {
                ypos = 6;
                // Loop to print the required tool qualities
                for(std::vector<quality_requirement>::const_iterator iter = current[line]->qualities.begin();
                        iter != current[line]->qualities.end(); ++iter){
                    xpos = 32;
                    mvwputch(w_data, ypos, 30, col, '>');
                    nc_color toolcol = c_red;
                    if(iter->available){
                        toolcol = c_green;
                    }

                    std::stringstream qualinfo;
                    qualinfo << string_format(_("Requires %d tools with %s of %d or more."), iter->count, qualities[iter->id].name.c_str(), iter->level);
                    ypos += fold_and_print(w_data, ypos, xpos, getmaxx(w_data)-xpos-1, toolcol, qualinfo.str().c_str());
                }
                ypos--;
                // Loop to print the required tools
                for (int i = 0; i < current[line]->tools.size() && current[line]->tools[i].size() > 0; i++)
                {
                    ypos++;
                    xpos = 32;
                    mvwputch(w_data, ypos, 30, col, '>');
                    for (int j = 0; j < current[line]->tools[i].size(); j++)
                    {
                        itype_id type = current[line]->tools[i][j].type;
                        int charges = current[line]->tools[i][j].count;
                        nc_color toolcol = c_red;

                        if (current[line]->tools[i][j].available == 0)
                        {
                            toolcol = c_brown;
                        }
                        else if (charges < 0 && crafting_inv.has_amount(type, 1))
                        {
                            toolcol = c_green;
                        }
                        else if (charges > 0 && crafting_inv.has_charges(type, charges))
                        {
                            toolcol = c_green;
                        }

                        std::stringstream toolinfo;
                        toolinfo << item_controller->find_template(type)->name << " ";

                        if (charges > 0)
                        {
                            toolinfo << string_format(_("(%d charges) "), charges);
                        }
                        std::string toolname = toolinfo.str();
                        if (xpos + utf8_width(toolname.c_str()) >= FULL_SCREEN_WIDTH)
                        {
                            xpos = 32;
                            ypos++;
                        }
                        mvwprintz(w_data, ypos, xpos, toolcol, toolname.c_str());
                        xpos += utf8_width(toolname.c_str());
                        if (j < current[line]->tools[i].size() - 1)
                        {
                            if (xpos >= FULL_SCREEN_WIDTH-3)
                            {
                            xpos = 32;
                            ypos++;
                            }
                            mvwprintz(w_data, ypos, xpos, c_white, _("OR "));
                            xpos += 3;
                        }
                    }
                }
            }
        // Loop to print the required components
            ypos++;
            mvwprintz(w_data, ypos, 30, col, _("Components required:"));
            for (int i = 0; i < current[line]->components.size(); i++)
            {
                if (current[line]->components[i].size() > 0)
                {
                    ypos++;
                    mvwputch(w_data, ypos, 30, col, '>');
                }
                xpos = 32;
                for (int j = 0; j < current[line]->components[i].size(); j++)
                {
                    int count = current[line]->components[i][j].count;
                    itype_id type = current[line]->components[i][j].type;
                    nc_color compcol = c_red;
                    if (current[line]->components[i][j].available == 0)
                    {
                        compcol = c_brown;
                    }
                    else if (item_controller->find_template(type)->count_by_charges() && count > 0)
                    {
                        if (crafting_inv.has_charges(type, count))
                        {
                            compcol = c_green;
                        }
                    }
                    else if (crafting_inv.has_amount(type, abs(count)))
                    {
                        compcol = c_green;
                    }
                    std::stringstream dump;
                    dump << abs(count) << "x " << item_controller->find_template(type)->name << " ";
                    std::string compname = dump.str();
                    if (xpos + utf8_width(compname.c_str()) >= FULL_SCREEN_WIDTH)
                    {
                        ypos++;
                        xpos = 32;
                    }
                    mvwprintz(w_data, ypos, xpos, compcol, compname.c_str());
                    xpos += utf8_width(compname.c_str());
                    if (j < current[line]->components[i].size() - 1)
                    {
                        if (xpos >= FULL_SCREEN_WIDTH-3)
                        {
                            ypos++;
                            xpos = 32;
                        }
                        mvwprintz(w_data, ypos, xpos, c_white, _("OR "));
                        xpos += 3;
                    }
                }
            }
        }

        //Draw Scrollbar
        draw_scrollbar(w_data, line, dataLines, recmax, 0);

        wrefresh(w_data);
        int ch=(int)getch();
        if(ch=='e'||ch=='E') { ch=(int)'?'; } // get_input is inflexible
        if(ch=='d'||ch=='D') { input = (InputEvent) 'd'; } else
        if(ch == KEY_NPAGE || ch == KEY_PPAGE) { input = (InputEvent) ch; } else
        if(ch == 'N') { input = (InputEvent) ch; } else
        input = get_input(ch);
        switch (input)
        {
            case KEY_NPAGE:
                line += dataLines;
                break;
            case KEY_PPAGE:
                line -= dataLines;
                break;
            case DirectionW:
            case DirectionUp:
                if (tab == "CC_WEAPON")
                {
                    tab = "CC_MISC";
                }
                else
                {
                    tab = prev_craft_cat(tab);
                }
                redraw = true;
                break;
            case DirectionE:
            case DirectionDown:
                if (tab == "CC_MISC")
                {
                    tab = "CC_WEAPON";
                }
                else
                {
                    tab = next_craft_cat(tab);
                }
                redraw = true;
                break;
            case DirectionS:
                line++;
                break;
            case DirectionN:
                line--;
                break;
            case 'd':
                {
                    crafting_inventory_t::solution s;
                    crafting_inv.has_all_requirements(*(current[line]), s);
                    
                    {
                        std::string msg;
                        msg += _("requirements for ");
                        msg += current[line]->result;
                        msg += ":\n";
                        msg += s.to_string(crafting_inventory_t::simple_req::ts_overlays | /*crafting_inventory_t::simple_req::ts_compress | */crafting_inventory_t::simple_req::ts_found_items);
                        popup_top(msg.c_str());
                    }
                    draw();
                }
                break;
            case 'N':
            case Confirm:
                if (!available[line])
                {
                    popup(_("You can't do that!"));
                }
                else
                {// is player making a liquid? Then need to check for valid container
                    if (item_controller->find_template(current[line]->result)->phase == LIQUID)
                    {
                        if (u.has_watertight_container() || u.has_matching_liquid(item_controller->find_template(current[line]->result)->id))
                        {
                            chosen = current[line];
                            done = true;
                        }
                        else
                        {
                            popup(_("You don't have anything to store that liquid in!"));
                        }
                    }
                    else
                    {
                        chosen = current[line];
                        done = true;
                    }
                }
                if(done && input == 'N') {
                    // fixme / todo make popup take numbers only (m = accept, q = cancel)
                    int amount = helper::to_int(
                        string_input_popup("craft how much?", 20, "1")
                    );
                    if(amount <= 0) {
                        done = false;
                        chosen = NULL;
                    } else {
                        multiply(*chosen, the_many_recipe, amount);
                        if(!crafting_inv.has_all_requirements(the_many_recipe)) {
                            std::ostringstream buffer;
                            ::list_missing_ones(buffer, the_many_recipe);
                            popup_top(buffer.str().c_str());
                            done = false;
                            chosen = NULL;
                        } else {
                            chosen = &the_many_recipe;
                            input = Confirm;
                        }
                    }
                }
                break;
            case Help:
                tmp = item(item_controller->find_template(current[line]->result), 0);
                full_screen_popup("%s", tmp.info(true).c_str());
                redraw = true;
                keepline = true;
                break;
            case Filter:
                filterstring = string_input_popup(_("Search:"), 55, filterstring);
                redraw = true;
                break;
            case Reset:
                filterstring = "";
                redraw = true;
                break;

        }
        if (line < 0)
        {
            line = current.size() - 1;
        }
        else if (line >= current.size())
        {
            line = 0;
        }
    } while (input != Cancel && !done);

    werase(w_head);
    werase(w_data);
    delwin(w_head);
    delwin(w_data);
    refresh_all();

    return chosen;
}

void draw_recipe_tabs(WINDOW *w, craft_cat tab,bool filtered)
{
    werase(w);
    for (int i = 0; i < FULL_SCREEN_WIDTH; i++)
    {
        mvwputch(w, 2, i, c_ltgray, LINE_OXOX);
    }

    mvwputch(w, 2,  0, c_ltgray, LINE_OXXO); // |^
    mvwputch(w, 2, 79, c_ltgray, LINE_OOXX); // ^|
    if(!filtered)
    {
        draw_tab(w,  2, _("WEAPONS"), (tab == "CC_WEAPON") ? true : false);
        draw_tab(w, 13, _("AMMO"),    (tab == "CC_AMMO")   ? true : false);
        draw_tab(w, 21, _("FOOD"),    (tab == "CC_FOOD")   ? true : false);
        draw_tab(w, 29, _("DRINKS"),  (tab == "CC_DRINK")  ? true : false);
        draw_tab(w, 39, _("CHEMS"),   (tab == "CC_CHEM")   ? true : false);
        draw_tab(w, 48, _("ELECTRONICS"), (tab == "CC_ELECTRONIC") ? true : false);
        draw_tab(w, 63, _("ARMOR"),   (tab == "CC_ARMOR")  ? true : false);
        draw_tab(w, 72, _("MISC"),    (tab == "CC_MISC")   ? true : false);
    }
    else
    {
        draw_tab(w,  2, _("Searched"), true);
    }

    wrefresh(w);
}

void game::pick_recipes(std::vector<recipe*> &current,
                        std::vector<bool> &available, craft_cat tab,std::string filter)
{
    current.clear();
    available.clear();

	crafting_inventory_t crafting_inv(this, &u);

    if (filter == "")
    {
        add_known_recipes(current, recipes[tab], crafting_inv);
    }
    else
    {
        for (recipe_map::iterator iter = recipes.begin(); iter != recipes.end(); ++iter)
        {
            add_known_recipes(current, iter->second, crafting_inv, filter);
        }
    }

    for (int i = 0; i < current.size(); i++)
    {
        //Check if we have the requisite tools and components
        if(can_make(current[i], crafting_inv))
        {
            available.push_back(true);
        }
        else
        {
            available.push_back(false);
        }
    }
}

void game::add_known_recipes(std::vector<recipe*> &current, recipe_list source, crafting_inventory_t &crafting_inv, std::string filter)
{
    std::vector<recipe*> can_craft;
    for (recipe_list::iterator iter = source.begin(); iter != source.end(); ++iter)
    {
        if (u.knows_recipe(*iter))
        {
            if ((*iter)->difficulty >= 0 )
            {
                if (filter == "" || item_controller->find_template((*iter)->result)->name.find(filter) != std::string::npos)
                {
                    if (can_make(*iter, crafting_inv))
                    {
                        can_craft.push_back(*iter);
                    }
                    else
                    {
                        current.push_back(*iter);
                    }
                }
            }
        }
    }
    current.insert(current.begin(),can_craft.begin(),can_craft.end());
}

void pop_recipe_to_top(recipe *r);
void move_ppoints_for_construction(const std::string &skillName, int difficulty, int &moves_left);

void game::make_craft(recipe *making)
{
 u.assign_activity(this, ACT_CRAFT, making->time, making->id);
 if(making->skill_used != 0) {
  move_ppoints_for_construction(making->skill_used->ident(), making->difficulty, u.activity.moves_left);
 }
 crafting_inventory_t craft_inv(g, &g->u);
 craft_inv.gather_input(*making, u.activity);
 u.moves = 0;
 u.lastrecipe = making;
 pop_recipe_to_top(making);
}


void game::make_all_craft(recipe *making)
{
 u.assign_activity(this, ACT_LONGCRAFT, making->time, making->id);
 if(making->skill_used != 0) {
  move_ppoints_for_construction(making->skill_used->ident(), making->difficulty, u.activity.moves_left);
 }
 crafting_inventory_t craft_inv(g, &g->u);
 craft_inv.gather_input(*making, u.activity);
 u.moves = 0;
 u.lastrecipe = making;
}

void serialize_item_list(picojson::array &arr, const std::list<item> &x) {
    for(std::list<item>::const_iterator a = x.begin(); a != x.end(); ++a) {
        arr.push_back(a->json_save(true));
    }
}

std::string to_uncraft_tag(const std::list<item> &comps, const std::list<item> &tools) {
    std::ostringstream buffer;
    picojson::array carr;
    picojson::array tarr;
    serialize_item_list(carr, comps);
    serialize_item_list(tarr, tools);
    picojson::array both;
    both.push_back(picojson::value(carr));
    both.push_back(picojson::value(tarr));
    return std::string("UNCRAFT:") + picojson::value(both).serialize();
}

bool deserialize_item_list(const picojson::value &arr, std::list<item> &x) {
    if(!arr.is<picojson::array>()) {
        return false;
    }
    item it;
    for(int i = 0; arr.contains(i); i++) {
        if(it.json_load(const_cast<picojson::value&>(arr.get(i)), g)) {
            x.push_back(it);
        }
    }
    return true;
}

bool from_uncraft_tag(const std::string &data, std::list<item> &comps, std::list<item> &tools) {
    if(data.length() < 10 || data.compare(0, 8, "UNCRAFT:") != 0) {
        return false;
    }
    picojson::value pjv;
    const char *s = data.c_str() + 8;
    std::string err = picojson::parse(pjv, s, s + data.length());
    if(!err.empty()) {
        return false;
    } else if(!pjv.is<picojson::array>() || !pjv.contains(1)) {
        return false;
    }
    if(!deserialize_item_list(pjv.get(0), comps)) { return false; }
    if(!deserialize_item_list(pjv.get(1), tools)) { return false; }
    return true;
}

void game::complete_craft()
{
 recipe* making = recipe_by_index(u.activity.index); // Which recipe is it?

 craft_count[making->ident]++;

 if(making->noise > 0) {
  sound(u.posx, u.posy, making->noise, making->noise_string);
 }



// # of dice is 75% primary skill, 25% secondary (unless secondary is null)
 int skill_dice = u.skillLevel(making->skill_used) * 4;

// farsightedness can impose a penalty on electronics and tailoring success
// it's equivalent to a 2-rank electronics penalty, 1-rank tailoring
 if (u.has_trait("HYPEROPIC") && !u.is_wearing("glasses_reading")
     && !u.is_wearing("glasses_bifocal")) {
  int main_rank_penalty = 0;
  if (making->skill_used == Skill::skill("electronics")) {
   main_rank_penalty = 2;
  } else if (making->skill_used == Skill::skill("tailoring")) {
   main_rank_penalty = 1;
  }
  skill_dice -= main_rank_penalty * 4;
 }

// Sides on dice is 16 plus your current intelligence
 int skill_sides = 16 + u.int_cur;

 int diff_dice = making->difficulty * 4; // Since skill level is * 4 also
 int diff_sides = 24; // 16 + 8 (default intelligence)

 int skill_roll = dice(skill_dice, skill_sides);
 int diff_roll  = dice(diff_dice,  diff_sides);

 if (making->skill_used) {
  if(u.skillLevel(making->skill_used) <= 1 || u.skillLevel(making->skill_used) < making->difficulty * 2) {
   u.practice(turn, making->skill_used, making->difficulty * 5 + 20);
  }
 }


 crafting_inventory_t crafting_inv(this, &u);



// Messed up badly; waste some components.
 if (making->difficulty != 0 && diff_roll > skill_roll * (1 + 0.1 * rng(1, 5))) {
  add_msg(_("You fail to make the %s, and waste some materials."),
          item_controller->find_template(making->result)->name.c_str());
  std::list<item> used;
  std::list<item> used_tools;
  crafting_inv.consume_gathered(*making, u.activity, used, used_tools);
  u.activity.type = ACT_NULL;
  return;
  // Messed up slightly; no components wasted.
 } else if (diff_roll > skill_roll) {
  add_msg(_("You fail to make the %s, but don't waste any materials."),
          item_controller->find_template(making->result)->name.c_str());
  //this method would only have been called from a place that nulls u.activity.type,
  //so it appears that it's safe to NOT null that variable here.
  //rationale: this allows certain contexts (e.g. ACT_LONGCRAFT) to distinguish major and minor failures
  return;
 }
// If we're here, the craft was a success!
// Use up the components and tools
// std::list<item> used = crafting_inv.consume_items(making->components);
// crafting_inv.consume_tools(making->tools, false);
 std::list<item> used;
 std::list<item> used_tools;
 crafting_inv.consume_gathered(*making, u.activity, used, used_tools);

 item newit(item_controller->find_template(making->result), turn);
 newit.item_tags.insert(to_uncraft_tag(used, used_tools));
 int new_count = 1;
 if(making->count > 0) {
   new_count = making->count;
   if(making->count_range > 0) {
     new_count += rng(0, making->count_range);
   }
 }

    if (newit.is_armor() && newit.has_flag("VARSIZE"))
    {
        newit.item_tags.insert("FIT");
    }
    float used_age_tally = 0;
    int used_age_count = 0;
    for (std::list<item>::iterator iter = used.begin(); iter != used.end(); ++iter)
    {
        if (iter->goes_bad())
        {
            used_age_tally += ((int)turn - iter->bday)/
                (float)(dynamic_cast<it_comest*>(iter->type)->spoils);
            ++used_age_count;
        }
    }
    if (used_age_count > 0 && newit.goes_bad())
    {
        const int average_used_age = int((used_age_tally / used_age_count) * dynamic_cast<it_comest*>(newit.type)->spoils);
        newit.bday = newit.bday - average_used_age;
    }
 // for food items
 if (newit.is_food())
  {
    int bday_tmp = newit.bday % 3600; // fuzzy birthday for stacking reasons
    newit.bday = int(newit.bday) + 3600 - bday_tmp;

        if (newit.has_flag("EATEN_HOT")) { // hot foods generated
            newit.item_tags.insert("HOT");
            newit.active = true;
            newit.item_counter = 600;
        }
  }
 if (!newit.craft_has_charges())
  newit.charges = 0;
 //newit = newit.in_its_container(&itypes);
 if (newit.made_of(LIQUID)) {
  newit.charges *= new_count;
  while(!handle_liquid(newit, false, false)) { ; }
 } else {
  if(newit.count_by_charges()) {
    newit.charges *= new_count;
    new_count = 1;
  }
  u.add_or_drop(newit, this, new_count);
  g->add_msg("%s", newit.tname(g).c_str());
 }
}

void game::disassemble(char ch)
{
    if (!ch)
    {
        ch = inv(_("Disassemble item:"));
    }
    if (ch == 27)
    {
        add_msg(_("Never mind."));
        return;
    }
    if (!u.has_item(ch))
    {
        add_msg(_("You don't have item '%c'!"), ch);
        return;
    }

    item* dis_item = &u.i_at(ch);
	crafting_inventory_t crafting_inv(this, &u);


    for (recipe_map::iterator cat_iter = recipes.begin(); cat_iter != recipes.end(); ++cat_iter)
    {
        for (recipe_list::iterator list_iter = cat_iter->second.begin();
             list_iter != cat_iter->second.end();
             ++list_iter)
        {
            recipe* cur_recipe = *list_iter;
            if (dis_item->type == item_controller->find_template(cur_recipe->result) && cur_recipe->reversible && cur_recipe->count <= 1 && cur_recipe->count_range <= 0)
            // ok, a valid recipe exists for the item, and it is reversible
            // assign the activity
            {
                // check tools are available
                // loop over the tools and see what's required...again
                bool have_all_tools = true;
                for (int j = 0; j < cur_recipe->tools.size(); j++)
                {
                    if (cur_recipe->tools[j].size() == 0) // no tools required, may change this
                    {
                        continue;
                    }
                    bool have_this_tool = false;
                    for (int k = 0; k < cur_recipe->tools[j].size(); k++)
                    {
                        itype_id type = cur_recipe->tools[j][k].type;
                        int req = cur_recipe->tools[j][k].count;	// -1 => 1

                        // if crafting recipe required a welder, disassembly requires a hacksaw or super toolkit
                        if (type == "welder" || type == "func:welder")
                        {
                            if (crafting_inv.has_amount("func:hacksaw", 1))
                            {
                                have_this_tool = true;
                            }
                            else
                            {
                                add_msg(_("You need a hacksaw to disassemble this."));
                            }
                        }
                        else if (type == "sewing_kit" || type == "func:sewing_kit")
                        {
                            if (crafting_inv.has_amount("scissors", 1) ||
                                crafting_inv.has_amount("func:blade", 1))
                            {
                                have_this_tool = true;
                            }
                            else
                            {
                                add_msg("You need scissors or somthing with a blade to disassemble this.");
                            }
                        }
                        else if ((req <= 0 && crafting_inv.has_amount (type, 1)) ||
                                 (req >  0 && crafting_inv.has_charges(type, req)))
                        {
                            have_this_tool = true;
                            k = cur_recipe->tools[j].size();
                        } else {
                            add_msg(_("You need a %s with %d charges to disassemble this."),
                            item_controller->find_template(type)->name.c_str(), req);
                            have_this_tool = false;
                        }
                        if(have_this_tool) {
                            break;
                        }
                            
                    }
                    if (!have_this_tool)
                    {
                        have_all_tools = false;
                    }
                }
                // all tools present, so assign the activity
                if (have_all_tools)
                {
                  // check to see if it's even possible to disassemble if it happens to be a count_by_charge item
                  // (num_charges / charges_required) > 0
                  // done before query because it doesn't make sense to query and then say "woops, can't do that!"
                  if (dis_item->count_by_charges()){
                    // required number of item in inventory for disassembly to succeed
                    int num_disassemblies_available = dis_item->charges / dis_item->type->stack_size;;

                    if (num_disassemblies_available == 0){
                      add_msg(_("You cannot disassemble the %s into its components, too few items."), dis_item->name.c_str());
                      return;
                    }
                  }
                  if (OPTIONS["QUERY_DISASSEMBLE"] && !(query_yn(_("Really disassemble your %s?"), dis_item->tname(this).c_str())))
                  {
                   return;
                  }
                    u.assign_activity(this, ACT_DISASSEMBLE, cur_recipe->time / 2, cur_recipe->id);
                    u.moves = 0;
                    std::vector<int> dis_items;
                    dis_items.push_back(ch);
                    u.activity.values = dis_items;
                }
                return; // recipe exists, but no tools, so do not start disassembly
            }
        }
    }

    //if we're trying to disassemble a book or magazine
    if(dis_item->is_book())
    {
       if (OPTIONS["QUERY_DISASSEMBLE"] && !(query_yn(_("Do you want to tear %s into pages?"), dis_item->tname(this).c_str())))
             return;
        else
        {
            //twice the volume then multiplied by 10 (a book with volume 3 will give 60 pages)
            int num_pages = (dis_item->volume() *2) * 10;
            m.spawn_item(u.posx, u.posy, "paper", 0, num_pages);
            u.inv.remove_item(dis_item);
        }
        return;
    }

    // no recipe exists, or the item cannot be disassembled
    add_msg(_("This item cannot be disassembled!"));
}

void game::complete_disassemble()
{
  // which recipe was it?
  recipe* dis = recipe_by_index(u.activity.index); // Which recipe is it?
  item* dis_item = &u.i_at(u.activity.values[0]);

  add_msg(_("You disassemble the %s into its components."), dis_item->name.c_str());
  // remove any batteries or ammo first
    if (dis_item->is_gun() && dis_item->curammo != NULL && dis_item->ammo_type() != "NULL")
    {
      item ammodrop;
      ammodrop = item(dis_item->curammo, turn);
      ammodrop.charges = dis_item->charges;
      if (ammodrop.made_of(LIQUID))
        while(!handle_liquid(ammodrop, false, false)) { ; }
      else
        m.add_item_or_charges(u.posx, u.posy, ammodrop);
    }
    if (dis_item->is_tool() && dis_item->charges > 0 && dis_item->ammo_type() != "NULL")
    {
      item ammodrop;
      ammodrop = item(item_controller->find_template(default_ammo(dis_item->ammo_type())), turn);
      ammodrop.charges = dis_item->charges;
      if (dis_item->typeId() == "adv_UPS_off" || dis_item->typeId() == "adv_UPS_on") {
          ammodrop.charges /= 500;
      }
      if (ammodrop.made_of(LIQUID))
        while(!handle_liquid(ammodrop, false, false)) { ; }
      else
        m.add_item_or_charges(u.posx, u.posy, ammodrop);
    }

    if (dis_item->count_by_charges()){
        dis_item->charges -= dis_item->type->stack_size;
        if (dis_item->charges == 0){
            u.i_rem(u.activity.values[0]);
        }
    }else{
        u.i_rem(u.activity.values[0]);  // remove the item
    }

	crafting_inventory_t crafting_inv(this, &u);
  // consume tool charges
  for (int j = 0; j < dis->tools.size(); j++)
  {
    if (dis->tools[j].size() > 0)
    crafting_inv.consume_any_tools(dis->tools[j], false);
  }

  // add the components to the map
  // Player skills should determine how many components are returned

  int skill_dice = 2 + u.skillLevel(dis->skill_used) * 3;
   skill_dice += u.skillLevel(dis->skill_used);

  // Sides on dice is 16 plus your current intelligence
   int skill_sides = 16 + u.int_cur;

   int diff_dice = dis->difficulty;
   int diff_sides = 24; // 16 + 8 (default intelligence)

   // disassembly only nets a bit of practice
   if (dis->skill_used) {
    if(u.skillLevel(dis->skill_used) <= 1 || u.skillLevel(dis->skill_used) < dis->difficulty * 2) {
     u.practice(turn, dis->skill_used, (dis->difficulty) * 2);
    }
   }

  for (int j = 0; j < dis->components.size(); j++)
  {
    if (dis->components[j].size() != 0)
    {
      int compcount = dis->components[j][0].count;
      bool comp_success = (dice(skill_dice, skill_sides) > dice(diff_dice,  diff_sides));
      do
      {
        item newit(item_controller->find_template(dis->components[j][0].type), turn);
		if(newit.type->id.compare(0, 5, "func:") == 0) {
          newit.make(item_controller->find_template(newit.type->id.substr(5)));
		}
        // skip item addition if component is a consumable like superglue
        if (newit.type->id == "superglue" || newit.type->id == "duct_tape")
          compcount--;
        else
        {
          if (newit.count_by_charges())
          {
            if (dis->difficulty == 0 || comp_success)
              m.spawn_item(u.posx, u.posy, newit.type->id, 0, compcount);
            else
              add_msg(_("You fail to recover a component."));
            compcount = 0;
          } else
          {
            if (dis->difficulty == 0 || comp_success)
              m.add_item_or_charges(u.posx, u.posy, newit);
            else
              add_msg(_("You fail to recover a component."));
            compcount--;
          }
        }
      } while (compcount > 0);
    }
  }

  if (dis->learn_by_disassembly >= 0 && !u.knows_recipe(dis))
  {
    if (dis->skill_used == NULL || dis->learn_by_disassembly <= u.skillLevel(dis->skill_used))
    {
      if (rng(0,3) == 0)
      {
        u.learn_recipe(dis);
        add_msg(_("You learned a recipe from this disassembly!"));
      }
      else
      {
        add_msg(_("You think you could learn a recipe from this item. Maybe you'll try again."));
      }
    }
    else
    {
      add_msg(_("With some more skill, you might learn a recipe from this."));
    }
  }
}

recipe* game::recipe_by_index(int index)
{
    if(index == the_many_recipe.id) {
        return &the_many_recipe;
    }
    for (recipe_map::iterator map_iter = recipes.begin(); map_iter != recipes.end(); ++map_iter)
    {
        for (recipe_list::iterator list_iter = map_iter->second.begin(); list_iter != map_iter->second.end(); ++list_iter)
        {
            if ((*list_iter)->id == index)
            {
                return *list_iter;
            }
        }
    }
    return NULL;
}

recipe* recipe_by_name(std::string name)
{
    for (recipe_map::iterator map_iter = recipes.begin(); map_iter != recipes.end(); ++map_iter)
    {
        for (recipe_list::iterator list_iter = map_iter->second.begin(); list_iter != map_iter->second.end(); ++list_iter)
        {
            if ((*list_iter)->ident == name)
            {
                return *list_iter;
            }
        }
    }
    return NULL;
}

void pop_recipe_to_top(recipe *r) {
    if(recipes.count(r->cat) == 0) {
        return;
    }
    recipe_list &list = recipes[r->cat];
    for(recipe_list::iterator b = list.begin(); b != list.end(); b++) {
        if(*b != r) {
            continue;
        }
        if(b != list.begin()) {
            list.erase(b);
            list.insert(list.begin(), r);
        }
        return;
    }
}

static void check(const std::vector<std::vector<component> > &vec, const std::string &rName) {
    for(std::vector<std::vector<component> >::const_iterator b = vec.begin(); b != vec.end(); b++) {
        for(std::vector<component>::const_iterator c = b->begin(); c != b->end(); c++) {
            if(!item_controller->has_template(c->type)) {
                debugmsg("%s in recipe %s is not a valid item template", c->type.c_str(), rName.c_str());
            }
        }
    }
}

void check_recipes() {
    for(recipe_map::const_iterator a = recipes.begin(); a != recipes.end(); a++) {
        for(recipe_list::const_iterator b = a->second.begin(); b != a->second.end(); b++) {
            const recipe &r = **b;
            ::check(r.tools, r.ident);
            ::check(r.components, r.ident);
            if(!item_controller->has_template(r.result)) {
                debugmsg("result %s in recipe %s is not a valid item template", r.result.c_str(), r.ident.c_str());
            }
        }
    }
}

static void multiply(std::vector<std::vector<component> > &vec, int factor) {
    for(std::vector<std::vector<component> >::iterator b = vec.begin(); b != vec.end(); b++) {
        for(std::vector<component>::iterator c = b->begin(); c != b->end(); c++) {
            if(c->count > 0) {
                c->count *= factor;
            }
        }
    }
}

void multiply(const recipe &in, recipe &r, int factor) {
    const int id = r.id;
    const std::string ident = r.ident;
    r = in;
    r.id = id;
    r.ident = ident;
    r.autolearn = false;
    r.time *= factor;
    ::multiply(r.tools, factor);
    ::multiply(r.components, factor);
    if(r.count > 0) {
        r.count = in.count * factor;
        r.count_range = in.count_range * factor;
    } else {
        r.count = factor;
        r.count_range = 0;
    }
}
