#pragma once
#ifndef CATA_SRC_ITEMINFO_H
#define CATA_SRC_ITEMINFO_H

#include <string>

#include "enum_traits.h"

/**
 *  Value and metadata for one property of an item
 *
 *  Contains the value of one property of an item, as well as various metadata items required to
 *  output that value.  This is used primarily for user output of information about an item, for
 *  example in the various inventory menus.  See @ref item::info() for the main example of how a
 *  class desiring to provide user output might obtain a class of this type.
 *
 *  As an example, if the item being queried was a piece of clothing, then several properties might
 *  be returned.  All would have sType "ARMOR".  There would be one for the coverage stat with
 *  sName "Coverage: ", another for the warmth stat with sName "Warmth: ", etc.
 */
struct iteminfo {
    public:
        /** Category of item that owns this iteminfo.  See @ref item_category. */
        std::string sType;

        /** Main text of this property's name */
        std::string sName;

        /** Formatting text to be placed between the name and value of this item. */
        std::string sFmt;

        /** Numerical value of this property. Set to -999 if no compare value is present */
        std::string sValue;

        /** Internal double floating point version of value, for numerical comparisons */
        double dValue;

        /** Same as dValue, adjusted for the minimum unit (for numerical comparisons) */
        double dUnitAdjustedVal;

        /** Flag indicating type of sValue.  True if integer, false if single decimal */
        bool is_int;

        /** Flag indicating whether a newline should be printed after printing this item */
        bool bNewLine;

        /** Reverses behavior of red/green text coloring; smaller values are green if true */
        bool bLowerIsBetter;

        /** Whether to print sName.  If false, use for comparisons but don't print for user. */
        bool bDrawName;

        /** Whether to print a sign on positive values */
        bool bShowPlus;

        /** Flag indicating decimal with three points of precision.  */
        bool three_decimal;

        /** info is ASCII art (prefer monospaced font) */
        bool bIsArt;

        /** info is displayed as a table */
        bool isTable;

        enum flags {
            no_flags = 0,
            is_decimal = 1 << 0, ///< Print as decimal rather than integer
            is_three_decimal = 1 << 1, ///< Print as decimal with three points of precision
            no_newline = 1 << 2, ///< Do not follow with a newline
            lower_is_better = 1 << 3, ///< Lower values are better for this stat
            no_name = 1 << 4, ///< Do not print the name
            show_plus = 1 << 5, ///< Use a + sign for positive values
            is_art = 1 << 6, ///< is ascii art (prefer monospaced font)
            is_table = 1 << 7, ///< is displayed as table
        };

        /**
         *  @param Type The item type of the item this iteminfo belongs to.
         *  @param Name The name of the property this iteminfo describes.
         *  @param Fmt Formatting text desired between item name and value
         *  @param Flags Additional flags to customize this entry
         *  @param Value Numerical value of this property, -999 for none.
         */
        iteminfo( const std::string &Type, const std::string &Name, const std::string &Fmt = "",
                  flags Flags = no_flags, double Value = -999, double UnitVal = 0 );
        iteminfo( const std::string &Type, const std::string &Name, flags Flags );
        iteminfo( const std::string &Type, const std::string &Name, double Value, double UnitVal = 0 );
};

template<>
struct enum_traits<iteminfo::flags> {
    static constexpr bool is_flag_enum = true;
};

#endif // CATA_SRC_ITEMINFO_H
