#include "game.h"
#include "setvector.h"
#include "output.h"
#include "keypress.h"
#include "player.h"
#include "inventory.h"
#include "mapdata.h"
#include "skill.h"
#include "crafting.h" // For the use_comps use_tools functions
#include "item_factory.h"
#include "catacharset.h"
#include "action.h"
#include <algorithm>
#include "crafting_inventory_t.h"

bool will_flood_stop(map *m, bool (&fill)[SEEX * MAPSIZE][SEEY * MAPSIZE],
                     int x, int y);


void construction_stage::add_tool(const component &c) {
    tools.resize(tools.size() + 1);
    add_alternativ_tool(c);
}

void construction_stage::add_alternativ_tool(const component &c) {
    assert(tools.size() > 0);
    tools.back().push_back(c);
}

void construction_stage::add_component(const component &c) {
    components.resize(components.size() + 1);
    add_alternativ_component(c);
}

void construction_stage::add_alternativ_component(const component &c) {
    assert(components.size() > 0);
    components.back().push_back(c);
}

void game::init_construction()
{
 int id = -1;
 construction_stage *cur_stage = NULL;

 #define CONSTRUCT(name, difficulty, able, done) \
  id++; \
  constructions.push_back( new constructable(id, name, difficulty, able, done))
  

 #define STAGE(...)\
  constructions[id]->stages.push_back(construction_stage(__VA_ARGS__)); \
  cur_stage = &(constructions[id]->stages.back());
 #define TOOL(item)             cur_stage->add_tool(component(item, -1))
 #define TOOLN(item,N)          cur_stage->add_tool(component(item, N))
 #define TOOLCONT(item)         cur_stage->add_alternativ_tool(component(item, -1))
 #define TOOLCONTN(item,N)      cur_stage->add_alternativ_tool(component(item, N))
 #define COMP(item, amount)     cur_stage->add_component(component(item,amount))
 #define COMPCONT(item, amount) cur_stage->add_alternativ_component(component(item,amount))


/* CONSTRUCT( name, time, able, done )
 * Name is the name as it appears in the menu; 30 characters or less, please.
 * time is the time in MINUTES that it takes to finish this construction.
 *  note that 10 turns = 1 minute.
 * able is a function which returns true if you can build it on a given tile
 *  See construction.h for options, and this file for definitions.
 * done is a function which runs each time the construction finishes.
 *  This is useful, for instance, for removing the trap from a pit, or placing
 *  items after a deconstruction.
 */

 CONSTRUCT(_("Dig Pit"), 0, &construct::able_dig, &construct::done_nothing);
  STAGE(t_pit_shallow, 10);
   TOOL("func:shovel");
   TOOLCONT("digging_stick");
  STAGE(t_pit, 10);
   TOOL("func:shovel");

 CONSTRUCT(_("Spike Pit"), 0, &construct::able_pit, &construct::done_nothing);
  STAGE(t_pit_spiked, 5);
   COMP("spear_wood", 4);
   COMPCONT("pointy_stick", 4);

 CONSTRUCT(_("Fill Pit"), 0, &construct::able_pit, &construct::done_nothing);
  STAGE(t_pit_shallow, 5);
   TOOL("func:shovel");
  STAGE(t_dirt, 5);
   TOOL("func:shovel");

 CONSTRUCT(_("Chop Down Tree"), 0, &construct::able_tree, &construct::done_tree);
  STAGE(t_dirt, 10);
   TOOL("func:ax");
   TOOLCONT("primitive_axe");

 CONSTRUCT(_("Chop Tree trunk into logs"), 0, &construct::able_trunk, &construct::done_trunk_log);
  STAGE(t_dirt, 20);
   TOOL("func:ax");
   TOOLCONT("primitive_axe");

 CONSTRUCT(_("Chop Tree trunk into planks"), 0, &construct::able_trunk, &construct::done_trunk_plank);
  STAGE(t_dirt, 23);
   TOOL("func:ax");
   TOOLCONT("primitive_axe");
   TOOLCONT("func:saw");

 CONSTRUCT(_("Move Furniture"), -1, &construct::able_move, &construct::done_move);
  STAGE(1);

 CONSTRUCT(_("Clean Broken Window"), 0, &construct::able_broken_window,
                                     &construct::done_nothing);
  STAGE(t_window_empty, 5);

/* CONSTRUCT("Remove Window Pane",  1, &construct::able_window_pane,
                                     &construct::done_window_pane);
  STAGE(t_window_empty, 10);
   TOOL("func:hammer");
   TOOLCONT("primitive_hammer");
   TOOLCONT("rock");
   TOOLCONT("hatchet");
   TOOLCONT("toolset_hammer");
   TOOL("screwdriver");
   TOOLCONT("knife_butter");
   TOOLCONT("toolset_knife");
*/

 CONSTRUCT(_("Repair Door"), 1, &construct::able_door_broken,
                             &construct::done_nothing);
  STAGE(t_door_c, 10);
   TOOL("func:hammer");
   TOOLCONT("hatchet");
   TOOLCONT("nailgun");
   COMP("2x4", 3);
   COMP("nail", 12);

 CONSTRUCT(_("Board Up Door"), 0, &construct::able_door, &construct::done_nothing);
  STAGE(t_door_boarded, 8);
   TOOL("func:hammer");
   TOOLCONT("hammer_sledge");
   TOOLCONT("hatchet");
   TOOLCONT("nailgun");
   COMP("2x4", 4);
   COMP("nail", 8);

 CONSTRUCT(_("Board Up Window"), 0, &construct::able_window,
                                 &construct::done_nothing);
  STAGE(t_window_boarded, 5);
   TOOL("func:hammer");
   TOOLCONT("hammer_sledge");
   TOOLCONT("hatchet");
   TOOLCONT("nailgun");
   COMP("2x4", 4);
   COMP("nail", 8);

 CONSTRUCT(_("Build Wall"), 2, &construct::able_empty, &construct::done_nothing);
  STAGE(t_wall_half, 10);
   TOOL("func:hammer");
   TOOLCONT("hatchet");
   TOOLCONT("nailgun");
   COMP("2x4", 10);
   COMP("nail", 20);
  STAGE(t_wall_wood, 10);
   TOOL("func:hammer");
   TOOLCONT("hatchet");
   TOOLCONT("nailgun");
   COMP("2x4", 10);
   COMP("nail", 20);

 CONSTRUCT(_("Build Log Wall"), 2, &construct::able_dig, &construct::done_nothing);
  STAGE(t_pit_shallow, 10);
   TOOL("func:shovel");
   TOOLCONT("digging_stick");
  STAGE(t_pit, 10);
   TOOL("func:shovel");
  STAGE(t_wall_log_half, 20);
   TOOL("func:shovel");
   COMP("log", 2);
   COMP("stick", 3);
   COMPCONT("2x4", 6);
  STAGE(t_wall_log, 20);
   TOOL("func:shovel");
   COMP("log", 2);
   COMP("stick", 3);
   COMPCONT("2x4", 6);

 CONSTRUCT(_("Build Palisade Wall"), 2, &construct::able_dig, &construct::done_nothing);
  STAGE(t_pit_shallow, 10);
   TOOL("func:shovel");
   TOOLCONT("digging_stick");
  STAGE(t_pit, 10);
   TOOL("func:shovel");
  STAGE(t_palisade, 20);
   TOOL("func:shovel");
   COMP("log", 3);
   COMP("rope_6", 2);

 CONSTRUCT(_("Build Rope & Pulley System"), 2, &construct::able_empty, &construct::done_nothing);
  STAGE(t_palisade_pulley, 0);
   COMP("rope_30", 1);
   COMP("stick", 8);
   COMPCONT("2x4", 8);

 CONSTRUCT(_("Build Palisade Gate"), 2, &construct::able_dig, &construct::done_nothing);
  STAGE(t_pit_shallow, 10);
   TOOL("func:shovel");
   TOOLCONT("digging_stick");
  STAGE(t_pit, 10);
   TOOL("func:shovel");
  STAGE(t_palisade_gate, 20);
   TOOL("func:shovel");
   COMP("log", 2);
   COMP("2x4", 3);
   COMP("rope_6", 2);

 CONSTRUCT(_("Build Window"), 2, &construct::able_make_window,
                              &construct::done_nothing);
  STAGE(t_window_empty, 10);
   TOOL("func:hammer");
   TOOLCONT("hatchet");
   TOOLCONT("nailgun");
   COMP("2x4", 15);
   COMPCONT("log", 2);
   COMP("nail", 30);
  STAGE(t_window, 5);
   COMP("glass_sheet", 1);
  STAGE(t_window_domestic, 5);
   TOOL("func:saw");
   COMP("nail", 4);
   COMP("sheet", 2);
   COMP("stick", 1);
   COMP("string_36", 1);

 CONSTRUCT(_("Build Door"), 2, &construct::able_empty,
                              &construct::done_nothing);
  STAGE(t_door_frame, 15);
   TOOL("func:hammer");
   TOOLCONT("hatchet");
   TOOLCONT("nailgun");
   COMP("2x4", 12);
   COMP("nail", 24);
  STAGE(t_door_c, 15);
   TOOL("func:hammer");
   TOOLCONT("hatchet");
   TOOLCONT("nailgun");
   COMP("2x4", 4);
   COMP("nail", 12);

 CONSTRUCT(_("Build Wire Fence"),3, &construct::able_dig,
                                 &construct::done_nothing);
  STAGE(t_chainfence_posts, 20);
   TOOL("func:hammer");
   TOOLCONT("hatchet");
   COMP("pipe", 6);
   COMP("scrap", 8);
  STAGE(t_chainfence_v, 20);
   COMP("wire", 20);

 CONSTRUCT(_("Realign Fence"),   0, &construct::able_chainlink,
                                 &construct::done_nothing);
  STAGE(t_chainfence_h, 0);
  STAGE(t_chainfence_v, 0);
  constructions[id]->loopstages = true;

 CONSTRUCT(_("Build Wire Gate"), 3, &construct::able_between_walls,
                                 &construct::done_nothing);
  STAGE(t_chaingate_c, 15);
   COMP("wire", 20);
   COMP("steel_chunk", 3);
   COMPCONT("scrap", 12);
   COMP("pipe", 6);

/*  Removed until we have some way of auto-aligning fences!
 CONSTRUCT("Build Fence", 1, 15, &construct::able_empty);
  STAGE(t_fence_h, 10);
   TOOL("func:hammer");
   TOOLCONT("primitive_hammer");
   TOOLCONT("hatchet");
   COMP("2x4", 5);
   COMPCONT("nail", 8);
   */

 CONSTRUCT(_("Build Roof"), 3, &construct::able_between_walls,
                            &construct::done_nothing);
  STAGE(t_floor, 40);
   TOOL("func:hammer");
   TOOLCONT("hatchet");
   TOOLCONT("nailgun");
   COMP("2x4", 8);
   COMP("nail", 40);

 CONSTRUCT(_("Build Log & Sod Roof"), 3, &construct::able_between_walls,
                            &construct::done_nothing);
  STAGE(t_floor, 80);
   TOOL("func:hammer");
   TOOLCONT("hatchet");
   TOOL("func:shovel");
   COMP("log", 2);
   COMP("stick", 4);
   COMPCONT("2x4", 8);


// Base stuff
 CONSTRUCT(_("Build Bulletin Board"), 0, &construct::able_empty,
                                         &construct::done_nothing);
  STAGE(furnlist[f_bulletin], 10)
   TOOL("func:saw");
   TOOL("func:hammer");
   TOOLCONT("hatchet");
   TOOLCONT("nailgun");
   COMP("2x4", 4);
   COMP("nail", 8);

// Household stuff
 CONSTRUCT(_("Build Dresser"), 1, &construct::able_empty,
                                &construct::done_nothing);
  STAGE(furnlist[f_dresser], 20);
   TOOL("func:saw");
   TOOL("func:hammer");
   TOOLCONT("hatchet");
   TOOLCONT("nailgun");
   COMP("nail", 8);
   COMP("2x4", 6);

 CONSTRUCT(_("Build Bookcase"), 1, &construct::able_empty,
                                &construct::done_nothing);
  STAGE(furnlist[f_bookcase], 20);
   TOOL("func:saw");
   TOOL("func:hammer");
   TOOLCONT("hatchet");
   TOOLCONT("nailgun");
   COMP("nail", 16);
   COMP("2x4", 12);

 CONSTRUCT(_("Build Locker"), 1, &construct::able_empty,
                                &construct::done_nothing);
  STAGE(furnlist[f_locker], 20);
   TOOL("func:hammer");
   TOOLCONT("hatchet");
   TOOL("func:wrench");
   COMP("sheet_metal", 2);
   COMP("pipe", 8);

 CONSTRUCT(_("Build Metal Rack"), 1, &construct::able_empty,
                                &construct::done_nothing);
  STAGE(furnlist[f_rack], 20);
   TOOL("func:hammer");
   TOOLCONT("hatchet");
   TOOL("func:wrench");
   COMP("pipe", 12);

 CONSTRUCT(_("Build Counter"), 0, &construct::able_empty,
                                &construct::done_nothing);
  STAGE(furnlist[f_counter], 20);
   TOOL("func:hammer");
   TOOLCONT("hatchet");
   TOOLCONT("nailgun");
   COMP("nail", 8);
   COMP("2x4", 6);

 CONSTRUCT(_("Build Makeshift Bed"), 0, &construct::able_empty,
                                &construct::done_nothing);
  STAGE(furnlist[f_makeshift_bed], 20);
   TOOL("func:hammer");
   TOOLCONT("hatchet");
   TOOLCONT("nailgun");
   COMP("nail", 8);
   COMP("2x4", 10);
   COMP("blanket", 1);

 CONSTRUCT(_("Tape up window"), 0, &construct::able_window_pane,
                                &construct::done_tape);
  STAGE(2);
  COMP("duct_tape", 50);

 CONSTRUCT(_("Deconstruct Furniture"), 0, &construct::able_deconstruct,
                                &construct::done_deconstruct);
  STAGE(20);
   TOOL("func:hammer");
   TOOLCONT("hatchet");
   TOOLCONT("nailgun");
   TOOL("func:screwdriver");

 CONSTRUCT(_("Start vehicle construction"), 0, &construct::able_empty, &construct::done_vehicle);
  STAGE(10);
   COMP("frame", 1);

 CONSTRUCT(_("Fence Posts"), 0, &construct::able_dig,
                             &construct::done_nothing);
  STAGE(t_fence_post, 5);
   TOOL("func:hammer");
   TOOLCONT("func:shovel");
   TOOLCONT("hatchet");
   TOOLCONT("func:ax");
   TOOLCONT("primitive_axe");
   COMP("pointy_stick", 2);
   COMPCONT("spear_wood", 2);

 CONSTRUCT(_("Build Wood Stove"), 0, &construct::able_empty,
                                     &construct::done_nothing);
  STAGE(furnlist[f_woodstove], 10);
   TOOL("hacksaw");
   COMP("metal_tank", 1);
   COMP("pipe", 1);

 CONSTRUCT(_("Build Stone Fireplace"), 0, &construct::able_empty,
                                          &construct::done_nothing);
  STAGE(furnlist[f_fireplace], 40);
   TOOL("func:hammer");
   TOOLCONT("func:shovel");
   COMP("rock", 40);

 CONSTRUCT("Build metal roof", 10, &construct::able_between_walls,
                            &construct::done_nothing);
  STAGE(t_metal_floor, 30);
   TOOL("func:wrench");
   TOOLN("func:welder", 200);
   TOOLCONTN("toolset_welder", 40);
   TOOL("func:hacksaw");
   TOOL("func:shovel");
   COMP("frame", 3);
   COMP("steel_plate", 2);
   COMPCONT("sheet_metal", 4);

 CONSTRUCT("Build metal wall (v)", 10, &construct::able_dig, &construct::done_nothing);
  STAGE(t_pit_shallow, 10);
   TOOL("func:shovel");
   TOOLCONT("digging_stick");
  STAGE(t_pit, 10);
   TOOL("func:shovel");
  STAGE(t_wall_metal_v, 30);
   TOOL("wrench");
   TOOLN("func:welder", 200);
   TOOLCONTN("toolset_welder", 40);
   TOOL("func:hacksaw");
   COMP("frame", 4);
   COMP("steel_plate", 1);
   COMPCONT("sheet_metal", 2);
   
 CONSTRUCT("Build metal wall (h)", 10, &construct::able_dig, &construct::done_nothing);
  STAGE(t_pit_shallow, 10);
   TOOL("func:shovel");
   TOOLCONT("digging_stick");
  STAGE(t_pit, 10);
   TOOL("func:shovel");
  STAGE(t_wall_metal_h, 30);
   TOOL("func:wrench");
   TOOLN("func:welder", 200);
   TOOLCONTN("toolset_welder", 40);
   TOOL("func:hacksaw");
   COMP("frame", 4);
   COMP("steel_plate", 1);
   COMPCONT("sheet_metal", 2);
   
 CONSTRUCT("Build steel compactor", 10, &construct::able_empty,
                              &construct::done_nothing);
  STAGE(t_recycler, 120);
   TOOL("func:hammer");
   TOOLCONT("hatchet");
   TOOL("func:hacksaw");
   TOOLN("func:welder", 70);
   TOOLCONTN("toolset_welder", 20);
   TOOL("func:shovel");
   TOOL("func:wrench");
   COMP("steel_plate", 4);
   COMP("frame", 8);
   COMP("2x4", 8);

}

void game::construction_menu()
{
    int iMaxY = TERMY;
    if (constructions.size()+2 < iMaxY) {
        iMaxY = constructions.size()+2;
    }
    if (iMaxY < FULL_SCREEN_HEIGHT) {
        iMaxY = FULL_SCREEN_HEIGHT;
    }

    WINDOW *w_con = newwin(iMaxY, FULL_SCREEN_WIDTH, (TERMY > iMaxY) ? (TERMY-iMaxY)/2 : 0, (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0);
    wborder(w_con, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                   LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
    mvwprintz(w_con, 0, 8, c_ltred, _(" Construction "));

    mvwputch(w_con,  0, 30, c_ltgray, LINE_OXXX);
    mvwputch(w_con, iMaxY-1, 30, c_ltgray, LINE_XXOX);
    for (int i = 1; i < iMaxY-1; ++i) {
        mvwputch(w_con, i, 30, c_ltgray, LINE_XOXO);
    }

    mvwprintz(w_con,  1, 31, c_white, _("Difficulty:"));

    wrefresh(w_con);

 bool update_info = true;
 int select = 0;
 int chosen = 0;
 long ch;
 bool exit = false;

 crafting_inventory_t total_inv(this, &u);

 do {
// Erase existing list of constructions
  for (int i = 1; i < iMaxY-1; i++) {
   for (int j = 1; j < 30; j++)
    mvwputch(w_con, i, j, c_black, ' ');
  }
  //Draw Scrollbar
  draw_scrollbar(w_con, select, iMaxY-2, constructions.size(), 1);
// Determine where in the master list to start printing
  //int offset = select - 11;
  int offset = 0;
  if (select >= iMaxY-2)
   offset = select - iMaxY + 3;
// Print the constructions between offset and max (or how many will fit)
  for (int i = 0; i < iMaxY-2 && (i + offset) < constructions.size(); i++) {
   int current = i + offset;
   nc_color col = (player_can_build(u, total_inv, constructions[current]) ?
                   c_white : c_dkgray);
   // Map menu items to hotkey letters, skipping j, k, l, and q.
   unsigned char hotkey = 97 + current;
   if (hotkey > 122)
    hotkey = hotkey - 58;

   if (current == select)
    col = hilite(col);
   mvwprintz(w_con, 1 + i, 1, col, "%c %s", hotkey, constructions[current]->name.c_str());
  }

  if (update_info) {
   update_info = false;
   constructable* current_con = constructions[select];
// Print difficulty
   int pskill = u.skillLevel("carpentry");
   int diff = current_con->difficulty > 0 ? current_con->difficulty : 0;
   mvwprintz(w_con, 1, 43, (pskill >= diff ? c_white : c_red),
             "%d   ", diff);
// Clear out lines for tools & materials
   for (int i = 2; i < iMaxY-1; i++) {
    for (int j = 31; j < 79; j++)
     mvwputch(w_con, i, j, c_black, ' ');
   }

// Print stages and their requirements
   int posx = 33, posy = 2;
   for (int n = 0; n < current_con->stages.size(); n++) {
     nc_color color_stage = (player_can_build(u, total_inv, current_con, n,
                             false, true) ? c_white : c_dkgray);

    const char* mes;
    if (current_con->stages[n].terrain != t_null)
      mes = _(terlist[current_con->stages[n].terrain].name.c_str()); // FIXME i18n
    else if (current_con->stages[n].furniture != f_null)
      mes = _(furnlist[current_con->stages[n].furniture].name.c_str()); // FIXME i18n
    else
      mes = "";
    posy++;
    mvwprintz(w_con, posy, 31, color_stage, _("Stage %1$d: %2$s"), n + 1, mes);
    posy++;
    mvwprintz(w_con, posy, 31, color_stage, _("Time: %1d minutes"), current_con->stages[n].time);
// Print tools
    construction_stage &stage = current_con->stages[n];
    posy++;
    posx = 33;
    for (int i = 0; i < stage.tools.size(); i++) {
     mvwprintz(w_con, posy, posx-2, c_white, ">");
     total_inv.has_any_tools(stage.tools[i]);
     for (int j = 0; j < stage.tools[i].size(); j++) {
      itype_id tool = stage.tools[i][j].type;
      nc_color col = c_red;
      if(stage.tools[i][j].available == 1) {
       col = c_green;
      }
      int length = utf8_width(item_controller->find_template(tool)->name.c_str());
      if (posx + length > FULL_SCREEN_WIDTH-1) {
       posy++;
       posx = 33;
      }
      mvwprintz(w_con, posy, posx, col, item_controller->find_template(tool)->name.c_str());
      posx += length + 1; // + 1 for an empty space
      if (j < stage.tools[i].size() - 1) { // "OR" if there's more
       if (posx > FULL_SCREEN_WIDTH-3) {
        posy++;
        posx = 33;
       }
       mvwprintz(w_con, posy, posx, c_white, _("OR"));
       posx += 3;
      }
     }
     posy ++;
     posx = 33;
    }
// Print components
    posx = 33;
    for (int i = 0; i < stage.components.size(); i++) {
     total_inv.has_any_components(stage.components[i]);
     mvwprintz(w_con, posy, posx-2, c_white, ">");
     for (int j = 0; j < stage.components[i].size(); j++) {
      nc_color col = c_red;
      component &comp = stage.components[i][j];
      if(comp.available == 1) {
       col = c_green;
      }
      int length = utf8_width(item_controller->find_template(comp.type)->name.c_str());
      if (posx + length > FULL_SCREEN_WIDTH-1) {
       posy++;
       posx = 33;
      }
      mvwprintz(w_con, posy, posx, col, "%s x%d",
                item_controller->find_template(comp.type)->name.c_str(), comp.count);
      posx += length + 3; // + 2 for " x", + 1 for an empty space
// Add more space for the length of the count
      if (comp.count < 10)
       posx++;
      else if (comp.count < 100)
       posx += 2;
      else
       posx += 3;

      if (j < stage.components[i].size() - 1) { // "OR" if there's more
       if (posx > FULL_SCREEN_WIDTH-3) {
        posy++;
        posx = 33;
       }
       mvwprintz(w_con, posy, posx, c_white, _("OR"));
       posx += 3;
      }
     }
     posy ++;
     posx = 33;
    }
   }
   wrefresh(w_con);
  } // Finished updating

  ch = getch();
  switch (ch) {
   case KEY_DOWN:
    update_info = true;
    if (select < constructions.size() - 1)
     select++;
    else
     select = 0;
    break;
   case KEY_UP:
    update_info = true;
    if (select > 0)
     select--;
    else
     select = constructions.size() - 1;
    break;
   case ' ':
   case KEY_ESCAPE:
   case 'q':
   case 'Q':
    exit = true;
    break;
   case '\n':
   default:
    if (ch > 64 && ch < 91) //A-Z
     chosen = ch - 65 + 26;

    else if (ch > 96 && ch < 123) //a-z
     chosen = ch - 97;

    else if (ch == '\n')
     chosen = select;

    if (chosen < constructions.size()) {
     if (player_can_build(u, total_inv, constructions[chosen])) {
      place_construction(constructions[chosen]);
      exit = true;
     } else {
      popup(_("You can't build that!"));
      select = chosen;
      for (int i = 1; i < iMaxY-1; i++)
       mvwputch(w_con, i, 30, c_ltgray, LINE_XOXO);
      update_info = true;
     }
    }
    break;
  }
 } while (!exit);

    for (int i = iMaxY-FULL_SCREEN_HEIGHT; i <= iMaxY; ++i) {
        for (int j = TERRAIN_WINDOW_WIDTH; j <= FULL_SCREEN_WIDTH; ++j) {
            mvwputch(w_con, i, j, c_black, ' ');
        }
    }

 wrefresh(w_con);
 refresh_all();
}

void move_ppoints_for_construction(const std::string &skillName, int difficulty, int &moves_left) {
    int skill = g->u.skillLevel(skillName);
    if(skill > 0 && skill > difficulty) {
        double skillfactor = 1.0;
        // if skill is big enough, the construction time changes
        // up to half the time (for infinit skill level)
        if(difficulty > 1.0) {
            skillfactor = static_cast<double>(difficulty) / skill;
        } else {
            skillfactor = 1.0 / skill;
        }
        // factor should be in the range (0, 1]
        moves_left /= 2;
        moves_left += static_cast<double>(moves_left) * skillfactor;
        g->add_msg("%s-factor: %f", skillName.c_str(), skillfactor);
    }
}

void move_ppoints_for_construction(constructable *con, int &moves_left) {
    move_ppoints_for_construction("carpentry", con->difficulty, moves_left);
}

bool game::player_can_build(player &p, crafting_inventory_t &crafting_inv, constructable* con,
                            const int level, bool cont, bool exact_level)
{
 int last_level = level;

 // default behavior: return true if any of the stages up to L can be constr'd
 // if exact_level, require that this level be constructable
 if (p.skillLevel("carpentry") < con->difficulty)
  return false;

 if (level < 0)
  last_level = con->stages.size();

 int start = 0;
 if (cont)
  start = level;

 bool can_build_any = false;
 for (int i = start; i < con->stages.size() && i <= last_level; i++) {
  construction_stage &stage = con->stages[i];
  const bool can_do = crafting_inv.has_all_requirements(stage);
  can_build_any |= can_do;
  if (exact_level && (i == level)) {
      return can_do;
  }
 }  // stage[i]
 return can_build_any;
}

void game::place_construction(constructable *con)
{
 refresh_all();
 crafting_inventory_t total_inv(this, &u);

 std::vector<point> valid;
 for (int x = u.posx - 1; x <= u.posx + 1; x++) {
  for (int y = u.posy - 1; y <= u.posy + 1; y++) {
   if (x == u.posx && y == u.posy)
    y++;
   construct test;
   bool place_okay = (test.*(con->able))(this, point(x, y));
   for (int i = 0; i < con->stages.size() && !place_okay; i++) {
    ter_id t = con->stages[i].terrain; furn_id f = con->stages[i].furniture;
    if ((t != t_null || f != f_null) &&
       (m.ter(x, y) == t || t == t_null) &&
       (m.furn(x, y) == f || f == f_null))
     place_okay = true;
   }

   if (place_okay) {
// Make sure we're not trying to continue a construction that we can't finish
    int starting_stage = 0, max_stage = -1;
    for (int i = 0; i < con->stages.size(); i++) {
     ter_id t = con->stages[i].terrain; furn_id f = con->stages[i].furniture;
     if ((t != t_null || f != f_null) &&
        (m.ter(x, y) == t || t == t_null) &&
        (m.furn(x, y) == f || f == f_null))
      starting_stage = i + 1;
    }

    if (starting_stage == con->stages.size() && con->loopstages)
     starting_stage = 0; // Looping stages

    for(int i = starting_stage; i < con->stages.size(); i++) {
     if (player_can_build(u, total_inv, con, i, true, true))
       max_stage = i;
     else
       break;
    }
    if (max_stage >= starting_stage) {
     valid.push_back(point(x, y));
     m.drawsq(w_terrain, u, x, y, true, false);
     wrefresh(w_terrain);
    }
   }
  }
 }
 // snip snip
 if (con->name == _("Move Furniture") ) {
   grab();
   return;
 }
 //
 int dirx, diry;
 if (!choose_adjacent(_("Contruct where?"), dirx, diry))
  return;
 bool point_is_okay = false;
 for (int i = 0; i < valid.size() && !point_is_okay; i++) {
  if (valid[i].x == dirx && valid[i].y == diry)
   point_is_okay = true;
 }
 if (!point_is_okay) {
   add_msg(_("You cannot build there!"));
   return;
 }

// Figure out what stage to start at, and what stage is the maximum
 int starting_stage = 0, max_stage = 0;
 for (int i = 0; i < con->stages.size(); i++) {
  ter_id t = con->stages[i].terrain; furn_id f = con->stages[i].furniture;
  if ((t != t_null || f != f_null) &&
     (m.ter(dirx, diry) == t || t == t_null) &&
     (m.furn(dirx, diry) == f || f == f_null))
   starting_stage = i + 1;
  if (player_can_build(u, total_inv, con, i, true))
   max_stage = i;
 }

 if (starting_stage == con->stages.size() && con->loopstages)
  starting_stage = 0; // Looping stages

 u.assign_activity(this, ACT_BUILD, con->stages[starting_stage].time * 1000, con->id);
 total_inv.gather_input(con->stages[starting_stage], u.activity);
 move_ppoints_for_construction(con, u.activity.moves_left);

 u.moves = 0;
 std::vector<int> stages;
 for (int i = starting_stage; i <= max_stage; i++)
  stages.push_back(i);
 u.activity.values = stages;
 u.activity.placement = point(dirx, diry);
}

void game::complete_construction()
{
    int stage_num = u.activity.values[0];
    constructable *built = constructions[u.activity.index];
    construction_stage &stage = built->stages[stage_num];

    u.practice(turn, "carpentry", std::max(built->difficulty, 1) * 10);

    crafting_inventory_t total_inv(this, &u);
    std::list<item> used_items;
    std::list<item> used_tools;
    total_inv.consume_gathered(stage, u.activity, used_items, used_tools);

    // Make the terrain change
    int terx = u.activity.placement.x, tery = u.activity.placement.y;
    if (stage.terrain != t_null) { m.ter_set(terx, tery, stage.terrain); }
    if (stage.furniture != f_null) { m.furn_set(terx, tery, stage.furniture); }

    // Strip off the first stage in our list...
    u.activity.values.erase(u.activity.values.begin());
    // ...and start the next one, if it exists
    if (u.activity.values.size() > 0) {
        stage_num = u.activity.values[0];
        if(total_inv.has_all_requirements(built->stages[stage_num])) {
            u.activity.moves_left = built->stages[stage_num].time * 1000;
            total_inv.gather_input(built->stages[stage_num], u.activity);
            move_ppoints_for_construction(built, u.activity.moves_left);
        } else {
            u.activity.type = ACT_NULL;
        }
    } else { // We're finished!
        u.activity.type = ACT_NULL;
    }

    // This comes after clearing the activity, in case the function interrupts
    // activities
    construct effects;
    (effects.*(built->done))(this, point(terx, tery));
}

bool construct::able_empty(game *g, point p)
{
 return (g->m.has_flag("FLAT", p.x, p.y) && !g->m.has_furn(p.x, p.y) &&
         g->is_empty(p.x, p.y) && g->m.tr_at(p.x, p.y) == tr_null);
}

bool construct::able_tree(game *g, point p)
{
 return (g->m.ter(p.x, p.y) == t_tree);
}

bool construct::able_trunk(game *g, point p)
{
 return (g->m.ter(p.x, p.y) == t_trunk);
}

bool construct::able_move(game *g, point p)
{
    return g->m.can_move_furniture( p.x, p.y, &(g->u) );
}

bool construct::able_window(game *g, point p)
{
 return (g->m.ter(p.x, p.y) == t_window_frame ||
         g->m.ter(p.x, p.y) == t_window_empty ||
         g->m.ter(p.x, p.y) == t_window_domestic ||
         g->m.ter(p.x, p.y) == t_window ||
         g->m.ter(p.x, p.y) == t_window_alarm);
}

bool construct::able_make_window(game *g, point p)
{
    return able_window(g, p) || able_empty(g, p);
}
bool construct::able_empty_window(game *g, point p)
{
 return (g->m.ter(p.x, p.y) == t_window_empty);
}

bool construct::able_window_pane(game *g, point p)
{
 return (g->m.ter(p.x, p.y) == t_window || g->m.ter(p.x, p.y) == t_window_domestic || g->m.ter(p.x, p.y) == t_window_alarm);
}

bool construct::able_broken_window(game *g, point p)
{
 return (g->m.ter(p.x, p.y) == t_window_frame);
}

bool construct::able_door(game *g, point p)
{
 return (g->m.ter(p.x, p.y) == t_door_c ||
         g->m.ter(p.x, p.y) == t_door_b ||
         g->m.ter(p.x, p.y) == t_door_o ||
         g->m.ter(p.x, p.y) == t_door_locked_interior ||
         g->m.ter(p.x, p.y) == t_door_locked);
}

bool construct::able_door_broken(game *g, point p)
{
 return (g->m.ter(p.x, p.y) == t_door_b);
}

bool construct::able_wall(game *g, point p)
{
 return (g->m.ter(p.x, p.y) == t_wall_h || g->m.ter(p.x, p.y) == t_wall_v ||
         g->m.ter(p.x, p.y) == t_wall_wood);
}

bool construct::able_wall_wood(game *g, point p)
{
 return (g->m.ter(p.x, p.y) == t_wall_wood);
}

bool construct::able_dig(game *g, point p)
{
 return (g->m.has_flag("DIGGABLE", p.x, p.y));
}

bool construct::able_chainlink(game *g, point p)
{
 return (g->m.ter(p.x, p.y) == t_chainfence_v || g->m.ter(p.x, p.y) == t_chainfence_h);
}

bool construct::able_pit(game *g, point p)
{
 return (g->m.ter(p.x, p.y) == t_pit);//|| g->m.ter(p.x, p.y) == t_pit_shallow);
}

bool construct::able_between_walls(game *g, point p)
{
    int num_supports = 0;
    if (g->m.move_cost(p.x, p.y) != 0) {
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                if (((dx == 0) ^ (dy == 0))
                        && g->m.has_flag("SUPPORTS_ROOF", p.x + dx, p.y + dy)) {
                    num_supports++;
                }
            }
        }
    }
    return num_supports >= 2;
}

bool construct::able_deconstruct(game *g, point p)
{
  return (g->m.has_flag("DECONSTRUCT", p.x, p.y));
}

void construct::done_window_pane(game *g, point p)
{
    (void)p; //unused
    g->m.spawn_item(g->u.posx, g->u.posy, "glass_sheet");
}

// STUB
void construct::done_move(game *g, point p)
{
    (void)g; (void)p; // TODO: something?
    return; // stub
}

void construct::done_tree(game *g, point p)
{
    mvprintz(0, 0, c_red, _("Press a direction for the tree to fall in:"));
    int x = 0, y = 0;
    do get_direction(x, y, input());
    while (x == -2 || y == -2);
    x = p.x + x * 3 + rng(-1, 1);
    y = p.y + y * 3 + rng(-1, 1);
    std::vector<point> tree = line_to(p.x, p.y, x, y, rng(1, 8));
    for (int i = 0; i < tree.size(); i++) {
        g->m.destroy(g, tree[i].x, tree[i].y, true);
        g->m.ter_set(tree[i].x, tree[i].y, t_trunk);
    }
}

void construct::done_trunk_log(game *g, point p)
{
    g->m.spawn_item(p.x, p.y, "log", 1, 0, g->turn, rng(5, 15));
}

void construct::done_trunk_plank(game *g, point p)
{
    (void)p; //unused
    int num_logs = rng(5, 15);
    for( int i = 0; i < num_logs; ++i ) {
        item tmplog(g->itypes["log"], int(g->turn), g->nextinv);
        iuse::cut_log_into_planks( g, &(g->u), &tmplog);
    }
}


void construct::done_vehicle(game *g, point p)
{
    std::string name = string_input_popup(_("Enter new vehicle name:"), 20);
    if(name.empty())
    {
        name = _("Car");
    }

    vehicle *veh = g->m.add_vehicle (g, "custom", p.x, p.y, 270, 0, 0);
    if (!veh)
    {
        debugmsg ("error constructing vehicle");
        return;
    }
    veh->name = name;

    //Update the vehicle cache immediately, or the vehicle will be invisible for the first couple of turns.
    g->m.update_vehicle_cache(veh, true);

}

void construct::done_tape(game *g, point p)
{
  g->add_msg(_("You tape up the %s."), g->m.tername(p.x, p.y).c_str());
  switch ( g->m.oldter(p.x, p.y) )
  {
    case old_t_window_alarm:
      g->m.ter_set(p.x, p.y, t_window_alarm_taped);

    case old_t_window_domestic:
      g->m.ter_set(p.x, p.y, t_window_domestic_taped);

    case old_t_window:
      g->m.ter_set(p.x, p.y, t_window_taped);
  }

}

void construct::done_deconstruct(game *g, point p)
{
  if (g->m.has_furn(p.x, p.y)) {
    g->add_msg(_("You disassemble the %s."), g->m.furnname(p.x, p.y).c_str());
    switch (g->m.oldfurn(p.x, p.y)){
      case old_f_makeshift_bed:
      case old_f_bed:
      case old_f_armchair:
        g->m.spawn_item(p.x, p.y, "2x4", 10);
        g->m.spawn_item(p.x, p.y, "rag", rng(10,15));
        g->m.spawn_item(p.x, p.y, "nail", 0, rng(6,8));
        g->m.furn_set(p.x, p.y, f_null);
      case old_f_bench:
      case old_f_crate_o:
      case old_f_crate_c:
      case old_f_chair:
      case old_f_cupboard:
      case old_f_desk:
      case old_f_bulletin:
        g->m.spawn_item(p.x, p.y, "2x4", 4);
        g->m.spawn_item(p.x, p.y, "nail", 0, rng(6,10));
        g->m.furn_set(p.x, p.y, f_null);
      break;
      case old_f_locker:
        g->m.spawn_item(p.x, p.y, "sheet_metal", rng(1,2));
        g->m.spawn_item(p.x, p.y, "pipe", rng(4,8));
        g->m.furn_set(p.x, p.y, f_null);
      break;
      case old_f_rack:
        g->m.spawn_item(p.x, p.y, "pipe", rng(6,12));
        g->m.furn_set(p.x, p.y, f_null);
      break;
      case old_f_oven:
        g->m.spawn_item(p.x, p.y, "scrap",       rng(2,6));
        g->m.spawn_item(p.x, p.y, "steel_chunk", rng(2,3));
        g->m.spawn_item(p.x, p.y, "element",     rng(1,4));
        g->m.spawn_item(p.x, p.y, "pilot_light", 1);
        g->m.furn_set(p.x, p.y, f_null);
      case old_f_fridge:
        g->m.spawn_item(p.x, p.y, "scrap", rng(2,6));
        g->m.spawn_item(p.x, p.y, "steel_chunk", rng(2,3));
        g->m.spawn_item(p.x, p.y, "hose", 1);
        g->m.spawn_item(p.x, p.y, "cu_pipe", rng(3, 6));

        g->m.furn_set(p.x, p.y, f_null);
      break;
      case old_f_glass_fridge:
        g->m.spawn_item(p.x, p.y, "scrap", rng(2,6));
        g->m.spawn_item(p.x, p.y, "steel_chunk", rng(2,3));
        g->m.spawn_item(p.x, p.y, "hose", 1);
        g->m.spawn_item(p.x, p.y, "glass_sheet", 1);
        g->m.spawn_item(p.x, p.y, "cu_pipe", rng(3, 6));
        g->m.furn_set(p.x, p.y, f_null);
      break;
      case old_f_counter:
      case old_f_dresser:
      case old_f_table:
        g->m.spawn_item(p.x, p.y, "2x4", 6);
        g->m.spawn_item(p.x, p.y, "nail", 0, rng(6,8));
        g->m.furn_set(p.x, p.y, f_null);
      break;
      case old_f_pool_table:
        g->m.spawn_item(p.x, p.y, "2x4", 4);
        g->m.spawn_item(p.x, p.y, "rag", 4);
        g->m.spawn_item(p.x, p.y, "nail", 0, rng(6,10));
        g->m.furn_set(p.x, p.y, f_null);
      break;
      case old_f_bookcase:
        g->m.spawn_item(p.x, p.y, "2x4", 12);
        g->m.spawn_item(p.x, p.y, "nail", 0, rng(12,16));
        g->m.furn_set(p.x, p.y, f_null);
      break;
      default:
        g->add_msg(_("You have to push away %s first."), g->m.furnname(p.x, p.y).c_str());
      break;
    }
  } else {
    g->add_msg(_("You disassemble the %s."), g->m.tername(p.x, p.y).c_str());
    switch (g->m.oldter(p.x, p.y))
    {
      case old_t_door_c:
      case old_t_door_o:
        g->m.spawn_item(p.x, p.y, "2x4", 4);
        g->m.spawn_item(p.x, p.y, "nail", 0, rng(6,12));
        g->m.ter_set(p.x, p.y, t_door_frame);
      break;
      case old_t_curtains:
      case old_t_window_domestic:
        g->m.spawn_item(g->u.posx, g->u.posy, "stick");
        g->m.spawn_item(g->u.posx, g->u.posy, "sheet", 2);
        g->m.spawn_item(g->u.posx, g->u.posy, "glass_sheet");
        g->m.spawn_item(g->u.posx, g->u.posy, "nail", 0, rng(3,4));
        g->m.spawn_item(g->u.posx, g->u.posy, "string_36", 0, 1);
        g->m.ter_set(p.x, p.y, t_window_empty);
      break;
      case old_t_window:
        g->m.spawn_item(p.x, p.y, "glass_sheet");
        g->m.ter_set(p.x, p.y, t_window_empty);
      break;
      case old_t_backboard:
        g->m.spawn_item(p.x, p.y, "2x4", 4);
        g->m.spawn_item(p.x, p.y, "nail", 0, rng(6,10));
        g->m.ter_set(p.x, p.y, t_pavement);
      break;
      case old_t_sandbox:
        g->m.spawn_item(p.x, p.y, "2x4", 4);
        g->m.spawn_item(p.x, p.y, "nail", 0, rng(6,10));
        g->m.ter_set(p.x, p.y, t_floor);
      break;
      case old_t_slide:
        g->m.spawn_item(p.x, p.y, "sheet_metal");
        g->m.spawn_item(p.x, p.y, "pipe", rng(4,8));
        g->m.ter_set(p.x, p.y, t_grass);
      break;
      case old_t_monkey_bars:
        g->m.spawn_item(p.x, p.y, "pipe", rng(6,12));
        g->m.ter_set(p.x, p.y, t_grass);
      break;
    }
  }
}
