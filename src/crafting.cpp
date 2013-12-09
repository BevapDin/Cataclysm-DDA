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
std::vector<craft_subcat> craft_subcat_list;
std::vector<std::string> recipe_names;
recipe_map recipes;
std::map<std::string,quality> qualities;
std::map<std::string, std::queue<std::pair<recipe*, int> > > recipe_booksets;

recipe the_many_recipe;
void multiply(const recipe &in, recipe &r, int factor);

void draw_recipe_tabs(WINDOW *w, craft_cat tab,bool filtered=false);
void draw_recipe_subtabs(WINDOW *w, craft_cat tab, craft_subcat subtab, bool filtered=false);

void load_recipe_category(JsonObject &jsobj)
{
    craft_cat_list.push_back(jsobj.get_string("id"));
}

void load_recipe_subcategory(JsonObject &jsobj)
{
    craft_subcat_list.push_back(jsobj.get_string("id"));
}

void load_recipe(JsonObject &jsobj)
{
    JsonArray jsarr;

    // required
    std::string result = jsobj.get_string("result");
    std::string category = jsobj.get_string("category");
    std::string subcategory = "";
    if ( !jsobj.has_string("subcategory") ) {
        if (category == "CC_NONCRAFT") { subcategory = "CSC_NONCRAFT"; }
        else if (category == "CC_WEAPON") { subcategory = "CSC_WEAPON_OTHER"; }
        else if (category == "CC_AMMO") { subcategory = "CSC_AMMO_OTHER"; }
        else if (category == "CC_FOOD") { subcategory = "CSC_FOOD_OTHER"; }
        else if (category == "CC_CHEM") { subcategory = "CSC_CHEM_OTHER"; }
        else if (category == "CC_ELECTRONIC") { subcategory = "CSC_ELECTRONIC_OTHER"; }
        else if (category == "CC_ARMOR") { subcategory = "CSC_ARMOR_OTHER"; }
        else if (category == "CC_OTHER") { subcategory = "CSC_OTHER_OTHER"; }
    } else {
    subcategory = jsobj.get_string("subcategory");
    }

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

    recipe* rec = new recipe(rec_name, id, result, category, subcategory, skill_used,
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
    if (u.fine_detail_vision_mod(g) > 4) {//minimum LL_LOW of LL_DARK + (ELFA_NV or atomic_light)
        g->add_msg(_("You can't see to craft!"));
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
    if(can_make_with_inventory(making, crafting_inv)) {
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

bool game::can_make_with_inventory(recipe *r, crafting_inventory_t &crafting_inv)
{
    bool retval = true;
    if (!u.knows_recipe(r)) {
        return false;
    }
    // under the assumption that all comp and tool's array contains
    // all the required stuffs at the start of the array

    // check all tool_quality requirements
    // this is an alternate method of checking for tools by using the tools qualities instead of the specific tool
    // You can specify the amount of tools with this quality required, but it does not work for consumed charges.
    std::vector<quality_requirement> &qualities = r->qualities;
    std::vector<quality_requirement>::iterator quality_iter = qualities.begin();
    while (quality_iter != qualities.end()) {
        std::string id = quality_iter->id;
        int amount = quality_iter->count;
        int level = quality_iter->level;
        if(crafting_inv.has_items_with_quality(id, level, amount)){
            quality_iter->available = true;
        } else {
            quality_iter->available = false;
            retval = false;
        }
        ++quality_iter;
    }

    // check all tools and check all components
    return retval && crafting_inv.has_all_requirements(*r);
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

craft_subcat game::next_craft_subcat(craft_subcat subcat)
{
    for (std::vector<craft_subcat>::iterator iter = craft_subcat_list.begin();
         iter != craft_subcat_list.end();
         ++iter)
    {
        if ((*iter) == subcat)
        {
            return *(++iter);
        }
    }
    return NULL;
}

craft_subcat game::prev_craft_subcat(craft_subcat subcat)
{
    for (std::vector<craft_subcat>::iterator iter = craft_subcat_list.begin();
         iter != craft_subcat_list.end();
         ++iter)
    {
        if ((*iter) == subcat)
        {
            return *(--iter);
        }
    }
    return NULL;
}

// return whether any of the listed components have been flagged as available
bool any_marked_available(const std::vector<component> &comps)
{
    for (std::vector<component>::const_iterator it = comps.begin();
            it != comps.end(); ++it) {
        if (it->available == 1) {
            return true;
        }
    }
    return false;
}

recipe* game::select_crafting_recipe()
{
    const int headHeight = 3;
    const int subHeadHeight = 2;
    const int freeWidth = TERMX - FULL_SCREEN_WIDTH;
    bool isWide = ( TERMX > FULL_SCREEN_WIDTH && freeWidth > 15 );

    const int width = isWide ? ( freeWidth > FULL_SCREEN_WIDTH ? FULL_SCREEN_WIDTH * 2 : TERMX ) : FULL_SCREEN_WIDTH;
    const int wStart = ( TERMX - width ) / 2;
    const int tailHeight = isWide ? 3 : 4;
    const int dataLines = TERMY - (headHeight + subHeadHeight) - tailHeight;
    const int dataHalfLines = dataLines / 2;
    const int dataHeight = TERMY - (headHeight + subHeadHeight);

    int lastid = -1;

    WINDOW *w_head = newwin(headHeight, width, 0, wStart);
    WINDOW *w_subhead = newwin(subHeadHeight, width, 3, wStart);
    WINDOW *w_data = newwin(dataHeight, width, headHeight + subHeadHeight, wStart);

    const int iInfoWidth = width - FULL_SCREEN_WIDTH - 3;
    std::vector<std::string> folded;
    craft_cat tab = "CC_WEAPON";
    craft_subcat subtab = "CSC_WEAPON_BASHING";
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
            draw_recipe_subtabs(w_subhead, tab, subtab, (filterstring == "")?false:true);
            current.clear();
            available.clear();
            // Set current to all recipes in the current tab; available are possible to make
            pick_recipes(crafting_inv, current, available, tab, subtab, filterstring);
        }

        // Clear the screen of recipe data, and draw it anew
        werase(w_data);

        if ( isWide ) {
            mvwprintz(w_data, dataLines+1, 5, c_white, _("Press <ENTER> to attempt to craft object."));
            wprintz(w_data, c_white, "  ");
            if (filterstring != "") {
                wprintz(w_data, c_white, _("[?/E]: Describe, [F]ind , [R]eset"));
            } else {
                wprintz(w_data, c_white, _("[?/E]: Describe, [F]ind"));
            }
        } else {
            if (filterstring != "") {
                mvwprintz(w_data, dataLines+1, 5, c_white, _("[?/E]: Describe, [F]ind , [R]eset"));
            } else {
                mvwprintz(w_data, dataLines+1, 5, c_white, _("[?/E]: Describe, [F]ind"));
            }
            mvwprintz(w_data, dataLines+2, 5, c_white, _("Press <ENTER> to attempt to craft object."));
        }
        // Draw borders
        for (int i = 1; i < width-1; ++i) { // _
            mvwputch(w_data, dataHeight-1, i, c_ltgray, LINE_OXOX);
        }
        for (int i = 0; i < dataHeight-1; ++i) { // |
            mvwputch(w_data, i, 0, c_ltgray, LINE_XOXO);
            mvwputch(w_data, i, width-1, c_ltgray, LINE_XOXO);
        }
        mvwputch(w_data, dataHeight-1,  0, c_ltgray, LINE_XXOO); // _|
        mvwputch(w_data, dataHeight-1, width-1, c_ltgray, LINE_XOOX); // |_

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
            for (unsigned i = 0; i < current.size() && i < dataHeight+1; ++i) {
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
            nc_color col = (available[line] ? c_white : c_ltgray);
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
                    ypos += fold_and_print(w_data, ypos, xpos, FULL_SCREEN_WIDTH-xpos-1, toolcol, qualinfo.str().c_str());
                }
                ypos--;
                // Loop to print the required tools
                for (int i = 0; i < current[line]->tools.size() && current[line]->tools[i].size() > 0; i++)
                {
                    ypos++;
                    xpos = 32;
                    mvwputch(w_data, ypos, 30, col, '>');
                    bool has_one = any_marked_available(current[line]->tools[i]);
                    for (unsigned j = 0; j < current[line]->tools[i].size(); j++)
                    {
                        itype_id type = current[line]->tools[i][j].type;
                        int charges = current[line]->tools[i][j].count;
                        nc_color toolcol = has_one ? c_dkgray : c_red;

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
                            mvwprintz(w_data, ypos, xpos, c_white, _("%s "), _("OR"));
                            xpos += utf8_width(_("OR"))+1;
                        }
                    }
                }
            }
        // Loop to print the required components
            ypos++;
            mvwprintz(w_data, ypos, 30, col, _("Components required:"));
            for (unsigned i = 0; i < current[line]->components.size(); i++)
            {
                if (current[line]->components[i].size() > 0)
                {
                    ypos++;
                    mvwputch(w_data, ypos, 30, col, '>');
                }
                xpos = 32;
                bool has_one = any_marked_available(current[line]->components[i]);
                for (unsigned j = 0; j < current[line]->components[i].size(); j++)
                {
                    int count = current[line]->components[i][j].count;
                    itype_id type = current[line]->components[i][j].type;
                    nc_color compcol = has_one ? c_dkgray : c_red;
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
                        mvwprintz(w_data, ypos, xpos, c_white, _("%s "), _("OR"));
                        xpos += utf8_width(_("OR"))+1;
                    }
                }
            }

            if ( isWide ) {
                if ( lastid != current[line]->id ) {
                    lastid = current[line]->id;
                    tmp = item(item_controller->find_template(current[line]->result), 0);
                    folded = foldstring(tmp.info(true), iInfoWidth);
                }
                int maxline = folded.size() > dataHeight ? dataHeight : folded.size();

                for(int i = 0; i < maxline; i++) {
                    mvwprintz(w_data, i, FULL_SCREEN_WIDTH+1, col, folded[i].c_str() );
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
                if (tab == "CC_WEAPON" && subtab == "CSC_WEAPON_BASHING") {
                    subtab = "CSC_WEAPON_OTHER";
                }
                else if (tab == "CC_AMMO" && subtab == "CSC_AMMO_BULLETS") {
                    subtab = "CSC_AMMO_OTHER";
                }
                else if (tab == "CC_FOOD" && subtab == "CSC_FOOD_DRINKS") {
                    subtab = "CSC_FOOD_OTHER";
                }
                else if (tab == "CC_CHEM" && subtab == "CSC_CHEM_DRUGS") {
                    subtab = "CSC_CHEM_OTHER";
                }
                else if (tab == "CC_ELECTRONIC" && subtab == "CSC_ELECTRONIC_CBMS") {
                    subtab = "CSC_ELECTRONIC_OTHER";
                }
                else if (tab == "CC_ARMOR" && subtab == "CSC_ARMOR_STORAGE") {
                    subtab = "CSC_ARMOR_OTHER";
                }
                else if (tab == "CC_OTHER" && subtab == "CSC_OTHER_TOOLS") {
                    subtab = "CSC_OTHER_OTHER";
                }
                else
                {
                    subtab = prev_craft_subcat(subtab);
                }
                redraw = true;
                break;
            case DirectionUp:
                if (tab == "CC_WEAPON")
                {
                    tab = "CC_OTHER";
                    subtab = "CSC_OTHER_TOOLS";
                }
                else
                {
                    tab = prev_craft_cat(tab);
                    if ( tab == "CC_WEAPON") { subtab = "CSC_WEAPON_BASHING"; }
                    else if (tab == "CC_AMMO") { subtab = "CSC_AMMO_BULLETS"; }
                    else if (tab == "CC_FOOD") { subtab = "CSC_FOOD_DRINKS"; }
                    else if (tab == "CC_CHEM") { subtab = "CSC_CHEM_DRUGS"; }
                    else if (tab == "CC_ELECTRONIC") { subtab = "CSC_ELECTRONIC_CBMS"; }
                    else if (tab == "CC_ARMOR") { subtab = "CSC_ARMOR_STORAGE"; }
                    else if (tab == "CC_OTHER") { subtab = "CSC_OTHER_TOOLS"; }
                }
                redraw = true;
                break;
            case DirectionE:
                if (tab == "CC_WEAPON" && subtab == "CSC_WEAPON_OTHER") {
                    subtab = "CSC_WEAPON_BASHING";
                }
                else if (tab == "CC_AMMO" && subtab == "CSC_AMMO_OTHER") {
                    subtab = "CSC_AMMO_BULLETS";
                }
                else if (tab == "CC_FOOD" && subtab == "CSC_FOOD_OTHER") {
                    subtab = "CSC_FOOD_DRINKS";
                }
                else if (tab == "CC_CHEM" && subtab == "CSC_CHEM_OTHER") {
                    subtab = "CSC_CHEM_DRUGS";
                }
                else if (tab == "CC_ELECTRONIC" && subtab == "CSC_ELECTRONIC_OTHER") {
                    subtab = "CSC_ELECTRONIC_CBMS";
                }
                else if (tab == "CC_ARMOR" && subtab == "CSC_ARMOR_OTHER") {
                    subtab = "CSC_ARMOR_STORAGE";
                }
                else if (tab == "CC_OTHER" && subtab == "CSC_OTHER_OTHER") {
                    subtab = "CSC_OTHER_TOOLS";
                }
                else
                {
                    subtab = next_craft_subcat(subtab);
                }
                redraw = true;
                break;
            case DirectionDown:
                if (tab == "CC_OTHER")
                {
                    tab = "CC_WEAPON";
                    subtab = "CSC_WEAPON_BASHING";
                }
                else
                {
                    tab = next_craft_cat(tab);
                    if ( tab == "CC_WEAPON") { subtab = "CSC_WEAPON_BASHING"; }
                    else if (tab == "CC_AMMO") { subtab = "CSC_AMMO_BULLETS"; }
                    else if (tab == "CC_FOOD") { subtab = "CSC_FOOD_DRINKS"; }
                    else if (tab == "CC_CHEM") { subtab = "CSC_CHEM_DRUGS"; }
                    else if (tab == "CC_ELECTRONIC") { subtab = "CSC_ELECTRONIC_CBMS"; }
                    else if (tab == "CC_ARMOR") { subtab = "CSC_ARMOR_STORAGE"; }
                    else if (tab == "CC_OTHER") { subtab = "CSC_OTHER_TOOLS"; }
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
    werase(w_subhead);
    werase(w_data);
    delwin(w_head);
    delwin(w_subhead);
    delwin(w_data);
    refresh_all();

    return chosen;
}

void draw_recipe_tabs(WINDOW *w, craft_cat tab,bool filtered)
{
    werase(w);
    int width = getmaxx(w);
    for (int i = 0; i < width; i++)
    {
        mvwputch(w, 2, i, c_ltgray, LINE_OXOX);
    }

    mvwputch(w, 2,  0, c_ltgray, LINE_OXXO); // |^
    mvwputch(w, 2, width-1, c_ltgray, LINE_OOXX); // ^|
    if(!filtered)
    {
        int pos_x = 2;//draw the tabs on each other
        int tab_step = 3;//step between tabs, two for tabs border
        draw_tab(w,  pos_x, _("WEAPONS"), (tab == "CC_WEAPON") ? true : false);
        pos_x += utf8_width(_("WEAPONS")) + tab_step;
        draw_tab(w, pos_x, _("AMMO"),    (tab == "CC_AMMO")   ? true : false);
        pos_x += utf8_width(_("AMMO")) + tab_step;
        draw_tab(w, pos_x, _("FOOD"),    (tab == "CC_FOOD")   ? true : false);
        pos_x += utf8_width(_("FOOD")) + tab_step;
        draw_tab(w, pos_x, _("CHEMS"),   (tab == "CC_CHEM")   ? true : false);
        pos_x += utf8_width(_("CHEMS")) + tab_step;
        draw_tab(w, pos_x, _("ELECTRONICS"), (tab == "CC_ELECTRONIC") ? true : false);
        pos_x += utf8_width(_("ELECTRONICS")) + tab_step;
        draw_tab(w, pos_x, _("ARMOR"),   (tab == "CC_ARMOR")  ? true : false);
        pos_x += utf8_width(_("ARMOR")) + tab_step;
        draw_tab(w, pos_x, _("OTHER"),    (tab == "CC_OTHER") ? true : false);
    }
    else
    {
        draw_tab(w,  2, _("Searched"), true);
    }

    wrefresh(w);
}

void draw_recipe_subtabs(WINDOW *w, craft_cat tab, craft_subcat subtab, bool filtered)
{
    werase(w);
    int width = getmaxx(w);
    for (int i = 0; i < width; i++)
    {
        if (i == 0) { mvwputch(w, 2, i, c_ltgray, LINE_XXXO); }
        else if (i == width) { mvwputch(w, 2, i, c_ltgray, LINE_XOXX); }
        else { mvwputch(w, 2, i, c_ltgray, LINE_OXOX); }
    }

    for (int i = 0; i < 3; i++) {
        mvwputch(w, i,  0, c_ltgray, LINE_XOXO); // |^
        mvwputch(w, i, width-1, c_ltgray,  LINE_XOXO); // ^|
    }

    if(!filtered)
    {
        if (tab == "CC_WEAPON") {
            draw_subtab(w,  2, _("BASHING"), (subtab == "CSC_WEAPON_BASHING") ? true : false);
            draw_subtab(w, 12, _("CUTTING"),    (subtab == "CSC_WEAPON_CUTTING")   ? true : false);
            draw_subtab(w, 22, _("PIERCING"),    (subtab == "CSC_WEAPON_PIERCING")   ? true : false);
            draw_subtab(w, 33, _("RANGED"),  (subtab == "CSC_WEAPON_RANGED")  ? true : false);
            draw_subtab(w, 42, _("EXPLOSIVE"),  (subtab == "CSC_WEAPON_EXPLOSIVE")  ? true : false);
            draw_subtab(w, 54, _("MODS"),  (subtab == "CSC_WEAPON_MODS")  ? true : false);
            draw_subtab(w, 61, _("OTHER"),   (subtab == "CSC_WEAPON_OTHER")   ? true : false);
        }
        else if (tab == "CC_AMMO") {
            draw_subtab(w,  2, _("BULLETS"), (subtab == "CSC_AMMO_BULLETS") ? true : false);
            draw_subtab(w,  12, _("ARROWS"), (subtab == "CSC_AMMO_ARROWS") ? true : false);
            draw_subtab(w,  21, _("COMPONENTS"), (subtab == "CSC_AMMO_COMPONENTS") ? true : false);
            draw_subtab(w,  34, _("OTHER"), (subtab == "CSC_AMMO_OTHER") ? true : false);
        }
        else if (tab == "CC_FOOD") {
            draw_subtab(w,  2, _("DRINKS"), (subtab == "CSC_FOOD_DRINKS") ? true : false);
            draw_subtab(w,  11, _("MEAT"), (subtab == "CSC_FOOD_MEAT") ? true : false);
            draw_subtab(w,  18, _("VEGGI"), (subtab == "CSC_FOOD_VEGGI") ? true : false);
            draw_subtab(w,  26, _("SNACK"), (subtab == "CSC_FOOD_SNACK") ? true : false);
            draw_subtab(w,  34, _("BREAD"), (subtab == "CSC_FOOD_BREAD") ? true : false);
            draw_subtab(w,  42, _("PASTA"), (subtab == "CSC_FOOD_PASTA") ? true : false);
            draw_subtab(w,  50, _("OTHER"), (subtab == "CSC_FOOD_OTHER") ? true : false);
        }
        else if (tab == "CC_CHEM") {
            draw_subtab(w,  2, _("DRUGS"), (subtab == "CSC_CHEM_DRUGS") ? true : false);
            draw_subtab(w,  10, _("MUTAGEN"), (subtab == "CSC_CHEM_MUTAGEN") ? true : false);
            draw_subtab(w,  20, _("CHEMICALS"), (subtab == "CSC_CHEM_CHEMICALS") ? true : false);
            draw_subtab(w,  32, _("OTHER"), (subtab == "CSC_CHEM_OTHER") ? true : false);
        }
        else if (tab == "CC_ELECTRONIC") {
            draw_subtab(w,  2, _("CBMS"), (subtab == "CSC_ELECTRONIC_CBMS") ? true : false);
            draw_subtab(w,  9, _("LIGHTING"), (subtab == "CSC_ELECTRONIC_LIGHTING") ? true : false);
            draw_subtab(w,  20, _("COMPONENTS"), (subtab == "CSC_ELECTRONIC_COMPONENTS") ? true : false);
            draw_subtab(w,  33, _("OTHER"), (subtab == "CSC_ELECTRONIC_OTHER") ? true : false);
        }
        else if (tab == "CC_ARMOR") {
            draw_subtab(w,  2, _("STORAGE"), (subtab == "CSC_ARMOR_STORAGE") ? true : false);
            draw_subtab(w,  12, _("SUIT"), (subtab == "CSC_ARMOR_SUIT") ? true : false);
            draw_subtab(w,  19, _("HEAD"), (subtab == "CSC_ARMOR_HEAD") ? true : false);
            draw_subtab(w,  26, _("BACK"), (subtab == "CSC_ARMOR_BACK") ? true : false);
            draw_subtab(w,  33, _("TORSO"), (subtab == "CSC_ARMOR_TORSO") ? true : false);
            draw_subtab(w,  41, _("ARMS"), (subtab == "CSC_ARMOR_ARMS") ? true : false);
            draw_subtab(w,  48, _("HANDS"), (subtab == "CSC_ARMOR_HANDS") ? true : false);
            draw_subtab(w,  56, _("LEGS"), (subtab == "CSC_ARMOR_LEGS") ? true : false);
            draw_subtab(w,  63, _("FEET"), (subtab == "CSC_ARMOR_FEET") ? true : false);
            draw_subtab(w,  70, _("OTHER"), (subtab == "CSC_ARMOR_OTHER") ? true : false);
        }
        else if (tab == "CC_OTHER") {
            draw_subtab(w,  2, _("TOOLS"), (subtab == "CSC_OTHER_TOOLS") ? true : false);
            draw_subtab(w,  10, _("MEDICAL"), (subtab == "CSC_OTHER_MEDICAL") ? true : false);
            draw_subtab(w,  20, _("CONTAINERS"), (subtab == "CSC_OTHER_CONTAINERS") ? true : false);
            draw_subtab(w,  33, _("MATERIALS"), (subtab == "CSC_OTHER_MATERIALS") ? true : false);
            draw_subtab(w,  45, _("PARTS"), (subtab == "CSC_OTHER_PARTS") ? true : false);
            draw_subtab(w,  53, _("TRAPS"), (subtab == "CSC_OTHER_TRAPS") ? true : false);
            draw_subtab(w,  61, _("OTHER"), (subtab == "CSC_OTHER_OTHER") ? true : false);
        }
    }
    else
    {
        werase(w);

        for (int i = 0; i < 3; i++) {
            mvwputch(w, i,  0, c_ltgray, LINE_XOXO); // |^
            mvwputch(w, i, width-1, c_ltgray,  LINE_XOXO); // ^|
        }
    }

    wrefresh(w);
}

void game::pick_recipes(crafting_inventory_t& crafting_inv, std::vector<recipe*> &current,
                         std::vector<bool> &available, craft_cat tab, craft_subcat subtab, std::string filter)
{
    recipe_list available_recipes;

    if (filter == "") {
        available_recipes = recipes[tab];
    } else {

        for (recipe_map::iterator iter = recipes.begin(); iter != recipes.end(); ++iter) {
            available_recipes.insert(available_recipes.begin(),
                                     iter->second.begin(), iter->second.end());
        }
    }

    current.clear();
    available.clear();

    for (recipe_list::iterator iter = available_recipes.begin();
         iter != available_recipes.end(); ++iter) {
        if ((*iter)->subcat == subtab || filter != "") {
            if (!u.knows_recipe(*iter)) {
                continue;
            }

            if ((*iter)->difficulty < 0 ) {
                continue;
            }

            if (filter != "" && item_controller->find_template((*iter)->result)->name.find(filter) == std::string::npos) {
                continue;
            }

            if (can_make_with_inventory(*iter, crafting_inv)) {
                current.insert(current.begin(), *iter);
                available.insert(available.begin(), true);
            } else {
                current.push_back(*iter);
                available.push_back(false);
            }
        }
    }
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

void serialize_item_list(JsonOut &json, const std::list<item> &x) {
    json.start_array();
    for(std::list<item>::const_iterator a = x.begin(); a != x.end(); ++a) {
        a->serialize(json);
    }
    json.end_array();
}

std::string to_uncraft_tag(const std::list<item> &comps, const std::list<item> &tools) {
    std::ostringstream buffer;
    JsonOut json(&buffer);
    json.start_array();
    serialize_item_list(json, comps);
    serialize_item_list(json, tools);
    json.end_array();
    return std::string("UNCRAFT:") + buffer.str();
}

bool deserialize_item_list(JsonIn &json, std::list<item> &x) {
    JsonArray data = json.get_array();
    item it;
    for(size_t i = 0; i < data.size(); i++) {
        JsonObject obj(data.get_object(i));
        it.deserialize(obj);
        if(!it.is_null()) {
            x.push_back(it);
        }
    }
    return true;
}

bool from_uncraft_tag(const std::string &data, std::list<item> &comps, std::list<item> &tools) {
    if(data.length() < 10 || data.compare(0, 8, "UNCRAFT:") != 0) {
        return false;
    }
    std::istringstream buffer(data.substr(8));
    try {
        bool result = false;
        JsonIn json(&buffer);
        json.start_array();
        if(deserialize_item_list(json, comps)) {
            if(deserialize_item_list(json, tools)) {
                result = true;
            }
        }
        json.end_array();
        return result;
    } catch(...) {
        return false;
    }
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
     && !u.is_wearing("glasses_bifocal") && !u.has_disease("contacts")) {
  int main_rank_penalty = 0;
  if (making->skill_used == Skill::skill("electronics")) {
   main_rank_penalty = 2;
  } else if (making->skill_used == Skill::skill("tailoring")) {
   main_rank_penalty = 1;
  }
  skill_dice -= main_rank_penalty * 4;
 }

 //It's tough to craft with paws.  Fortunately it's just a matter of grip and fine-motor, not inability to see what you're doing
 if (u.has_trait("PAWS")) {
  int paws_rank_penalty = 0;
  if (making->skill_used == Skill::skill("electronics")) {
   paws_rank_penalty = 1;
  } else if (making->skill_used == Skill::skill("tailoring")) {
   paws_rank_penalty = 1;
  } else if (making->skill_used == Skill::skill("mechanics")) {
   paws_rank_penalty = 1;
  }
  skill_dice -= paws_rank_penalty * 4;
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

 item newit(item_controller->find_template(making->result), turn, 0);
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
 if (newit.made_of(LIQUID)) {
  newit.charges *= new_count;
  while(!handle_liquid(newit, false, false)) { ; }
 } else {
  if(newit.count_by_charges()) {
    newit.charges *= new_count;
    new_count = 1;
  }
  u.i_add_or_drop(newit, this, new_count);
  g->add_msg("%s", newit.tname(g).c_str());
 }
}

void game::disassemble(int pos)
{
    if (pos == INT_MIN)
    {
        pos = inv(_("Disassemble item:"));
    }
    if (!u.has_item(pos))
    {
        add_msg(_("You don't have that item!"), pos);
        return;
    }

    item* dis_item = &u.i_at(pos);
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
                for (unsigned j = 0; j < cur_recipe->tools.size(); j++)
                {
                    if (cur_recipe->tools[j].size() == 0) // no tools required, may change this
                    {
                        continue;
                    }
                    bool have_this_tool = false;
                    for (unsigned k = 0; k < cur_recipe->tools[j].size(); k++)
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
                  if (OPTIONS["QUERY_DISASSEMBLE"] && !(query_yn(_("Really disassemble your %s?"), dis_item->tname().c_str())))
                  {
                   return;
                  }
                    u.assign_activity(this, ACT_DISASSEMBLE, cur_recipe->time / 2, cur_recipe->id);
                    u.moves = 0;
                    std::vector<int> dis_items;
                    dis_items.push_back(pos);
                    u.activity.values = dis_items;
                }
                return; // recipe exists, but no tools, so do not start disassembly
            }
        }
    }

    //if we're trying to disassemble a book or magazine
    if(dis_item->is_book())
    {
       if (OPTIONS["QUERY_DISASSEMBLE"] && !(query_yn(_("Do you want to tear %s into pages?"), dis_item->tname().c_str())))
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
  for (unsigned j = 0; j < dis->tools.size(); j++)
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

  for (unsigned j = 0; j < dis->components.size(); j++)
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
    if(list.empty() || list[list.size() - 1] == r) {
        return;
    }
    for(size_t i = 0; i < list.size(); i++) {
        if(list[i] != r) {
            continue;
        }
        list.erase(list.begin() + i);
        list.push_back(r);
        return;
    }
}

static void check_component_list(const std::vector<std::vector<component> > &vec, const std::string &rName)
{
    for (std::vector<std::vector<component> >::const_iterator b = vec.begin(); b != vec.end(); b++)
    {
        for (std::vector<component>::const_iterator c = b->begin(); c != b->end(); c++)
        {
            if (!item_controller->has_template(c->type))
            {
                debugmsg("%s in recipe %s is not a valid item template", c->type.c_str(), rName.c_str());
            }
        }
    }
}

void check_recipe_definitions()
{
    for (recipe_map::iterator map_iter = recipes.begin(); map_iter != recipes.end(); ++map_iter)
    {
        for (recipe_list::iterator list_iter = map_iter->second.begin(); list_iter != map_iter->second.end(); ++list_iter)
        {
            const recipe &r = **list_iter;
            ::check_component_list(r.tools, r.ident);
            ::check_component_list(r.components, r.ident);
            if (!item_controller->has_template(r.result))
            {
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
