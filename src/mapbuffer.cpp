#include "mapbuffer.h"
#include "game.h"
#include "output.h"
#include "debug.h"
#include "translations.h"
#include <fstream>
#include "savegame.h"

#include <unistd.h>

#define dbg(x) dout((DebugLevel)(x),D_MAP) << __FILE__ << ":" << __LINE__ << ": "
const int savegame_minver_map = 11;

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
 seek_map.clear();
 input_stream.close();
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

 sm->turn_last_touched = int(g->turn);
 submap_list.push_back(sm);
 submaps[p] = sm;

 return true;
}

submap* mapbuffer::lookup_submap(int x, int y, int z)
{
    dbg(D_INFO) << "mapbuffer::lookup_submap( x["<< x <<"], y["<< y <<"], z["<< z <<"])";
    const tripoint p(x, y, z);
    if (submaps.count(p) == 0) {
        if (seek_map.count(p) == 0) {
            return NULL;
        }
        const std::streampos spos = seek_map[p];
        seek_map.erase(p);
        return load_from_offset(spos, p);
    }
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
 std::stringstream mapfile;
 mapfile << world_generator->active_world->world_path << "/maps.txt";

 // move the old maps file to a temporary location (.tmp)
 input_stream.close();
 std::stringstream mapfile_tmp;
 mapfile_tmp << mapfile.str() << ".tmp";
 ::rename(mapfile.str().c_str(), mapfile_tmp.str().c_str());
 // open the old maps file again
 input_stream.open(mapfile_tmp.str().c_str());
 // store the seek_map for later and clear it
 SeekMap old_seek_map;
 old_seek_map.swap(seek_map);

 fout.open(mapfile.str().c_str());
 fout << "# version " << savegame_version << std::endl;

    JsonOut jsout(fout);
    jsout.start_object();
    jsout.member("listsize", (unsigned int)(submap_list.size()+old_seek_map.size()));

    // To keep load speedy, we're saving ints, but since these are ints
    // that will change with revisions and loaded mods, we're also
    // including a rosetta stone.
    jsout.member("terrain_key");
    jsout.start_array();
    for (int i=0; i < terlist.size(); i++) {
        jsout.write(terlist[i].id);
    }
    jsout.end_array();

    jsout.member("furniture_key");
    jsout.start_array();
    for (int i=0; i < furnlist.size(); i++) {
        jsout.write(furnlist[i].id);
    }
    jsout.end_array();

    jsout.member("trap_key");
    jsout.start_array();
    for (int i=0; i < g->traps.size(); i++) {
        jsout.write(g->traps[i]->id);
    }
    jsout.end_array();

    jsout.end_object();

    fout << std::endl;

 int num_saved_submaps = 0;
 int num_total_submaps = submap_list.size();

 for (it = submaps.begin(); it != submaps.end(); it++) {
  if (num_saved_submaps % 1000 == 0)
   popup_nowait(_("Please wait as the map saves [%d/%d]"),
                num_saved_submaps, num_total_submaps);

  seek_map[it->first] = fout.tellp();
  fout << it->first.x << " " << it->first.y << " " << it->first.z << std::endl;
  submap *sm = it->second;
  fout << sm->turn_last_touched << std::endl;
  fout << sm->temperature << std::endl;

 std::stringstream terout;
 std::stringstream radout;
 std::stringstream furnout;
 std::stringstream itemout;
 std::stringstream trapout;
 std::stringstream fieldout;
 std::stringstream graffout;
 int count = 0;
 int lastrad = -1;
 for(int j = 0; j < SEEY; j++){
     for(int i = 0; i < SEEX; i++){
         // Save terrains
         terout << int(sm->ter[i][j]) << " ";

         // Save radiation, re-examine this because it doesnt look like it works right
         int r = sm->rad[i][j];
         if (r == lastrad) {
            count++;
         } else {
            if (count) {
                radout << count << " ";
            }
            radout << r << " ";
            lastrad = r;
            count = 1;
         }

         // Save furniture
         if (sm->frn[i][j] != f_null)
         {
            furnout << "f " << i << " " << j << " " << sm->frn[i][j] << std::endl;
         }

         // Save items
         item tmp;
         for (int k = 0; k < sm->itm[i][j].size(); k++) {
             tmp = sm->itm[i][j][k];
             itemout << "I " << i << " " << j << std::endl;
             itemout << tmp.save_info() << std::endl;
             for (int l = 0; l < tmp.contents.size(); l++)
             {
                itemout << "C " << std::endl << tmp.contents[l].save_info() << std::endl;
             }
         }

         // Save traps
         if (sm->trp[i][j] != tr_null)
         {
            trapout << "T " << i << " " << j << " " << sm->trp[i][j] << std::endl;
         }

         // Save fields
         if (sm->fld[i][j].fieldCount() > 0){
            for(std::map<field_id, field_entry*>::iterator it = sm->fld[i][j].getFieldStart();
              it != sm->fld[i][j].getFieldEnd(); ++it){
                if(it->second != NULL){
                    fieldout << "F " << i << " " << j << " " << int(it->second->getFieldType()) << " " <<
                      int(it->second->getFieldDensity()) << " " << (it->second->getFieldAge()) << std::endl;
                }
            }
         }

         // Save graffiti
         if (sm->graf[i][j].contents)
         {
            graffout << "G " << i << " " << j << *sm->graf[i][j].contents << std::endl;
         }
     }
     terout << std::endl;
 }
 radout << count << std::endl;

 fout << terout.str() << radout.str() << furnout.str() << itemout.str() << trapout.str() << fieldout.str() << graffout.str();

 // Output the spawn points
  spawn_point tmpsp;
  for (int i = 0; i < sm->spawns.size(); i++) {
   tmpsp = sm->spawns[i];
   fout << "S " << (tmpsp.type) << " " << tmpsp.count << " " << tmpsp.posx <<
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


  fout << "----" << std::endl;
  num_saved_submaps++;
 }

 // Now read the old maps file and copy submaps that are not
 // yet loaded to the new maps file.
 for(SeekMap::const_iterator a = old_seek_map.begin(); a != old_seek_map.end(); ++a) {
  if(seek_map.count(a->first) == 0) {
   std::string line;
   seek_map[a->first] = fout.tellp();
   input_stream.seekg(a->second);
   while(std::getline(input_stream, line, '\n')) {
    if(line.compare(0, 4, "----") == 0 || input_stream.eof()) {
     fout << "----\n";
     break;
    }
    fout << line << "\n";
   }
  }
 }
 // Close the file; that's all we need.
 fout.close();

 input_stream.close();
 ::unlink(mapfile_tmp.str().c_str());
 input_stream.open(mapfile.str().c_str());
}

void mapbuffer::load(std::string worldname)
{
 std::ifstream fin;
 std::stringstream worldmap;
 worldmap << world_generator->all_worlds[worldname]->world_path << "/maps.txt";

 fin.open(worldmap.str().c_str());
 if (!fin.is_open())
  return;
 unserialize(fin);
 fin.close();
 input_stream.open(worldmap.str().c_str());

    if(savegame_loading_version != savegame_version) {
        // previous version, load all maps at once, to update ids
        SeekMap sm;
        sm.swap(seek_map);
        for(SeekMap::iterator a = sm.begin(); a != sm.end(); ++a) {
            load_from_offset(a->second, a->first);
        }
    }
}

void mapbuffer::unserialize(std::ifstream & fin) {
 seek_map.clear();
 input_stream.close();
 std::map<tripoint, submap*>::iterator it;
 int num_submaps = 0, num_loaded = 0;
 item it_tmp;
 std::string databuff;
 std::string st;

   if ( fin.peek() == '#' ) {
       std::string vline;
       getline(fin, vline);
       std::string tmphash, tmpver;
       int savedver=-1;
       std::stringstream vliness(vline);
       vliness >> tmphash >> tmpver >> savedver;
       if ( tmpver == "version" && savedver != -1 ) {
           savegame_loading_version = savedver;
       }
   }
   if (savegame_loading_version != savegame_version && savegame_loading_version < savegame_minver_map) { // We're version x but this is a save from version y, let's check to see if there's a loader
       if ( unserialize_legacy(fin) == true ) { // loader returned true, we're done.
            return;
       } else { // no unserialize_legacy for version y, continuing onwards towards possible disaster. Or not?
           popup_nowait(_("Cannot find loader for map save data in old version %d, attempting to load as current version %d."),savegame_loading_version, savegame_version);
       }
   }

    std::stringstream jsonbuff;
    getline(fin, databuff);
    jsonbuff.str(databuff);
    JsonIn jsin(jsonbuff);

    ter_key.clear();
    furn_key.clear();
    trap_key.clear();

    jsin.start_object();
    while (!jsin.end_object()) {
        std::string name = jsin.get_member_name();
        if (name == "listsize") {
            num_submaps = jsin.get_int();
        } else if (name == "terrain_key") {
            int i = 0;
            jsin.start_array();
            while (!jsin.end_array()) {
                std::string tstr = jsin.get_string();
                if ( termap.find(tstr) == termap.end() ) {
                    debugmsg("Can't find terrain '%s' (%d)", tstr.c_str(), i);
                } else {
                    ter_key[i] = termap[tstr].loadid;
                }
                ++i;
            }
        } else if (name == "furniture_key") {
            int i = 0;
            jsin.start_array();
            while (!jsin.end_array()) {
                std::string fstr = jsin.get_string();
                if ( furnmap.find(fstr) == furnmap.end() ) {
                    debugmsg("Can't find furniture '%s' (%d)", fstr.c_str(), i);
                } else {
                    furn_key[i] = furnmap[fstr].loadid;
                }
                ++i;
            }
        } else if (name == "trap_key") {
            int i = 0;
            jsin.start_array();
            while (!jsin.end_array()) {
                std::string trstr = jsin.get_string();
                if ( trapmap.find(trstr) == trapmap.end() ) {
                    debugmsg("Can't find trap '%s' (%d)", trstr.c_str(), i);
                } else {
                    trap_key[i] = trapmap[trstr];
                }
                ++i;
            }
        } else {
            debugmsg("unrecognized mapbuffer json member '%s'", name.c_str());
            jsin.skip_value();
        }
    }

    if (trap_key.empty()) { // old, snip when this moves to legacy
        for (int i = 0; i < num_legacy_trap; i++) {
            std::string trstr = legacy_trap_id[i];
            if ( trapmap.find( trstr ) == trapmap.end() ) {
                debugmsg("Can't find trap '%s' (%d)", trstr.c_str(), i);
                trap_key[i] = trapmap["tr_null"];
            } else { 
                trap_key[i] = trapmap[trstr];
            }
        }
    }

 while (!fin.eof()) {
  if (num_loaded % 1000 == 0)
   popup_nowait(_("Please wait as the map loads [%d/%d]"),
                num_loaded, num_submaps);
  int locx, locy, locz;
  const std::streampos spos = fin.tellg();
  fin >> locx >> locy >> locz;
  if(fin.eof()) {
      break;
  }
  seek_map[tripoint(locx, locy, locz)] = spos;
  while(std::getline(fin, databuff)) {
      if(databuff.compare(0, 4, "----") == 0 || fin.eof()) {
          break;
      }
  }
  num_loaded++;
 }
}

submap *mapbuffer::load_from_offset(std::streampos pos, const tripoint &) {
  input_stream.seekg(pos);
  std::ifstream &fin = input_stream;
  int locx, locy, locz;

 std::map<tripoint, submap*>::iterator it;
 int itx, ity, t, d, a;
 item it_tmp;
 std::string databuff;
 std::string st;
 int turn, temperature;
 
  submap* sm = new submap();
  fin >> locx >> locy >> locz >> turn >> temperature;
  sm->turn_last_touched = turn;
  sm->temperature = temperature;
  int turndif = int(g->turn) - turn;
  if (turndif < 0)
   turndif = 0;
// Load terrain
  for (int j = 0; j < SEEY; j++) {
   for (int i = 0; i < SEEX; i++) {
    int tmpter;
    fin >> tmpter;
    tmpter = ter_key[tmpter];
    sm->ter[i][j] = ter_id(tmpter);

    sm->frn[i][j] = f_null;
    sm->itm[i][j].clear();
    sm->trp[i][j] = tr_null;
    //sm->fld[i][j] = field(); //not needed now
    sm->graf[i][j] = graffiti();
   }
  }
// Load irradiation
  int radtmp;
  int count = 0;
  for (int j = 0; j < SEEY; j++) {
   for (int i = 0; i < SEEX; i++) {
    if (count == 0) {
     fin >> radtmp >> count;
     radtmp -= int(turndif / 100); // Radiation slowly decays
     if (radtmp < 0) {
      radtmp = 0;
     }
    }
    count--;
    sm->rad[i][j] = radtmp;
   }
  }
// Load items and traps and fields and spawn points and vehicles
  std::string string_identifier;
  do {
   fin >> string_identifier; // "----" indicates end of this submap
   t = 0;
   st = "";
   if (string_identifier == "I") {
    fin >> itx >> ity;
    getline(fin, databuff); // Clear out the endline
    getline(fin, databuff);
    it_tmp.load_info(databuff);
    if(it_tmp.is_null()) {
     debugmsg("found a nul item %s at %d,%d", databuff.c_str(), itx, ity);
    } else {
    sm->itm[itx][ity].push_back(it_tmp);
    if (it_tmp.active)
     sm->active_item_count++;
    }
   } else if (string_identifier == "C") {
    getline(fin, databuff); // Clear out the endline
    getline(fin, databuff);
	if(!sm->itm[itx][ity].empty()) {
    int index = sm->itm[itx][ity].size() - 1;
    it_tmp.load_info(databuff);
    sm->itm[itx][ity][index].put_in(it_tmp);
    if (it_tmp.active)
     sm->active_item_count++;
	}
   } else if (string_identifier == "T") {
    fin >> itx >> ity >> t;
    sm->trp[itx][ity] = trap_id(trap_key[t]);
   } else if (string_identifier == "f") {
    fin >> itx >> ity >> t;
    sm->frn[itx][ity] = furn_id(furn_key[t]);
   } else if (string_identifier == "F") {
    fin >> itx >> ity >> t >> d >> a;
    if(!sm->fld[itx][ity].findField(field_id(t)))
     sm->field_count++;
    sm->fld[itx][ity].addField(field_id(t), d, a);
   } else if (string_identifier == "S") {
    char tmpfriend;
    int tmpfac = -1, tmpmis = -1;
    std::string spawnname;
    fin >> st >> a >> itx >> ity >> tmpfac >> tmpmis >> tmpfriend >> spawnname;
    spawn_point tmp((st), a, itx, ity, tmpfac, tmpmis, (tmpfriend == '1'),
                    spawnname);
    sm->spawns.push_back(tmp);
   } else if (string_identifier == "V") {
    vehicle * veh = new vehicle();
    veh->load (fin);
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
  return sm;
}

int mapbuffer::size()
{
 return submap_list.size();
}
