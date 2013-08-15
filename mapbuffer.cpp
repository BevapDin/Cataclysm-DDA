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

 if (submaps.count(p) == 0) {
  submap *sm = load_submap(p);
  if(sm != 0) {
   submap_list.push_back(sm);
   submaps[p] = sm;
   return sm;
  }
  return NULL;
 }

 dbg(D_INFO) << "mapbuffer::lookup_submap success: "<< submaps[p];

 return submaps[p];
}

void mapbuffer::save_if_dirty()
{
 if(dirty)
  save();
}

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

bool is_dir_or_make_dir(const char *d) {
	struct stat st;
	if(stat(d, &st) == 0) {
		return S_ISDIR(st.st_mode);
	} else {
		return mkdir(d, 0755) == 0;
	}
}

bool mkdirp(const char *d) {
	for(char *a = (char*) d; *a != '\0'; a++) {
		if(*a != '/' || a == d) {
			continue;
		}
		*a = '\0';
		if(!is_dir_or_make_dir(d)) {
			return false;
		}
		*a = '/';
	}
	return true;
//	return is_dir_or_make_dir(d);
}

std::string replace(const std::string &haystack, const std::string &needle, const std::string &replacement) {
	std::string result = haystack;
	size_t i = 0;
	while((i = result.find(needle, i)) != std::string::npos) {
		result.replace(i, needle.length(), replacement);
		i += replacement.length();
		i -= needle.length();
	}
	return result;
}

std::string replace(const std::string &haystack, char needle, char replacement) {
	std::string result = haystack;
	for(size_t i = 0; i < result.length(); i++) {
		if(result[i] == needle) {
			result[i] = replacement;
		}
	}
	return result;
}

void mapbuffer::save_submap(const tripoint &p, submap *sm)
{
 std::ostringstream buffer;
 buffer << "save/maps/" << p.x << "/" << p.y << "," << p.z << ".txt";
 if(!mkdirp(buffer.str().c_str())) {
  debugmsg("Could not save submap %s", buffer.str().c_str());
  return;
 }
 
 ::unlink(buffer.str().c_str());
 std::ofstream fout;
 fout.open(buffer.str().c_str(), std::ios::out);

 std::map<ter_id, int> termap;
 fout << num_terrain_types << std::endl;
 for(int t = t_null; t < num_terrain_types; t++) {
  const ter_t &terx = terlist[t];
  fout << t << " ";
  fout << terx.sym << " ";
  fout << terx.name << std::endl;
  termap[(ter_id) t] = (int) t;
 }
 
 std::map<furn_id, int> furnmap;
 fout << num_furniture_types << std::endl;
 for(int t = f_null; t < num_furniture_types; t++) {
  const furn_t &furnx = furnlist[t];
  fout << t << " ";
  fout << furnx.sym << " ";
  fout << furnx.name << std::endl;
  furnmap[(furn_id) t] = (int) t;
 }

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
     fout << "f " << i << " " << j << " " << furnmap[sm->frn[i][j]] <<
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
}

void mapbuffer::save()
{
 std::map<tripoint, submap*, pointcomp>::iterator it;
 
 int num_saved_submaps = 0;
 int num_total_submaps = submap_list.size();
 for (it = submaps.begin(); it != submaps.end(); it++) {
  if (num_saved_submaps % 100 == 0)
   popup_nowait(_("Please wait as the map saves [%d/%d]"),
                num_saved_submaps, num_total_submaps);

  save_submap(it->first, it->second);
  num_saved_submaps++;
 }
 return;
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
 
 std::map<furn_id, int> furnmap;
 fout << num_furniture_types << std::endl;
 for(int t = f_null; t < num_furniture_types; t++) {
  const furn_t &furnx = furnlist[t];
  fout << t << " ";
  fout << furnx.sym << " ";
  fout << ((int) furnx.color) << " ";
  fout << ((int) furnx.movecost) << " ";
  fout << furnx.name << std::endl;
  furnmap[(furn_id) t] = (int) t;
 }

 fout << submap_list.size() << std::endl;
 
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
     fout << "f " << i << " " << j << " " << furnmap[sm->frn[i][j]] <<
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

void add_terrain(int tn, long ts, int, int, int, const std::string &tname, std::map<int, ter_id> &tmap, std::map<ter_id, int> &tbmap) {
 // Loading assumes: no reordering, no addition of items
 // between others with the same name.
 for(int t = t_null; t < num_terrain_types; t++) {
  if(tbmap.find((ter_id) t) != tbmap.end()) {
   // Already have a mapping to t
   continue;
  }
  const ter_t &terx = terlist[t];
  if(ts == terx.sym && tname == terx.name) {
   tmap[tn] = (ter_id) t;
   tbmap[(ter_id) t] = tn;
   return;
  }
 }
 if(tn >= t_null && tn < num_terrain_types) {
  if(tbmap.find((ter_id) tn) != tbmap.end()) {
   tmap[tn] = (ter_id) tn;
   tbmap[(ter_id) tn] = tn;
   return;
  } else {
   popup("Could not find unique terrain %s (id: %d)", tname.c_str(), tn);
  }
 } else {
  popup("Could not find terrain %s (id: %d)", tname.c_str(), tn);
 }
}

void add_furniture(int tn, long ts, int, int, const std::string &tname, std::map<int, furn_id> &fmap, std::map<furn_id, int> &fbmap) {
 // Loading assumes: no reordering, no addition of items
 // between others with the same name.
 for(int t = t_null; t < num_furniture_types; t++) {
  if(fbmap.find((furn_id) t) != fbmap.end()) {
   // Already have a mapping to t
   continue;
  }
  const furn_t &terx = furnlist[t];
  if(ts == terx.sym && tname == terx.name) {
   fmap[tn] = (furn_id) t;
   fbmap[(furn_id) t] = tn;
   return;
  }
 }
 if(tn >= t_null && tn < num_furniture_types) {
  if(fbmap.find((furn_id) tn) != fbmap.end()) {
   fmap[tn] = (furn_id) tn;
   fbmap[(furn_id) tn] = tn;
   return;
  } else {
   popup("Could not find unique furniture %s (id: %d)", tname.c_str(), tn);
  }
 } else {
  popup("Could not find furniture %s (id: %d)", tname.c_str(), tn);
 }
}

void mapbuffer::load()
{
	struct stat st;
	if(stat("save/maps/", &st) == 0 && S_ISDIR(st.st_mode)) {
		return;
	}
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
 std::map<ter_id, int> terbmap;
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
   add_terrain(tn, ts, tc, tm, tt, tname, termap, terbmap);
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

submap *mapbuffer::load_submap(const tripoint &p) {
 if (!master_game) {
  debugmsg("Can't load mapbuffer without a master_game");
  return NULL;
 }
 std::ostringstream buffer;
 buffer << "save/maps/" << p.x << "/" << p.y << "," << p.z << ".txt";

 std::ifstream fin;
 fin.open(buffer.str().c_str(), std::ios::in);
 if (!fin.is_open())
  return NULL;

 std::map<int, ter_id> termap;
 std::map<ter_id, int> terbmap;
 if(true) {
  int tnum;
  fin >> tnum;
  if(tnum > num_terrain_types) {
   popup("Map has more terrain types than this version knows of");
   return NULL;
  }
  for(int t = 0; t < tnum; t++) {
   int tn;
   long ts;
   std::string tname;
   fin >> tn >> ts;
   getline(fin, tname);
   const int g = tname.find_first_not_of(" ");
   if(g != 0 && g != std::string::npos) {
    tname.erase(0, g);
   }
   add_terrain(tn, ts, 0, 0, 0, tname, termap, terbmap);
  }
 } else {
  for(int t = t_null; t < num_terrain_types; t++) {
   termap[(int) t] = (ter_id) t;
  }
 }

 std::map<int, furn_id> furnmap;
 std::map<furn_id, int> furnbmap;
 if(true) {
  int fnum;
  fin >> fnum;
  if(fnum > num_furniture_types) {
   popup("Map has more furniture types than this version knows of");
   return NULL;
  }
  for(int t = 0; t < fnum; t++) {
   int tn;
   long ts;
   std::string tname;
   fin >> tn >> ts;
   getline(fin, tname);
   const int g = tname.find_first_not_of(" ");
   if(g != 0 && g != std::string::npos) {
    tname.erase(0, g);
   }
   add_furniture(tn, ts, 0, 0, tname, furnmap, furnbmap);
  }
 } else {
  for(int t = f_null; t < num_furniture_types; t++) {
   furnmap[(int) t] = (furn_id) t;
  }
 }

 int itx, ity, t, d, a;
 item it_tmp;
 std::string databuff;
 
  int turn;
  submap* sm = new submap;
  fin >> turn;
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
  while(!fin.eof()) {
   fin >> string_identifier;
   if(fin.eof()) { break; }
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
    sm->frn[itx][ity] = furnmap[t];
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
  }
  return sm;
}

int mapbuffer::size()
{
 return submap_list.size();
}
