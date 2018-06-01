#include "Cursor.h"

#include "common-clang.h"
#include "common.h"
#include "Type.h"
#include "Parser.h"
#include "FullyQualifiedId.h"
#include "TranslationUnit.h"
#include "TokenRange.h"

#include <cstring>
#include <set>
#include <iostream>
#include <map>
#include <algorithm>

#define STRINGIFY(x) #x
#define PAIR(x) { x, STRINGIFY(x) }
static const std::map<int, const char *> cursor_kind_names = { {
        PAIR( CXCursor_UnexposedDecl ),
        PAIR( CXCursor_StructDecl ),
        PAIR( CXCursor_UnionDecl ),
        PAIR( CXCursor_ClassDecl ),
        PAIR( CXCursor_EnumDecl ),
        PAIR( CXCursor_FieldDecl ),
        PAIR( CXCursor_EnumConstantDecl ),
        PAIR( CXCursor_FunctionDecl ),
        PAIR( CXCursor_VarDecl ),
        PAIR( CXCursor_ParmDecl ),
        PAIR( CXCursor_ObjCInterfaceDecl ),
        PAIR( CXCursor_ObjCCategoryDecl ),
        PAIR( CXCursor_ObjCProtocolDecl ),
        PAIR( CXCursor_ObjCPropertyDecl ),
        PAIR( CXCursor_ObjCIvarDecl ),
        PAIR( CXCursor_ObjCInstanceMethodDecl ),
        PAIR( CXCursor_ObjCClassMethodDecl ),
        PAIR( CXCursor_ObjCImplementationDecl ),
        PAIR( CXCursor_ObjCCategoryImplDecl ),
        PAIR( CXCursor_TypedefDecl ),
        PAIR( CXCursor_CXXMethod ),
        PAIR( CXCursor_Namespace ),
        PAIR( CXCursor_LinkageSpec ),
        PAIR( CXCursor_Constructor ),
        PAIR( CXCursor_Destructor ),
        PAIR( CXCursor_ConversionFunction ),
        PAIR( CXCursor_TemplateTypeParameter ),
        PAIR( CXCursor_NonTypeTemplateParameter ),
        PAIR( CXCursor_TemplateTemplateParameter ),
        PAIR( CXCursor_FunctionTemplate ),
        PAIR( CXCursor_ClassTemplate ),
        PAIR( CXCursor_ClassTemplatePartialSpecialization ),
        PAIR( CXCursor_NamespaceAlias ),
        PAIR( CXCursor_UsingDirective ),
        PAIR( CXCursor_UsingDeclaration ),
        PAIR( CXCursor_TypeAliasDecl ),
        PAIR( CXCursor_ObjCSynthesizeDecl ),
        PAIR( CXCursor_ObjCDynamicDecl ),
        PAIR( CXCursor_CXXAccessSpecifier ),
        PAIR( CXCursor_FirstDecl ),
        PAIR( CXCursor_LastDecl ),
        PAIR( CXCursor_FirstRef ),
        PAIR( CXCursor_ObjCSuperClassRef ),
        PAIR( CXCursor_ObjCProtocolRef ),
        PAIR( CXCursor_ObjCClassRef ),
        PAIR( CXCursor_TypeRef ),
        PAIR( CXCursor_CXXBaseSpecifier ),
        PAIR( CXCursor_TemplateRef ),
        PAIR( CXCursor_NamespaceRef ),
        PAIR( CXCursor_MemberRef ),
        PAIR( CXCursor_LabelRef ),
        PAIR( CXCursor_OverloadedDeclRef ),
        PAIR( CXCursor_VariableRef ),
        PAIR( CXCursor_LastRef ),
        PAIR( CXCursor_FirstInvalid ),
        PAIR( CXCursor_InvalidFile ),
        PAIR( CXCursor_NoDeclFound ),
        PAIR( CXCursor_NotImplemented ),
        PAIR( CXCursor_InvalidCode ),
        PAIR( CXCursor_LastInvalid ),
        PAIR( CXCursor_FirstExpr ),
        PAIR( CXCursor_UnexposedExpr ),
        PAIR( CXCursor_DeclRefExpr ),
        PAIR( CXCursor_MemberRefExpr ),
        PAIR( CXCursor_CallExpr ),
        PAIR( CXCursor_ObjCMessageExpr ),
        PAIR( CXCursor_BlockExpr ),
        PAIR( CXCursor_IntegerLiteral ),
        PAIR( CXCursor_FloatingLiteral ),
        PAIR( CXCursor_ImaginaryLiteral ),
        PAIR( CXCursor_StringLiteral ),
        PAIR( CXCursor_CharacterLiteral ),
        PAIR( CXCursor_ParenExpr ),
        PAIR( CXCursor_UnaryOperator ),
        PAIR( CXCursor_ArraySubscriptExpr ),
        PAIR( CXCursor_BinaryOperator ),
        PAIR( CXCursor_CompoundAssignOperator ),
        PAIR( CXCursor_ConditionalOperator ),
        PAIR( CXCursor_CStyleCastExpr ),
        PAIR( CXCursor_CompoundLiteralExpr ),
        PAIR( CXCursor_InitListExpr ),
        PAIR( CXCursor_AddrLabelExpr ),
        PAIR( CXCursor_StmtExpr ),
        PAIR( CXCursor_GenericSelectionExpr ),
        PAIR( CXCursor_GNUNullExpr ),
        PAIR( CXCursor_CXXStaticCastExpr ),
        PAIR( CXCursor_CXXDynamicCastExpr ),
        PAIR( CXCursor_CXXReinterpretCastExpr ),
        PAIR( CXCursor_CXXConstCastExpr ),
        PAIR( CXCursor_CXXFunctionalCastExpr ),
        PAIR( CXCursor_CXXTypeidExpr ),
        PAIR( CXCursor_CXXBoolLiteralExpr ),
        PAIR( CXCursor_CXXNullPtrLiteralExpr ),
        PAIR( CXCursor_CXXThisExpr ),
        PAIR( CXCursor_CXXThrowExpr ),
        PAIR( CXCursor_CXXNewExpr ),
        PAIR( CXCursor_CXXDeleteExpr ),
        PAIR( CXCursor_UnaryExpr ),
        PAIR( CXCursor_ObjCStringLiteral ),
        PAIR( CXCursor_ObjCEncodeExpr ),
        PAIR( CXCursor_ObjCSelectorExpr ),
        PAIR( CXCursor_ObjCProtocolExpr ),
        PAIR( CXCursor_ObjCBridgedCastExpr ),
        PAIR( CXCursor_PackExpansionExpr ),
        PAIR( CXCursor_SizeOfPackExpr ),
        PAIR( CXCursor_LambdaExpr ),
        PAIR( CXCursor_ObjCBoolLiteralExpr ),
        PAIR( CXCursor_ObjCSelfExpr ),
        PAIR( CXCursor_OMPArraySectionExpr ),
        PAIR( CXCursor_ObjCAvailabilityCheckExpr ),
        PAIR( CXCursor_LastExpr ),
        PAIR( CXCursor_FirstStmt ),
        PAIR( CXCursor_UnexposedStmt ),
        PAIR( CXCursor_LabelStmt ),
        PAIR( CXCursor_CompoundStmt ),
        PAIR( CXCursor_CaseStmt ),
        PAIR( CXCursor_DefaultStmt ),
        PAIR( CXCursor_IfStmt ),
        PAIR( CXCursor_SwitchStmt ),
        PAIR( CXCursor_WhileStmt ),
        PAIR( CXCursor_DoStmt ),
        PAIR( CXCursor_ForStmt ),
        PAIR( CXCursor_GotoStmt ),
        PAIR( CXCursor_IndirectGotoStmt ),
        PAIR( CXCursor_ContinueStmt ),
        PAIR( CXCursor_BreakStmt ),
        PAIR( CXCursor_ReturnStmt ),
        PAIR( CXCursor_GCCAsmStmt ),
        PAIR( CXCursor_AsmStmt ),
        PAIR( CXCursor_ObjCAtTryStmt ),
        PAIR( CXCursor_ObjCAtCatchStmt ),
        PAIR( CXCursor_ObjCAtFinallyStmt ),
        PAIR( CXCursor_ObjCAtThrowStmt ),
        PAIR( CXCursor_ObjCAtSynchronizedStmt ),
        PAIR( CXCursor_ObjCAutoreleasePoolStmt ),
        PAIR( CXCursor_ObjCForCollectionStmt ),
        PAIR( CXCursor_CXXCatchStmt ),
        PAIR( CXCursor_CXXTryStmt ),
        PAIR( CXCursor_CXXForRangeStmt ),
        PAIR( CXCursor_SEHTryStmt ),
        PAIR( CXCursor_SEHExceptStmt ),
        PAIR( CXCursor_SEHFinallyStmt ),
        PAIR( CXCursor_MSAsmStmt ),
        PAIR( CXCursor_NullStmt ),
        PAIR( CXCursor_DeclStmt ),
        PAIR( CXCursor_OMPParallelDirective ),
        PAIR( CXCursor_OMPSimdDirective ),
        PAIR( CXCursor_OMPForDirective ),
        PAIR( CXCursor_OMPSectionsDirective ),
        PAIR( CXCursor_OMPSectionDirective ),
        PAIR( CXCursor_OMPSingleDirective ),
        PAIR( CXCursor_OMPParallelForDirective ),
        PAIR( CXCursor_OMPParallelSectionsDirective ),
        PAIR( CXCursor_OMPTaskDirective ),
        PAIR( CXCursor_OMPMasterDirective ),
        PAIR( CXCursor_OMPCriticalDirective ),
        PAIR( CXCursor_OMPTaskyieldDirective ),
        PAIR( CXCursor_OMPBarrierDirective ),
        PAIR( CXCursor_OMPTaskwaitDirective ),
        PAIR( CXCursor_OMPFlushDirective ),
        PAIR( CXCursor_SEHLeaveStmt ),
        PAIR( CXCursor_OMPOrderedDirective ),
        PAIR( CXCursor_OMPAtomicDirective ),
        PAIR( CXCursor_OMPForSimdDirective ),
        PAIR( CXCursor_OMPParallelForSimdDirective ),
        PAIR( CXCursor_OMPTargetDirective ),
        PAIR( CXCursor_OMPTeamsDirective ),
        PAIR( CXCursor_OMPTaskgroupDirective ),
        PAIR( CXCursor_OMPCancellationPointDirective ),
        PAIR( CXCursor_OMPCancelDirective ),
        PAIR( CXCursor_OMPTargetDataDirective ),
        PAIR( CXCursor_OMPTaskLoopDirective ),
        PAIR( CXCursor_OMPTaskLoopSimdDirective ),
        PAIR( CXCursor_OMPDistributeDirective ),
        PAIR( CXCursor_OMPTargetEnterDataDirective ),
        PAIR( CXCursor_OMPTargetExitDataDirective ),
        PAIR( CXCursor_OMPTargetParallelDirective ),
        PAIR( CXCursor_OMPTargetParallelForDirective ),
        PAIR( CXCursor_OMPTargetUpdateDirective ),
        PAIR( CXCursor_OMPDistributeParallelForDirective ),
        PAIR( CXCursor_OMPDistributeParallelForSimdDirective ),
        PAIR( CXCursor_OMPDistributeSimdDirective ),
        PAIR( CXCursor_OMPTargetParallelForSimdDirective ),
        PAIR( CXCursor_OMPTargetSimdDirective ),
        PAIR( CXCursor_OMPTeamsDistributeDirective ),
        PAIR( CXCursor_OMPTeamsDistributeSimdDirective ),
        PAIR( CXCursor_OMPTeamsDistributeParallelForSimdDirective ),
        PAIR( CXCursor_OMPTeamsDistributeParallelForDirective ),
        PAIR( CXCursor_OMPTargetTeamsDirective ),
        PAIR( CXCursor_OMPTargetTeamsDistributeDirective ),
        PAIR( CXCursor_OMPTargetTeamsDistributeParallelForDirective ),
        PAIR( CXCursor_OMPTargetTeamsDistributeParallelForSimdDirective ),
        PAIR( CXCursor_OMPTargetTeamsDistributeSimdDirective ),
        PAIR( CXCursor_LastStmt ),
        PAIR( CXCursor_TranslationUnit ),
        PAIR( CXCursor_FirstAttr ),
        PAIR( CXCursor_UnexposedAttr ),
        PAIR( CXCursor_IBActionAttr ),
        PAIR( CXCursor_IBOutletAttr ),
        PAIR( CXCursor_IBOutletCollectionAttr ),
        PAIR( CXCursor_CXXFinalAttr ),
        PAIR( CXCursor_CXXOverrideAttr ),
        PAIR( CXCursor_AnnotateAttr ),
        PAIR( CXCursor_AsmLabelAttr ),
        PAIR( CXCursor_PackedAttr ),
        PAIR( CXCursor_PureAttr ),
        PAIR( CXCursor_ConstAttr ),
        PAIR( CXCursor_NoDuplicateAttr ),
        PAIR( CXCursor_CUDAConstantAttr ),
        PAIR( CXCursor_CUDADeviceAttr ),
        PAIR( CXCursor_CUDAGlobalAttr ),
        PAIR( CXCursor_CUDAHostAttr ),
        PAIR( CXCursor_CUDASharedAttr ),
        PAIR( CXCursor_VisibilityAttr ),
        PAIR( CXCursor_DLLExport ),
        PAIR( CXCursor_DLLImport ),
        PAIR( CXCursor_LastAttr ),
        PAIR( CXCursor_PreprocessingDirective ),
        PAIR( CXCursor_MacroDefinition ),
        PAIR( CXCursor_MacroExpansion ),
        PAIR( CXCursor_MacroInstantiation ),
        PAIR( CXCursor_InclusionDirective ),
        PAIR( CXCursor_FirstPreprocessing ),
        PAIR( CXCursor_LastPreprocessing ),
        PAIR( CXCursor_ModuleImportDecl ),
        PAIR( CXCursor_TypeAliasTemplateDecl ),
        PAIR( CXCursor_StaticAssert ),
        PAIR( CXCursor_FriendDecl ),
        PAIR( CXCursor_FirstExtraDecl ),
        PAIR( CXCursor_LastExtraDecl ),
        PAIR( CXCursor_OverloadCandidate ),
    }
};
#undef PAIR
#undef STRINGIFY

std::string Cursor::spelling()const
{
    return string( &clang_getCursorSpelling, cursor_ );
}

Type Cursor::type() const
{
    return Type( clang_getCursorType( cursor_ ), tu );
}

std::vector<Cursor> Cursor::get_arguments() const
{
    std::vector<Cursor> result;
    const int n = clang_Cursor_getNumArguments( cursor_ );
    for( int i = 0; i < n; ++i ) {
        result.emplace_back( clang_Cursor_getArgument( cursor_, i ), tu );
    }
    return result;
}

std::vector<Type> Cursor::get_template_arguments() const
{
    std::vector<Type> result;
    //@todo
    /*
    const int n = clang_Cursor_getNumTemplateArguments( cursor_ );
    for( int i = 0; i < n; ++i ) {
        result.emplace_back( clang_Cursor_getTemplateArgumentType( cursor_, i ) );
    }
    */
    return result;
}

TokenRange Cursor::tokens() const
{
    return TokenRange( tu.get().tu, clang_getCursorExtent( cursor_ ) );
}

bool Cursor::has_default_value() const
{
    // Sadly, this is not directly exposed by clang, one has to "parse" the parameter
    // declaration oneself.
    for( const Token &t : tokens() ) {
        if( t.spelling() == "=" ) {
            return true;
        }
    }
    return false;
}

bool Cursor::is_static_method() const
{
    return clang_CXXMethod_isStatic( cursor_ );
}

bool Cursor::is_const_method() const
{
    return false;
    //    return clang_CXXMethod_isConst(cursor_);
}

bool Cursor::is_public() const
{
    return clang_getCXXAccessSpecifier( cursor_ ) == CX_CXXPublic;
}

Cursor Cursor::get_definition() const
{
    return Cursor( clang_getCursorDefinition( cursor_ ), tu );
}

Type Cursor::get_result_type() const
{
    return Type( clang_getCursorResultType( cursor_ ), tu );
}

Type Cursor::get_underlying_type() const
{
    return Type( clang_getTypedefDeclUnderlyingType( cursor_ ), tu );
}
bool Cursor::is_definition() const
{
    return clang_isCursorDefinition( cursor_ );
}

class static_visitor_data
{
    public:
        const Cursor::VisitorType &func;
        const TranslationUnit &tu;
};

static inline CXChildVisitResult static_visitor( CXCursor cursor, CXCursor parent,
        CXClientData data )
{
    static_visitor_data &da = *static_cast<static_visitor_data *>( data );
    return da.func( Cursor( cursor, da.tu ), Cursor( parent, da.tu ) );
}

void Cursor::visit_children( const VisitorType &func ) const
{
    static_visitor_data data{ func, tu.get() };
    clang_visitChildren( cursor_, &static_visitor, static_cast<CXClientData>( &data ) );
}

Cursor Cursor::get_semantic_parent() const
{
    return Cursor( clang_getCursorSemanticParent( cursor_ ), tu );
}

FullyQualifiedId Cursor::fully_qualifid() const
{
    if( kind() == CXCursor_TranslationUnit ) {
        return FullyQualifiedId();
    }
    FullyQualifiedId result( spelling() );
    Cursor p = get_semantic_parent();
    while( true ) {
        const CXCursorKind k = p.kind();
        if( k == CXCursor_TranslationUnit ) {
            break;
        } else if( k == CXCursor_FirstInvalid ) {
            break;
        } else if( k == CXCursor_Namespace || k == CXCursor_ClassDecl || k == CXCursor_StructDecl ) {
            result = FullyQualifiedId( p.fully_qualifid(), result.as_string() );
        }
        p = p.get_semantic_parent();
    }
    return result;
}

void dump( std::ostream &stream, const std::string &msg, const Cursor &c, size_t i,
           std::vector<Cursor> &cursors, std::vector<Type> &types );

void dump( std::ostream &stream, const std::string &msg, const Type &t, size_t i,
           std::vector<Cursor> &cursors, std::vector<Type> &types )
{
    if( t.kind() == CXType_Invalid ) {
        return;
    }
    const std::string tab( i * 2, ' ' );
    stream << tab << msg << ": " << t.spelling() << " [" << str( t.kind() ) << "]\n";

    if( std::find( types.begin(), types.end(), t ) != types.end() ) {
        return;
    }
    types.emplace_back( t );
    i++;

    dump( stream, "canonical type is", t.get_canonical_type(), i, cursors, types );
    dump( stream, "pointee is", t.get_pointee(), i, cursors, types );
    dump( stream, "declaration is", t.get_declaration(), i, cursors, types );
}

void dump( std::ostream &stream, const std::string &msg, const Cursor &c, size_t i,
           std::vector<Cursor> &cursors, std::vector<Type> &types )
{
    if( c.kind() == CXCursor_FirstInvalid ) {
        return;
    }
    const std::string tab( i * 2, ' ' );
    stream << tab << msg << ": " << c.spelling() << " [" << str( c.kind() ) << "]\n";

    if( std::find( cursors.begin(), cursors.end(), c ) != cursors.end() ) {
        return;
    }
    cursors.emplace_back( c );
    i++;

    stream << tab << "fully qualified: " << c.fully_qualifid().as_string() << "\n";
    stream << tab << "location: " << c.location() << "\n";

    dump( stream, "parent", c.get_semantic_parent(), i, cursors, types );
    dump( stream, "definition is", c.get_definition(), i, cursors, types );
    dump( stream, "underlying type is", c.get_underlying_type(), i, cursors, types );
    dump( stream, "result type is", c.get_result_type(), i, cursors, types );
    dump( stream, "type is", c.type(), i, cursors, types );
    for( const Cursor &o : c.get_arguments() ) {
        dump( stream, "argument", o, i, cursors, types );
    }
    for( const Type &t : c.get_template_arguments() ) {
        dump( stream, "template argument", t, i, cursors, types );
    }
}

std::string Cursor::location() const
{
    CXSourceLocation sl = clang_getCursorLocation( cursor_ );
    CXString filename;
    unsigned line;
    unsigned column;
    clang_getPresumedLocation( sl, &filename, &line, &column );
    std::string s( clang_getCString( filename ) );
    clang_disposeString( filename );
    return s + ":" + std::to_string( line ) + ":" + std::to_string( column );
}

std::string Cursor::location_path() const
{
    CXSourceLocation sl = clang_getCursorLocation( cursor_ );
    CXString filename;
    unsigned line;
    unsigned column;
    clang_getPresumedLocation( sl, &filename, &line, &column );
    std::string s( clang_getCString( filename ) );
    clang_disposeString( filename );
    return s;
}

std::string Cursor::location_file() const
{
    const std::string p = location_path();
    const size_t n = p.find_last_of( "\\//" );
    return n != std::string::npos ? p.substr( n + 1 ) : p;
}

void Cursor::dump( const std::string &msg ) const
{
    std::vector<Cursor> cursors;
    std::vector<Type> types;
    ::dump( std::cout, msg, *this, 0, cursors, types );
}

void Type::dump( const std::string &msg ) const
{
    std::vector<Cursor> cursors;
    std::vector<Type> types;
    ::dump( std::cout, msg, *this, 0, cursors, types );
}

bool Cursor::operator==( const Cursor &rhs ) const
{
    return clang_equalCursors( cursor_, rhs.cursor_ );
}

std::string str( const CXCursorKind &c )
{
    const auto iter = cursor_kind_names.find( c );
    return iter == cursor_kind_names.end() ? std::to_string( static_cast<int>( c ) ) : iter->second;
}

std::string Cursor::raw_comment() const
{
    //@todo
    return string( &clang_Cursor_getRawCommentText, cursor_ ) ;
    //    return "";
}
