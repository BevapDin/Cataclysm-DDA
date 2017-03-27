
import clang.cindex
clang.cindex.Config.set_library_path('/usr/lib')

import re

class Matcher:
    def __init__(self):
        self.matches = [ ]
    def match(self, name):
        for i in self.matches:
            if type(i) is str:
                if i == name:
                    return True
            else:
                if i.match(name):
                    return True
        return False
    def add(self, match):
        self.matches.append(match)

def xdump(cursor):
    print("%s[%s]" % (cursor.spelling, str(cursor.kind)))
    for c in cursor.get_children():
        xdump(c)
    print("--")

def xydump(cursor):
    visited = [ ]
    xydump2(cursor, 0, '', visited)

def xydump2(cursor, ident, prefix, visited):
    if not cursor:
        print("%s%s%s!" % (prefix, "   " * ident, 'invalid'))
        return

    print("%s%s%s[%s]" % (prefix, "   " * ident, cursor.spelling, str(cursor.kind)))

    if cursor in visited:
        return
    visited.append(cursor)

    for i in xrange(cursor.get_num_template_arguments()):
        print("%s%stemplate: %s - %s - %s - %s - %s" % (prefix, "   " * ident, str(i), \
            str(cursor.get_template_argument_kind(i)), str(cursor.get_template_argument_type(i)), \
            str(cursor.get_template_argument_value(i)), str(cursor.get_template_argument_unsigned_value(i))))

    defi = cursor.get_definition()
    if defi:
        print("%s%sdefinition: %s[%s]" % (prefix, "   " * ident, defi.spelling, str(defi.kind)))
        xydump2(defi, 0, '   ' * ident + 'd  ', visited)

    ty = cursor.type
    if ty.kind != clang.cindex.TypeKind.INVALID:
        print("%s%stype: %s[%s]" % (prefix, "   " * ident, ty.spelling, str(ty.kind)))
        xydump2(ty.get_declaration(), 0, '   ' * ident + 't  ', visited)

        tp = ty.get_pointee()
        if tp.kind != clang.cindex.TypeKind.INVALID:
            print("%s%spointee: %s[%s]" % (prefix, "   " * ident, tp.spelling, str(tp.kind)))
            xydump2(tp.get_declaration(), 0, '   ' * ident + 'p  ', visited)

    for c in cursor.get_children():
        xydump2(c, ident + 1, prefix, visited)

'''
Thrown by the translate functions below to indicate the C++ type does not
have a matching Lua type and can therefor not be used in the Lua wrapper.
'''
class TypeTranslationError:
    def __init__(self, message):
        self.message = message
    def __str__(self):
        return self.message



'''
Raised when an object from C++ is not exported to Lua.
'''
class SkippedObjectError:
    def __init__(self, message):
        self.message = message
    def __str__(self):
        return self.message



def is_public(cursor):
    return cursor.access_specifier == clang.cindex.AccessSpecifier.PUBLIC



class Parser:
    def __init__(self):
        '''
        C++ types that should be exported by-value. They must support copy-construction
        and copy-assignment. Use the plain C++ name of their type. Note: enumerations
        and string_id and int_id are automatically exported as value, they don't need to
        be listed here.
        '''
        self.types_exported_by_value = [ ]
        '''
        C++ types that should be exported by-reference. They don't need to support
        copy-construction nor copy-assignment. Types may be exported by-value and
        by-reference.
        '''
        self.types_exported_by_reference = [ ]
        '''
        All the C++ types that should be exported to Lua. This should contain the
        real type, not a type alias. It can contain enumeration or class types.
        Enumerations are automatically added to types_exported_by_value. Class types
        must be added manually to types_exported_by_value or types_exported_by_reference
        or to both.
        '''
        self.types_to_export = [ ]
        self.readonly_identifiers = Matcher()
        self.blocked_identifiers = Matcher()
        self.ignore_result_of = Matcher()

        self.string_ids = { }
        self.int_ids = { }

        self.enums = { }
        self.classes = { }
        self.visited_files = [ ]
        self.index = clang.cindex.Index.create()

        self.generic_types = { }
        self.build_in_typedefs = {
        }

    numeric_fixed_points = [
        clang.cindex.TypeKind.CHAR_U, clang.cindex.TypeKind.UCHAR, clang.cindex.TypeKind.CHAR16,
        clang.cindex.TypeKind.CHAR32, clang.cindex.TypeKind.USHORT, clang.cindex.TypeKind.UINT,
        clang.cindex.TypeKind.ULONG, clang.cindex.TypeKind.ULONGLONG, clang.cindex.TypeKind.UINT128,
        clang.cindex.TypeKind.CHAR_S, clang.cindex.TypeKind.SCHAR, clang.cindex.TypeKind.WCHAR,
        clang.cindex.TypeKind.SHORT, clang.cindex.TypeKind.INT, clang.cindex.TypeKind.LONG,
        clang.cindex.TypeKind.LONGLONG, clang.cindex.TypeKind.INT128
    ]
    numeric_floating_pints = [
        clang.cindex.TypeKind.FLOAT, clang.cindex.TypeKind.DOUBLE, clang.cindex.TypeKind.LONGDOUBLE,
    ]



    def add_export_by_value(self, cpp_name):
        self.types_exported_by_value.append(cpp_name)
        self.types_to_export.append(cpp_name)
    def add_export_by_reference(self, cpp_name):
        self.types_exported_by_reference.append(cpp_name)
        self.types_to_export.append(cpp_name)
    def add_export_by_value_and_reference(self, cpp_name):
        self.types_exported_by_value.append(cpp_name)
        self.types_exported_by_reference.append(cpp_name)
        self.types_to_export.append(cpp_name)
    def add_export_enumeration(self, cpp_name):
        self.add_export_by_value(cpp_name)



    def derived_class(self, t):
        if t.kind == clang.cindex.TypeKind.TYPEDEF:
            return t.get_declaration().underlying_typedef_type
        if t.kind == clang.cindex.TypeKind.RECORD or t.kind == clang.cindex.TypeKind.ENUM:
            return re.sub('^const ', '', t.spelling)
        return t.spelling

    def export_enabled(self, name):
        if type(name) is str:
            return name in self.types_to_export
        return self.export_enabled(self.derived_class(name))
    def export_by_value(self, name):
        if type(name) is str:
            return name in self.types_exported_by_value
        elif self.export_by_value(re.sub('^const ', '', name.spelling)):
            return True
        return self.export_by_value(self.derived_class(name))
    def export_by_reference(self, name):
        if type(name) is str:
            return name in self.types_exported_by_reference
        elif self.export_by_reference(re.sub('^const ', '', name.spelling)):
            return True
        return self.export_by_reference(self.derived_class(name))




    def get_string_id_for(self, name):
        for sid, base_type in self.string_ids.iteritems():
            if base_type == name:
                return sid
        return None
    def get_int_id_for(self, name):
        for iid, base_type in self.int_ids.iteritems():
            if base_type == name:
                return iid
        return None




    def parse_enum(self, cursor):
        if not self.export_enabled(cursor.type):
            return
        # Just a declaration, skip the actual parsing as it requires a proper definition.
        if not cursor.is_definition():
            return
        e = CppEnum(self, cursor);
        if not e.cpp_name in self.enums:
            self.enums[e.cpp_name] = e

    def parse_class(self, cursor):
        sp = cursor.spelling
        if not self.export_enabled(cursor.type):
            return
        if not self.export_by_reference(cursor.type) and not self.export_by_value(cursor.type):
            raise RuntimeError("Class %s should be exported, but is not marked as by-value nor by-reference!" % sp)
        # Just a declaration, skip the actual parsing as it requires a proper definition.
        if not cursor.is_definition():
            return
        c = CppClass.from_cursor(self, cursor)
        if not c.cpp_name in self.classes:
            self.classes[c.cpp_name] = c

    def parse_id_typedef(self, cursor, id_type, ids_map):
        t = cursor.underlying_typedef_type

        m = re.match('^' + id_type + '<([a-zA-Z][_a-zA-Z0-9]*)>$', re.sub('^const ', '', t.spelling))
        if not m:
            return

        typedef_name = re.sub('^const ', '', cursor.spelling)
        base_type = m.group(1)
        if typedef_name in ids_map:
            return

        if not self.export_enabled(base_type):
            return

        ids_map[typedef_name] = base_type
        # A id itself is always handled by value. It's basically a std::string/int.
        self.types_exported_by_value.append(typedef_name)



    def parse_typedef(self, cursor):
        # string_id and int_id typedefs are handled separately so we can include the typedef
        # name in the definition of the class. This allows Lua code to use the typedef name
        # instead of the underlying type `string_id<T>`.
        self.parse_id_typedef(cursor, 'string_id', self.string_ids)
        self.parse_id_typedef(cursor, 'int_id', self.int_ids)

        # We need this later in `register_container`
        bt = self.build_in_lua_type(cursor.underlying_typedef_type)
        if bt:
            a = re.sub('^const ', '', cursor.spelling)
            self.build_in_typedefs[a] = bt


    def parse(self, header):
        if header in self.visited_files:
            print('Skipping file %s as it was already included by another file' % (header))
            return
        try:
            opts = clang.cindex.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES | clang.cindex.TranslationUnit.PARSE_INCOMPLETE
            foo = ['-x', 'c++', '-std=c++11', '-isystem', '/usr/lib/clang/3.8.0/include']
            tu = self.index.parse(header, foo, options=opts)
            for d in tu.diagnostics:
                print("Diagnostic at %s:%s: %s" % (d.location.file.name, d.location.line, d.spelling))
            self.parse_cursor(tu.cursor)
            for i in tu.get_includes():
                self.visited_files.append(i.source.name)

        except clang.cindex.TranslationUnitLoadError as e:
            print("Failed to parse %s: %s" % (header, str(e)))




    def export(self, lua_file):
        # Export "dummy" class definitions for classes that are used in string/int ids.
        for c in self.string_ids:
            base_type = self.string_ids[c]
            if not base_type in self.classes:
                self.classes[base_type] = CppClass.from_name(self, base_type)
                self.types_to_export.append(base_type)
        for c in self.int_ids:
            base_type = self.int_ids[c]
            if not base_type in self.classes:
                self.classes[base_type] = CppClass.from_name(self, base_type)
                self.types_to_export.append(base_type)

        handled_types = [ ]
        data = ''
        data = data + "classes = {\n"
        for c in self.types_to_export:
            if c in self.classes:
                data = data + self.classes[c].export() + ",\n"
                handled_types.append(c)
        data = data + "}\n"

        data = data + "\n" + "enums = {\n"
        for e in self.types_to_export:
            if e in self.enums:
                data = data + self.enums[e].export() + ",\n"
                handled_types.append(e)
        data = data + "}\n"

        data = data + "\n"
        for e in sorted(set(self.generic_types.itervalues())):
            data = data + e + "\n"

        with open(lua_file, 'wb') as f:
            f.write(data)

        for t in self.types_to_export:
            if not t in handled_types:
                print("Type %s not found in any input source file" % (t))



    def parse_cursor(self, cursor):
        k = cursor.kind
        if k == clang.cindex.CursorKind.STRUCT_DECL or k == clang.cindex.CursorKind.CLASS_DECL or k == clang.cindex.CursorKind.CLASS_TEMPLATE:
            self.parse_class(cursor)
            return

        if k == clang.cindex.CursorKind.ENUM_DECL:
            self.parse_enum(cursor)
            return

        if k == clang.cindex.CursorKind.TYPEDEF_DECL or k == clang.cindex.CursorKind.TYPE_ALIAS_DECL:
            self.parse_typedef(cursor)
            return

        for c in cursor.get_children():
            self.parse_cursor(c)



    def register_iterator(self, name, t):
        sp = re.sub('^const ', '', t.spelling)
        m = re.match('^std::' + name + '<([a-zA-Z][_a-zA-Z0-9:]*)>::iterator$', sp)
        if not m:
            if t.kind == clang.cindex.TypeKind.TYPEDEF:
                return self.register_iterator(name, t.get_declaration().underlying_typedef_type)
            return None

        element_type = m.group(1)
        if self.build_in_lua_type(element_type) == 'std::string':
            element_type = 'std::string'
        elif self.export_by_value(element_type):
            pass
        else:
            print("%s is a %s based on %s, but is not exported" % (t.spelling, name, element_type))
            return None

        self.generic_types[sp] = 'make_' + name + '_class("%s")' % element_type
        return '"std::' + name + '<' + element_type + '>::iterator"'

    def register_container(self, name, t):
        res = self.register_iterator(name, t)
        if res: return res

        sp = re.sub('^const ', '', t.spelling)
        m = re.match('^std::' + name + '<([a-zA-Z][_a-zA-Z0-9:]*)>$', sp)
        if not m:
            if t.kind == clang.cindex.TypeKind.TYPEDEF:
                return self.register_container(name, t.get_declaration().underlying_typedef_type)
            return None

        element_type = m.group(1)
        if self.build_in_lua_type(element_type) == 'std::string':
            element_type = 'std::string'
        elif self.export_by_value(element_type):
            pass
        else:
            print("%s is a %s based on %s, but is not exported" % (t.spelling, name, element_type))
            return None

        self.generic_types[sp] = 'make_' + name + '_class("%s")' % element_type
        return '"std::' + name + '<' + element_type + '>"'

    def register_id_type(self, t, ids_map, id_type):
        typedef = t
        if t.kind == clang.cindex.TypeKind.TYPEDEF:
            t = t.get_declaration().underlying_typedef_type

        m = re.match('^' + id_type + '<([a-zA-Z][_a-zA-Z0-9]*)>$', t.spelling)
        if not m:
            m = re.match('^const ' + id_type + '<([a-zA-Z][_a-zA-Z0-9]*)>$', t.spelling)
            if not m:
                return None

        base_type = m.group(1)
        name = re.sub('^const ', '', typedef.spelling)
        if not name in ids_map:
            if not self.export_enabled(base_type):
                return None
            ids_map[name] = base_type
            # A id itself is always handled by value. It's basically a std::string/int.
            self.types_exported_by_value.append(name)

        return '"%s"' % name

    def register_generic(self, t):
        res = self.register_container('list', t)
        if res: return res
        res = self.register_container('vector', t)
        if res: return res
        res = self.register_container('set', t)
        if res: return res

        return None

    '''
    Returns the build-in Lua type of the given C++ type, if there is one. It returns
    None if there is no matching build-in type.
    Input can either be a string or a clang.cindex.Type.
    '''
    def build_in_lua_type(self, t):
        if type(t) is str:
            # Lua has build in support for strings and the bindings generator translates
            # std::string to Lua strings.
            if t == 'std::string':
                return 'std::string'
            # Typedefs encountered earlier that have been resolved already.
            elif t in self.build_in_typedefs:
                return self.build_in_typedefs[t]
            # If the input is a string, we can't really do anything more as we don't have
            # any information about the base type itself.
            else:
                return None

        # First try the canonical type as reported by clang. This will handle all common
        # typedefs correctly.
        ct = t.get_canonical()
        if ct.kind in Parser.numeric_fixed_points:
            return 'int'
        elif ct.kind in Parser.numeric_floating_pints:
            return 'float'
        elif ct.kind == clang.cindex.TypeKind.BOOL:
            return 'bool'

        # It's not a build-in C++ type that we can handle, so look at its actual name.
        # This removes the const because we export 'const int' and 'const std::string' the
        # same as non-const counterparts.
        # This calls the function with a string argument, and thereby handles std::string
        res = self.build_in_lua_type(re.sub('^const ', '', t.spelling))
        if res: return res

        # Not a known type, but if it's a typedef, maybe the underlying type is known.
        if t.kind == clang.cindex.TypeKind.TYPEDEF:
            return self.build_in_lua_type(t.get_declaration().underlying_typedef_type)

        return None



    '''
    Translate a return type (of a function call) to the wrapped type, suitable for
    the class definition.
    '''
    def translate_result_type(self, t):
        res = self.build_in_lua_type(t)
        if res: return '"%s"' % res

        if self.export_by_value(t):
            return '"%s"' % t.spelling

        res = self.register_generic(t)
        if res: return res

        # Only allowed as result type, therefor hard coded here.
        if t.get_canonical().kind == clang.cindex.TypeKind.VOID:
            return "nil"

        if t.kind == clang.cindex.TypeKind.LVALUEREFERENCE:
            pt = t.get_pointee()
            spt = pt.spelling
            if pt.is_const_qualified():
                # A const reference. Export it like a value (which means Lua will get a copy)
                res = self.build_in_lua_type(pt)
                if res: return '"%s"' % res

                if self.export_by_value(pt):
                    return '"%s"' % re.sub('^const ', '', spt)

            # const and non-const reference:
            if self.export_by_reference(pt):
                return '"%s&"' % re.sub('^const ', '', spt)

            # Generic types are exported as values and as reference
            res = self.register_generic(pt)
            if res: return res

        if t.kind == clang.cindex.TypeKind.POINTER:
            pt = t.get_pointee()
            spt = pt.spelling
            # "const foo *" and "foo *" can be satisfied by an by-reference object.
            if self.export_by_reference(pt):
                return '"%s&"' % re.sub('^const ', '', spt)

        if t.kind == clang.cindex.TypeKind.TYPEDEF:
            return self.translate_result_type(t.get_declaration().underlying_typedef_type)

        ct = t.get_canonical()
        raise TypeTranslationError("unhandled type %s[%s] as result (%s[%s])" % (t.spelling, str(t.kind), ct.spelling, str(ct.kind)))



    '''
    Translate a function argument type to the wrapped type, suitable for
    the class definition.
    '''
    def translate_argument_type(self, t):
        res = self.build_in_lua_type(t)
        if res: return '"%s"' % res

        if self.export_by_value(t):
            return '"%s"' % re.sub('^const ', '', t.spelling)

        res = self.register_generic(t)
        if res: return res

        if t.kind == clang.cindex.TypeKind.LVALUEREFERENCE:
            pt = t.get_pointee()
            spt = pt.spelling
            if pt.is_const_qualified():
                # "const int &" can be satisfied by the build-in Lua "int".
                # But it won't work correctly with "int &" as the value passed to
                # the C++ functions is a temporary.
                res = self.build_in_lua_type(pt)
                if res: return '"%s"' % res

            # "const foo &" and "foo &" can be satisfied by an exported object.
            if self.export_by_value(pt):
                return '"%s"' % re.sub('^const ', '', spt)
            if self.export_by_reference(pt):
                return '"%s"' % re.sub('^const ', '', spt)
            res = self.register_generic(pt)
            if res: return res

        if t.kind == clang.cindex.TypeKind.POINTER:
            pt = t.get_pointee()
            spt = pt.spelling
            # "const foo *" and "foo *" can be satisfied with by-reference values, they
            # have an overload that provides the pointer. It does not work for by-value objects.
            if self.export_by_reference(pt):
                return '"%s"' % re.sub('^const ', '', spt)
            res = self.register_generic(pt)
            if res: return res

        if t.kind == clang.cindex.TypeKind.TYPEDEF:
            return self.translate_argument_type(t.get_declaration().underlying_typedef_type)

        ct = t.get_canonical()
        raise TypeTranslationError("unhandled type %s[%s] as argument (%s[%s])" % (t.spelling, str(t.kind), ct.spelling, str(ct.kind)))

    '''
    Translate a class member type to the wrapped type.
    '''
    def translate_member_type(self, t):
        res = self.build_in_lua_type(t)
        if res: return '"%s"' % res

        if self.export_by_reference(t):
            return '"%s"' % re.sub('^const ', '', t.spelling)
        if self.export_by_value(t):
            return '"%s"' % re.sub('^const ', '', t.spelling)

        res = self.register_generic(t)
        if res: return res

        if t.kind == clang.cindex.TypeKind.LVALUEREFERENCE:
            pt = t.get_pointee()
            # We can return a reference to the member in Lua, therefor allow types that
            # support by-reference semantic.
            if self.export_by_reference(pt):
                return '"%s"' % re.sub('^const ', '', pt.spelling)

        if t.kind == clang.cindex.TypeKind.POINTER:
            pt = t.get_pointee()
            if self.export_by_reference(pt):
                return '"%s"' % re.sub('^const ', '', pt.spelling)

        if t.kind == clang.cindex.TypeKind.TYPEDEF:
            return self.translate_member_type(t.get_declaration().underlying_typedef_type)

        ct = t.get_canonical()
        raise TypeTranslationError("unhandled type %s[%s] as member (%s[%s])" % (t.spelling, str(t.kind), ct.spelling, str(ct.kind)))



class CppClass:
    ''' Generic callable thing, it contains an argument list and can export the list. '''
    class CppCallable:
        def __init__(self, parent, cursor):
            self.parent = parent
            self.cursor = cursor
            # The min/max number of arguments. May differ if some parameters are optional.
            self.min_arguments = 0
            self.max_arguments = 0
            for a in self.cursor.get_arguments():
                self.max_arguments = self.max_arguments + 1
                if not self.has_default_value(a):
                    self.min_arguments = self.min_arguments + 1

        def has_default_value(self, a):
            # Sadly, this is not directly exposed by clang, one has to "parse" the parameter
            # declaration oneself.
            for t in a.get_tokens():
                if t.spelling == '=':
                    return True
            return False

        def export_argument_list(self, m):
            if m == 0:
                return '{ }'
            args = list(self.cursor.get_arguments())[0:m]
#            for a in args:
#                print(">> %s[%s]" % (a.spelling, a.kind))
#                xydump(a.type.get_pointee().get_declaration())
#                for c in a.type.get_pointee().get_declaration().get_children():
#                    print("%s[%s]" % (c.spelling, c.kind))

            args = [ self.parent.parser.translate_argument_type(a.type) for a in args]
            return "{ " + ", ".join(args) + " }"

        # Export all versions of the callable (multiple versions if there are optional parameters).
        # Result is an array. Each entry is created by calling the callback with exported
        # list of arguments (as string, ready to be printed) as parameter.
        def export_cb(self, callback):
            if self.parent.parser.blocked_identifiers.match(self.pretty_name() + '(' + ', '.join([ a.type.spelling for a in self.cursor.get_arguments()]) + ')'):
                return [ ] # silently ignored.
            result = [ ]
            try:
                m = self.min_arguments
                while m <= self.max_arguments:
                    args = callback(self.export_argument_list(m))
                    if args:
                        result.append(args)
                    m = m + 1
            except TypeTranslationError as e:
                result.append("-- %s ignored because: %s" % (self.pretty_name(), e))
            except SkippedObjectError as e:
                result.append("-- %s ignored because: %s" % (self.pretty_name(), e))
            return result

        def pretty_name(self):
            return ('static ' if self.cursor.is_static_method() else '') + self.parent.cpp_name + '::' + self.cursor.spelling

    class CppFunction(CppCallable):
        def __init__(self, parent, cursor):
            CppClass.CppCallable.__init__(self, parent, cursor)
            self.const_overload = False
            self.overridden = False
            for c in cursor.walk_preorder():
                if c.kind == clang.cindex.CursorKind.CXX_OVERRIDE_ATTR:
                    self.overridden = True

        # Function names that are keywords in Lua need to be translated to valid names.
        function_name_translation_table = {
            'begin': 'cppbegin',
            'end': 'cppend',
        }

        def cb(self, args):
            # TODO: add support for *some* operators
            if re.match('^operator[^a-zA-Z0-9_]', self.cursor.spelling):
                raise SkippedObjectError("operator")

            if self.const_overload:
                return None # silently ignored.

            # Overridden methods are ignored because the parent class already contains
            # them and we scan the parent class and include it in the output anyway.
            if self.overridden:
                return None # silently ignored.

            result = 'nil'
            try:
                result = self.parent.parser.translate_result_type(self.cursor.result_type)
            except TypeTranslationError as e:
                if not self.parent.parser.ignore_result_of.match(self.pretty_name() + '(' + ', '.join([ a.type.spelling for a in self.cursor.get_arguments()]) + ')'):
                    raise e
            line = ""
            line = line + "{ "
            name = self.cursor.spelling
            if name in self.function_name_translation_table:
                name = self.function_name_translation_table[name]
            line = line + "name = \"" + name + "\", "
            if self.cursor.is_static_method():
                line = line + "static = true, "
            line = line + "rval = " + result + ", "
            if name != self.cursor.spelling:
                line = line + "cpp_name = \"" + self.cursor.spelling + "\", "
            line = line + "args = " + args
            line = line + " }"
            return line

        def export(self):
            return self.export_cb(lambda x : self.cb(x))

        def has_same_arguments(self, other):
            fargs = list(self.cursor.get_arguments())
            oargs = list(other.cursor.get_arguments())
            if len(fargs) != len(oargs):
                return False
            for i in xrange(len(fargs)):
                if fargs[i].type.get_canonical() != oargs[i].type.get_canonical():
                    return False
            return True

    class CppConstructor(CppCallable):
        def export(self):
            return self.export_cb(lambda x : x)

    class CppAttribute:
        def __init__(self, parent, cursor):
            self.parent = parent
            self.cursor = cursor
            self.cpp_name = cursor.spelling
        def export(self):
            try:
                if self.parent.parser.blocked_identifiers.match(self.parent.cpp_name + "::" + self.cpp_name):
                    return [ ] # silently ignored.

                line = ""
                line = line + self.cpp_name + " = { "
                line = line + "type = " + self.parent.parser.translate_member_type(self.cursor.type)
                readonly = self.parent.parser.readonly_identifiers.match(self.parent.cpp_name + "::" + self.cpp_name)
                if not self.cursor.type.is_const_qualified() and not readonly:
                    line = line + ", writable = true"
                line = line + " }"
                return [ line ]
            except TypeTranslationError as e:
                return [ "-- %s::%s ignored because: %s" % (self.parent.cpp_name, self.cpp_name, e) ]
            except SkippedObjectError as e:
                return [ "-- %s::%s ignored because: %s" % (self.parent.cpp_name, self.cpp_name, e) ]

    def __init__(self, parser, cpp_name):
        self.functions = [ ]
        self.attributes = [ ]
        self.constructors = [ ]
        self.parser = parser
        self.cpp_name = cpp_name
        self.parents = [ ]
        self.has_equal = False

    @staticmethod
    def from_cursor(parser, cursor):
        result = CppClass(parser, cursor.spelling)

        for c in cursor.get_children():
            result.parse(c)

        # Overloads that only differ on the const of the method are merged into one
        # because Lua doesn't know about const and handles everything as non-const.
        # Example `bar &foo::get(int)` and `const bar &foo:get(int) const` are the
        # same to Lua. For convenience we only export the non-const version.
        for f in result.functions:
            if f.cursor.is_const_method() and result.has_non_const_overload(f):
                f.const_overload = True

        return result

    @staticmethod
    def from_name(parser, cpp_name):
        result = CppClass(parser, cpp_name)
        return result

    def has_non_const_overload(self, func):
        for f in self.functions:
            if f == func or f.cursor.is_const_method() or f.pretty_name() != func.pretty_name():
                continue
            if f.has_same_arguments(func):
                return True
        return False

    def parse(self, cursor):
        if not is_public(cursor):
            return

        k = cursor.kind
        if k == clang.cindex.CursorKind.STRUCT_DECL or k == clang.cindex.CursorKind.CLASS_DECL:
            pass # Inner classes. Skipped for now. Maybe later. TODO.
        elif k == clang.cindex.CursorKind.CXX_METHOD:
            self.functions.append(CppClass.CppFunction(self, cursor))
            if cursor.spelling == 'operator==':
                # Note: this assumes any `operator==` compares with the same class type,
                # and not with a different type. TODO: check the arguments.
                self.has_equal = True
        elif k == clang.cindex.CursorKind.FIELD_DECL:
            self.attributes.append(CppClass.CppAttribute(self, cursor))
        elif k == clang.cindex.CursorKind.CONSTRUCTOR:
            self.constructors.append(CppClass.CppConstructor(self, cursor))
        elif k == clang.cindex.CursorKind.CXX_BASE_SPECIFIER:
            self.parents.append(cursor)

    def print_objects(self, objects, prefix, postfix):
        lines = [ ]
        for o in objects:
            for line in o.export():
                if line.startswith('--'):
                    print(line)
                else:
                    lines.append(line)

        if len(lines) == 0:
            return ''
        r = prefix
        for l in sorted(lines):
            r = r + ' ' * 12 + l + ',\n'
        r = r + postfix
        return r

    def gather_parent(self, pc, functions, attributes):
        definition = pc.get_definition()
        # Special construct to find templated classes like `visitable<T>`
        for c in pc.get_children():
            if c.kind == clang.cindex.CursorKind.TEMPLATE_REF:
                definition = c.get_definition()
                break
        parent = CppClass.from_cursor(self.parser, definition)
        functions += parent.functions
        attributes += parent.attributes
        for pc in parent.parents:
            parent.gather_parent(pc, functions, attributes)

    def export(self):
        tab = ' ' * 8
        # Both lists will include the functions of parent classes
        functions = self.functions
        attributes = self.attributes

        r = ""
        r = r + "    " + self.cpp_name + " = {\n"

        for pc in self.parents:
            definition = pc.get_definition()
            if self.parser.export_enabled(definition.type):
                r = r + tab + "parent = \"" + definition.spelling + "\",\n"
            else:
                # Parent class is not exported directly, but we still have to include its
                # functions and attributes
                self.gather_parent(pc, functions, attributes)

        # Exporting the constructor only makes sense for types that can have by-value semantic
        if self.parser.export_by_value(self.cpp_name):
            r = r + self.print_objects(self.constructors, tab + 'new = {\n', tab + '},\n')

        sid = self.parser.get_string_id_for(self.cpp_name)
        if sid:
            r = r + tab + "string_id = \"" + sid + "\",\n"
        iid = self.parser.get_int_id_for(self.cpp_name)
        if iid:
            r = r + tab + "int_id = \"" + iid + "\",\n"

        if self.parser.export_by_value(self.cpp_name) and self.parser.export_by_reference(self.cpp_name):
            r = r + tab + "by_value_and_reference = true,\n"
        elif self.parser.export_by_value(self.cpp_name):
            r = r + tab + "by_value = true,\n"
        else:
            pass # by reference is the default.

        if self.has_equal:
            r = r + tab + "has_equal = true,\n"

        r = r + self.print_objects(attributes, tab + 'attributes = {\n', tab + '},\n')
        r = r + self.print_objects(functions, tab + 'functions = {\n', tab + '}\n')

        r = r + "    }"

        return r



class CppEnum:
    def __init__(self, parser, cursor):
        self.values = [ ]
        self.cpp_name = cursor.spelling

        for c in cursor.get_children():
            if c.kind == clang.cindex.CursorKind.ENUM_CONSTANT_DECL:
                self.values.append(c.spelling)

    def export(self):
        r = ""
        r = r + "    " + self.cpp_name + " = {\n"
        for a in self.values:
            r = r + "        \"" + a + "\",\n"
        r = r + "    }"
        return r
