#include "itype.h"
#include "ammo.h"
#include "game.h"
#include "monstergenerator.h"
#include <fstream>

std::vector<std::string> artifact_itype_ids;
std::vector<std::string> standard_itype_ids;

std::map<std::string, itype*> itypes;

// GENERAL GUIDELINES
// When adding a new item, you MUST REMEMBER to insert it in the itype_id enum
//  at the top of itype.h!
//  Additionally, you should check mapitemsdef.cpp and insert the new item in
//  any appropriate lists.
void game::init_itypes ()
{
// First, the null object.  NOT REALLY AN OBJECT AT ALL.  More of a concept.
 itypes["null"]=
  new itype("null", 0, "none", "", '#', c_white, "null", "null", PNULL, 0, 0, 0, 0, 0);
// Corpse - a special item
 itypes["corpse"]=
  new itype("corpse", 0, _("corpse"), _("A dead body."), '%', c_white, "null", "null", PNULL, 0, 0,
            0, 0, 1);
 itypes["corpse"]->item_tags.insert("NO_UNLOAD");
// This must -always- be set, or bad mojo in map::drawsq and whereever we check 'typeId() == "corpse" instead of 'corpse != NULL' ....
 itypes["corpse"]->corpse=GetMType("mon_null");
// Fire - only appears in crafting recipes
 itypes["fire"]=
  new itype("fire", 0, _("nearby fire"),
            "Some fire - if you are reading this it's a bug! (itypdef:fire)",
            '$', c_red, "null", "null", PNULL, 0, 0, 0, 0, 0);
// Integrated toolset - ditto
#define INTEGRATED_TOOLSET(toolname) \
 itypes["toolset_"toolname]= \
  new itype("toolset_"toolname, 0, "integrated "toolname, \
            "A fake item. If you are reading this it's a bug! (itypdef.cpp)", \
            '$', c_red, "null", "null", PNULL, 0, 0, 0, 0, 0)

            (INTEGRATED_TOOLSET("press"))->addFunctionality("func:press", 1, 0.5);
            (INTEGRATED_TOOLSET("knife"))->addFunctionality("func:blade", 1, 1);
            (INTEGRATED_TOOLSET("screwdriver"))->addFunctionality("func:screwdriver", 1, 1);
            (INTEGRATED_TOOLSET("wrench"))->addFunctionality("func:wrench", 1, 1);
            (INTEGRATED_TOOLSET("hacksaw"))->addFunctionality("func:hacksaw", 1, 1);
            (INTEGRATED_TOOLSET("hammer"))->addFunctionality("func:hammer", 1, 1);
            (INTEGRATED_TOOLSET("welder"))->addFunctionality("func:welder", 1, 0.1f);
            (INTEGRATED_TOOLSET("hotplate"))->addFunctionality("func:hotplate", 1, 0.1);
            (INTEGRATED_TOOLSET("soldering_iron"))->addFunctionality("func:soldering_iron", 1, 0.1f);
            /**
             * Note: to add a new integrated toolset type, add it here and
             * add it in crafting.cpp (game::crafting_inventory)
             * and add them at the of of this (search for BIO_SINGLE_TOOLSET)
             * */

// For smoking drugs
 itypes["apparatus"]=
  new itype("apparatus", 0, _("a smoking device and a source of flame"),
            "A fake item. If you are reading this it's a bug! (itypdef:apparatus)",
            '$', c_red, "null", "null", PNULL, 0, 0, 0, 0, 0);
// For CVD Forging
 itypes["cvd_machine"]=
  new itype("cvd_machine", 0, _("cvd machine"),
            "A fake item. If you are reading this it's a bug! (itypdef:apparatus)",
            '$', c_red, "null", "null", PNULL, 0, 0, 0, 0, 0);

// SOFTWARE
#define SOFTWARE(id, name, price, swtype, power, description) \
itypes[id]=new it_software(id, price, name, description,\
    ' ', c_white, "null", "null", 0, 0, 0, 0, 0, swtype, power)

//Macguffins
#define MACGUFFIN(id, name, price, sym, color, mat1, mat2, volume, wgt, dam, cut,\
                  to_hit, readable, function, description) \
itypes[id]=new it_macguffin(id, price, name, description,\
    sym, color, mat1, mat2, volume, wgt, dam, cut, to_hit, readable,\
    function)

SOFTWARE("software_useless", _("misc software"), 300, SW_USELESS, 0, _("\
A miscellaneous piece of hobby software. Probably useless."));

SOFTWARE("software_hacking", _("hackPRO"), 800, SW_HACKING, 2, _("\
A piece of hacking software."));

SOFTWARE("software_medical", _("MediSoft"), 600, SW_MEDICAL, 2, _("\
A piece of medical software."));

SOFTWARE("software_math", _("MatheMAX"), 500, SW_SCIENCE, 3, _("\
A piece of mathematical software."));

SOFTWARE("software_blood_data", _("infection data"), 200, SW_DATA, 5, _("\
Medical data on zombie blood."));

MACGUFFIN("note", _("note"), 0, '?', c_white, "paper", "null", 1, 3, 0, 0, 0,
    true, &iuse::mcg_note, _("\
A hand-written paper note."));

#define STATIONARY(id, name, price, category, description) \
itypes[id] = new it_stationary(id, price, name, description,\
',', c_white, "paper", "null", 0, 3, 0, 0, 0, category)

STATIONARY("flyer", _("flyer"), 1, "flier", _("A scrap of paper."));

// Finally, add all the keys from the map to a vector of all possible items
for(std::map<std::string,itype*>::iterator iter = itypes.begin(); iter != itypes.end(); ++iter){
    if(iter->first == "null" || iter->first == "corpse" || iter->first == "fire" || iter->first == "apparatus" || iter->first.compare(0, 6, "vpart_") == 0 || iter->first.compare(0, 8, "toolset_") == 0) {
        // pseudo item, do not add to standard_itype_ids
    } else {
        standard_itype_ids.push_back(iter->first);
    }
}
}

std::string ammo_name(std::string t)
{
    return ammunition_type::find_ammunition_type(t)->name();
}

itype_id default_ammo(std::string t)
{
    return ammunition_type::find_ammunition_type(t)->default_ammotype();
}
