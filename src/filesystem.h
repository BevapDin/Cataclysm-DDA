#pragma once
#ifndef CATA_FILE_SYSTEM_H
#define CATA_FILE_SYSTEM_H

#include <string>
#include <vector>

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
        path operator/( const std::string &p ) const {
            return operator/( path( p ) );
        }
};
/**
 * @returns Whether the given path corresponds to an existing filesystem
 * object.
 */
bool exists( const path &path );
} // namespace cata

bool assure_dir_exist( std::string const &path );
// Remove a file, does not remove folders,
// returns true on success
bool remove_file( const std::string &path );
bool remove_directory( const std::string &path );
// Rename a file, overriding the target!
bool rename_file( const std::string &old_path, const std::string &new_path );

namespace cata_files
{
const char *eol();
}

//--------------------------------------------------------------------------------------------------
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
std::vector<std::string> get_files_from_path( std::string const &pattern,
        std::string const &root_path = "", bool recursive_search = false,
        bool match_extension = false );

//--------------------------------------------------------------------------------------------------
/**
 * Returns a vector of directories which contain files matching any of @p patterns.
 *
 * @param patterns A vector or patterns to match.
 * @see get_files_from_path
 */
std::vector<std::string> get_directories_with( std::vector<std::string> const &patterns,
        std::string const &root_path = "", bool recursive_search = false );

std::vector<std::string> get_directories_with( std::string const &pattern,
        std::string const &root_path = "", bool const recurse = false );

bool copy_file( const std::string &source_path, const std::string &dest_path );

#endif //CATA_FILE_SYSTEM_H
