#include "mapbuffer.h"
#include "game.h"
#include "output.h"
#include "debug.h"
#include "translations.h"
#include <fstream>

#define dbg(x) dout((DebugLevel)(x),D_MAP) << __FILE__ << ":" << __LINE__ << ": "

mapbuffer MAPBUFFER;

// g defaults to NULL
mapbuffer::mapbuffer()
{
 dirty = false;
}

mapbuffer::~mapbuffer()
{
 reset();
}

void mapbuffer::reset(){
 std::list<submap*>::iterator it;
 for (it = submap_list.begin(); it != submap_list.end(); it++)
  delete *it;

 submaps.clear();
 submap_list.clear();
}

// game g's existance does not imply that it has been identified, started, or loaded.
void mapbuffer::set_game(game *g)
{
 master_game = g;
}

// set to dirty right before the game starts & the player starts changing stuff.
void mapbuffer::set_dirty()
{
 dirty = true;
}
// initial state; no need to synchronize.
// make volatile after game has ended.
void mapbuffer::make_volatile()
{
 dirty = false;
}

bool mapbuffer::add_submap(int x, int y, int z, submap *sm)
{
 dbg(D_INFO) << "mapbuffer::add_submap( x["<< x <<"], y["<< y <<"], z["<< z <<"], submap["<< sm <<"])";

 tripoint p(x, y, z);
 if (submaps.count(p) != 0)
  return false;

 if (master_game)
  sm->turn_last_touched = int(master_game->turn);
 submap_list.push_back(sm);
 submaps[p] = sm;

 return true;
}

submap* mapbuffer::lookup_submap(int x, int y, int z)
{
 dbg(D_INFO) << "mapbuffer::lookup_submap( x["<< x <<"], y["<< y <<"], z["<< z <<"])";

 tripoint p(x, y, z);

 if (submaps.count(p) == 0)
  return NULL;

 dbg(D_INFO) << "mapbuffer::lookup_submap success: "<< submaps[p];

 return submaps[p];
}

void mapbuffer::save_if_dirty()
{
 if(dirty)
  save();
}

void mapbuffer::save()
{
 std::map<tripoint, submap*, pointcomp>::iterator it;
 std::ofstream fout;
 fout.open("save/maps.txt");

 std::map<ter_id, int> termap;
 fout << num_terrain_types << std::endl;
 for(int t = t_null; t < num_terrain_types; t++) {
  const ter_t &terx = terlist[t];
  fout << t << " ";
  fout << terx.sym << " ";
  fout << ((int) terx.color) << " ";
  fout << ((int) terx.movecost) << " ";
  fout << ((int) terx.trap) << " ";
  fout << terx.name << std::endl;
  termap[(ter_id) t] = (int) t;
 }

 fout << submap_list.size() << std::endl;
 int num_saved_submaps = 0;
 int num_total_submaps = submap_list.size();
 
 for (it = submaps.begin(); it != submaps.end(); it++) {
  if (num_saved_submaps % 100 == 0)
   popup_nowait(_("Please wait as the map saves [%d/%d]"),
                num_saved_submaps, num_total_submaps);

  fout << it->first.x << " " << it->first.y << " " << it->first.z << std::endl;
  submap *sm = it->second;
  fout << sm->turn_last_touched << std::endl;
// Dump the terrain.
  for (int j = 0; j < SEEY; j++) {
   for (int i = 0; i < SEEX; i++)
    fout << termap[sm->ter[i][j]] << " ";
   fout << "\n";
  }
 // Dump the radiation
  for (int j = 0; j < SEEY; j++) {
   for (int i = 0; i < SEEX; i++)
    fout << sm->rad[i][j] << " ";
  }
  fout << "\n";

 // Furniture
  for (int j = 0; j < SEEY; j++) {
   for (int i = 0; i < SEEX; i++) {
    if (sm->frn[i][j] != f_null)
     fout << "f " << i << " " << j << " " << sm->frn[i][j] <<
     "\n";
   }
  }
 // Items section; designate it with an I.  Then check itm[][] for each square
 //   in the grid and print the coords and the item's details.
 // Designate it with a C if it's contained in the prior item.
 // Also, this wastes space since we print the coords for each item, when we
 //   could be printing a list of items for each coord (except the empty ones)
  for (int j = 0; j < SEEY; j++) {
   for (int i = 0; i < SEEX; i++) {
    std::vector<item> &items = sm->itm[i][j];
    for (int k = 0; k < items.size(); k++) {
     item &tmp = items[k];
     if(tmp.is_null()) { continue; }
     fout << "I " << i << " " << j << std::endl;
     fout << tmp.save_info() << std::endl;
     for (int l = 0; l < tmp.contents.size(); l++)
      fout << "C " << std::endl << tmp.contents[l].save_info() << std::endl;
    }
   }
  }
 // Output the traps
  for (int j = 0; j < SEEY; j++) {
   for (int i = 0; i < SEEX; i++) {
    if (sm->trp[i][j] != tr_null)
     fout << "T " << i << " " << j << " " << sm->trp[i][j] <<
     std::endl;
   }
  }

 // Output the fields
  for (int j = 0; j < SEEY; j++) {
   for (int i = 0; i < SEEX; i++) {
    if (sm->fld[i][j].fieldCount() > 0){
     for(std::map<field_id, field_entry*>::iterator it = sm->fld[i][j].getFieldStart();
         it != sm->fld[i][j].getFieldEnd(); ++it){
      if(it->second != NULL){
       fout << "F " << i << " " << j << " " << int(it->second->getFieldType()) << " " <<
        int(it->second->getFieldDensity()) << " " << (it->second->getFieldAge()) << std::endl;
      }
     }
    }
   }
  }
 // Output the spawn points
  for (int i = 0; i < sm->spawns.size(); i++) {
   spawn_point &tmpsp = sm->spawns[i];
   fout << "S " << int(tmpsp.type) << " " << tmpsp.count << " " << tmpsp.posx <<
           " " << tmpsp.posy << " " << tmpsp.faction_id << " " <<
           tmpsp.mission_id << (tmpsp.friendly ? " 1 " : " 0 ") <<
           tmpsp.name << std::endl;
  }
 // Output the vehicles
  for (int i = 0; i < sm->vehicles.size(); i++) {
   fout << "V ";
   sm->vehicles[i]->save (fout);
  }
 // Output the computer
  if (sm->comp.name != "")
   fout << "c " << sm->comp.save_data() << std::endl;

 // Output base camp if any
  if (sm->camp.is_valid())
  	fout << "B " << sm->camp.save_data() << std::endl;

 // Output the graffiti
 for (int j = 0; j < SEEY; j++) {
  for (int i = 0; i < SEEX; i++) {
   if (sm->graf[i][j].contents)
    fout << "G " << i << " " << j << *sm->graf[i][j].contents << std::endl;
  }
 }

  fout << "----" << std::endl;
  num_saved_submaps++;
 }
 // Close the file; that's all we need.
 fout.close();
}

void add_terrain(int tn, long ts, int tc, int tm, int tt, const std::string &tname, std::map<int, ter_id> &tmap) {
 if(tn >= t_null && tn < num_terrain_types) {
  const ter_t &terx = terlist[tn];
  if(ts == terx.sym && tc == ((int) terx.color) && tm == ((int) terx.movecost) && tt == ((int) terx.trap) && tname == terx.name) {
   tmap[tn] = (ter_id) tn;
  } else if(ts == terx.sym && tname == terx.name) {
   tmap[tn] = (ter_id) tn;
  }
  return;
 }
 for(int t = t_null; t < num_terrain_types; t++) {
  const ter_t &terx = terlist[t];
  if(ts == terx.sym && tc == ((int) terx.color) && tm == ((int) terx.movecost) && tt == ((int) terx.trap) && tname == terx.name) {
   tmap[tn] = (ter_id) t;
  } else if(ts == terx.sym && tname == terx.name) {
   tmap[tn] = (ter_id) t;
  }
  return;
 }
 if(tmap.find(tn) != tmap.end()) {
  tmap[tn] = (ter_id) tn;
  popup("Could not find terrain %s (id: %d)", tname.c_str(), tn);
 } else {
  popup("Could not find terrain %s (id: %d), id already used", tname.c_str(), tn);
 }
}

void mapbuffer::load()
{
 if (!master_game) {
  debugmsg("Can't load mapbuffer without a master_game");
  return;
 }
 std::map<tripoint, submap*>::iterator it;
 std::ifstream fin;
 fin.open("save/maps.txt");
 if (!fin.is_open())
  return;

 std::map<int, ter_id> termap;
 if(true) {
  int tnum;
  fin >> tnum;
  if(tnum > num_terrain_types) {
   popup("Map has more terrain types than this version knows of");
   return;
  }
  for(int t = 0; t < tnum; t++) {
   int tn, tc, tm, tt;
   long ts;
   std::string tname;
   fin >> tn >> ts >> tc >> tm >> tt;
   getline(fin, tname);
   const int g = tname.find_first_not_of(" ");
   if(g != 0 && g != std::string::npos) {
    tname.erase(0, g);
   }
   add_terrain(tn, ts, tc, tm, tt, tname, termap);
  }
 } else {
  for(int t = t_null; t < num_terrain_types; t++) {
   termap[(int) t] = (ter_id) t;
  }
 }

 int itx, ity, t, d, a, num_submaps, num_loaded = 0;
 item it_tmp;
 std::string databuff;
 fin >> num_submaps;

 while (!fin.eof()) {
  if (num_loaded % 100 == 0)
   popup_nowait(_("Please wait as the map loads [%d/%d]"),
                num_loaded, num_submaps);
  int locx, locy, locz, turn;
  submap* sm = new submap;
  fin >> locx >> locy >> locz >> turn;
  sm->turn_last_touched = turn;
  int turndif = (master_game ? int(master_game->turn) - turn : 0);
  if (turndif < 0)
   turndif = 0;
// Load terrain
  for (int j = 0; j < SEEY; j++) {
   for (int i = 0; i < SEEX; i++) {
    int tmpter;
    fin >> tmpter;
    sm->ter[i][j] = termap[tmpter];
    sm->frn[i][j] = f_null;
    sm->itm[i][j].clear();
    sm->trp[i][j] = tr_null;
    //sm->fld[i][j] = field(); //not needed now
    sm->graf[i][j] = graffiti();
   }
  }
// Load irradiation
  for (int j = 0; j < SEEY; j++) {
   for (int i = 0; i < SEEX; i++) {
    int radtmp;
    fin >> radtmp;
    radtmp -= int(turndif / 100);	// Radiation slowly decays
    if (radtmp < 0)
     radtmp = 0;
    sm->rad[i][j] = radtmp;
   }
  }
// Load items and traps and fields and spawn points and vehicles
  std::string string_identifier;
  do {
   fin >> string_identifier; // "----" indicates end of this submap
   t = 0;
   if (string_identifier == "I") {
    fin >> itx >> ity;
    getline(fin, databuff); // Clear out the endline
    getline(fin, databuff);
    it_tmp.load_info(databuff, master_game);
    sm->itm[itx][ity].push_back(it_tmp);
    if (it_tmp.active)
     sm->active_item_count++;
   } else if (string_identifier == "C") {
    getline(fin, databuff); // Clear out the endline
    getline(fin, databuff);
    int index = sm->itm[itx][ity].size() - 1;
    it_tmp.load_info(databuff, master_game);
    sm->itm[itx][ity][index].put_in(it_tmp);
    if (it_tmp.active)
     sm->active_item_count++;
   } else if (string_identifier == "T") {
    fin >> itx >> ity >> t;
    sm->trp[itx][ity] = trap_id(t);
   } else if (string_identifier == "f") {
    fin >> itx >> ity >> t;
    sm->frn[itx][ity] = furn_id(t);
   } else if (string_identifier == "F") {
    fin >> itx >> ity >> t >> d >> a;
	if(!sm->fld[itx][ity].findField(field_id(t)))
		sm->field_count++;
    sm->fld[itx][ity].addField(field_id(t), d, a);
   } else if (string_identifier == "S") {
    char tmpfriend;
    int tmpfac = -1, tmpmis = -1;
    std::string spawnname;
    fin >> t >> a >> itx >> ity >> tmpfac >> tmpmis >> tmpfriend >> spawnname;
    spawn_point tmp(mon_id(t), a, itx, ity, tmpfac, tmpmis, (tmpfriend == '1'),
                    spawnname);
    sm->spawns.push_back(tmp);
   } else if (string_identifier == "V") {
    vehicle * veh = new vehicle(master_game);
    veh->load (fin);
    //veh.smx = gridx;
    //veh.smy = gridy;
    master_game->m.vehicle_list.insert(veh);
    sm->vehicles.push_back(veh);
   } else if (string_identifier == "c") {
    getline(fin, databuff);
    sm->comp.load_data(databuff);
   } else if (string_identifier == "B") {
    getline(fin, databuff);
    sm->camp.load_data(databuff);
   } else if (string_identifier == "G") {
     std::string s;
    int j;
    int i;
    fin >> j >> i;
    getline(fin,s);
    sm->graf[j][i] = graffiti(s);
   }
  } while (string_identifier != "----" && !fin.eof());

  submap_list.push_back(sm);
  submaps[ tripoint(locx, locy, locz) ] = sm;
  num_loaded++;
 }
 fin.close();
}

int mapbuffer::size()
{
 return submap_list.size();
}
