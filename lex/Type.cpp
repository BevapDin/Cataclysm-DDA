#include "Type.h"

#include "Cursor.h"
#include "common-clang.h"

#include <cstring>
#include <map>

#define STRINGIFY(x) #x
#define PAIR(x) { x, STRINGIFY(x) }
static const std::map<int, const char *> type_kind_names = { {
        PAIR( CXType_Invalid ),
        PAIR( CXType_Unexposed ),
        PAIR( CXType_Void ),
        PAIR( CXType_Bool ),
        PAIR( CXType_Char_U ),
        PAIR( CXType_UChar ),
        PAIR( CXType_Char16 ),
        PAIR( CXType_Char32 ),
        PAIR( CXType_UShort ),
        PAIR( CXType_UInt ),
        PAIR( CXType_ULong ),
        PAIR( CXType_ULongLong ),
        PAIR( CXType_UInt128 ),
        PAIR( CXType_Char_S ),
        PAIR( CXType_SChar ),
        PAIR( CXType_WChar ),
        PAIR( CXType_Short ),
        PAIR( CXType_Int ),
        PAIR( CXType_Long ),
        PAIR( CXType_LongLong ),
        PAIR( CXType_Int128 ),
        PAIR( CXType_Float ),
        PAIR( CXType_Double ),
        PAIR( CXType_LongDouble ),
        PAIR( CXType_NullPtr ),
        PAIR( CXType_Overload ),
        PAIR( CXType_Dependent ),
        PAIR( CXType_ObjCId ),
        PAIR( CXType_ObjCClass ),
        PAIR( CXType_ObjCSel ),
        PAIR( CXType_Float128 ),
        //PAIR(CXType_Half),
        //PAIR(CXType_Float16),
        PAIR( CXType_FirstBuiltin ),
        PAIR( CXType_LastBuiltin ),
        PAIR( CXType_Complex ),
        PAIR( CXType_Pointer ),
        PAIR( CXType_BlockPointer ),
        PAIR( CXType_LValueReference ),
        PAIR( CXType_RValueReference ),
        PAIR( CXType_Record ),
        PAIR( CXType_Enum ),
        PAIR( CXType_Typedef ),
        PAIR( CXType_ObjCInterface ),
        PAIR( CXType_ObjCObjectPointer ),
        PAIR( CXType_FunctionNoProto ),
        PAIR( CXType_FunctionProto ),
        PAIR( CXType_ConstantArray ),
        PAIR( CXType_Vector ),
        PAIR( CXType_IncompleteArray ),
        PAIR( CXType_VariableArray ),
        PAIR( CXType_DependentSizedArray ),
        PAIR( CXType_MemberPointer ),
        PAIR( CXType_Auto ),
        PAIR( CXType_Elaborated ),
        //PAIR(CXType_Pipe),
        /*
        PAIR(CXType_OCLImage1dRO),
        PAIR(CXType_OCLImage1dArrayRO),
        PAIR(CXType_OCLImage1dBufferRO),
        PAIR(CXType_OCLImage2dRO),
        PAIR(CXType_OCLImage2dArrayRO),
        PAIR(CXType_OCLImage2dDepthRO),
        PAIR(CXType_OCLImage2dArrayDepthRO),
        PAIR(CXType_OCLImage2dMSAARO),
        PAIR(CXType_OCLImage2dArrayMSAARO),
        PAIR(CXType_OCLImage2dMSAADepthRO),
        PAIR(CXType_OCLImage2dArrayMSAADepthRO),
        PAIR(CXType_OCLImage3dRO),
        PAIR(CXType_OCLImage1dWO),
        PAIR(CXType_OCLImage1dArrayWO),
        PAIR(CXType_OCLImage1dBufferWO),
        PAIR(CXType_OCLImage2dWO),
        PAIR(CXType_OCLImage2dArrayWO),
        PAIR(CXType_OCLImage2dDepthWO),
        PAIR(CXType_OCLImage2dArrayDepthWO),
        PAIR(CXType_OCLImage2dMSAAWO),
        PAIR(CXType_OCLImage2dArrayMSAAWO),
        PAIR(CXType_OCLImage2dMSAADepthWO),
        PAIR(CXType_OCLImage2dArrayMSAADepthWO),
        PAIR(CXType_OCLImage3dWO),
        PAIR(CXType_OCLImage1dRW),
        PAIR(CXType_OCLImage1dArrayRW),
        PAIR(CXType_OCLImage1dBufferRW),
        PAIR(CXType_OCLImage2dRW),
        PAIR(CXType_OCLImage2dArrayRW),
        PAIR(CXType_OCLImage2dDepthRW),
        PAIR(CXType_OCLImage2dArrayDepthRW),
        PAIR(CXType_OCLImage2dMSAARW),
        PAIR(CXType_OCLImage2dArrayMSAARW),
        PAIR(CXType_OCLImage2dMSAADepthRW),
        PAIR(CXType_OCLImage2dArrayMSAADepthRW),
        PAIR(CXType_OCLImage3dRW),
        PAIR(CXType_OCLSampler),
        PAIR(CXType_OCLEvent),
        PAIR(CXType_OCLQueue),
        PAIR(CXType_OCLReserveID),
        */
    }
};
#undef PAIR
#undef STRINGIFY

std::string Type::spelling() const
{
    return string( &clang_getTypeSpelling, type_ );
}

Type Type::get_canonical_type() const
{
    return Type( clang_getCanonicalType( type_ ), tu );
}

bool Type::operator==( const Type &rhs ) const
{
    return clang_equalTypes( type_, rhs.type_ );
}

Cursor Type::get_declaration() const
{
    return Cursor( clang_getTypeDeclaration( type_ ), tu );
}

Type Type::get_pointee() const
{
    return Type( clang_getPointeeType( type_ ), tu );
}

bool Type::is_const_qualified() const
{
    return clang_isConstQualifiedType( type_ );
}

std::string str( const CXTypeKind &c )
{
    const auto iter = type_kind_names.find( c );
    return iter == type_kind_names.end() ? std::to_string( static_cast<int>( c ) ) : iter->second;
}
