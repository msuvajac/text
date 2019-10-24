#ifndef BOOST_TEXT_SAVE_LOAD_TABLE_HPP
#define BOOST_TEXT_SAVE_LOAD_TABLE_HPP

#include <boost/text/collation_table.hpp>

#include <boost/filesystem.hpp>

#include <exception>


namespace boost { namespace text { inline namespace v1 {

    struct collation_table;

    /** The type of exception thrown by load_table() when it encounters bad
        invalid data. */
    struct invalid_table : std::exception
    {
        invalid_table(char const * msg) : msg_(msg) {}

        virtual char const * what() const noexcept override { return msg_; }

    private:
        char const * msg_;
    };

    /** Writes the given collation table to `path`.  Returns `true` iff the
        path is valid and the save succeeded. */
    bool
    save_table(collation_table const & table, filesystem::path const & path);

    /** Reads a collation table from `path`.  May throw invalid_table if given
        an unreadable path, or a path to bad data. */
    collation_table load_table(filesystem::path const & path);

}}}

#endif
