#pragma once
#ifndef CATA_FILE_SYSTEM_H
#define CATA_FILE_SYSTEM_H

#include <string>
#include <vector>
#include <iosfwd>

// NOTE: this is supposed to be compatible with std::filesystem.
// Do not add anything that is does not exist in std::filesystem!
namespace cata
{
class path
{
    private:
        std::string data;

    public:
        path() = default;
        path( const path & ) = default;
        path( path && ) = default;
        // mostly compatible with std::filesystem, but not as generic
        template<typename S>
        path( const S &source ) : data( source ) { }

        path &operator=( const path & ) = default;
        path &operator=( path && ) = default;

        /**
         * Returns the file name component of the given path, that is everything
         * after the last directory separator. Handles Windows and Unix path separators.
         * If the path does not contain a separator, the input is returned directly.
         *
         * Examples:
         * <code>
         * assert( path( "/a/b" ).filename() == "b" );
         * assert( path( "b" ).filename() == "b" );
         * assert( path( "/a/b/" ).filename() == "." );
         * </code>
         */
        path filename() const;
        /**
         * Returns the filename identified by the path stripped of its extension.
         * Returns the substring from the beginning of @ref filename up to and not including
         * the last period (.) character.
         * If the filename is one of the special filesystem components dot or dot-dot, or
         * if it has no periods, the function returns the entire filename().
         */
        path stem() const;
        /**
         * Returns the extension of the filename component of this path.
         * That is: if the @ref filename contains a period ('.') and it's not the first
         * character and the filename is not the special filesystem element "." or "..",
         * this returns the (rightmost) period and everything after it.
         * An empty path is returned otherwise.
         */
        path extension() const;
        /**
         * Returns the directory name of the given path. Handles Windows and Unix
         * path separators. If the path does not contain a separator, an empty
         * string will be returned.
         *
         * Examples:
         * <code>
         * assert( path( "/a/b" ).parent_path() == "/a" );
         * assert( path( "b" ).parent_path() == "b" );
         * </code>
         */
        path parent_path() const;

        const std::string &native() const {
            return data;
        }
        const char *c_str() const {
            return native().c_str();
        }
        operator std::string() const {
            return native();
        }

        path operator/( const path &p ) const;
        bool operator==( const path &p ) const {
            return native() == p.native();
        }
};

inline std::ostream &operator<<( std::ostream &s, const path &p )
{
    return s << '"' << p.string() << '"';
}

/**
 * @returns Whether the given path corresponds to an existing filesystem
 * object.
 */
bool exists( const path &path );
/**
 * The file or empty directory identified by the path is deleted.
 * @return `true` if the object was deleted, `false` if it did not exist, or could
 * not be deleted. This does not delete non-empty directories!
 */
bool remove( const path &path );
/**
 * Rename a file or directory. If the @p old_path is a file, the @p new_path will
 * be overridden.
 * @returns Whether renaming was successful.
 */
bool rename( const path &old_path, const path &new_path );
/**
 * Returns a vector of directories which contain files matching any of @p patterns.
 *
 * @param patterns A vector or patterns to match.
 * @see get_files_from_path
 */
std::vector<path> get_directories_with( const std::vector<std::string> &patterns,
                                        const path &root_path, bool recursive_search = false );
std::vector<path> get_directories_with( const std::string &pattern, const path &root_path,
                                        bool const recurse = false );
/**
 * Returns a vector of files or directories matching pattern at @p root_path.
 *
 * Searches through the directory tree breadth-first. Directories are searched in lexical
 * order. Matching files within in each directory are also ordered lexically.
 *
 * @param pattern The sub-string to match.
 * @param root_path The path relative to the current working directory to search; empty means ".".
 * @param recursive_search Whether to recursively search sub directories.
 * @param match_extension If true, match pattern at the end of file names. Otherwise, match anywhere
 *                        in the file name.
 */
std::vector<path> get_files_from_path( const std::string &pattern, const path &root_path,
                                       bool recursive_search = false, bool match_extension = false );
} // namespace cata

// Note: not part of namespace cata as it is not part of std::filesystem either
bool assure_dir_exist( const cata::path &path );

namespace cata_files
{
const char *eol();
}

// this remains to be used by chkjson.cpp
std::vector<std::string> get_files_from_path( std::string const &pattern,
        std::string const &root_path = "", bool recursive_search = false,
        bool match_extension = false );

// Note: not part of the cata namespace as there is no such function in std::filesystem
bool copy_file( const cata::path &source_path, const cata::path &dest_path );

#endif //CATA_FILE_SYSTEM_H
