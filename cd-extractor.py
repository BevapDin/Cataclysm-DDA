#! /usr/bin/python3

import os
import os.path
import json
import shutil
import cgi

OUTPUT_PATH = "/tmp/cata/"
ITEM_TYPE_TYPES = [ "GENERIC", "AMMO", "BOOK", "COMESTIBLE", "CONTAINER", "ARMOR_CONTAINER",
                    "BIONIC_ITEM", "VAR_VEH_PART", "ARMOR", "GUN", "GUNMOD", "TOOL", "TOOL_ARMOR" ]
TYPES_TO_IGNORE = [ "overmap_terrain", "BULLET_PULLING", "dream", "mutation", "mapgen",
                    "monstergroup", "region_settings", "effect_type", "tutorial_messages", "SPECIES",
                    "MONSTER", "hint", "speech", "skill", "ITEM_CATEGORY", "technique",
                    "profession", "start_location", "overmap_special", "npc", "faction", "recipe_category",
                    "lab_note", "martial_art", "bionic", "snippet", "vehicle", "scenario", "item_action",
                    "MONSTER_FACTION", "MOD_INFO", "ITEM_BLACKLIST", "MONSTER_BLACKLIST",
                    "MONSTER_WHITELIST", "mutation_category" ]

# First create some output directories in one go. If this fails, the script is aborted.
for d in [ "items", "qualities", "ammo_types", "vparts", "materials", "furnitures", "terrains",
           "constructions", "traps", "gunmod_locations", "mods" ]:
    d = os.path.join( OUTPUT_PATH, d )
    if not os.path.exists( d ):
        os.makedirs( d )

# Copy the static files, if it fails, the script is aborted.
for f in [ "THE-style-sheet.css", "THE-java-script.js" ]:
    if os.path.exists( f ):
        shutil.copyfile( f, os.path.join( OUTPUT_PATH, f ) )

def scandir( dir ):
    result = []
    for f in os.listdir( dir ):
        f = os.path.join( dir, f )
        if os.path.isdir( f ):
            result.extend( scandir( f ) )
            continue
        if f[-5:] != ".json":
            continue
        result.append( f )
    return result

def write_html_header( fp, title, otype ):
    title = cgi.escape( title, True )
    fp.write( "<!DOCTYPE html>\n" )
    fp.write( "<html>\n" )
    fp.write( "<head>\n" )
    fp.write( "<title>" + title + "</title>\n" )
    fp.write( "<link rel=\"stylesheet\" type=\"text/css\" href=\"../THE-style-sheet.css\" />\n" )
    fp.write( "<script src=\"../THE-java-script.js\"></script>\n" )
    fp.write( "</head>\n" )
    # assumes that otype is already a proper identifier
    fp.write( "<body class=\"" + otype + "\" onload=\"close_all(this)\">\n" )
    fp.write( "<h1>" + title + "</h1>\n" )

def dump_json( fp, obj, css_class ):
    fp.write( "<table class=\"" + css_class + "\">\n" )
    for x in obj:
        fp.write( "<tr>\n<td>" + cgi.escape( x ) + "</td>\n<td>" + cgi.escape( str( obj[ x ] ) ) + "</td>\n</tr>\n" )
    fp.write( "</table>\n")

def open_close( text ):
    return "<a href=\"#\" onclick=\"javascript:return oc(this)\">" + text + "</a>";

def scanfiles( files, mod = None ):
    for f in files:
        scanfile( f, mod )

def scanmods( files ):
    for f in files:
        if not f[-12:] == "modinfo.json":
            continue
        scanmod( f )

def scanmod( file ):
    with open( file, 'r' ) as fp:
        try:
            jdata = json.load( fp )
        except UnicodeDecodeError as e:
            print("Failed to load " + file + ": " + str(e))
            return
    if type( jdata ) is list:
        for obj in jdata:
            if obj[ "type" ] == "MOD_INFO":
                dummy_load( mod( obj, file ) )
    else:
        dummy_load( mod( jdata, file ) )

def scanfile( file, mod ):
    with open( file, 'r' ) as fp:
        try:
            jdata = json.load( fp )
        except UnicodeDecodeError as e:
            print("Failed to load " + file + ": " + str(e))
            return
    if type( jdata ) is list:
        for obj in jdata:
            load_obj( obj, mod )
    else:
        load_obj( jdata, mod )

def unknown( iid ):
    return "<span class=\"unknown-id\">" + cgi.escape( iid ) + "</span>"

def xname( obj, iid ):
    if iid in obj.types:
        return obj.types[ iid ].link()
    else:
        return unknown( iid )

def iname( iid ):
    return xname( item, iid )

def mname( iid ):
    return xname( material, iid )

def qname( iid ):
    return xname( quality, iid )

def aname( iid ):
    return xname( ammotype, iid )

def vpname( iid ):
    return xname( vehicle_part, iid )

def cntname( arr ):
    if arr[1] == -1:
        return iname( arr[0] )
    else:
        return iname( arr[0] ) + " (" + str( arr[1] ) + ")"

def qualname( arr ):
    return qname( arr[ "id" ] ) + " [" + str( arr[ "level" ] ) + "]"

def concat( arr, sep, callback ):
    if len( arr ) == 0:
        return ""
    result = ""
    for x in arr:
        if not result == "":
            result += sep
        result += callback( x )
    return result

def orify( arr ):
    return concat( arr, " or ", cntname )

def andify( arrarr ):
    if len( arrarr ) == 0:
        return ""
    return "<ul><li>" + concat( arrarr, "</li><li>and ", orify ) + "</li></ul>"

class ammotype:
    types = { }
    ammodata = [ "name", "range", "dispersion", "recoil", "damage" ]
    def __init__(self, obj, mod):
        self.iid = obj[ "id" ]
        self.mod = mod
        self.default = obj[ "default" ]
        self.name = obj[ "name" ]
        self.usage = []
    def link( self ):
        return "<a href=\"../ammo_types/" + self.iid + ".html\">" + cgi.escape( self.name ) + "</a>"
    def write_row( self, fp, a ):
        itm = a.obj
        for x in self.ammodata:
            if x == "name":
                fp.write( "<td>" + a.link() + "</td>\n" )
            elif x in itm:
                fp.write( "<td>" + str( itm[ x ] ) + "</td>\n" )
            else:
                fp.write( "<td>&nbsp;</td>\n" )
    def dump( self ):
        path = os.path.join( OUTPUT_PATH, "ammo_types", self.iid + ".html" )
        with open( path, 'w' ) as fp:
            write_html_header( fp, "Ammo type " + self.name, "ammo" )
            if not self.mod is None:
                fp.write( "Mod: " + self.mod.link() + "<br/>\n" )
            fp.write( "Used by:\n<table>")
            fp.write( "<tr>\n" )
            for x in self.ammodata:
                fp.write( "<td>" + x + "</td>\n" )
            fp.write( "</tr>\n" )
            for a in self.usage:
                fp.write( "<tr>\n" )
                self.write_row( fp, a )
                fp.write( "</tr>\n" )
            fp.write( "</table>\n")
            fp.write( "</body>\n</html>\n")
    def crossref( self ):
        if not self.mod is None:
            self.mod.objects.append( self )

class furniture:
    types = { }
    def __init__(self, obj, mod):
        self.iid = obj[ "id" ]
        self.mod = mod
        self.name = obj[ "name" ]
        self.deconstruct = []
        self.bash = []
        self.obj = obj
        if "deconstruct" in obj:
            d = obj[ "deconstruct" ]
            if "items" in d:
                d = d[ "items" ]
                for a in d:
                    self.deconstruct.append( a[ "item" ] )
        if "bash" in obj:
            d = obj[ "bash" ]
            if "items" in d:
                d = d[ "items" ]
                for a in d:
                    self.bash.append( a[ "item" ] )
    def dump( self ):
        path = os.path.join( OUTPUT_PATH, "furnitures", self.iid + ".html" )
        with open( path, 'w' ) as fp:
            write_html_header( fp, "Furniture " + self.name, "furniture" )
            if not self.mod is None:
                fp.write( "Mod: " + self.mod.link() + "<br/>\n" )
            dump_json( fp, self.obj, "furniture-data" )
            fp.write( "Deconstructs into:<ul>\n" )
            for v in self.deconstruct:
                fp.write( "<li>" + iname( v ) + "</li>\n" )
            fp.write( "</ul>\n" )
            fp.write( "Bashed into:<ul>\n" )
            for v in self.bash:
                fp.write( "<li>" + iname( v ) + "</li>\n" )
            fp.write( "</ul>\n" )
            fp.write( "Constructed:<ul>\n" )
            for c in construction.types:
                if c.post_terrain == self.iid:
                    fp.write( "<li>" + open_close( c.desc ) + "\n" )
                    c.write_line( fp )
                    fp.write( "</li>" )
            fp.write( "</ul>\n" )
            if "crafting_pseudo_item" in self.obj:
                fp.write( "<div>Acts as a " + iname( self.obj["crafting_pseudo_item"] ) + " when crafting</div>\n" )
            fp.write( "</body>\n</html>\n")
    def link( self ):
        return "<a href=\"../furnitures/" + self.iid + ".html\">" + cgi.escape( self.name ) + "</a>"
    def crossref( self ):
        for i in self.deconstruct:
            if i in item.types:
                item.types[ i ].deconstruct.append( self )
        for i in self.bash:
            if i in item.types:
                item.types[ i ].bash.append( self )
        if not self.mod is None:
            self.mod.objects.append( self )

class terrain:
    types = { }
    def __init__(self, obj, mod):
        self.iid = obj[ "id" ]
        self.mod = mod
        self.name = obj[ "name" ]
        self.deconstruct = []
        self.bash = []
        self.obj = obj
        if "deconstruct" in obj:
            d = obj[ "deconstruct" ]
            if "items" in d:
                d = d[ "items" ]
                for a in d:
                    self.deconstruct.append( a[ "item" ] )
        if "bash" in obj:
            d = obj[ "bash" ]
            if "items" in d:
                d = d[ "items" ]
                for a in d:
                    self.bash.append( a[ "item" ] )
    def dump( self ):
        path = os.path.join( OUTPUT_PATH, "terrains", self.iid + ".html" )
        with open( path, 'w' ) as fp:
            write_html_header( fp, "Terrain " + self.name, "terrain" )
            if not self.mod is None:
                fp.write( "Mod: " + self.mod.link() + "<br/>\n" )
            dump_json( fp, self.obj, "terrain-data" )
            fp.write( "Deconstructs into:<ul>\n" )
            for v in self.deconstruct:
                fp.write( "<li>" + iname( v ) + "</li>\n" )
            fp.write( "</ul>\n" )
            fp.write( "Bashed into:<ul>\n" )
            for v in self.bash:
                fp.write( "<li>" + iname( v ) + "</li>\n" )
            fp.write( "</ul>\n" )
            fp.write( "Constructed:<ul>\n" )
            for c in construction.types:
                if c.post_terrain == self.iid:
                    fp.write( "<li>" + open_close( c.desc ) + "\n" )
                    c.write_line( fp )
                    fp.write( "</li>" )
            fp.write( "</ul>\n" )
            fp.write( "</body>\n</html>\n")
    def link( self ):
        return "<a href=\"../terrains/" + self.iid + ".html\">" + cgi.escape( self.name ) + "</a>"
    def crossref( self ):
        for i in self.deconstruct:
            if i in item.types:
                item.types[ i ].deconstruct.append( self )
        for i in self.bash:
            if i in item.types:
                item.types[ i ].bash.append( self )
        if not self.mod is None:
            self.mod.objects.append( self )

class item_group:
    types = { }
    def __init__(self, obj, mod):
        self.iid = obj[ "id" ]
        self.mod = mod
        self.content = []
        if "items" in obj:
            i = obj[ "items" ]
#            if type( i ) is dict:
            for a in i:
                if type( a ) is list:
                    self.content.append( a[0] )
                else:
                    self.content.append( a[ "item" ] )
        if "entries" in obj:
            i = obj[ "entries" ]
            for a in i:
                if "item" in a:
                    self.content.append( a[ "item" ] )
    def has_item( self, iid ):
        for a in self.content:
            if a == iid:
                return True
        return False
    def link( self ):
        return cgi.escape( self.iid )
    def crossref( self ):
        True
#        if not self.mod is None:
#            self.mod.objects.append( self )

class mod:
    types = { }
    def __init__(self, obj, file):
        self.iid = obj[ "ident" ]
        self.name = obj[ "name" ]
        self.objects = []
        if "path" in obj:
            path = obj[ "path" ]
            if path == "":
                scanfiles( scandir( file[:-12] ), self )
            elif path == "modinfo.json":
                scanfiles( [ file ], self )
            else:
                print("Unknown path: " + path + " in mod " + self.name)
        else:
            print("Missing path in mod " + self.name)
    def link( self ):
        return "<a href=\"../mods/" + self.iid + ".html\">" + cgi.escape( self.name ) + "</a>"
    def dump( self ):
        path = os.path.join( OUTPUT_PATH, "mods", self.iid + ".html" )
        with open( path, 'w' ) as fp:
            write_html_header( fp, "Mod " + self.name, "mod" )
            fp.write( "Contains:\n<ul>\n" )
            for o in self.objects:
                fp.write( "<li>" + o.link() + "</li>\n" )
            fp.write( "</ul>\n" )
            fp.write( "</body>\n</html>\n")
    def crossref( self ):
        True # currently nothing.

class material:
    types = { }
    def __init__(self, obj, mod):
        self.iid = obj[ "ident" ]
        self.mod = mod
        self.name = obj[ "name" ]
        self.usage = []
        self.obj = obj
        if "salvage_id" in obj:
            self.salvage_id = obj[ "salvage_id" ]
        else:
            self.salvage_id = False
    def link( self ):
        return "<a href=\"../materials/" + self.iid + ".html\">" + cgi.escape( self.name ) + "</a>"
    def dump( self ):
        path = os.path.join( OUTPUT_PATH, "materials", self.iid + ".html" )
        with open( path, 'w' ) as fp:
            write_html_header( fp, "Material " + self.name, "material" )
            if not self.mod is None:
                fp.write( "Mod: " + self.mod.link() + "<br/>\n" )
            dump_json( fp, self.obj, "meterial-data" )
            if self.salvage_id:
                fp.write( "salvaged into " + iname( self.salvage_id ) + "<br/>\n" )
            fp.write( "Part of:\n<ul>\n" )
            for a in self.usage:
                if len( a.material ) > 1:
                    fp.write( "<li>" + a.link() + " (and other materials)</li>\n" )
                else:
                    fp.write( "<li>" + a.link() + "</li>\n" )
            fp.write( "</ul>\n" )
            fp.write( "</body>\n</html>\n")
    def crossref( self ):
        if not self.mod is None:
            self.mod.objects.append( self )

class gunmod_location:
    types = { }
    def __init__(self, iid):
        self.iid = iid
        self.name = iid
        self.guns = []
        self.mods = []
    def link( self ):
        return "<a href=\"../gunmod_locations/" + self.iid + ".html\">" + cgi.escape( self.name ) + "</a>"
    def dump( self ):
        path = os.path.join( OUTPUT_PATH, "gunmod_locations", self.iid + ".html" )
        with open( path, 'w' ) as fp:
            write_html_header( fp, "Gunmod location " + self.iid, "material" )
            fp.write( "Guns having this:<ul>\n" );
            for a in self.guns:
                fp.write( "<li>" + a.link() + "</li>\n" )
            fp.write( "</ul>\n" )
            fp.write( "Gunmods using this:<ul>\n" );
            for a in self.mods:
                fp.write( "<li>" + a.link() + "</li>\n" )
            fp.write( "</ul>\n" )
            fp.write( "</body>\n</html>\n")
    def crossref( self ):
        True # currently nothing.

class vehicle_part:
    types = { }
    def __init__(self, obj, mod):
        self.iid = obj[ "id" ]
        self.mod = mod
        self.item = obj[ "item" ]
        self.name = obj[ "name" ]
        self.obj = obj
        self.breaks_into = [];
        if "breaks_into" in obj:
            for i in obj[ "breaks_into" ]:
                self.breaks_into.append( i[ "item" ] )
    def link( self ):
        return "<a href=\"../vparts/" + self.iid + ".html\">" + cgi.escape( self.name ) + "</a>"
    def dump( self ):
        path = os.path.join( OUTPUT_PATH, "vparts", self.iid + ".html" )
        with open( path, 'w' ) as fp:
            write_html_header( fp, "Vehicle part " + self.name, "vpart" )
            if not self.mod is None:
                fp.write( "Mod: " + self.mod.link() + "<br/>\n" )
            dump_json( fp, self.obj, "vpart-data" )
            fp.write( "Made from " + iname( self.item ) + "<br>\n" )
            fp.write( "Breaks into:<ul>\n" )
            for x in self.breaks_into:
                fp.write( "<li>" + iname( x ) + "</li>\n" )
            fp.write( "</ul>\n" )
            fp.write( "</body>\n</html>\n")
    def crossref( self ):
        for i in self.breaks_into:
            if i in item.types:
                item.types[ i ].broken_vparts.append( self )
        if self.item in item.types:
                item.types[ self.item ].the_vparts.append( self )
        if not self.mod is None:
            self.mod.objects.append( self )

class recipe:
    types = []
    def __init__(self, obj, mod):
        self.result = obj["result"]
        self.iid = self.result;
        if "id_suffix" in obj:
            self.iid += obj[ "id_suffix" ]
        self.mod = mod
        self.byproducts = []
        if "byproducts" in obj:
            by = obj[ "byproducts" ];
            if type( by ) is str:
                self.byproducts.append( { "id": by, "charges_mult": 1, "amount": 1 } )
            elif type( by ) is dict:
                if not "charges_mult" in by:
                    by[ "charges_mult" ] = 1
                if not "amount" in by:
                    by[ "amount" ] = 1
                self.byproducts.append( by )
            else:
                for b in by:
                    if type( b ) is str:
                        self.byproducts.append( { "id": b, "charges_mult": 1, "amount": 1 } )
                    else:
                        if not "charges_mult" in b:
                            b[ "charges_mult" ] = 1
                        if not "amount" in b:
                            b[ "amount" ] = 1
                        self.byproducts.append( b )
        if "charges_mult" in obj:
            self.charges_mult = obj["charges_mult"]
        else:
            self.charges_mult = 1
        if "reversible" in obj:
            self.reversible = obj["reversible"]
        else:
            self.reversible = False
        if "autolearn" in obj:
            self.autolearn = obj["autolearn"]
        else:
            self.autolearn = False
        if "book_learn" in obj:
            self.book_learn = obj["book_learn"]
        else:
            self.book_learn = []
        self.components = obj["components"]
        if "tools" in obj:
            self.tools = obj["tools"]
        else:
            self.tools = []
        if "qualities" in obj:
            self.qualities = obj["qualities"]
        else:
            self.qualities = []
        self.volume = [ 0, 0 ]
        self.weight = [ 0, 0 ]

    def uncraft_only( self ):
        return self.reversible and len( self.book_learn ) == 0 and not self.autolearn

    def is_result( self, iid ):
        if self.result == iid:
            return True
        for b in self.byproducts:
            if b[ "id" ] == iid:
                return True
        return False
    def is_dissasemble_result( self, iid ):
        if not self.reversible:
            return False
        uo = self.uncraft_only();
        for a in self.components:
            if uo:
                if a[0][0] == iid:
                    return True
            else:
                for b in a:
                    if b[0] == iid:
                        return True
        return False

    def is_item_usage( self, iid ):
        for a in self.components:
            for b in a:
                if b[0] == iid:
                    return True
        return False

    def is_tool_usage( self, iid ):
        for a in self.tools:
            for b in a:
                if b[0] == iid:
                    return True
        return False

    def is_quality_usage( self, iid ):
        for a in self.qualities:
            if a["id"] == iid:
                return True
        return False

    def write_line( self, fp ):
        fp.write( "<ul>\n" )
        if len( self.tools ) > 0:
            fp.write( "<li>tools:\n" )
            fp.write( andify( self.tools ) )
            fp.write( "</li>\n" )
        fp.write( "<li>components (" )
        if self.volume[0] == self.volume[1]:
            fp.write( "v: " + str( self.volume[0] ) + " " )
        else:
            fp.write( "v: " + str( self.volume[0] ) + "/" + str( self.volume[1] ) + " " )
        if self.volume[0] == self.volume[1]:
            fp.write( "w: " + str( self.weight[0] ) )
        else:
            fp.write( "w: " + str( self.weight[0] ) + "/" + str( self.weight[1] ) )
        fp.write( "):\n" )
        fp.write( andify( self.components ) )
        fp.write( "</li>\n" )
        if len( self.qualities ) > 0:
            fp.write( "<li>qualities:\n" )
            fp.write( "<ul>\n<li>\n" )
            fp.write( concat( self.qualities, " and</li><li>", qualname ) )
            fp.write( "</li>\n</ul>\n" )
            fp.write( "</li>\n" )
        for b in self.book_learn:
            fp.write( "<li>learned from " + iname( b[0] ) + "</li>\n" )
        if self.autolearn:
            fp.write( "<li>learned by intuition</li>\n" )
        fp.write( "</ul>\n" )
    def crossref( self ):
        for c in self.components:
            v = [ 999999, 0 ]
            w = [ 999999, 0 ]
            for e in c:
                if e[0] in item.types:
                    i = item.types[ e[0] ]
                    v[0] = min(v[0], i.volume(e[1]))
                    v[1] = max(v[1], i.volume(e[1]))
                    w[0] = min(w[0], i.weight * e[1])
                    w[1] = max(w[1], i.weight * e[1])
            self.volume[0] += v[0]
            self.volume[1] += v[1]
            self.weight[0] += w[0]
            self.weight[1] += w[1]

class construction:
    types = []
    def __init__(self, obj, mod):
        self.desc = obj["description"]
        self.mod = mod
        if "pre_terrain" in obj:
            self.pre_terrain = obj["pre_terrain"]
        else:
            self.pre_terrain = None
        if "post_terrain" in obj:
            self.post_terrain = obj["post_terrain"]
        else:
            self.post_terrain = None
        if "components" in obj:
            self.components = obj["components"]
        else:
            self.components = []
        if "tools" in obj:
            self.tools = obj["tools"]
        else:
            self.tools = []
        if "qualities" in obj:
            self.qualities = obj["qualities"]
        else:
            self.qualities = []
    def is_item_usage( self, iid ):
        for a in self.components:
            for b in a:
                if b[0] == iid:
                    return True
        return False
    def is_tool_usage( self, iid ):
        for a in self.tools:
            for b in a:
                if b[0] == iid:
                    return True
        return False
    def is_quality_usage( self, iid ):
        for a in self.qualities:
            if type( a ) is dict:
                if a["id"] == iid:
                    return True
            else:
                for b in a:
                    if b["id"] == iid:
                        return True
        return False
    def result( self ):
        r = self.desc;
        if not self.post_terrain is None:
            if self.post_terrain in terrain.types:
                r = r + " (" + terrain.types[ self.post_terrain ].link() + ")"
            elif self.post_terrain in furniture.types:
                r = r + " (" + furniture.types[ self.post_terrain ].link() + ")"
        return r
    def write_line( self, fp ):
        fp.write( "<ul>\n" )
        if len( self.tools ) > 0:
            fp.write( "<li>tools:\n" )
            fp.write( andify( self.tools ) )
            fp.write( "</li>\n" )
        fp.write( "<li>components:\n" )
        fp.write( andify( self.components ) )
        fp.write( "</li>\n" )
        if len( self.qualities ) > 0:
            fp.write( "<li>qualities:\n" )
            fp.write( "<ul>\n" )
            for a in self.qualities:
                if type( a ) is dict:
                    fp.write( "<li>" + qualname( a ) + "</li>\n" )
                else:
                    fp.write( "<li>" + concat( a, " or ", qualname ) + "</li>" )
            fp.write( "</ul>\n" )
            fp.write( "</li>\n" )
        fp.write( "</ul>\n" )
    def crossref( self ):
        True

class quality:
    types = { }
    def __init__( self, obj, mod ):
        self.iid = obj[ "id" ]
        self.name = obj[ "name" ]
        self.mod = mod
    def link( self ):
        return "<a href=\"../qualities/" + self.iid + ".html\">" + cgi.escape( self.name ) + "</a>"
    def dump( self ):
        path = os.path.join( OUTPUT_PATH, "qualities", self.iid + ".html" )
        with open( path, 'w' ) as fp:
            write_html_header( fp, "Quality " + self.name, "quality" )
            if not self.mod is None:
                fp.write( "Mod: " + self.mod.link() + "<br/>\n" )
            fp.write( "Provided by:\n<ul>")
            provided = []
            for i in item.types:
                q = item.types[ i ].get_quality( self.iid )
                if type( q ) is int:
                    provided.append( [ q, i ] )
            provided.sort( key = lambda x: x[0] )
            for i in provided:
                fp.write( "<li>" + iname( i[ 1 ] ) + " [" + str( i[ 0 ] ) + "]</li>" )
            fp.write( "</ul>\n")
            fp.write( "<div class=\"usage\">Usage:\n<ul>\n" )
            for r in recipe.types:
                if not r.is_quality_usage( self.iid ):
                    continue
                fp.write("<li>")
                if r.uncraft_only():
                    fp.write( "to dissasemble " + iname( r.result ) )
                elif r.reversible:
                    fp.write( "to make and dissasemble " + iname( r.result ) )
                else:
                    fp.write( "to make " + iname( r.result ) )
                fp.write("</li>")
            for c in construction.types:
                if c.is_quality_usage( self.iid ):
                    fp.write( "<li>" + open_close( "to construct" ) + " " + c.result() + "</li>\n" )
            fp.write( "</ul>\n</div>\n" )
            fp.write( "</body>\n</html>\n" )
    def crossref( self ):
        if not self.mod is None:
            self.mod.objects.append( self )

class item:
    types = { }
    def __init__( self, obj, mod ):
        self.iid = obj[ "id" ]
        self.name = obj[ "name" ]
        self.mod = mod
        # List of terrains / furniture that produce this item when deconstructed
        self.deconstruct = []
        # List of terrains / furniture that produce this item when bashed
        self.bash = []
        # List of items that use this item as their default container
        self.container_for = []
        # List of vehicle parts that produce this item when broken down
        self.broken_vparts = []
        # List of vehicle parts that can be created from this item
        self.the_vparts = []
        self.weight = obj["weight"]
        self._volume = obj["volume"]
        self.desc = obj["description"]
        if "qualities" in obj:
            self.qualities = obj[ "qualities" ]
        else:
            self.qualities = []
        self.material = []
        if "material" in obj:
            if type( obj[ "material" ] ) is str:
                if not obj[ "material" ] == "null":
                    self.material = [ obj[ "material" ] ]
            else:
                for m in obj[ "material" ]:
                    if not m == "null":
                        self.material.append( m )
        if "phase" in obj:
            self.phase = obj["phase"]
        else:
            self.phase = "solid"
        if "valid_mod_locations" in obj:
            self.valid_mod_locations = obj["valid_mod_locations"]
        else:
            self.valid_mod_locations = []
        if "container" in obj:
            self.container = obj["container"]
        else:
            self.container = "null"
        self.obj = { }
        for x in obj:
            if x in [ "container", "valid_mod_locations", "id", "name", "phase", "qualities",
                      "material", "volume", "weight", "description" ]:
                continue
            self.obj[ x ] = obj[ x ]
        self.count_by_charges = (obj["type"] == "COMESTIBLE" or obj["type"] == "AMMO" or "ammo_data" in obj or self.phase == "liquid")
        self.count = 1;
        if "count" in obj:
            self.count = obj["count"]
        self.stack_size = 1
        if self.count_by_charges:
            if "stack_size" in obj:
                self.stack_size = obj["stack_size"]
            elif "count" in obj:
                self.stack_size = obj["count"]
            elif "charges" in obj:
                self.stack_size = obj["charges"]
    def volume( self, count ):
        return self._volume * count / self.stack_size
    def link( self ):
        return "<a href=\"../items/" + self.iid + ".html\">" + cgi.escape( self.name ) + "</a>"
    def get_quality( self, iid ):
        for q in self.qualities:
            if q[ 0 ] == iid:
                return q[ 1 ]
        return False
    def write_ammo( self, fp, t, am ):
        if not am in self.obj:
            return
        am = self.obj[ am ]
        if am == "NULL" or am == "":
            return
        fp.write( "<div type=\"ammo-id\">" + cgi.escape( t ) + ": " + aname( am ) + "</div>\n" )
    def dump( self ):
        path = os.path.join( OUTPUT_PATH, "items", self.iid + ".html" )
        with open( path, 'w' ) as fp:
            write_html_header( fp, "Item " + self.name, "item" )
            if not self.mod is None:
                fp.write( "Mod: " + self.mod.link() + "<br/>\n" )
            fp.write( "<div>" + cgi.escape(self.desc) + "</div>\n" )
            dump_json( fp, self.obj, "item-data" )
            fp.write( "<table><tr>" )
            fp.write( "<td>volume</td><td>" + str( self.volume( self.count ) ) + "</td>\n" )
            fp.write( "<td>weight</td><td>" + str( self.weight ) + "</td>\n" )
            if self.stack_size > 1:
                fp.write( "<td>stack_size</td><td>" + str( self.stack_size ) + "</td>\n" )
            if self.count > 1:
                fp.write( "<td>count</td><td>" + str( self.count ) + "</td>\n" )
            if "location" in self.obj:
                v = self.obj["location"]
                fp.write( "<td>location</td><td>" + gunmod_location.types[v].link() + "</td>\n" )
            if not self.container == "null":
                fp.write( "<td>Spawns in a " + iname( self.container ) + "</td>\n" )
            fp.write( "</tr></table>" )
            self.write_ammo( fp, "Ammo", "ammo_type" )
            self.write_ammo( fp, "Ammo", "ammo" )
            self.write_ammo( fp, "New ammo", "newtype" ) # for gunmods
            if len( self.valid_mod_locations ) > 0:
                fp.write( "<div>Gunmod locations:\n<ul>\n" )
                for v in self.valid_mod_locations:
                    fp.write( "<li>" + gunmod_location.types[ v[ 0 ] ].link() + " (" + str( v[ 1 ] ) + ")</li>\n" )
                fp.write( "</ul></div>\n" )
            if len( self.qualities ) > 0:
                fp.write( "<div class=\"qualities\">Qualities:\n<ul>\n" )
                for q in self.qualities:
                    # no escaping because q[1] is a number
                    fp.write( "<li>" + qname( q[ 0 ] ) + " [" + str( q[ 1 ] ) + "]</li>\n" )
                fp.write( "</ul></div>\n" )
            if self.iid[ -3: ] == "_on":
                off_id = self.iid[ :-3 ] + "_off"
                if off_id in item.types:
                    fp.write( "<div>There is also an <b>off</b> item there: " + iname( off_id ) + "</div>\n" )
            if self.iid[ -4: ] == "_off":
                on_id = self.iid[ :-4 ] + "_on"
                if on_id in item.types:
                    fp.write( "<div>There is also an <b>on</b> item there: " + iname( on_id ) + "</div>\n" )
            if "seed_" + self.iid in item.types:
                fp.write( "<div>There is also a seed item for this: " + iname( "seed_" + self.iid ) + "</div>\n" )
            if self.iid[ 0:5 ] == "seed_":
                non_seed_id = self.iid[ 5: ]
                if non_seed_id in item.types:
                    fp.write( "<div>There is also an non-seed item: " + iname( non_seed_id ) + "</div>\n" )
            if len( self.container_for ) > 0:
                fp.write( "<div class=\"container-for\">\n<h6>Container for:</h6>\n<ul>\n" )
                for c in self.container_for:
                    fp.write( "<li>" + c.link() + "</li>\n" )
                fp.write( "</ul></div>\n" )
            if len( self.material ) > 0:
                fp.write( "<div class=\"made-of\">\n<h6>Made of:</h6>\n<ul>\n" )
                for m in self.material:
                    fp.write( "<li>" + mname( m ) + "</li>\n" )
                fp.write( "</ul></div>\n" )

            fp.write( "<div class=\"sources\">\n<h6>Sources:</h6>\n<ul>\n" )
            for i in item_group.types:
                i = item_group.types[ i ]
                if i.has_item( self.iid ):
                    fp.write( "<li>item group " + i.link() + "</li>\n" )
            for r in recipe.types:
                if r.is_result( self.iid ) and not r.uncraft_only():
                    fp.write( "<li>" )
                    if r.result == self.iid:
                        fp.write( open_close( "crafting" ) )
                        if r.reversible:
                            fp.write( " (reversible)" )
                        fp.write( " it:\n" )
                    else:
                        fp.write( "byproduct of " + open_close( "crafting" ) + " " + iname( r.result ) + ":\n" )
                    r.write_line( fp )
                    fp.write( "</li>" )
                if r.is_dissasemble_result( self.iid ):
                    if r.uncraft_only():
                        fp.write( "<li>" + open_close( "disassembling" ) + " " + iname( r.result ) + ":\n" );
                        r.write_line( fp )
                        fp.write( "</li>" )
                    else:
                        fp.write( "<li>" + open_close( "disassembling" ) + " (and making) " + iname( r.result ) + ":\n" )
                        r.write_line( fp )
                        fp.write( "</li>" )
            for v in self.deconstruct:
                fp.write( "<li>deconstructing " + v.link() + "</li>" )
            for v in self.bash:
                fp.write( "<li>bashing " + v.link() + "</li>" )
            for v in self.broken_vparts:
                fp.write("<li>broken from " + v.link() + "</li>\n" )
            for v in self.the_vparts:
                fp.write( "<li>removing vehicle part " + v.link() + "</li>" )
            for m in material.types:
                if material.types[ m ].salvage_id == self.iid:
                    fp.write("<li>cutting items made from " + mname( m ) + "</li>\n" )
            fp.write( "</ul></div>\n" )

            fp.write( "<div class=\"usage\">\n<h6>Usage:</h6>\n<ul>\n" )
            for r in recipe.types:
                for b in r.book_learn:
                    if b[ 0 ] == self.iid:
                        if r.uncraft_only():
                            fp.write( "<li>to learn to uncraft " + iname( r.result ) + "</li>\n" )
                        else:
                            fp.write( "<li>to learn a recipe for " + iname( r.result ) + "</li>\n" )
                        break
                if r.result == self.iid and r.uncraft_only():
                    fp.write( "<li>" + open_close( "dissasembled" ) + " into:\n" )
                    r.write_line( fp )
                    fp.write( "</li>" )
                if ( r.is_item_usage( self.iid ) or r.is_tool_usage( self.iid ) ) and not r.uncraft_only():
                    fp.write( "<li>" + open_close( "to make" ) + " " + iname( r.result ) + ":\n" )
                    r.write_line( fp )
                    fp.write( "</li>" )
            for c in construction.types:
                if c.is_item_usage( self.iid ) or c.is_tool_usage( self.iid ):
                    fp.write( "<li>" + open_close( "to construct" ) + " " + c.result() + ":\n" )
                    c.write_line( fp )
                    fp.write( "</li>" )
            for v in self.the_vparts:
                fp.write( "<li>installing vehicle part " + v.link() + "</li>" )
            fp.write( "</ul></div>\n")

            fp.write( "</body>\n</html>\n")
    def crossref( self ):
        if not self.container == "null":
            item.types[ self.container ].container_for.append( self )
        for v in self.valid_mod_locations:
            v = v[0]
            if not v in gunmod_location.types:
                gunmod_location.types[ v ] = gunmod_location( v )
            gunmod_location.types[ v ].guns.append( self )
        if "location" in self.obj:
            v = self.obj["location"]
            if not v in gunmod_location.types:
                gunmod_location.types[ v ] = gunmod_location( v )
            gunmod_location.types[ v ].mods.append( self )
        for m in self.material:
            material.types[ m ].usage.append( self )
        for am in [ "ammo_type", "ammo", "newtype" ]:
            if not am in self.obj:
                continue
            am = self.obj[ am ]
            if am == "NULL" or am == "":
                continue
            ammotype.types[ am ].usage.append( self )
        if not self.mod is None:
            self.mod.objects.append( self )

def dummy_load( obj ):
    if type( obj.types ) is dict:
        obj.types[ obj.iid ] = obj
    else:
        obj.types.append( obj )

def load_obj( obj, mod ):
    t = obj["type"]
    if t in ITEM_TYPE_TYPES:
        dummy_load( item( obj, mod ) )
    elif t == "recipe":
        dummy_load( recipe( obj, mod ) )
    elif t == "tool_quality":
        dummy_load( quality( obj, mod ) )
    elif t == "ammunition_type":
        dummy_load( ammotype( obj, mod ) )
    elif t == "furniture":
        dummy_load( furniture( obj, mod ) )
    elif t == "terrain":
        dummy_load( terrain( obj, mod ) )
    elif t == "item_group":
        dummy_load( item_group( obj, mod ) )
    elif t == "construction":
        dummy_load( construction( obj, mod ) )
    elif t == "trap":
        ...
    elif t == "material":
        dummy_load( material( obj, mod ) )
    elif t == "vehicle_part":
        dummy_load( vehicle_part( obj, mod ) )
    elif not t in TYPES_TO_IGNORE:
        print( "unknown type: " + t )

dummy_load( item( { "id": "toolset", "name": "bionic tools", "weight": 0, "volume": 0, "description": "", "type": "GENERIC" }, None ) )
dummy_load( item( { "id": "fire", "name": "a nearby fire", "weight": 0, "volume": 0, "description": "", "type": "GENERIC" }, None ) )

if "null" in material.types:
    del materials.types[ "null" ]

scanfiles( scandir( "data/json" ) )
#scanmods( scandir( "data/mods") )

def crossref( dd ):
    if type( dd.types ) is dict:
        for iid in dd.types:
            dd.types[ iid ].crossref()
    else:
        for v in dd.types:
            v.crossref()
    

crossref( quality )
crossref( vehicle_part )
crossref( item )
crossref( ammotype )
crossref( material )
crossref( furniture )
crossref( terrain )
crossref( item_group )
crossref( recipe )
crossref( construction )

def print_index( obj, path, title ):
    index = { }
    for iid in obj.types:
        index[ obj.types[ iid ].name[ 0:1 ] ] = ""
    with open( os.path.join( OUTPUT_PATH, path, "index.html" ), 'w' ) as fp:
        write_html_header( fp, title + " index", "index" )
        for i in sorted( list( index.keys() ) ):
            fp.write( "<a href=\"#" + i + "\">" + cgi.escape( i ) + "</a>\n" )
        fp.write( "<ul>\n" )
        prev = " "
        count = 0
        for iid in sorted( list( obj.types.values() ), key = lambda x: x.name ):
            count = count + 1
            if prev != iid.name[ 0:1 ] and count > 64:
                prev = iid.name[ 0:1 ]
                count = 0
                fp.write( "<a name=\"" + prev + "\"><hr/></a>\n" )
            fp.write( "<li>" + iid.link() + "</li>\n" )
        fp.write( "</ul>\n" )
        fp.write( "</body>\n</html>\n" )

print_index( item, "items", "Item" )
print_index( material, "materials", "Material" )
print_index( quality, "qualities", "Qualities" )
print_index( ammotype, "ammo_types", "Ammo type" )
print_index( vehicle_part, "vparts", "Vehicle part" )
print_index( furniture, "furnitures", "Furniture" )
print_index( terrain, "terrains", "Terrain" )
print_index( gunmod_location, "gunmod_locations", "Gunmod locations" )
print_index( mod, "mods", "Mods" )

def dump( dd, name ):
    for iid in dd.types:
        dd.types[ iid ].dump()
    print( name + ": " + str( len( dd.types ) ) )

dump( quality, "qualities" )
dump( vehicle_part, "vehicle parts" )
dump( furniture, "furnitures" )
dump( terrain, "terrains" )
dump( ammotype, "ammotypes" )
dump( material, "materials" )
dump( gunmod_location, "gunmod_locations" )
dump( mod, "mods" )
dump( item, "items" )
