#include "TranslationUnit.h"

#include "common-clang.h"
#include "Cursor.h"
#include "Index.h"

#include <stdexcept>

TranslationUnit::TranslationUnit( const Index &index, const std::vector<const char*> &args, const std::string &text, const unsigned flags )
{
    CXUnsavedFile unsaved[1] = { { args.back(), text.c_str(), text.length() } };
    const unsigned unsaved_count = text.empty() ? 0 : 1;
#if 0
    const CXErrorCode err = clang_parseTranslationUnit2( index.index_, 0, args.data(), args.size(), unsaved, unsaved_count,
                            flags, &tu );
    switch( err ) {
        case CXError_Success:
            break;
        case CXError_Failure:
            throw std::runtime_error( "failed to parse " + header );
        case CXError_Crashed:
            throw std::runtime_error( "clang crashed while parsing " + header );
        case CXError_InvalidArguments:
            throw std::runtime_error( "invalid arguments" );
        case CXError_ASTReadError:
            throw std::runtime_error( "AST read error" );
    }
#else
    tu = clang_parseTranslationUnit( index.index_, 0, args.data(), args.size(), unsaved, unsaved_count, flags );
    if( !tu ) {
        throw std::runtime_error( "failed to create translation unit" );
    }
#endif
}

TranslationUnit::TranslationUnit( const Index &index, const std::vector<const char*> &args, const unsigned flags )
{
#if 0
    const CXErrorCode err = clang_parseTranslationUnit2( index.index_, 0, args.data(), args.size(), 0, 0,
                            flags, &tu );
    switch( err ) {
        case CXError_Success:
            break;
        case CXError_Failure:
            throw std::runtime_error( "failed to parse " + header );
        case CXError_Crashed:
            throw std::runtime_error( "clang crashed while parsing " + header );
        case CXError_InvalidArguments:
            throw std::runtime_error( "invalid arguments" );
        case CXError_ASTReadError:
            throw std::runtime_error( "AST read error" );
    }
#else
    tu = clang_parseTranslationUnit( index.index_, 0, args.data(), args.size(), 0, 0, flags );
    if( !tu ) {
        throw std::runtime_error( "failed to create translation unit" );
    }
#endif
}

TranslationUnit::~TranslationUnit()
{
    clang_disposeTranslationUnit( tu );
}

std::vector<std::string> TranslationUnit::get_diagnostics() const
{
    std::vector<std::string> result;
    for( unsigned int i = 0, N = clang_getNumDiagnostics( tu ); i < N; ++i ) {
        const CXDiagnostic dia = clang_getDiagnostic( tu, i );
        result.emplace_back( string( &clang_formatDiagnostic, dia,
                                     clang_defaultDiagnosticDisplayOptions() ) );
    }
    return result;
}

Cursor TranslationUnit::cursor() const
{
    return Cursor( clang_getTranslationUnitCursor( tu ), *this );
}


static void include_visitor( CXFile f, CXSourceLocation */*stack*/, unsigned /*len*/,
                             CXClientData data )
{
    auto &files = *static_cast<std::vector<std::string>*>( data );
    files.emplace_back( string( &clang_getFileName, f ) );
}

std::vector<std::string> TranslationUnit::get_includes() const
{
    std::vector<std::string> result;
    clang_getInclusions( tu, &include_visitor, static_cast<CXClientData>( &result ) );
    return result;
}
