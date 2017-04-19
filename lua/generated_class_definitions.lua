classes = {
    effect_type = {
        string_id = "efftype_id",
        attributes = {
            id = { type = "efftype_id" },
        },
        functions = {
            { name = "gain_game_message_type", rval = "game_message_type", comment = "/** Returns the appropriate game_message_type when a new effect is obtained. This is equal to
         *  an effect's "rating" value. */", args = { } },
            { name = "get_apply_memorial_log", rval = "std::string", comment = "/** Returns the memorial log added when a new effect is obtained. */", args = { } },
            { name = "get_apply_message", rval = "std::string", comment = "/** Returns the message displayed when a new effect is obtained. */", args = { } },
            { name = "get_main_parts", rval = "bool", comment = "/** Returns true if an effect will only target main body parts (i.e., those with HP). */", args = { } },
            { name = "get_remove_memorial_log", rval = "std::string", comment = "/** Returns the memorial log added when an effect is removed. */", args = { } },
            { name = "get_remove_message", rval = "std::string", comment = "/** Returns the message displayed when an effect is removed. */", args = { } },
            { name = "lose_game_message_type", rval = "game_message_type", comment = "/** Returns the appropriate game_message_type when an effect is lost. This is opposite to
         *  an effect's "rating" value. */", args = { } },
            { name = "register_ma_buff_effect", static = true, rval = nil, comment = "/** Registers the effect in the global map */", args = { "effect_type" } },
            { name = "use_desc_ints", rval = "bool", comment = "/** Returns true if there is a listed description in the JSON entry for each intensity
         *  from 1 to max_intensity with the matching reduced value. */", args = { "bool" } },
            { name = "use_name_ints", rval = "bool", comment = "/** Returns true if there is a listed name in the JSON entry for each intensity from
         *  1 to max_intensity. */", args = { } },
        }
    },
    calendar = {
        new = {
            { "calendar" },
            { "int" },
            { "int", "int", "int", "season_type", "int" },
            { },
        },
        by_value_and_reference = true,
        has_equal = true,
        functions = {
            { name = "current_daylight_level", rval = "float", comment = "/** Returns the current seasonally-adjusted maximum daylight level */", args = { } },
            { name = "day_of_week", rval = "std::string", comment = "/**
         * Returns the name of the current day of the week
         *
         * @note: Day 0 is a Thursday for highly technical reasons which are hard to explain
         */", args = { } },
            { name = "day_of_year", rval = "int", comment = "/** @returns Number of days elapsed in current year */", args = { } },
            { name = "days", rval = "int", comment = "/** Days since start of current season */", args = { } },
            { name = "diurnal_time_before", rval = "int", comment = "/**
         * Calculate number of turns until a specified time.
         *
         * @param turn Specified diurnal time and date (in turns)
         * @returns the remaining time (in turns) before the specified diurnal time */", args = { "int" } },
            { name = "get_season", rval = "season_type", comment = "/** Current season */", args = { } },
            { name = "get_turn", rval = "int", comment = "/** @returns the current turn_number. */", args = { } },
            { name = "hours", rval = "int", comment = "/** Hour portion of current time of day */", args = { } },
            { name = "increment", rval = nil, comment = "/** Increases turn_number by 1. (6 seconds) */", args = { } },
            { name = "is_night", rval = "bool", comment = "/** Returns true if it's currently after sunset + TWILIGHT_SECONDS or before sunrise - TWILIGHT_SECONDS. */", args = { } },
            { name = "minutes", rval = "int", comment = "/** Minutes portion of current time of day */", args = { } },
            { name = "minutes_past_midnight", rval = "int", comment = "/** Returns the number of minutes past midnight. Used for weather calculations. */", args = { } },
            { name = "name_season", static = true, rval = "std::string", comment = "/** Returns the translated name of the season. */", args = { "season_type" } },
            { name = "once_every", static = true, rval = "bool", comment = "/**
         * Predicate to handle rate-limiting.
         *
         * @param event_frequency Number of turns between true returns
         * @returns true after every 'event_frequency' turns
         */", args = { "int" } },
            { name = "print_approx_duration", static = true, rval = "std::string", comment = "/**
         * Returns approximate duration.
         * @param turns Duration to print, measured in six-second turns
         * @param verbose If true, 'less than' and 'more than' will be printed instead of '<' and '>' respectively.
         */", args = { "int" } },
            { name = "print_approx_duration", static = true, rval = "std::string", comment = "/**
         * Returns approximate duration.
         * @param turns Duration to print, measured in six-second turns
         * @param verbose If true, 'less than' and 'more than' will be printed instead of '<' and '>' respectively.
         */", args = { "int", "bool" } },
            { name = "print_clipped_duration", static = true, rval = "std::string", comment = "/**
         * Print approximate duration in largest practical units
         *
         * Generates a string showing a duration in appropriate units.  Durations of under one
         * minute are shown in seconds.  Durations of one minute to one hour are shown as a whole
         * number of minutes.  Durations of one hour to one day are shown as a whole number of hours.
         * Larger durations are shown as a number of whole days.
         *
         * @param turns Specified duration in six-second turns.
         * @returns String with value and units, suitable for display to player
         */", args = { "int" } },
            { name = "print_duration", static = true, rval = "std::string", comment = "/** Returns normal duration. */", args = { "int" } },
            { name = "print_time", rval = "std::string", comment = "/** Returns the current time in a string according to the options set */", args = { "bool" } },
            { name = "print_time", rval = "std::string", comment = "/** Returns the current time in a string according to the options set */", args = { } },
            { name = "season_length", static = true, rval = "int", comment = "/** @returns Number of days in a season (configured in current world settings) */", args = { } },
            { name = "season_ratio", static = true, rval = "float", args = { } },
            { name = "season_turns", static = true, rval = "int", comment = "/** @returns Number of six-second turns in a season (configured in current world settings) */", args = { } },
            { name = "seconds", rval = "int", comment = "/** Seconds portion of current time of day */", args = { } },
            { name = "seconds_past_midnight", rval = "int", comment = "/** Returns the number of seconds past midnight. Used for sunrise/set calculations. */", args = { } },
            { name = "sunlight", rval = "float", comment = "/** Returns the current sunlight or moonlight level through the preceding functions. */", args = { } },
            { name = "sunrise", rval = "calendar", comment = "/** Returns the current sunrise time based on the time of year. */", args = { } },
            { name = "sunset", rval = "calendar", comment = "/** Returns the current sunset time based on the time of year. */", args = { } },
            { name = "textify_period", rval = "std::string", comment = "/** Returns the period a calendar has been running in word form; i.e. "1 second", "2 days". */", args = { } },
            { name = "turn_of_year", rval = "int", comment = "/** @returns Number of turns elapsed in current year */", args = { } },
            { name = "year_length", static = true, rval = "int", comment = "/** @returns Number of days in a year, (configured in current world settings) */", args = { } },
            { name = "year_turns", static = true, rval = "int", comment = "/** Number of six-second turns in a year, (configured in current world settings) */", args = { } },
            { name = "years", rval = "int", comment = "/** Current year, with default game start as year 0 */", args = { } },
        }
    },
    Character = {
        parent = "Creature",
        attributes = {
            dex_cur = { type = "int", writable = true },
            dex_max = { type = "int", writable = true },
            int_cur = { type = "int", writable = true },
            int_max = { type = "int", writable = true },
            last_item = { type = "std::string", writable = true },
            male = { type = "bool", writable = true },
            name = { type = "std::string", writable = true },
            nv_cached = { type = "bool", writable = true },
            per_cur = { type = "int", writable = true },
            per_max = { type = "int", writable = true },
            ret_null = { type = "item", writable = true },
            str_cur = { type = "int", writable = true },
            str_max = { type = "int", writable = true },
            weapon = { type = "item", writable = true },
            worn = { type = "std::list<item>", writable = true },
        },
        functions = {
            { name = "add_traits", rval = nil, comment = "/** Adds mandatory scenario and profession traits unless you already have them */", args = { } },
            { name = "aim_per_move", rval = "float", args = { "item", "float" } },
            { name = "ammo_count_for", rval = "int", comment = "/**
         * Counts ammo and UPS charges (lower of) for a given gun on the character.
         */", args = { "item" } },
            { name = "amount_of", rval = "int", comment = "/**
         * Count items matching id including both this instance and any contained items
         * @param what ID of items to count
         * @param pseudo whether pseudo-items (from map/vehicle tiles, bionics etc) are considered
         * @param limit stop searching after this many matches
         * @note items must be empty to be considered a match
         */", args = { "std::string" } },
            { name = "amount_of", rval = "int", comment = "/**
         * Count items matching id including both this instance and any contained items
         * @param what ID of items to count
         * @param pseudo whether pseudo-items (from map/vehicle tiles, bionics etc) are considered
         * @param limit stop searching after this many matches
         * @note items must be empty to be considered a match
         */", args = { "std::string", "bool" } },
            { name = "amount_of", rval = "int", comment = "/**
         * Count items matching id including both this instance and any contained items
         * @param what ID of items to count
         * @param pseudo whether pseudo-items (from map/vehicle tiles, bionics etc) are considered
         * @param limit stop searching after this many matches
         * @note items must be empty to be considered a match
         */", args = { "std::string", "bool", "int" } },
            { name = "body_window", rval = "hp_part", args = { "std::string", "bool", "bool", "int", "int", "int", "bool", "bool", "bool" } },
            { name = "body_window", rval = "hp_part", comment = "/**
         * Displays menu with body part hp, optionally with hp estimation after healing.
         * Returns selected part.
         */", args = { "bool" } },
            { name = "body_window", rval = "hp_part", comment = "/**
         * Displays menu with body part hp, optionally with hp estimation after healing.
         * Returns selected part.
         */", args = { } },
            { name = "boost_skill_level", rval = nil, args = { "skill_id", "int" } },
            { name = "bp_to_hp", static = true, rval = "hp_part", comment = "/** Converts a body_part to an hp_part */", args = { "body_part" } },
            { name = "can_pickVolume", rval = "bool", args = { "item" } },
            { name = "can_pickVolume", rval = "bool", args = { "item", "bool" } },
            { name = "can_pickWeight", rval = "bool", args = { "item" } },
            { name = "can_pickWeight", rval = "bool", args = { "item", "bool" } },
            { name = "can_use", rval = "bool", comment = "/**
         * Checks if character stats and skills meet minimum requirements for the item.
         * Prints an appropriate message if requirements not met.
         * @param it Item we are checking
         * @param context optionally override effective item when checking contextual skills
         */", args = { "item" } },
            { name = "can_use", rval = "bool", comment = "/**
         * Checks if character stats and skills meet minimum requirements for the item.
         * Prints an appropriate message if requirements not met.
         * @param it Item we are checking
         * @param context optionally override effective item when checking contextual skills
         */", args = { "item", "item" } },
            { name = "charges_of", rval = "int", comment = "/**
         * Count maximum available charges from this instance and any contained items
         * @param what ID of item to count charges of
         * @param limit stop searching after this many charges have been found
         */", args = { "std::string" } },
            { name = "charges_of", rval = "int", comment = "/**
         * Count maximum available charges from this instance and any contained items
         * @param what ID of item to count charges of
         * @param limit stop searching after this many charges have been found
         */", args = { "std::string", "int" } },
            { name = "drop_inventory_overflow", rval = nil, args = { } },
            { name = "effective_dispersion", rval = "int", args = { "int" } },
            { name = "empty_skills", rval = nil, args = { } },
            { name = "empty_traits", rval = nil, comment = "/** Empties the trait list */", args = { } },
            { name = "encumb", rval = "int", comment = "/** Returns ENC provided by armor, etc. */", args = { "body_part" } },
            { name = "enumerate_unmet_requirements", rval = "std::string", comment = "/** Returns a string of missed requirements (both stats and skills) */", args = { "item" } },
            { name = "enumerate_unmet_requirements", rval = "std::string", comment = "/** Returns a string of missed requirements (both stats and skills) */", args = { "item", "item" } },
            { name = "find_parent", rval = "item&", comment = "/**
         * Determine the immediate parent container (if any) for an item.
         * @param it item to search for which must be contained (at any depth) by this object
         * @return parent container or nullptr if the item is not within a container
         */", args = { "item" } },
            { name = "get_base_traits", rval = "std::vector<std::string>", comment = "/** Get the idents of all base traits. */", args = { } },
            { name = "get_dex", rval = "int", args = { } },
            { name = "get_dex_base", rval = "int", args = { } },
            { name = "get_dex_bonus", rval = "int", args = { } },
            { name = "get_fatigue", rval = "int", args = { } },
            { name = "get_healthy", rval = "int", comment = "/** Getters for health values exclusive to characters */", args = { } },
            { name = "get_healthy_mod", rval = "int", args = { } },
            { name = "get_hunger", rval = "int", comment = "/** Getter for need values exclusive to characters */", args = { } },
            { name = "get_int", rval = "int", args = { } },
            { name = "get_int_base", rval = "int", args = { } },
            { name = "get_int_bonus", rval = "int", args = { } },
            { name = "get_item_position", rval = "int", comment = "/**
         * Returns the item position (suitable for @ref i_at or similar) of a
         * specific item. Returns INT_MIN if the item is not found.
         * Note that this may lose some information, for example the returned position is the
         * same when the given item points to the container and when it points to the item inside
         * the container. All items that are part of the same stack have the same item position.
         */", args = { "item" } },
            { name = "get_mutations", rval = "std::vector<std::string>", comment = "/** Get the idents of all traits/mutations. */", args = { } },
            { name = "get_per", rval = "int", args = { } },
            { name = "get_per_base", rval = "int", args = { } },
            { name = "get_per_bonus", rval = "int", args = { } },
            { name = "get_stomach_food", rval = "int", args = { } },
            { name = "get_stomach_water", rval = "int", args = { } },
            { name = "get_str", rval = "int", comment = "/** Getters for stats exclusive to characters */", args = { } },
            { name = "get_str_base", rval = "int", args = { } },
            { name = "get_str_bonus", rval = "int", args = { } },
            { name = "get_thirst", rval = "int", args = { } },
            { name = "get_turn_died", rval = "int", comment = "/** return the calendar::turn the character expired */", args = { } },
            { name = "get_vision_threshold", rval = "float", comment = "/**
         * Returns the apparent light level at which the player can see.
         * This is adjusted by the light level at the *character's* position
         * to simulate glare, etc, night vision only works if you are in the dark.
         */", args = { "float" } },
            { name = "has_active_bionic", rval = "bool", comment = "/** Returns true if the player has the entered bionic id and it is powered on */", args = { "std::string" } },
            { name = "has_active_item", rval = "bool", comment = "/**
         * Whether the player carries an active item of the given item type.
         */", args = { "std::string" } },
            { name = "has_active_mutation", rval = "bool", args = { "std::string" } },
            { name = "has_amount", rval = "bool", comment = "/** Check instance provides at least qty of an item (@see amount_of) */", args = { "std::string", "int" } },
            { name = "has_amount", rval = "bool", comment = "/** Check instance provides at least qty of an item (@see amount_of) */", args = { "std::string", "int", "bool" } },
            { name = "has_base_trait", rval = "bool", comment = "/** Returns true if the player has the entered starting trait */", args = { "std::string" } },
            { name = "has_bionic", rval = "bool", comment = "/** Returns true if the player has the entered bionic id */", args = { "std::string" } },
            { name = "has_item", rval = "bool", comment = "/** Returns true if this visitable instance contains the item */", args = { "item" } },
            { name = "has_nv", rval = "bool", comment = "/** Returns true if the player has some form of night vision */", args = { } },
            { name = "has_quality", rval = "bool", comment = "/** Returns true if instance has amount (or more) items of at least quality level */", args = { "quality_id" } },
            { name = "has_quality", rval = "bool", comment = "/** Returns true if instance has amount (or more) items of at least quality level */", args = { "quality_id", "int" } },
            { name = "has_quality", rval = "bool", comment = "/** Returns true if instance has amount (or more) items of at least quality level */", args = { "quality_id", "int", "int" } },
            { name = "has_trait_flag", rval = "bool", comment = "/** Returns true if player has a trait with a flag */", args = { "std::string" } },
            { name = "healing_rate", rval = "float", comment = "/**
         * Average hit points healed per turn.
         */", args = { "float" } },
            { name = "hp_to_bp", static = true, rval = "body_part", comment = "/** Converts an hp_part to a body_part */", args = { "hp_part" } },
            { name = "i_add", rval = "item&", args = { "item" } },
            { name = "i_add_or_drop", rval = "bool", comment = "/** Sets invlet and adds to inventory if possible, drops otherwise, returns true if either succeeded.
         *  An optional qty can be provided (and will perform better than separate calls). */", args = { "item" } },
            { name = "i_add_or_drop", rval = "bool", comment = "/** Sets invlet and adds to inventory if possible, drops otherwise, returns true if either succeeded.
         *  An optional qty can be provided (and will perform better than separate calls). */", args = { "item", "int" } },
            { name = "i_at", rval = "item&", args = { "int" } },
            { name = "i_rem", rval = "item", comment = "/**
         * Remove a specific item from player possession. The item is compared
         * by pointer. Contents of the item are removed as well.
         * @param it A pointer to the item to be removed. The item *must* exists
         * in the players possession (one can use @ref has_item to check for this).
         * @return A copy of the removed item.
         */", args = { "item" } },
            { name = "i_rem", rval = "item", comment = "/**
         * Remove a specific item from player possession. The item is compared
         * by pointer. Contents of the item are removed as well.
         * @param pos The item position of the item to be removed. The item *must*
         * exists, use @ref has_item to check this.
         * @return A copy of the removed item.
         */", args = { "int" } },
            { name = "i_rem_keep_contents", rval = nil, args = { "int" } },
            { name = "is_blind", rval = "bool", comment = "/** Returns true if the player isn't able to see */", args = { } },
            { name = "is_wearing", rval = "bool", comment = "/** Returns true if the player is wearing the item. */", args = { "std::string" } },
            { name = "is_wearing_active_power_armor", rval = "bool", comment = "/** Returns true if the character is wearing active power */", args = { } },
            { name = "is_wearing_on_bp", rval = "bool", comment = "/** Returns true if the player is wearing the item on the given body_part. */", args = { "std::string", "body_part" } },
            { name = "is_worn", rval = "bool", args = { "item" } },
            { name = "limb_color", rval = "int", args = { "body_part", "bool", "bool", "bool" } },
            { name = "max_quality", rval = "int", comment = "/** Return maximum tool quality level provided by instance or INT_MIN if not found */", args = { "quality_id" } },
            { name = "meets_requirements", rval = "bool", comment = "/** Checks whether the character meets overall requirements to be able to use the item */", args = { "item" } },
            { name = "meets_requirements", rval = "bool", comment = "/** Checks whether the character meets overall requirements to be able to use the item */", args = { "item", "item" } },
            { name = "meets_stat_requirements", rval = "bool", comment = "/** Checks whether the character's stats meets the stats required by the item */", args = { "item" } },
            { name = "mod_dex_bonus", rval = nil, args = { "int" } },
            { name = "mod_fatigue", rval = nil, args = { "int" } },
            { name = "mod_healthy", rval = nil, comment = "/** Modifiers for health values exclusive to characters */", args = { "int" } },
            { name = "mod_healthy_mod", rval = nil, args = { "int", "int" } },
            { name = "mod_hunger", rval = nil, comment = "/** Modifiers for need values exclusive to characters */", args = { "int" } },
            { name = "mod_int_bonus", rval = nil, args = { "int" } },
            { name = "mod_per_bonus", rval = nil, args = { "int" } },
            { name = "mod_stomach_food", rval = nil, args = { "int" } },
            { name = "mod_stomach_water", rval = nil, args = { "int" } },
            { name = "mod_str_bonus", rval = nil, args = { "int" } },
            { name = "mod_thirst", rval = nil, args = { "int" } },
            { name = "mutation_effect", rval = nil, comment = "/** Handles things like destruction of armor, etc. */", args = { "std::string" } },
            { name = "mutation_loss_effect", rval = nil, comment = "/** Handles what happens when you lose a mutation. */", args = { "std::string" } },
            { name = "mutation_value", rval = "float", comment = "/**
         * Goes over all mutations, gets min and max of a value with given name
         * @return min( 0, lowest ) + max( 0, highest )
         */", args = { "std::string" } },
            { name = "on_item_takeoff", rval = nil, args = { "item" } },
            { name = "on_item_wear", rval = nil, args = { "item" } },
            { name = "pick_name", rval = nil, comment = "/** Returns a random name from NAMES_* */", args = { "bool" } },
            { name = "pick_name", rval = nil, comment = "/** Returns a random name from NAMES_* */", args = { } },
            { name = "pour_into", rval = "bool", comment = "/**
         * Try to pour the given liquid into the given container/vehicle. The transferred charges are
         * removed from the liquid item. Check the charges of afterwards to see if anything has
         * been transferred at all.
         * The functions do not consume any move points.
         * @return Whether anything has been moved at all. `false` indicates the transfer is not
         * possible at all. `true` indicates at least some of the liquid has been moved.
         */
        /**@{*/", args = { "item", "item" } },
            { name = "random_bad_trait", rval = "std::string", comment = "/** Returns the id of a random starting trait that costs < 0 points */", args = { } },
            { name = "random_good_trait", rval = "std::string", comment = "/** Returns the id of a random starting trait that costs >= 0 points */", args = { } },
            { name = "recalc_hp", rval = nil, comment = "/** Recalculates HP after a change to max strength */", args = { } },
            { name = "recalc_sight_limits", rval = nil, comment = "/** Modifies the player's sight values
         *  Must be called when any of the following change:
         *  This must be called when any of the following change:
         * - effects
         * - bionics
         * - traits
         * - underwater
         * - clothes
         */", args = { } },
            { name = "remove_item", rval = "item", comment = "/** Removes and returns the item which must be contained by this instance */", args = { "item" } },
            { name = "remove_mission_items", rval = nil, args = { "int" } },
            { name = "remove_weapon", rval = "item", args = { } },
            { name = "reset_encumbrance", rval = nil, comment = "/** Recalculates encumbrance cache. */", args = { } },
            { name = "rest_quality", rval = "float", comment = "/**
         * Returns >0 if character is sitting/lying and relatively inactive.
         * 1 represents sleep on comfortable bed, so anything above that should be rare.
         */", args = { } },
            { name = "set_dex_bonus", rval = nil, args = { "int" } },
            { name = "set_fatigue", rval = nil, args = { "int" } },
            { name = "set_healthy", rval = nil, comment = "/** Setters for health values exclusive to characters */", args = { "int" } },
            { name = "set_healthy_mod", rval = nil, args = { "int" } },
            { name = "set_hunger", rval = nil, comment = "/** Setters for need values exclusive to characters */", args = { "int" } },
            { name = "set_int_bonus", rval = nil, args = { "int" } },
            { name = "set_mutation", rval = nil, comment = "/** Add or removes a mutation on the player, but does not trigger mutation loss/gain effects. */", args = { "std::string" } },
            { name = "set_per_bonus", rval = nil, args = { "int" } },
            { name = "set_skill_level", rval = nil, args = { "skill_id", "int" } },
            { name = "set_stomach_food", rval = nil, args = { "int" } },
            { name = "set_stomach_water", rval = nil, args = { "int" } },
            { name = "set_str_bonus", rval = nil, comment = "/** Setters for stats exclusive to characters */", args = { "int" } },
            { name = "set_thirst", rval = nil, args = { "int" } },
            { name = "set_turn_died", rval = nil, comment = "/** set the turn the turn the character died if not already done */", args = { "int" } },
            { name = "throw_dispersion_per_dodge", rval = "int", comment = "/** How much dispersion does one point of target's dodge add when throwing at said target? */", args = { "bool" } },
            { name = "throw_dispersion_per_dodge", rval = "int", comment = "/** How much dispersion does one point of target's dodge add when throwing at said target? */", args = { } },
            { name = "throw_range", rval = "int", comment = "/** Maximum thrown range with a given item, taking all active effects into account. */", args = { "item" } },
            { name = "throwing_dispersion", rval = "int", comment = "/** Dispersion of a thrown item, against a given target. */", args = { "item" } },
            { name = "throwing_dispersion", rval = "int", comment = "/** Dispersion of a thrown item, against a given target. */", args = { "item", "Creature" } },
            { name = "toggle_trait", rval = nil, comment = "/** Toggles a trait on the player and in their mutation list */", args = { "std::string" } },
            { name = "trait_by_invlet", rval = "std::string", comment = "/** Returns the trait id with the given invlet, or an empty string if no trait has that invlet */", args = { "int" } },
            { name = "unset_mutation", rval = nil, args = { "std::string" } },
            { name = "update_health", rval = nil, comment = "/** Handles health fluctuations over time */", args = { "int" } },
            { name = "update_health", rval = nil, comment = "/** Handles health fluctuations over time */", args = { } },
            { name = "volume_capacity", rval = "units::volume", args = { } },
            { name = "volume_capacity_reduced_by", rval = "units::volume", args = { "units::volume" } },
            { name = "volume_carried", rval = "units::volume", args = { } },
            { name = "weight_carried", rval = "int", args = { } },
            { name = "worn_position_to_index", static = true, rval = "int", args = { "int" } },
            { name = "worn_with_flag", rval = "bool", comment = "/** Returns true if the player is wearing an item with the given flag. */", args = { "std::string" } },
            { name = "worn_with_flag", rval = "bool", comment = "/** Returns true if the player is wearing an item with the given flag. */", args = { "std::string", "body_part" } },
        }
    },
    map_stack = {
        by_value = true,
        functions = {
            { name = "amount_can_fit", rval = "int", comment = "/**
         * Returns how many of the specified item (or how many charges if it's counted by charges)
         * could be added without violating either the volume or itemcount limits.
         *
         * @returns Value of zero or greater for all items. For items counted by charges, it is always at
         * most it.charges.
         */", args = { "item" } },
            { name = "count_limit", rval = "int", comment = "/** Maximum number of items allowed here */", args = { } },
            { name = "cppbegin", rval = "std::list<item>::iterator", cpp_name = "begin", args = { } },
            { name = "cppend", rval = "std::list<item>::iterator", cpp_name = "end", args = { } },
            { name = "empty", rval = "bool", args = { } },
            { name = "erase", rval = "std::list<item>::iterator", args = { "std::list<item>::iterator" } },
            { name = "free_volume", rval = "units::volume", args = { } },
            { name = "front", rval = "item&", args = { } },
            { name = "insert_at", rval = nil, args = { "std::list<item>::iterator", "item" } },
            { name = "max_volume", rval = "units::volume", comment = "/** Maximum volume allowed here */", args = { } },
            { name = "push_back", rval = nil, args = { "item" } },
            { name = "size", rval = "int", args = { } },
            { name = "stacks_with", rval = "item&", comment = "/** Return the item (or nullptr) that stacks with the argument */", args = { "item" } },
            { name = "stored_volume", rval = "units::volume", comment = "/** Total volume of the items here */", args = { } },
        }
    },
    game = {
        attributes = {
            dangerous_proximity = { type = "int" },
            driving_view_offset = { type = "point", writable = true },
            fullscreen = { type = "bool", writable = true },
            lightning_active = { type = "bool", writable = true },
            monstairz = { type = "int", writable = true },
            narrow_sidebar = { type = "bool", writable = true },
            new_game = { type = "bool", writable = true },
            pixel_minimap_option = { type = "int", writable = true },
            right_sidebar = { type = "bool", writable = true },
            temperature = { type = "int", writable = true },
            ter_view_x = { type = "int", writable = true },
            ter_view_y = { type = "int", writable = true },
            ter_view_z = { type = "int", writable = true },
            was_fullscreen = { type = "bool", writable = true },
            weight_dragged = { type = "int", writable = true },
        },
        functions = {
            { name = "add_zombie", rval = "bool", args = { "monster", "bool" } },
            { name = "add_zombie", rval = "bool", comment = "/** Calls the creature_tracker add function. Returns true if successful. */", args = { "monster" } },
            { name = "assign_faction_id", rval = "int", args = { } },
            { name = "assign_mission_id", rval = "int", comment = "/** Returns the next available mission id. */", args = { } },
            { name = "assign_npc_id", rval = "int", args = { } },
            { name = "calc_driving_offset", rval = nil, args = { } },
            { name = "cancel_activity", rval = nil, comment = "/** Redirects to player::cancel_activity(). */", args = { } },
            { name = "check_mod_data", rval = "bool", comment = "/**
         *  Check if mods can be sucessfully loaded
         *  @param opts check specific mods (or all if unspecified)
         *  @return whether all mods were successfully loaded
         */", args = { "std::vector<std::string>" } },
            { name = "check_safe_mode_allowed", rval = "bool", comment = "/**
         * Check whether movement is allowed according to safe mode settings.
         * @return true if the movement is allowed, otherwise false.
         */", args = { "bool" } },
            { name = "check_safe_mode_allowed", rval = "bool", comment = "/**
         * Check whether movement is allowed according to safe mode settings.
         * @return true if the movement is allowed, otherwise false.
         */", args = { } },
            { name = "check_zone", rval = "bool", args = { "std::string", "tripoint" } },
            { name = "cleanup_at_end", rval = "bool", args = { } },
            { name = "clear_zombies", rval = nil, comment = "/** Redirects to the creature_tracker clear() function. */", args = { } },
            { name = "consume_liquid", rval = "bool", comment = "/**
         * Consume / handle as much of the liquid as possible in varying ways. This function can
         * be used when the action can be canceled, which implies the liquid can be put back
         * to wherever it came from and is *not* lost if the player cancels the action.
         * It returns when all liquid has been handled or if the player has explicitly canceled
         * the action (use the charges count to distinguish).
         * @return Whether any of the liquid has been consumed. `false` indicates the player has
         * declined all options to handle the liquid and no charges of the liquid have been transferred.
         * `true` indicates some charges have been transferred (but not necessarily all of them).
         */", args = { "item" } },
            { name = "consume_liquid", rval = "bool", comment = "/**
         * Consume / handle as much of the liquid as possible in varying ways. This function can
         * be used when the action can be canceled, which implies the liquid can be put back
         * to wherever it came from and is *not* lost if the player cancels the action.
         * It returns when all liquid has been handled or if the player has explicitly canceled
         * the action (use the charges count to distinguish).
         * @return Whether any of the liquid has been consumed. `false` indicates the player has
         * declined all options to handle the liquid and no charges of the liquid have been transferred.
         * `true` indicates some charges have been transferred (but not necessarily all of them).
         */", args = { "item", "int" } },
            { name = "critter_at", rval = "Creature&", comment = "/** Returns the Creature at tripoint p */", args = { "tripoint" } },
            { name = "critter_at", rval = "Creature&", comment = "/** Returns the Creature at tripoint p */", args = { "tripoint", "bool" } },
            { name = "delete_world", rval = nil, comment = "/** Deletes the given world. If delete_folder is true delete all the files and directories
         *  of the given world folder. Else just avoid deleting the two config files and the directory
         *  itself. */", args = { "std::string", "bool" } },
            { name = "do_blast", rval = nil, comment = "/** Helper for explosion, does the actual blast. */", args = { "tripoint", "float", "float", "bool" } },
            { name = "do_turn", rval = "bool", comment = "/** MAIN GAME LOOP. Returns true if game is over (death, saved, quit, etc.). */", args = { } },
            { name = "draw", rval = nil, args = { } },
            { name = "draw_bullet", rval = nil, args = { "Creature", "tripoint", "int", "std::vector<tripoint>", "int" } },
            { name = "draw_critter", rval = nil, args = { "Creature", "tripoint" } },
            { name = "draw_explosion", rval = nil, args = { "tripoint", "int", "int" } },
            { name = "draw_hit_mon", rval = nil, args = { "tripoint", "monster" } },
            { name = "draw_hit_mon", rval = nil, args = { "tripoint", "monster", "bool" } },
            { name = "draw_hit_player", rval = nil, args = { "player", "int" } },
            { name = "draw_line", rval = nil, args = { "tripoint", "std::vector<tripoint>" } },
            { name = "draw_line", rval = nil, args = { "tripoint", "tripoint", "std::vector<tripoint>" } },
            { name = "draw_sct", rval = nil, args = { } },
            { name = "draw_ter", rval = nil, args = { "bool" } },
            { name = "draw_ter", rval = nil, args = { "tripoint" } },
            { name = "draw_ter", rval = nil, args = { "tripoint", "bool" } },
            { name = "draw_ter", rval = nil, args = { "tripoint", "bool", "bool" } },
            { name = "draw_ter", rval = nil, args = { } },
            { name = "draw_trail_to_square", rval = nil, args = { "tripoint", "bool" } },
            { name = "draw_veh_dir_indicator", rval = nil, args = { "bool" } },
            { name = "draw_zones", rval = nil, args = { "tripoint", "tripoint", "tripoint" } },
            { name = "emp_blast", rval = nil, comment = "/** Triggers an emp blast at p. */", args = { "tripoint" } },
            { name = "explosion", rval = nil, comment = "/** Create explosion at p of intensity (power) with (shrapnel) chunks of shrapnel.
            Explosion intensity formula is roughly power*factor^distance.
            If factor <= 0, no blast is produced */", args = { "tripoint", "float" } },
            { name = "explosion", rval = nil, comment = "/** Create explosion at p of intensity (power) with (shrapnel) chunks of shrapnel.
            Explosion intensity formula is roughly power*factor^distance.
            If factor <= 0, no blast is produced */", args = { "tripoint", "float", "float" } },
            { name = "explosion", rval = nil, comment = "/** Create explosion at p of intensity (power) with (shrapnel) chunks of shrapnel.
            Explosion intensity formula is roughly power*factor^distance.
            If factor <= 0, no blast is produced */", args = { "tripoint", "float", "float", "bool" } },
            { name = "explosion", rval = nil, comment = "/** Create explosion at p of intensity (power) with (shrapnel) chunks of shrapnel.
            Explosion intensity formula is roughly power*factor^distance.
            If factor <= 0, no blast is produced */", args = { "tripoint", "float", "float", "bool", "int" } },
            { name = "explosion", rval = nil, comment = "/** Create explosion at p of intensity (power) with (shrapnel) chunks of shrapnel.
            Explosion intensity formula is roughly power*factor^distance.
            If factor <= 0, no blast is produced */", args = { "tripoint", "float", "float", "bool", "int", "int" } },
            { name = "extended_description", rval = nil, comment = "/** Long description of (visible) things at tile. */", args = { "tripoint" } },
            { name = "flashbang", rval = nil, comment = "/** Triggers a flashbang explosion at p. */", args = { "tripoint" } },
            { name = "flashbang", rval = nil, comment = "/** Triggers a flashbang explosion at p. */", args = { "tripoint", "bool" } },
            { name = "fling_creature", rval = nil, comment = "/** Flings the input creature in the given direction. */", args = { "Creature", "int", "float" } },
            { name = "fling_creature", rval = nil, comment = "/** Flings the input creature in the given direction. */", args = { "Creature", "int", "float", "bool" } },
            { name = "forced_door_closing", rval = "bool", args = { "tripoint", "ter_id", "int" } },
            { name = "game_error", rval = "bool", comment = "/** Returns true if the game quits through some error. */", args = { } },
            { name = "get_cur_om", rval = "overmap&", comment = "/**
         * The overmap which contains the center submap of the reality bubble.
         */", args = { } },
            { name = "get_levx", rval = "int", comment = "/**
         * The top left corner of the reality bubble (in submaps coordinates). This is the same
         * as @ref map::abs_sub of the @ref m map.
         */", args = { } },
            { name = "get_levy", rval = "int", args = { } },
            { name = "get_levz", rval = "int", args = { } },
            { name = "get_seed", rval = "int", args = { } },
            { name = "get_temperature", rval = "int", args = { } },
            { name = "get_user_action_counter", rval = "int", args = { } },
            { name = "get_veh_dir_indicator_location", rval = "tripoint", comment = "/**
         * Returns the location where the indicator should go relative to the reality bubble,
         * or tripoint_min to indicate no indicator should be drawn.
         * Based on the vehicle the player is driving, if any.
         * @param next If true, bases it on the vehicle the vehicle will turn to next turn,
         * instead of the one it is currently facing.
         */", args = { "bool" } },
            { name = "handle_all_liquid", rval = nil, comment = "/**
         * @name Liquid handling
         */
        /**@{*/
        /**
         * Consume / handle all of the liquid. The function can be used when the liquid needs
         * to be handled and can not be put back to where it came from (e.g. when it's a newly
         * created item from crafting).
         * The player is forced to handle all of it, which may required them to pour it onto
         * the ground (if they don't have enough container space available) and essentially
         * loose the item.
         * @return Whether any of the liquid has been consumed. `false` indicates the player has
         * declined all options to handle the liquid (essentially canceled the action) and no
         * charges of the liquid have been transferred.
         * `true` indicates some charges have been transferred (but not necessarily all of them).
         */", args = { "item", "int" } },
            { name = "handle_liquid", rval = "bool", comment = "/**
         * This may start a player activity if either \p source_pos or \p source_veh is not
         * null.
         * The function consumes moves of the player as needed.
         * Supply one of the source parameters to prevent the player from pouring the liquid back
         * into that "container". If no source parameter is given, the liquid must not be in a
         * container at all (e.g. freshly crafted, or already removed from the container).
         * @param liquid The actual liquid
         * @param source The container that currently contains the liquid.
         * @param radius Radius to look for liquid around pos
         * @param source_pos The source of the liquid when it's from the map.
         * @param source_veh The vehicle that currently contains the liquid in its tank.
         * @return Whether the user has handled the liquid (at least part of it). `false` indicates
         * the user has rejected all possible actions. But note that `true` does *not* indicate any
         * liquid was actually consumed, the user may have chosen an option that turned out to be
         * invalid (chose to fill into a full/unsuitable container).
         * Basically `false` indicates the user does not *want* to handle the liquid, `true`
         * indicates they want to handle it.
         */", args = { "item" } },
            { name = "handle_liquid", rval = "bool", comment = "/**
         * This may start a player activity if either \p source_pos or \p source_veh is not
         * null.
         * The function consumes moves of the player as needed.
         * Supply one of the source parameters to prevent the player from pouring the liquid back
         * into that "container". If no source parameter is given, the liquid must not be in a
         * container at all (e.g. freshly crafted, or already removed from the container).
         * @param liquid The actual liquid
         * @param source The container that currently contains the liquid.
         * @param radius Radius to look for liquid around pos
         * @param source_pos The source of the liquid when it's from the map.
         * @param source_veh The vehicle that currently contains the liquid in its tank.
         * @return Whether the user has handled the liquid (at least part of it). `false` indicates
         * the user has rejected all possible actions. But note that `true` does *not* indicate any
         * liquid was actually consumed, the user may have chosen an option that turned out to be
         * invalid (chose to fill into a full/unsuitable container).
         * Basically `false` indicates the user does not *want* to handle the liquid, `true`
         * indicates they want to handle it.
         */", args = { "item", "item" } },
            { name = "handle_liquid", rval = "bool", comment = "/**
         * This may start a player activity if either \p source_pos or \p source_veh is not
         * null.
         * The function consumes moves of the player as needed.
         * Supply one of the source parameters to prevent the player from pouring the liquid back
         * into that "container". If no source parameter is given, the liquid must not be in a
         * container at all (e.g. freshly crafted, or already removed from the container).
         * @param liquid The actual liquid
         * @param source The container that currently contains the liquid.
         * @param radius Radius to look for liquid around pos
         * @param source_pos The source of the liquid when it's from the map.
         * @param source_veh The vehicle that currently contains the liquid in its tank.
         * @return Whether the user has handled the liquid (at least part of it). `false` indicates
         * the user has rejected all possible actions. But note that `true` does *not* indicate any
         * liquid was actually consumed, the user may have chosen an option that turned out to be
         * invalid (chose to fill into a full/unsuitable container).
         * Basically `false` indicates the user does not *want* to handle the liquid, `true`
         * indicates they want to handle it.
         */", args = { "item", "item", "int" } },
            { name = "handle_liquid_from_container", rval = "bool", comment = "/**
         * Handle liquid from inside a container item. The function also handles consuming move points.
         * @param in_container Iterator to the liquid. Must be valid and point to an
         * item in the @ref item::contents of the container.
         * @param container Container of the liquid
         * @param radius around position to handle liquid for
         * @return Whether the item has been removed (which implies it was handled completely).
         * The iterator is invalidated in that case. Otherwise the item remains but may have
         * fewer charges.
         */", args = { "std::list<item>::iterator", "item" } },
            { name = "handle_liquid_from_container", rval = "bool", comment = "/**
         * Handle liquid from inside a container item. The function also handles consuming move points.
         * @param in_container Iterator to the liquid. Must be valid and point to an
         * item in the @ref item::contents of the container.
         * @param container Container of the liquid
         * @param radius around position to handle liquid for
         * @return Whether the item has been removed (which implies it was handled completely).
         * The iterator is invalidated in that case. Otherwise the item remains but may have
         * fewer charges.
         */", args = { "std::list<item>::iterator", "item", "int" } },
            { name = "handle_liquid_from_container", rval = "bool", comment = "/**
         * Shortcut to the above: handles the first item in the container.
         */", args = { "item" } },
            { name = "handle_liquid_from_container", rval = "bool", comment = "/**
         * Shortcut to the above: handles the first item in the container.
         */", args = { "item", "int" } },
            { name = "handle_liquid_from_ground", rval = "bool", comment = "/**
         * Handle finite liquid from ground. The function also handles consuming move points.
         * This may start a player activity.
         * @param on_ground Iterator to the item on the ground. Must be valid and point to an
         * item in the stack at `m.i_at(pos)`
         * @param pos The position of the item on the map.
         * @param radius around position to handle liquid for
         * @return Whether the item has been removed (which implies it was handled completely).
         * The iterator is invalidated in that case. Otherwise the item remains but may have
         * fewer charges.
         */", args = { "std::list<item>::iterator", "tripoint" } },
            { name = "handle_liquid_from_ground", rval = "bool", comment = "/**
         * Handle finite liquid from ground. The function also handles consuming move points.
         * This may start a player activity.
         * @param on_ground Iterator to the item on the ground. Must be valid and point to an
         * item in the stack at `m.i_at(pos)`
         * @param pos The position of the item on the map.
         * @param radius around position to handle liquid for
         * @return Whether the item has been removed (which implies it was handled completely).
         * The iterator is invalidated in that case. Otherwise the item remains but may have
         * fewer charges.
         */", args = { "std::list<item>::iterator", "tripoint", "int" } },
            { name = "has_gametype", rval = "bool", args = { } },
            { name = "increase_kill_count", rval = nil, comment = "/** Increments the number of kills of the given mtype_id by the player upwards. */", args = { "mtype_id" } },
            { name = "init_ui", rval = nil, comment = "/** Initializes the UI. */", args = { } },
            { name = "inv_for_all", rval = "int", args = { "std::string" } },
            { name = "inv_for_all", rval = "int", args = { "std::string", "std::string" } },
            { name = "inv_for_flag", rval = "int", args = { "std::string", "std::string" } },
            { name = "inv_for_id", rval = "int", args = { "std::string", "std::string" } },
            { name = "inventory_item_menu", rval = "int", args = { "int" } },
            { name = "inventory_item_menu", rval = "int", args = { "int", "int" } },
            { name = "inventory_item_menu", rval = "int", args = { "int", "int", "int" } },
            { name = "is_core_data_loaded", rval = "bool", comment = "/** Returns whether the core data is currently loaded. */", args = { } },
            { name = "is_empty", rval = "bool", comment = "/** Returns true if there is no player, NPC, or monster on the tile and move_cost > 0. */", args = { "tripoint" } },
            { name = "is_hostile_nearby", rval = "Creature&", args = { } },
            { name = "is_hostile_very_close", rval = "Creature&", args = { } },
            { name = "is_in_sunlight", rval = "bool", comment = "/** Returns true if p is outdoors and it is sunny. */", args = { "tripoint" } },
            { name = "is_sheltered", rval = "bool", comment = "/** Returns true if p is indoors, underground, or in a car. */", args = { "tripoint" } },
            { name = "kill_count", rval = "int", comment = "/** Returns the number of kills of the given mon_id by the player. */", args = { "mtype_id" } },
            { name = "knockback", rval = nil, args = { "std::vector<tripoint>", "int", "int", "int" } },
            { name = "knockback", rval = nil, args = { "tripoint", "tripoint", "int", "int", "int" } },
            { name = "light_level", rval = "int", comment = "/** Returns coarse number-of-squares of visibility at the current light level.
         * Used by monster and NPC AI.
         */", args = { "int" } },
            { name = "list_active_characters", rval = "std::vector<std::string>", comment = "/** Returns a list of currently active character saves. */", args = { } },
            { name = "load", rval = "bool", comment = "/** Attempt to load first valid save (if any) in world */", args = { "std::string" } },
            { name = "load_core_data", rval = nil, comment = "/** Loads core dynamic data. May throw. */", args = { } },
            { name = "load_map", rval = nil, comment = "/**
         * Load the main map at given location, see @ref map::load, in global, absolute submap
         * coordinates.
         */", args = { "tripoint" } },
            { name = "load_mission_npcs", rval = nil, comment = "/** Pulls the NPCs that were dumped into the world map on save back into mission_npcs */", args = { } },
            { name = "load_npcs", rval = nil, comment = "/** Makes any nearby NPCs on the overmap active. */", args = { } },
            { name = "load_packs", rval = "bool", comment = "/**
         *  Load content packs
         *  @param msg string to display whilst loading prompt
         *  @param packs content packs to load in correct dependent order
         *  @return true if all packs were found, false if any were missing
         */", args = { "std::string", "std::vector<std::string>" } },
            { name = "load_static_data", rval = nil, comment = "/** Loads static data that does not depend on mods or similar. */", args = { } },
            { name = "look_around", rval = "tripoint", args = { } },
            { name = "look_debug", rval = "tripoint", args = { } },
            { name = "mon_at", rval = "int", comment = "/** Returns the monster index of the monster at the given tripoint. Returns -1 if no monster is present. */", args = { "tripoint" } },
            { name = "mon_at", rval = "int", comment = "/** Returns the monster index of the monster at the given tripoint. Returns -1 if no monster is present. */", args = { "tripoint", "bool" } },
            { name = "monster_at", rval = "monster&", comment = "/** Returns a pointer to the monster at the given tripoint. */", args = { "tripoint" } },
            { name = "monster_at", rval = "monster&", comment = "/** Returns a pointer to the monster at the given tripoint. */", args = { "tripoint", "bool" } },
            { name = "moving_vehicle_dismount", rval = nil, comment = "/** Handles players exiting from moving vehicles. */", args = { "tripoint" } },
            { name = "natural_light_level", rval = "float", args = { "int" } },
            { name = "npc_at", rval = "int", comment = "/** Returns the NPC index of the npc at p. Returns -1 if no NPC is present. */", args = { "tripoint" } },
            { name = "npc_by_id", rval = "int", comment = "/** Returns the NPC index of the npc with a matching ID. Returns -1 if no NPC is present. */", args = { "int" } },
            { name = "nuke", rval = nil, comment = "/** Nuke the area at p - global overmap terrain coordinates! */", args = { "tripoint" } },
            { name = "num_zombies", rval = "int", comment = "/** Returns the number of creatures through the creature_tracker size() function. */", args = { } },
            { name = "open_gate", rval = nil, comment = "/**@}*/", args = { "tripoint" } },
            { name = "peek", rval = nil, args = { "tripoint" } },
            { name = "peek", rval = nil, args = { } },
            { name = "place_player", rval = nil, args = { "tripoint" } },
            { name = "place_player_overmap", rval = nil, args = { "tripoint" } },
            { name = "plfire", rval = "bool", comment = "/**
         * Handles interactive parts of gun firing (target selection, etc.).
         * @return Whether an attack was actually performed.
         */", args = { } },
            { name = "plfire", rval = "bool", comment = "/**
         * Handles interactive parts of gun firing (target selection, etc.).
         * This version stores targeting parameters for weapon, used for calls to the nullary form.
         * @param weapon Reference to a weapon we want to start aiming.
         * @param bp_cost The amount by which the player's power reserve is decreased after firing.
         * @return Whether an attack was actually performed.
         */", args = { "item" } },
            { name = "plfire", rval = "bool", comment = "/**
         * Handles interactive parts of gun firing (target selection, etc.).
         * This version stores targeting parameters for weapon, used for calls to the nullary form.
         * @param weapon Reference to a weapon we want to start aiming.
         * @param bp_cost The amount by which the player's power reserve is decreased after firing.
         * @return Whether an attack was actually performed.
         */", args = { "item", "int" } },
            { name = "plswim", rval = nil, comment = "/** Handles swimming by the player. Called by plmove(). */", args = { "tripoint" } },
            { name = "process_artifact", rval = nil, args = { "item", "player" } },
            { name = "reenter_fullscreen", rval = nil, args = { } },
            { name = "refresh_all", rval = nil, args = { } },
            { name = "reload_npcs", rval = nil, comment = "/** Unloads, then loads the NPCs */", args = { } },
            { name = "remove_zombie", rval = nil, args = { "int" } },
            { name = "reset_light_level", rval = nil, args = { } },
            { name = "reset_zoom", rval = nil, args = { } },
            { name = "resonance_cascade", rval = nil, comment = "/** Triggers a resonance cascade at p. */", args = { "tripoint" } },
            { name = "revive_corpse", rval = "bool", comment = "/**
         * Revives a corpse at given location. The monster type and some of its properties are
         * deducted from the corpse. If reviving succeeds, the location is guaranteed to have a
         * new monster there (see @ref mon_at).
         * @param location The place where to put the revived monster.
         * @param corpse The corpse item, it must be a valid corpse (see @ref item::is_corpse).
         * @return Whether the corpse has actually been redivided. Reviving may fail for many
         * reasons, including no space to put the monster, corpse being to much damaged etc.
         * If the monster was revived, the caller should remove the corpse item.
         * If reviving failed, the item is unchanged, as is the environment (no new monsters).
         */", args = { "tripoint", "item" } },
            { name = "save", rval = "bool", comment = "/** Returns false if saving failed. */", args = { } },
            { name = "scrambler_blast", rval = nil, comment = "/** Triggers a scrambler blast at p. */", args = { "tripoint" } },
            { name = "set_critter_died", rval = nil, comment = "/** If invoked, dead will be cleaned this turn. */", args = { } },
            { name = "set_driving_view_offset", rval = nil, args = { "point" } },
            { name = "set_npcs_dirty", rval = nil, comment = "/** If invoked, NPCs will be reloaded before next turn. */", args = { } },
            { name = "setup", rval = nil, args = { } },
            { name = "shockwave", rval = nil, args = { "tripoint", "int", "int", "int", "int", "bool" } },
            { name = "spawn_hallucination", rval = "bool", comment = "/** Spawns a hallucination close to the player. */", args = { } },
            { name = "start_calendar", rval = nil, args = { } },
            { name = "summon_mon", rval = "bool", comment = "/** Summons a brand new monster at the current time. Returns the summoned monster. */", args = { "mtype_id", "tripoint" } },
            { name = "swap_critters", rval = "bool", comment = "/** Swaps positions of two creatures */", args = { "Creature", "Creature" } },
            { name = "teleport", rval = nil, comment = "/** Performs a random short-distance teleport on the given player, granting teleglow if needed. */", args = { "player" } },
            { name = "teleport", rval = nil, comment = "/** Performs a random short-distance teleport on the given player, granting teleglow if needed. */", args = { "player", "bool" } },
            { name = "teleport", rval = nil, comment = "/** Performs a random short-distance teleport on the given player, granting teleglow if needed. */", args = { } },
            { name = "temp_exit_fullscreen", rval = nil, args = { } },
            { name = "toggle_fullscreen", rval = nil, args = { } },
            { name = "toggle_pixel_minimap", rval = nil, args = { } },
            { name = "toggle_sidebar_style", rval = nil, args = { } },
            { name = "unload", rval = "bool", args = { "item" } },
            { name = "unload", rval = nil, args = { "int" } },
            { name = "unload", rval = nil, args = { } },
            { name = "unload_npcs", rval = nil, comment = "/** Unloads all NPCs */", args = { } },
            { name = "update_map", rval = nil, args = { "player" } },
            { name = "update_overmap_seen", rval = nil, args = { } },
            { name = "update_zombie_pos", rval = "bool", comment = "/** Redirects to the creature_tracker update_pos() function. */", args = { "monster", "tripoint" } },
            { name = "use_computer", rval = nil, comment = "/** Checks to see if a player can use a computer (not illiterate, etc.) and uses if able. */", args = { "tripoint" } },
            { name = "vertical_move", rval = nil, comment = "/** Moves the player vertically. If force == true then they are falling. */", args = { "int", "bool" } },
            { name = "vertical_notes", rval = nil, comment = "/** Add goes up/down auto_notes (if turned on) */", args = { "int", "int" } },
            { name = "vertical_shift", rval = nil, comment = "/** Actual z-level movement part of vertical_move. Doesn't include stair finding, traps etc. */", args = { "int" } },
            { name = "write_memorial_file", rval = nil, args = { "std::string" } },
            { name = "zombie", rval = "monster&", comment = "/** Returns the monster with match index. Redirects to the creature_tracker find() function. */", args = { "int" } },
            { name = "zones_manager", rval = nil, args = { } },
            { name = "zoom_in", rval = nil, args = { } },
            { name = "zoom_out", rval = nil, args = { } },
        }
    },
    encumbrance_data = {
        by_value = true,
        has_equal = true,
        attributes = {
            armor_encumbrance = { type = "int", writable = true },
            encumbrance = { type = "int", writable = true },
            layer_penalty = { type = "int", writable = true },
        },
    },
    stats = {
        attributes = {
            damage_healed = { type = "int", writable = true },
            damage_taken = { type = "int", writable = true },
            headshots = { type = "int", writable = true },
            squares_walked = { type = "int", writable = true },
        },
        functions = {
            { name = "deserialize", rval = nil, args = { "std::string" } },
            { name = "reset", rval = nil, args = { } },
            { name = "serialize", rval = "std::string", args = { } },
        }
    },
    player = {
        parent = "Character",
        attributes = {
            blocks_left = { type = "int", writable = true },
            cash = { type = "int", writable = true },
            controlling_vehicle = { type = "bool", writable = true },
            dodges_left = { type = "int", writable = true },
            focus_pool = { type = "int", writable = true },
            grab_point = { type = "tripoint", writable = true },
            in_vehicle = { type = "bool", writable = true },
            keep_hands_free = { type = "bool", writable = true },
            last_batch = { type = "int", writable = true },
            last_climate_control_ret = { type = "bool", writable = true },
            lastconsumed = { type = "std::string", writable = true },
            lastrecipe = { type = "std::string", writable = true },
            ma_styles = { type = "std::vector<matype_id>", writable = true },
            max_power_level = { type = "int", writable = true },
            memorial_log = { type = "std::vector<std::string>", writable = true },
            move_mode = { type = "std::string", writable = true },
            movecounter = { type = "int", writable = true },
            next_climate_control_check = { type = "int", writable = true },
            oxygen = { type = "int", writable = true },
            power_level = { type = "int", writable = true },
            radiation = { type = "int", writable = true },
            reactor_plut = { type = "int", writable = true },
            recoil = { type = "float", writable = true },
            scent = { type = "int", writable = true },
            slow_rad = { type = "int", writable = true },
            stamina = { type = "int", writable = true },
            start_location = { type = "start_location_id", writable = true },
            stim = { type = "int", writable = true },
            style_selected = { type = "matype_id", writable = true },
            tank_plut = { type = "int", writable = true },
            view_offset = { type = "tripoint", writable = true },
            volume = { type = "int", writable = true },
        },
        functions = {
            { name = "action_taken", rval = nil, comment = "/** Called after every action, invalidates player caches */", args = { } },
            { name = "activate_bionic", rval = "bool", comment = "/** Handles bionic activation effects of the entered bionic, returns if anything activated */", args = { "int" } },
            { name = "activate_bionic", rval = "bool", comment = "/** Handles bionic activation effects of the entered bionic, returns if anything activated */", args = { "int", "bool" } },
            { name = "active_light", rval = "float", comment = "/** Returns player lumination based on the brightest active item they are carrying */", args = { } },
            { name = "add_addiction", rval = nil, comment = "/** Adds an addiction to the player */", args = { "add_type", "int" } },
            { name = "add_bionic", rval = nil, comment = "/** Adds a bionic to my_bionics[] */", args = { "std::string" } },
            { name = "add_known_trap", rval = nil, args = { "tripoint", "trap" } },
            { name = "add_martialart", rval = nil, comment = "/** Adds the entered martial art to the player's list */", args = { "matype_id" } },
            { name = "add_pain_msg", rval = nil, args = { "int", "body_part" } },
            { name = "addiction_level", rval = "int", comment = "/** Returns the intensity of the specified addiction */", args = { "add_type" } },
            { name = "adjacent_tile", rval = "tripoint", comment = "/** Returns an unoccupied, safe adjacent point. If none exists, returns player position. */", args = { } },
            { name = "adjust_for_focus", rval = "int", args = { "int" } },
            { name = "amount_worn", rval = "int", comment = "/** Returns the amount of item `type' that is currently worn */", args = { "std::string" } },
            { name = "apply_persistent_morale", rval = nil, comment = "/** Ensures persistent morale effects are up-to-date */", args = { } },
            { name = "apply_wetness_morale", rval = nil, comment = "/** Recalculates morale penalty/bonus from wetness based on mutations, equipment and temperature */", args = { "int" } },
            { name = "attack_speed", rval = "int", comment = "/** Returns cost (in moves) of attacking with given item (no modifiers, like stuck) */", args = { "item" } },
            { name = "best_shield", rval = "item&", comment = "/** Returns the best item for blocking with */", args = { } },
            { name = "blossoms", rval = nil, args = { } },
            { name = "bodytemp_color", rval = "int", comment = "/** Define color for displaying the body temperature */", args = { "int" } },
            { name = "bonus_damage", rval = "float", comment = "/** Returns the bonus bashing damage the player deals based on their stats */", args = { "bool" } },
            { name = "bonus_item_warmth", rval = "int", comment = "/** Returns warmth provided by an armor's bonus, like hoods, pockets, etc. */", args = { "body_part" } },
            { name = "burn_move_stamina", rval = nil, args = { "int" } },
            { name = "calc_focus_equilibrium", rval = "int", comment = "/** Uses morale and other factors to return the player's focus gain rate */", args = { } },
            { name = "can_arm_block", rval = "bool", comment = "/** Returns true if the player has the arm block technique available */", args = { } },
            { name = "can_consume", rval = "bool", comment = "/** Check player's capability of consumption overall */", args = { "item" } },
            { name = "can_hear", rval = "bool", args = { "tripoint", "int" } },
            { name = "can_interface_armor", rval = "bool", comment = "/**
        * Check whether player has a bionic power armor interface.
        * @return true if player has an active bionic capable of powering armor, false otherwise.
        */", args = { } },
            { name = "can_leg_block", rval = "bool", comment = "/** Returns true if the player has the leg block technique available */", args = { } },
            { name = "can_limb_block", rval = "bool", comment = "/** Returns true if either can_leg_block() or can_arm_block() returns true */", args = { } },
            { name = "can_melee", rval = "bool", comment = "/** Returns true if the current martial art works with the player's current weapon */", args = { } },
            { name = "can_reload", rval = "bool", comment = "/**
         * Whether a tool or gun is potentially reloadable (optionally considering a specific ammo)
         * @param it Thing to be reloaded
         * @param ammo if set also check item currently compatible with this specific ammo or magazine
         * @note items currently loaded with a detachable magazine are considered reloadable
         * @note items with integral magazines are reloadable if free capacity permits (+/- ammo matches)
         */", args = { "item" } },
            { name = "can_reload", rval = "bool", comment = "/**
         * Whether a tool or gun is potentially reloadable (optionally considering a specific ammo)
         * @param it Thing to be reloaded
         * @param ammo if set also check item currently compatible with this specific ammo or magazine
         * @note items currently loaded with a detachable magazine are considered reloadable
         * @note items with integral magazines are reloadable if free capacity permits (+/- ammo matches)
         */", args = { "item", "std::string" } },
            { name = "can_sleep", rval = "bool", comment = "/** Checked each turn during "lying_down", returns true if the player falls asleep */", args = { } },
            { name = "can_unwield", rval = "bool", comment = "/**
         * Check player capable of unwielding an item.
         * @param it Thing to be unwielded
         * @param alert display reason for any failure
         */", args = { "item" } },
            { name = "can_unwield", rval = "bool", comment = "/**
         * Check player capable of unwielding an item.
         * @param it Thing to be unwielded
         * @param alert display reason for any failure
         */", args = { "item", "bool" } },
            { name = "can_weapon_block", rval = "bool", comment = "/** Returns true if the player has a weapon with a block technique */", args = { } },
            { name = "can_wear", rval = "bool", comment = "/**
         * Check player capable of wearing an item.
         * @param it Thing to be worn
         * @param alert display reason for any failure
         */", args = { "item" } },
            { name = "can_wear", rval = "bool", comment = "/**
         * Check player capable of wearing an item.
         * @param it Thing to be worn
         * @param alert display reason for any failure
         */", args = { "item", "bool" } },
            { name = "can_wield", rval = "bool", comment = "/**
         * Check player capable of wielding an item.
         * @param it Thing to be wielded
         * @param alert display reason for any failure
         */", args = { "item" } },
            { name = "can_wield", rval = "bool", comment = "/**
         * Check player capable of wielding an item.
         * @param it Thing to be wielded
         * @param alert display reason for any failure
         */", args = { "item", "bool" } },
            { name = "cancel_activity", rval = nil, args = { } },
            { name = "change_side", rval = "bool", args = { "int" } },
            { name = "change_side", rval = "bool", args = { "int", "bool" } },
            { name = "change_side", rval = "bool", comment = "/** Swap side on which item is worn; returns false on fail. If interactive is false, don't alert player or drain moves */", args = { "item" } },
            { name = "change_side", rval = "bool", comment = "/** Swap side on which item is worn; returns false on fail. If interactive is false, don't alert player or drain moves */", args = { "item", "bool" } },
            { name = "charge_power", rval = nil, comment = "/** Adds the entered amount to the player's bionic power_level */", args = { "int" } },
            { name = "check_and_recover_morale", rval = nil, comment = "/** Checks permanent morale for consistency and recovers it when an inconsistency is found. */", args = { } },
            { name = "check_needs_extremes", rval = nil, comment = "/** Kills the player if too hungry, stimmed up etc., forces tired player to sleep and prints warnings. */", args = { } },
            { name = "clairvoyance", rval = "int", comment = "/** Returns the distance the player can see through walls */", args = { } },
            { name = "clear_destination", rval = nil, args = { } },
            { name = "clear_miss_reasons", rval = nil, comment = "/** Clears the list of reasons for why the player would miss a melee attack. */", args = { } },
            { name = "climbing_cost", rval = "int", comment = "/**
         * Checks both the neighborhoods of from and to for climbable surfaces,
         * returns move cost of climbing from `from` to `to`.
         * 0 means climbing is not possible.
         * Return value can depend on the orientation of the terrain.
         */", args = { "tripoint", "tripoint" } },
            { name = "complete_craft", rval = nil, args = { } },
            { name = "complete_disassemble", rval = nil, args = { } },
            { name = "consume", rval = "bool", comment = "/** Used for eating object at pos, returns true if object is removed from inventory (last charge was consumed) */", args = { "int" } },
            { name = "consume_charges", rval = "bool", comment = "/** Consume charges of a tool or comestible item, potentially destroying it in the process
         *  @param used item consuming the charges
         *  @param qty number of charges to consume which must be non-zero
         *  @return true if item was destroyed */", args = { "item", "int" } },
            { name = "consume_effects", rval = nil, comment = "/** Handles the effects of consuming an item */", args = { "item" } },
            { name = "consume_effects", rval = nil, comment = "/** Handles the effects of consuming an item */", args = { "item", "bool" } },
            { name = "consume_item", rval = "bool", comment = "/** Used for eating a particular item that doesn't need to be in inventory.
         *  Returns true if the item is to be removed (doesn't remove). */", args = { "item" } },
            { name = "cough", rval = nil, args = { "bool" } },
            { name = "cough", rval = nil, args = { "bool", "int" } },
            { name = "cough", rval = nil, args = { } },
            { name = "craft", rval = nil, args = { } },
            { name = "crit_chance", rval = "float", comment = "/** Returns the chance to crit given a hit roll and target's dodge roll */", args = { "float", "float", "item" } },
            { name = "crossed_threshold", rval = "bool", comment = "/** Returns true if the player has crossed a mutation threshold
         *  Player can only cross one mutation threshold.
         */", args = { } },
            { name = "deactivate_bionic", rval = "bool", comment = "/** Handles bionic deactivation effects of the entered bionic, returns if anything deactivated */", args = { "int" } },
            { name = "deactivate_bionic", rval = "bool", comment = "/** Handles bionic deactivation effects of the entered bionic, returns if anything deactivated */", args = { "int", "bool" } },
            { name = "deserialize", rval = nil, args = { "std::string" } },
            { name = "disassemble", rval = "bool", args = { "int" } },
            { name = "disassemble", rval = "bool", args = { "item", "int", "bool" } },
            { name = "disassemble", rval = "bool", args = { "item", "int", "bool", "bool" } },
            { name = "disassemble", rval = "bool", args = { } },
            { name = "disassemble_all", rval = nil, args = { "bool" } },
            { name = "disp_info", rval = nil, comment = "/** Handles and displays detailed character info for the '@' screen */", args = { } },
            { name = "disp_morale", rval = nil, comment = "/** Provides the window and detailed morale data */", args = { } },
            { name = "do_read", rval = nil, comment = "/** Completes book reading action. **/", args = { "item" } },
            { name = "drench", rval = nil, comment = "/** Drenches the player with water, saturation is the percent gotten wet */", args = { "int", "int", "bool" } },
            { name = "drench_mut_calc", rval = nil, comment = "/** Recalculates mutation drench protection for all bodyparts (ignored/good/neutral stats) */", args = { } },
            { name = "drink_from_hands", rval = "int", comment = "/** used for drinking from hands, returns how many charges were consumed */", args = { "item" } },
            { name = "drop", rval = nil, comment = "/** Drops an item to the specified location */", args = { "int" } },
            { name = "drop", rval = nil, comment = "/** Drops an item to the specified location */", args = { "int", "tripoint" } },
            { name = "dump_memorial", rval = "std::string", args = { } },
            { name = "eat", rval = "bool", comment = "/** Used for eating entered comestible, returns true if comestible is successfully eaten */", args = { "item" } },
            { name = "eat", rval = "bool", comment = "/** Used for eating entered comestible, returns true if comestible is successfully eaten */", args = { "item", "bool" } },
            { name = "environmental_revert_effect", rval = nil, args = { } },
            { name = "fall_asleep", rval = nil, comment = "/** Adds "sleep" to the player */", args = { "int" } },
            { name = "fine_detail_vision_mod", rval = "float", comment = "/** Returns a value from 1.0 to 5.0 that acts as a multiplier
         * for the time taken to perform tasks that require detail vision,
         * above 4.0 means these activities cannot be performed. */", args = { } },
            { name = "fire_gun", rval = "int", comment = "/**
         *  Fires a gun or auxiliary gunmod (ignoring any current mode)
         *  @param target where the first shot is aimed at (may vary for later shots)
         *  @param shots maximum number of shots to fire (less may be fired in some circumstances)
         *  @param gun item to fire (which does not necessary have to be in the players possession)
         *  @return number of shots actually fired
         */", args = { "tripoint", "int", "item" } },
            { name = "fire_gun", rval = "int", comment = "/**
         *  Fires a gun or auxiliary gunmod (ignoring any current mode)
         *  @param target where the first shot is aimed at (may vary for later shots)
         *  @param shots maximum number of shots to fire (less may be fired in some circumstances)
         *  @return number of shots actually fired
         */", args = { "tripoint" } },
            { name = "fire_gun", rval = "int", comment = "/**
         *  Fires a gun or auxiliary gunmod (ignoring any current mode)
         *  @param target where the first shot is aimed at (may vary for later shots)
         *  @param shots maximum number of shots to fire (less may be fired in some circumstances)
         *  @return number of shots actually fired
         */", args = { "tripoint", "int" } },
            { name = "footwear_factor", rval = "float", comment = "/** Returns 1 if the player is wearing something on both feet, .5 if on one, and 0 if on neither */", args = { } },
            { name = "fun_to_read", rval = "bool", args = { "item" } },
            { name = "getID", rval = "int", args = { } },
            { name = "get_active_mission_target", rval = "tripoint", comment = "/**
         * Returns the target of the active mission or @ref overmap::invalid_tripoint if there is
         * no active mission.
         */", args = { } },
            { name = "get_all_techniques", rval = "std::vector<matec_id>", args = { } },
            { name = "get_armor_acid", rval = "int", comment = "/** Returns overall acid resistance for the body part */", args = { "body_part" } },
            { name = "get_armor_fire", rval = "int", comment = "/** Returns overall fire resistance for the body part */", args = { "body_part" } },
            { name = "get_auto_move_route", rval = "std::vector<tripoint>", args = { } },
            { name = "get_book_reader", rval = "player&", comment = "/**
         * Helper function for player::read.
         *
         * @param book Book to read
         * @param reasons Starting with g->u, for each player/NPC who cannot read, a message will be pushed back here.
         * @returns nullptr, if neither the player nor his followers can read to the player, otherwise the player/NPC
         * who can read and can read the fastest
         */", args = { "item", "std::vector<std::string>" } },
            { name = "get_category_dream", rval = "std::string", comment = "/** Returns a dream's description selected randomly from the player's highest mutation category */", args = { "std::string", "int" } },
            { name = "get_combat_style", rval = "martialart&", args = { } },
            { name = "get_free_bionics_slots", rval = "int", args = { "body_part" } },
            { name = "get_highest_category", rval = "std::string", comment = "/** Returns the highest mutation category */", args = { } },
            { name = "get_hit_weapon", rval = "float", comment = "/** Gets melee accuracy component from weapon+skills */", args = { "item" } },
            { name = "get_morale_level", rval = "int", args = { } },
            { name = "get_overlay_ids", rval = "std::vector<std::string>", comment = "/**
         * Returns a list of the IDs of overlays on this character,
         * sorted from "lowest" to "highest".
         *
         * Only required for rendering.
         */", args = { } },
            { name = "get_painkiller", rval = "int", comment = "/** Returns intensity of painkillers  */", args = { } },
            { name = "get_sick", rval = nil, comment = "/** Handles the chance to be infected by random diseases */", args = { } },
            { name = "get_stamina_max", rval = "int", args = { } },
            { name = "get_total_bionics_slots", rval = "int", args = { "body_part" } },
            { name = "get_used_bionics_slots", rval = "int", args = { "body_part" } },
            { name = "get_weapon_dispersion", rval = "float", comment = "/**
         * Returns a weapon's modified dispersion value.
         * @param obj Weapon to check dispersion on
         * @param range Distance to target against which we're calculating the dispersion
         */", args = { "item", "float" } },
            { name = "get_wind_resistance", rval = "int", comment = "/** Returns wind resistance provided by armor, etc **/", args = { "body_part" } },
            { name = "global_omt_location", rval = "tripoint", comment = "/**
        * Returns the location of the player in global overmap terrain coordinates.
        */", args = { } },
            { name = "global_sm_location", rval = "tripoint", comment = "/**
        * Returns the location of the player in global submap coordinates.
        */", args = { } },
            { name = "global_square_location", rval = "tripoint", comment = "/**
         * Global position, expressed in map square coordinate system
         * (the most detailed coordinate system), used by the @ref map.
         */", args = { } },
            { name = "gun_current_range", rval = "float", comment = "/**
         *  Calculate range at which given chance of hit considering player stats, clothing and recoil
         *  @param gun ranged weapon which must have sufficient ammo for at least one shot
         *  @param penalty if set (non-negative) use this value instead of @ref recoil_total()
         *  @param chance probability of hit, range [0-100) with zero returning absolute maximum range
         *  @param accuracy minimum accuracy required
         */", args = { "item" } },
            { name = "gun_current_range", rval = "float", comment = "/**
         *  Calculate range at which given chance of hit considering player stats, clothing and recoil
         *  @param gun ranged weapon which must have sufficient ammo for at least one shot
         *  @param penalty if set (non-negative) use this value instead of @ref recoil_total()
         *  @param chance probability of hit, range [0-100) with zero returning absolute maximum range
         *  @param accuracy minimum accuracy required
         */", args = { "item", "float" } },
            { name = "gun_current_range", rval = "float", comment = "/**
         *  Calculate range at which given chance of hit considering player stats, clothing and recoil
         *  @param gun ranged weapon which must have sufficient ammo for at least one shot
         *  @param penalty if set (non-negative) use this value instead of @ref recoil_total()
         *  @param chance probability of hit, range [0-100) with zero returning absolute maximum range
         *  @param accuracy minimum accuracy required
         */", args = { "item", "float", "int" } },
            { name = "gun_current_range", rval = "float", comment = "/**
         *  Calculate range at which given chance of hit considering player stats, clothing and recoil
         *  @param gun ranged weapon which must have sufficient ammo for at least one shot
         *  @param penalty if set (non-negative) use this value instead of @ref recoil_total()
         *  @param chance probability of hit, range [0-100) with zero returning absolute maximum range
         *  @param accuracy minimum accuracy required
         */", args = { "item", "float", "int", "float" } },
            { name = "gun_engagement_moves", rval = "int", comment = "/** How many moves does it take to aim gun to maximum accuracy? */", args = { "item" } },
            { name = "gun_value", rval = "float", args = { "item" } },
            { name = "gun_value", rval = "float", args = { "item", "int" } },
            { name = "gunmod_add", rval = nil, comment = "/** Starts activity to install gunmod having warned user about any risk of failure or irremovable mods s*/", args = { "item", "item" } },
            { name = "gunmod_remove", rval = "bool", comment = "/** Removes gunmod after first unloading any contained ammo and returns true on success */", args = { "item", "item" } },
            { name = "handle_gun_damage", rval = "bool", comment = "/** Returns true if a gun misfires, jams, or has other problems, else returns false */", args = { "item" } },
            { name = "handle_melee_wear", rval = "bool", args = { "item" } },
            { name = "handle_melee_wear", rval = "bool", args = { "item", "float" } },
            { name = "handle_melee_wear", rval = "bool", comment = "/** Calculates melee weapon wear-and-tear through use, returns true if item is destroyed. */", args = { "float" } },
            { name = "handle_melee_wear", rval = "bool", comment = "/** Calculates melee weapon wear-and-tear through use, returns true if item is destroyed. */", args = { } },
            { name = "has_active_optcloak", rval = "bool", comment = "/** Returns true if the player is wearing an active optical cloak */", args = { } },
            { name = "has_addiction", rval = "bool", comment = "/** Returns true if the player has an addiction of the specified type */", args = { "add_type" } },
            { name = "has_alarm_clock", rval = "bool", comment = "/** Returns true if the player or their vehicle has an alarm clock */", args = { } },
            { name = "has_charges", rval = "bool", args = { "std::string", "int" } },
            { name = "has_child_flag", rval = "bool", comment = "/** Returns true if the player has the entered mutation child flag */", args = { "std::string" } },
            { name = "has_conflicting_trait", rval = "bool", comment = "/** Returns true if the player has a conflicting trait to the entered trait
         *  Uses has_opposite_trait(), has_lower_trait(), and has_higher_trait() to determine conflicts.
         */", args = { "std::string" } },
            { name = "has_destination", rval = "bool", args = { } },
            { name = "has_enough_charges", rval = "bool", comment = "/**
         * Has the item enough charges to invoke its use function?
         * Also checks if UPS from this player is used instead of item charges.
         */", args = { "item", "bool" } },
            { name = "has_higher_trait", rval = "bool", comment = "/** Returns true if the player has a trait which is an upgrade of the entered trait */", args = { "std::string" } },
            { name = "has_identified", rval = "bool", comment = "/** Note that we've read a book at least once. **/", args = { "std::string" } },
            { name = "has_item_with_flag", rval = "bool", args = { "std::string" } },
            { name = "has_lower_trait", rval = "bool", comment = "/** Returns true if the player has a trait which upgrades into the entered trait */", args = { "std::string" } },
            { name = "has_mabuff", rval = "bool", comment = "/** Returns true if the player has any martial arts buffs attached */", args = { "mabuff_id" } },
            { name = "has_martialart", rval = "bool", comment = "/** Returns true if the player has access to the entered martial art */", args = { "matype_id" } },
            { name = "has_miss_recovery_tec", rval = "bool", comment = "/** Returns true if the player has technique-based miss recovery */", args = { } },
            { name = "has_mission_item", rval = "bool", args = { "int" } },
            { name = "has_morale_to_craft", rval = "bool", args = { } },
            { name = "has_morale_to_read", rval = "bool", args = { } },
            { name = "has_opposite_trait", rval = "bool", comment = "/** Returns true if the player has a trait which cancels the entered trait */", args = { "std::string" } },
            { name = "has_technique", rval = "bool", comment = "/** Returns true if the player has a weapon or martial arts skill available with the entered technique */", args = { "matec_id" } },
            { name = "has_two_arms", rval = "bool", comment = "/** Returns true if the player has two functioning arms */", args = { } },
            { name = "has_watch", rval = "bool", comment = "/** Returns true if the player or their vehicle has a watch */", args = { } },
            { name = "heal", rval = nil, comment = "/** Heals a body_part for dam */", args = { "body_part", "int" } },
            { name = "heal", rval = nil, comment = "/** Heals an hp_part for dam */", args = { "hp_part", "int" } },
            { name = "healall", rval = nil, comment = "/** Heals all body parts for dam */", args = { "int" } },
            { name = "hearing_ability", rval = "float", args = { } },
            { name = "hitall", rval = "int", comment = "/** Harms all body parts for dam, with armor reduction. If vary > 0 damage to parts are random within vary % (1-100) */", args = { "int", "int", "Creature" } },
            { name = "hunger_speed_penalty", static = true, rval = "int", comment = "/** Returns the penalty to speed from hunger */", args = { "int" } },
            { name = "hurtall", rval = nil, comment = "/** Hurts all body parts for dam, no armor reduction */", args = { "int", "Creature" } },
            { name = "hurtall", rval = nil, comment = "/** Hurts all body parts for dam, no armor reduction */", args = { "int", "Creature", "bool" } },
            { name = "in_climate_control", rval = "bool", comment = "/** Returns true if the player is in a climate controlled area or armor */", args = { } },
            { name = "install_bionics", rval = "bool", comment = "/** Attempts to install bionics, returns false if the player cancels prior to installation */", args = { "itype" } },
            { name = "install_bionics", rval = "bool", comment = "/** Attempts to install bionics, returns false if the player cancels prior to installation */", args = { "itype", "int" } },
            { name = "intimidation", rval = "int", comment = "/** Returns a value used when attempting to intimidate NPC's */", args = { } },
            { name = "invalidate_crafting_inventory", rval = nil, args = { } },
            { name = "invlet_to_position", rval = "int", comment = "/** Return the item position of the item with given invlet, return INT_MIN if
         * the player does not have such an item with that invlet. Don't use this on npcs.
         * Only use the invlet in the user interface, otherwise always use the item position. */", args = { "int" } },
            { name = "invoke_item", rval = "bool", args = { "item", "std::string" } },
            { name = "invoke_item", rval = "bool", comment = "/**
         * Asks how to use the item (if it has more than one use_method) and uses it.
         * Returns true if it destroys the item. Consumes charges from the item.
         * Multi-use items are ONLY supported when all use_methods are iuse_actor!
         */", args = { "item", "tripoint" } },
            { name = "invoke_item", rval = "bool", comment = "/** As above two, but with position equal to current position */", args = { "item" } },
            { name = "invoke_item", rval = "bool", comment = "/** As above, but with a pre-selected method. Debugmsg if this item doesn't have this method. */", args = { "item", "std::string", "tripoint" } },
            { name = "is_allergic", rval = "bool", comment = "/** This block is to be moved to character.h */", args = { "item" } },
            { name = "is_armed", rval = "bool", comment = "/** Returns true if the player is wielding something */", args = { } },
            { name = "is_deaf", rval = "bool", args = { } },
            { name = "is_hibernating", rval = "bool", comment = "/** Returns if the player has hibernation mutation and is asleep and well fed */", args = { } },
            { name = "is_invisible", rval = "bool", args = { } },
            { name = "is_quiet", rval = "bool", comment = "/** Returns true if the player has quiet melee attacks */", args = { } },
            { name = "is_snuggling", rval = "std::string", comment = "/** Checks to see if the player is using floor items to keep warm, and return the name of one such item if so */", args = { } },
            { name = "is_throw_immune", rval = "bool", comment = "/** Returns true if the player is immune to throws */", args = { } },
            { name = "is_wearing_power_armor", rval = "bool", comment = "/** Returns true if the player is wearing power armor */", args = { } },
            { name = "is_wearing_shoes", rval = "bool", comment = "/** Returns true if the player is wearing something on their feet that is not SKINTIGHT */", args = { "std::string" } },
            { name = "is_wearing_shoes", rval = "bool", comment = "/** Returns true if the player is wearing something on their feet that is not SKINTIGHT */", args = { } },
            { name = "item_handling_cost", rval = "int", comment = "/**
         * Calculate (but do not deduct) the number of moves required when handling (eg. storing, drawing etc.) an item
         * @param it Item to calculate handling cost for
         * @param penalties Whether item volume and temporary effects (eg. GRABBED, DOWNED) should be considered.
         * @param base_cost Cost due to storage type.
         * @return cost in moves ranging from 0 to MAX_HANDLING_COST
         */", args = { "item" } },
            { name = "item_handling_cost", rval = "int", comment = "/**
         * Calculate (but do not deduct) the number of moves required when handling (eg. storing, drawing etc.) an item
         * @param it Item to calculate handling cost for
         * @param penalties Whether item volume and temporary effects (eg. GRABBED, DOWNED) should be considered.
         * @param base_cost Cost due to storage type.
         * @return cost in moves ranging from 0 to MAX_HANDLING_COST
         */", args = { "item", "bool" } },
            { name = "item_handling_cost", rval = "int", comment = "/**
         * Calculate (but do not deduct) the number of moves required when handling (eg. storing, drawing etc.) an item
         * @param it Item to calculate handling cost for
         * @param penalties Whether item volume and temporary effects (eg. GRABBED, DOWNED) should be considered.
         * @param base_cost Cost due to storage type.
         * @return cost in moves ranging from 0 to MAX_HANDLING_COST
         */", args = { "item", "bool", "int" } },
            { name = "item_reload_cost", rval = "int", comment = "/**
         * Calculate (but do not deduct) the number of moves required to reload an item with specified quantity of ammo
         * @param it Item to calculate reload cost for
         * @param ammo either ammo or magazine to use when reloading the item
         * @param qty maximum units of ammo to reload. Capped by remaining capacity and ignored if reloading using a magazine.
         */", args = { "item", "item", "int" } },
            { name = "item_store_cost", rval = "int", comment = "/**
         * Calculate (but do not deduct) the number of moves required when storing an item in a container
         * @param it Item to calculate storage cost for
         * @param container Container to store item in
         * @param penalties Whether item volume and temporary effects (eg. GRABBED, DOWNED) should be considered.
         * @param base_cost Cost due to storage type.
         * @return cost in moves ranging from 0 to MAX_HANDLING_COST
         */", args = { "item", "item" } },
            { name = "item_store_cost", rval = "int", comment = "/**
         * Calculate (but do not deduct) the number of moves required when storing an item in a container
         * @param it Item to calculate storage cost for
         * @param container Container to store item in
         * @param penalties Whether item volume and temporary effects (eg. GRABBED, DOWNED) should be considered.
         * @param base_cost Cost due to storage type.
         * @return cost in moves ranging from 0 to MAX_HANDLING_COST
         */", args = { "item", "item", "bool" } },
            { name = "item_store_cost", rval = "int", comment = "/**
         * Calculate (but do not deduct) the number of moves required when storing an item in a container
         * @param it Item to calculate storage cost for
         * @param container Container to store item in
         * @param penalties Whether item volume and temporary effects (eg. GRABBED, DOWNED) should be considered.
         * @param base_cost Cost due to storage type.
         * @return cost in moves ranging from 0 to MAX_HANDLING_COST
         */", args = { "item", "item", "bool", "int" } },
            { name = "item_wear_cost", rval = "int", comment = "/** Calculate (but do not deduct) the number of moves required to wear an item */", args = { "item" } },
            { name = "knows_trap", rval = "bool", args = { "tripoint" } },
            { name = "leak_level", rval = "int", args = { "std::string" } },
            { name = "lifetime_stats", rval = "stats&", args = { } },
            { name = "load_info", rval = nil, comment = "/** Deserializes string data when loading files */", args = { "std::string" } },
            { name = "load_template", rval = "bool", args = { "std::string" } },
            { name = "long_craft", rval = nil, args = { } },
            { name = "ma_onattack_effects", rval = nil, comment = "/** Fires all attack-triggered martial arts events */", args = { } },
            { name = "ma_onblock_effects", rval = nil, comment = "/** Fires all block-triggered martial arts events */", args = { } },
            { name = "ma_ondodge_effects", rval = nil, comment = "/** Fires all dodge-triggered martial arts events */", args = { } },
            { name = "ma_ongethit_effects", rval = nil, comment = "/** Fires all get hit-triggered martial arts events */", args = { } },
            { name = "ma_onhit_effects", rval = nil, comment = "/** Fires all hit-triggered martial arts events */", args = { } },
            { name = "ma_onmove_effects", rval = nil, comment = "/** Fires all move-triggered martial arts events */", args = { } },
            { name = "ma_static_effects", rval = nil, comment = "/** Fires all non-triggered martial arts events */", args = { } },
            { name = "mabuff_attack_cost_mult", rval = "float", comment = "/** Returns the multiplier on move cost of attacks. */", args = { } },
            { name = "mabuff_attack_cost_penalty", rval = "int", comment = "/** Returns the flat penalty to move cost of attacks. If negative, that's a bonus. Applied after multiplier. */", args = { } },
            { name = "mabuff_block_bonus", rval = "int", comment = "/** Returns the block bonus from martial arts buffs */", args = { } },
            { name = "mabuff_dodge_bonus", rval = "float", comment = "/** Returns the dodge bonus from martial arts buffs */", args = { } },
            { name = "mabuff_speed_bonus", rval = "int", comment = "/** Returns the speed bonus from martial arts buffs */", args = { } },
            { name = "mabuff_tohit_bonus", rval = "float", comment = "/** Returns the to hit bonus from martial arts buffs */", args = { } },
            { name = "make_all_craft", rval = nil, args = { "std::string", "int" } },
            { name = "make_craft", rval = nil, args = { "std::string", "int" } },
            { name = "make_craft_with_command", rval = nil, args = { "std::string", "int" } },
            { name = "make_craft_with_command", rval = nil, args = { "std::string", "int", "bool" } },
            { name = "making_would_work", rval = "bool", args = { "std::string", "int" } },
            { name = "melee_value", rval = "float", args = { "item" } },
            { name = "mend", rval = nil, comment = "/** Handles the chance for broken limbs to spontaneously heal to 1 HP */", args = { "int" } },
            { name = "metabolic_rate", rval = "float", comment = "/** Current metabolic rate due to traits, hunger, speed, etc. */", args = { } },
            { name = "metabolic_rate_base", rval = "float", comment = "/** Stable base metabolic rate due to traits */", args = { } },
            { name = "mod_painkiller", rval = nil, comment = "/** Modifies intensity of painkillers  */", args = { "int" } },
            { name = "mutate", rval = nil, comment = "/** Picks a random valid mutation and gives it to the player, possibly removing/changing others along the way */", args = { } },
            { name = "mutate_category", rval = nil, comment = "/** Picks a random valid mutation in a category and mutate_towards() it */", args = { "std::string" } },
            { name = "mutate_towards", rval = "bool", comment = "/** Mutates toward the entered mutation, upgrading or removing conflicts if necessary */", args = { "std::string" } },
            { name = "mutation_ok", rval = "bool", comment = "/** Returns true if the player doesn't have the mutation or a conflicting one and it complies with the force typing */", args = { "std::string", "bool", "bool" } },
            { name = "natural_attack_restricted_on", rval = "bool", comment = "/** Returns true if the player is wearing something on the entered body_part, ignoring items with the ALLOWS_NATURAL_ATTACKS flag */", args = { "body_part" } },
            { name = "num_bionics", rval = "int", comment = "/** Returns the size of my_bionics[] */", args = { } },
            { name = "nutrition_for", rval = "int", comment = "/** Handles the nutrition value for a comestible **/", args = { "itype" } },
            { name = "on_hurt", rval = nil, comment = "/** Handles effects that happen when the player is damaged and aware of the fact. */", args = { "Creature" } },
            { name = "on_hurt", rval = nil, comment = "/** Handles effects that happen when the player is damaged and aware of the fact. */", args = { "Creature", "bool" } },
            { name = "overmap_los", rval = "bool", comment = "/** Returns true if overmap tile is within player line-of-sight */", args = { "tripoint", "int" } },
            { name = "overmap_sight_range", rval = "int", comment = "/** Returns the distance the player can see on the overmap */", args = { "int" } },
            { name = "pause", rval = nil, args = { } },
            { name = "perform_special_attacks", rval = nil, comment = "/** Performs special attacks and their effects (poisonous, stinger, etc.) */", args = { "Creature" } },
            { name = "pick_style", rval = "bool", comment = "/** Creates the UI and handles player input for picking martial arts styles */", args = { } },
            { name = "pick_technique", rval = "matec_id", comment = "/** Returns a random valid technique */", args = { "Creature", "bool", "bool", "bool" } },
            { name = "pick_usb", rval = "item&", args = { } },
            { name = "place_corpse", rval = nil, args = { } },
            { name = "power_bionics", rval = nil, comment = "/** Generates and handles the UI for player interaction with installed bionics */", args = { } },
            { name = "power_mutations", rval = nil, args = { } },
            { name = "practice", rval = nil, args = { "skill_id", "int" } },
            { name = "practice", rval = nil, args = { "skill_id", "int", "int" } },
            { name = "print_health", rval = nil, args = { } },
            { name = "process_active_items", rval = nil, args = { } },
            { name = "process_bionic", rval = nil, comment = "/** Handles bionic effects over time of the entered bionic */", args = { "int" } },
            { name = "purifiable", rval = "bool", comment = "/** Returns true if the entered trait may be purified away
         *  Defaults to true
         */", args = { "std::string" } },
            { name = "reach_attack", rval = nil, comment = "/** Handles reach melee attacks */", args = { "tripoint" } },
            { name = "read", rval = "bool", comment = "/** Handles reading effects and returns true if activity started */", args = { "int" } },
            { name = "read", rval = "bool", comment = "/** Handles reading effects and returns true if activity started */", args = { "int", "bool" } },
            { name = "read_speed", rval = "int", comment = "/** Returns the player's reading speed */", args = { "bool" } },
            { name = "read_speed", rval = "int", comment = "/** Returns the player's reading speed */", args = { } },
            { name = "reassign_item", rval = nil, comment = "/** Reassign letter. */", args = { "item", "int" } },
            { name = "recalc_speed_bonus", rval = nil, comment = "/** Calculates the various speed bonuses we will get from mutations, etc. */", args = { } },
            { name = "recoil_total", rval = "float", comment = "/** Current total maximum recoil penalty from all sources */", args = { } },
            { name = "recoil_vehicle", rval = "float", comment = "/** Get maximum recoil penalty due to vehicle motion */", args = { } },
            { name = "recraft", rval = nil, args = { } },
            { name = "reduce_charges", rval = "item", comment = "/**
         * Remove charges from a specific item (given by a pointer to it).
         * Otherwise identical to @ref reduce_charges(int,long)
         * @param it A pointer to the item, it *must* exist.
         * @param quantity How many charges to remove
         */", args = { "item", "int" } },
            { name = "reduce_charges", rval = "item", comment = "/**
         * Remove charges from a specific item (given by its item position).
         * The item must exist and it must be counted by charges.
         * @param position Item position of the item.
         * @param quantity The number of charges to remove, must not be larger than
         * the current charges of the item.
         * @return An item that contains the removed charges, it's effectively a
         * copy of the item with the proper charges.
         */", args = { "int", "int" } },
            { name = "regen", rval = nil, comment = "/**
          * Handles passive regeneration of pain and maybe hp.
          */", args = { "int" } },
            { name = "rem_addiction", rval = nil, comment = "/** Removes an addition from the player */", args = { "add_type" } },
            { name = "remove_bionic", rval = nil, comment = "/** Removes a bionic from my_bionics[] */", args = { "std::string" } },
            { name = "remove_child_flag", rval = nil, comment = "/** Removes the mutation's child flag from the player's list */", args = { "std::string" } },
            { name = "remove_mutation", rval = nil, comment = "/** Removes a mutation, downgrading to the previous level if possible */", args = { "std::string" } },
            { name = "remove_random_bionic", rval = "bool", comment = "/** Randomly removes a bionic from my_bionics[] */", args = { } },
            { name = "rooted", rval = nil, args = { } },
            { name = "rooted_message", rval = nil, comment = "/** Handles rooting effects */", args = { } },
            { name = "run_cost", rval = "int", comment = "/** Returns the player's modified base movement cost */", args = { "int" } },
            { name = "run_cost", rval = "int", comment = "/** Returns the player's modified base movement cost */", args = { "int", "bool" } },
            { name = "rust_rate", rval = "int", comment = "/** Returns the player's skill rust rate */", args = { "bool" } },
            { name = "rust_rate", rval = "int", comment = "/** Returns the player's skill rust rate */", args = { } },
            { name = "save_info", rval = "std::string", comment = "/** Outputs a serialized json string for saving */", args = { } },
            { name = "scored_crit", rval = "bool", comment = "/** Returns true if the player scores a critical hit */", args = { "float" } },
            { name = "scored_crit", rval = "bool", comment = "/** Returns true if the player scores a critical hit */", args = { } },
            { name = "search_surroundings", rval = nil, comment = "/** Search surrounding squares for traps (and maybe other things in the future). */", args = { } },
            { name = "sees_with_infrared", rval = "bool", comment = "/**
         * Check whether the this player can see the other creature with infrared. This implies
         * this player can see infrared and the target is visible with infrared (is warm).
         * And of course a line of sight exists.
         */", args = { "Creature" } },
            { name = "serialize", rval = "std::string", args = { } },
            { name = "setID", rval = nil, args = { "int" } },
            { name = "set_cat_level_rec", rval = nil, comment = "/** Modifies mutation_category_level[] based on the entered trait */", args = { "std::string" } },
            { name = "set_destination", rval = nil, args = { "std::vector<tripoint>" } },
            { name = "set_highest_cat_level", rval = nil, comment = "/** Recalculates mutation_category_level[] values for the player */", args = { } },
            { name = "set_painkiller", rval = nil, comment = "/** Sets intensity of painkillers  */", args = { "int" } },
            { name = "set_underwater", rval = nil, args = { "bool" } },
            { name = "setx", rval = nil, args = { "int" } },
            { name = "sety", rval = nil, args = { "int" } },
            { name = "setz", rval = nil, args = { "int" } },
            { name = "shift_destination", rval = nil, args = { "int", "int" } },
            { name = "shoe_type_count", rval = "int", comment = "/** Returns 1 if the player is wearing an item of that count on one foot, 2 if on both, and zero if on neither */", args = { "std::string" } },
            { name = "shout", rval = nil, args = { "std::string" } },
            { name = "shout", rval = nil, args = { } },
            { name = "sight_impaired", rval = "bool", comment = "/** Returns true if the player has some form of impaired sight */", args = { } },
            { name = "sleep_spot", rval = "int", comment = "/** Rate point's ability to serve as a bed. Takes mutations, fatigue and stimms into account. */", args = { "tripoint" } },
            { name = "sort_armor", rval = nil, comment = "/** Draws the UI and handles player input for the armor re-ordering window */", args = { } },
            { name = "spores", rval = nil, args = { } },
            { name = "stomach_capacity", rval = "int", comment = "/** Gets player's minimum hunger and thirst */", args = { } },
            { name = "store", rval = nil, comment = "/**
         * Stores an item inside another consuming moves proportional to weapon skill and volume
         * @param container Container in which to store the item
         * @param put Item to add to the container
         * @param penalties Whether item volume and temporary effects (eg. GRABBED, DOWNED) should be considered.
         * @param base_cost Cost due to storage type.
         */", args = { "item", "item" } },
            { name = "store", rval = nil, comment = "/**
         * Stores an item inside another consuming moves proportional to weapon skill and volume
         * @param container Container in which to store the item
         * @param put Item to add to the container
         * @param penalties Whether item volume and temporary effects (eg. GRABBED, DOWNED) should be considered.
         * @param base_cost Cost due to storage type.
         */", args = { "item", "item", "bool" } },
            { name = "store", rval = nil, comment = "/**
         * Stores an item inside another consuming moves proportional to weapon skill and volume
         * @param container Container in which to store the item
         * @param put Item to add to the container
         * @param penalties Whether item volume and temporary effects (eg. GRABBED, DOWNED) should be considered.
         * @param base_cost Cost due to storage type.
         */", args = { "item", "item", "bool", "int" } },
            { name = "studied_all_recipes", rval = "bool", args = { "itype" } },
            { name = "suffer", rval = nil, comment = "/** Handles a large number of timers decrementing and other randomized effects */", args = { } },
            { name = "swim_speed", rval = "int", comment = "/** Returns the player's speed for swimming across water tiles */", args = { } },
            { name = "takeoff", rval = "bool", args = { "int" } },
            { name = "takeoff", rval = "bool", comment = "/** Takes off an item, returning false on fail. The taken off item is processed in the interact */", args = { "item" } },
            { name = "takeoff", rval = "bool", comment = "/** Takes off an item, returning false on fail. The taken off item is processed in the interact */", args = { "item", "std::list<item>" } },
            { name = "talk_skill", rval = "int", comment = "/** Returns a value used when attempting to convince NPC's of something */", args = { } },
            { name = "temp_equalizer", rval = nil, args = { "body_part", "body_part" } },
            { name = "thirst_speed_penalty", static = true, rval = "int", comment = "/** Returns the penalty to speed from thirst */", args = { "int" } },
            { name = "thrown_current_range", rval = "float", comment = "/**
         *  Calculate range at which given chance of hit considering player stats, clothing and recoil
         *  @param to_throw item that will be thrown
         *  @param chance probability of hit, range [0-100) with zero returning absolute maximum range
         *  @param accuracy minimum accuracy required
         *  @param target the target creature (can be null) who may try to dodge the thrown item
         */", args = { "item" } },
            { name = "thrown_current_range", rval = "float", comment = "/**
         *  Calculate range at which given chance of hit considering player stats, clothing and recoil
         *  @param to_throw item that will be thrown
         *  @param chance probability of hit, range [0-100) with zero returning absolute maximum range
         *  @param accuracy minimum accuracy required
         *  @param target the target creature (can be null) who may try to dodge the thrown item
         */", args = { "item", "int" } },
            { name = "thrown_current_range", rval = "float", comment = "/**
         *  Calculate range at which given chance of hit considering player stats, clothing and recoil
         *  @param to_throw item that will be thrown
         *  @param chance probability of hit, range [0-100) with zero returning absolute maximum range
         *  @param accuracy minimum accuracy required
         *  @param target the target creature (can be null) who may try to dodge the thrown item
         */", args = { "item", "int", "float" } },
            { name = "thrown_current_range", rval = "float", comment = "/**
         *  Calculate range at which given chance of hit considering player stats, clothing and recoil
         *  @param to_throw item that will be thrown
         *  @param chance probability of hit, range [0-100) with zero returning absolute maximum range
         *  @param accuracy minimum accuracy required
         *  @param target the target creature (can be null) who may try to dodge the thrown item
         */", args = { "item", "int", "float", "Creature" } },
            { name = "time_to_read", rval = "int", comment = "/**
         * Helper function for get_book_reader
         * @warning This function assumes that the everyone is able to read
         *
         * @param book The book being read
         * @param reader the player/NPC who's reading to the caller
         * @param learner if not nullptr, assume that the caller and reader read at a pace that isn't too fast for him
         */", args = { "item", "player" } },
            { name = "time_to_read", rval = "int", comment = "/**
         * Helper function for get_book_reader
         * @warning This function assumes that the everyone is able to read
         *
         * @param book The book being read
         * @param reader the player/NPC who's reading to the caller
         * @param learner if not nullptr, assume that the caller and reader read at a pace that isn't too fast for him
         */", args = { "item", "player", "player" } },
            { name = "toggle_move_mode", rval = nil, args = { } },
            { name = "try_to_sleep", rval = nil, comment = "/** Handles sleep attempts by the player, adds "lying_down" */", args = { } },
            { name = "unarmed_attack", rval = "bool", comment = "/** True if unarmed or wielding a weapon with the UNARMED_WEAPON flag */", args = { } },
            { name = "unarmed_value", rval = "float", args = { } },
            { name = "unimpaired_range", rval = "int", comment = "/** Returns the player maximum vision range factoring in mutations, diseases, and other effects */", args = { } },
            { name = "uninstall_bionic", rval = "bool", comment = "/** Used by the player to perform surgery to remove bionics and possibly retrieve parts */", args = { "std::string" } },
            { name = "uninstall_bionic", rval = "bool", comment = "/** Used by the player to perform surgery to remove bionics and possibly retrieve parts */", args = { "std::string", "int" } },
            { name = "update_body", rval = nil, comment = "/** Updates all "biology" as if time between `from` and `to` passed. */", args = { "int", "int" } },
            { name = "update_body", rval = nil, comment = "/** Updates all "biology" by one turn. Should be called once every turn. */", args = { } },
            { name = "update_body_wetness", rval = nil, comment = "/** Maintains body wetness and handles the rate at which the player dries */", args = { "w_point" } },
            { name = "update_bodytemp", rval = nil, comment = "/** Maintains body temperature */", args = { } },
            { name = "update_mental_focus", rval = nil, comment = "/** Uses calc_focus_equilibrium to update the player's current focus */", args = { } },
            { name = "update_morale", rval = nil, comment = "/** Ticks down morale counters and removes them */", args = { } },
            { name = "update_needs", rval = nil, comment = "/** Increases hunger, thirst, fatigue and stimms wearing off. `rate_multiplier` is for retroactive updates. */", args = { "int" } },
            { name = "update_stamina", rval = nil, comment = "/** Regenerates stamina */", args = { "int" } },
            { name = "use", rval = nil, comment = "/** Uses a tool */", args = { "int" } },
            { name = "use_amount", rval = "std::list<item>", args = { "std::string", "int" } },
            { name = "use_charges", rval = "std::list<item>", args = { "std::string", "int" } },
            { name = "use_charges_if_avail", rval = "bool", args = { "std::string", "int" } },
            { name = "use_wielded", rval = nil, comment = "/** Uses the current wielded weapon */", args = { } },
            { name = "visibility", rval = "int", args = { "bool" } },
            { name = "visibility", rval = "int", args = { "bool", "int" } },
            { name = "visibility", rval = "int", args = { } },
            { name = "vomit", rval = nil, comment = "/** Handles player vomiting effects */", args = { } },
            { name = "vomit_mod", rval = "float", comment = "/** Returns the modifier value used for vomiting effects. */", args = { } },
            { name = "wake_up", rval = nil, comment = "/** Removes "sleep" and "lying_down" from the player */", args = { } },
            { name = "warmth", rval = "int", comment = "/** Returns warmth provided by armor, etc. */", args = { "body_part" } },
            { name = "weapname", rval = "std::string", comment = "/** Get the formatted name of the currently wielded item (if any) */", args = { } },
            { name = "weapon_value", rval = "float", comment = "/** NPC-related item rating functions */", args = { "item" } },
            { name = "weapon_value", rval = "float", comment = "/** NPC-related item rating functions */", args = { "item", "int" } },
            { name = "wear", rval = "bool", comment = "/** Wear item; returns false on fail. If interactive is false, don't alert the player or drain moves on completion. */", args = { "int" } },
            { name = "wear", rval = "bool", comment = "/** Wear item; returns false on fail. If interactive is false, don't alert the player or drain moves on completion. */", args = { "int", "bool" } },
            { name = "wear_item", rval = "bool", comment = "/** Wear item; returns false on fail. If interactive is false, don't alert the player or drain moves on completion. */", args = { "item" } },
            { name = "wear_item", rval = "bool", comment = "/** Wear item; returns false on fail. If interactive is false, don't alert the player or drain moves on completion. */", args = { "item", "bool" } },
            { name = "wearing_something_on", rval = "bool", comment = "/** Returns true if the player is wearing something on the entered body_part */", args = { "body_part" } },
            { name = "wield", rval = "bool", comment = "/**
         * Removes currently wielded item (if any) and replaces it with the target item
         * @param target replacement item to wield or null item to remove existing weapon without replacing it
         * @return whether both removal and replacement were successful (they are performed atomically)
         */", args = { "item" } },
            { name = "wield_contents", rval = "bool", comment = "/**
         * Try to wield a contained item consuming moves proportional to weapon skill and volume.
         * @param container Containter containing the item to be wielded
         * @param pos index of contained item to wield. Set to -1 to show menu if container has more than one item
         * @param penalties Whether item volume and temporary effects (eg. GRABBED, DOWNED) should be considered.
         * @param base_cost Cost due to storage type.
         */", args = { "item" } },
            { name = "wield_contents", rval = "bool", comment = "/**
         * Try to wield a contained item consuming moves proportional to weapon skill and volume.
         * @param container Containter containing the item to be wielded
         * @param pos index of contained item to wield. Set to -1 to show menu if container has more than one item
         * @param penalties Whether item volume and temporary effects (eg. GRABBED, DOWNED) should be considered.
         * @param base_cost Cost due to storage type.
         */", args = { "item", "int" } },
            { name = "wield_contents", rval = "bool", comment = "/**
         * Try to wield a contained item consuming moves proportional to weapon skill and volume.
         * @param container Containter containing the item to be wielded
         * @param pos index of contained item to wield. Set to -1 to show menu if container has more than one item
         * @param penalties Whether item volume and temporary effects (eg. GRABBED, DOWNED) should be considered.
         * @param base_cost Cost due to storage type.
         */", args = { "item", "int", "bool" } },
            { name = "wield_contents", rval = "bool", comment = "/**
         * Try to wield a contained item consuming moves proportional to weapon skill and volume.
         * @param container Containter containing the item to be wielded
         * @param pos index of contained item to wield. Set to -1 to show menu if container has more than one item
         * @param penalties Whether item volume and temporary effects (eg. GRABBED, DOWNED) should be considered.
         * @param base_cost Cost due to storage type.
         */", args = { "item", "int", "bool", "int" } },
        }
    },
    item = {
        new = {
            { "item" },
            { "itype" },
            { "itype", "int" },
            { "itype", "int", "int" },
            { "std::string" },
            { "std::string", "int" },
            { "std::string", "int", "int" },
            { },
        },
        by_value_and_reference = true,
        attributes = {
            active = { type = "bool", writable = true },
            bday = { type = "int", writable = true },
            burnt = { type = "int", writable = true },
            charges = { type = "int", writable = true },
            components = { type = "std::vector<item>", writable = true },
            contents = { type = "std::list<item>", writable = true },
            frequency = { type = "int", writable = true },
            fridge = { type = "int", writable = true },
            invlet = { type = "int", writable = true },
            irridation = { type = "int", writable = true },
            item_counter = { type = "int", writable = true },
            item_tags = { type = "std::set<std::string>", writable = true },
            mission_id = { type = "int", writable = true },
            note = { type = "int", writable = true },
            player_id = { type = "int", writable = true },
            poison = { type = "int", writable = true },
            type = { type = "itype", writable = true },
        },
        functions = {
            { name = "acid_resist", rval = "int", args = { "bool" } },
            { name = "acid_resist", rval = "int", args = { } },
            { name = "activate", rval = "item&", comment = "/** Filter converting instance to active state */", args = { } },
            { name = "add_rain_to_container", rval = nil, args = { "bool" } },
            { name = "add_rain_to_container", rval = nil, args = { "bool", "int" } },
            { name = "add_technique", rval = nil, comment = "/**
         * Add the given technique to the item specific @ref techniques. Note that other items of
         * the same type are not affected by this.
         */", args = { "matec_id" } },
            { name = "allow_crafting_component", rval = "bool", comment = "/** Can item can be used as crafting component in current state? */", args = { } },
            { name = "already_used_by_player", rval = "bool", comment = "/**
     * Check whether the item has been marked (by calling mark_as_used_by_player)
     * as used by this specific player.
     */", args = { "player" } },
            { name = "ammo_capacity", rval = "int", comment = "/** Maximum quantity of ammunition loadable for tool, gun or axuiliary gunmod */", args = { } },
            { name = "ammo_consume", rval = "int", comment = "/**
         * Consume ammo (if available) and return the amount of ammo that was consumed
         * @param qty maximum amount of ammo that should be consumed
         * @param pos current location of item, used for ejecting magazines and similar effects
         * @return amount of ammo consumed which will be between 0 and qty
         */", args = { "int", "tripoint" } },
            { name = "ammo_current", rval = "std::string", comment = "/** Specific ammo type, returns "null" if item is neither ammo nor loaded with any */", args = { } },
            { name = "ammo_data", rval = "itype&", comment = "/** Specific ammo data, returns nullptr if item is neither ammo nor loaded with any */", args = { } },
            { name = "ammo_default", rval = "std::string", comment = "/** Get default ammo used by item or "NULL" if item does not have a default ammo type
         *  @param conversion whether to include the effect of any flags or mods which convert the type
         *  @return NULL if item does not use a specific ammo type (and is consequently not reloadable) */", args = { "bool" } },
            { name = "ammo_default", rval = "std::string", comment = "/** Get default ammo used by item or "NULL" if item does not have a default ammo type
         *  @param conversion whether to include the effect of any flags or mods which convert the type
         *  @return NULL if item does not use a specific ammo type (and is consequently not reloadable) */", args = { } },
            { name = "ammo_effects", rval = "std::set<std::string>", comment = "/** Get ammo effects for item optionally inclusive of any resulting from the loaded ammo */", args = { "bool" } },
            { name = "ammo_effects", rval = "std::set<std::string>", comment = "/** Get ammo effects for item optionally inclusive of any resulting from the loaded ammo */", args = { } },
            { name = "ammo_remaining", rval = "int", comment = "/** Quantity of ammunition currently loaded in tool, gun or axuiliary gunmod */", args = { } },
            { name = "ammo_required", rval = "int", comment = "/** Quantity of ammunition consumed per usage of tool or with each shot of gun */", args = { } },
            { name = "ammo_set", rval = "item&", comment = "/**
         * Filter setting the ammo for this instance
         * Any existing ammo is removed. If necessary a magazine is also added.
         * @param ammo specific type of ammo (must be compatible with item ammo type)
         * @param qty maximum ammo (capped by item capacity) or negative to fill to capacity
         * @return same instance to allow method chaining
         */", args = { "std::string" } },
            { name = "ammo_set", rval = "item&", comment = "/**
         * Filter setting the ammo for this instance
         * Any existing ammo is removed. If necessary a magazine is also added.
         * @param ammo specific type of ammo (must be compatible with item ammo type)
         * @param qty maximum ammo (capped by item capacity) or negative to fill to capacity
         * @return same instance to allow method chaining
         */", args = { "std::string", "int" } },
            { name = "ammo_sufficient", rval = "bool", comment = "/**
         * Check if sufficient ammo is loaded for given number of uses.
         *
         * Check if there is enough ammo loaded in a tool for the given number of uses
         * or given number of gun shots.  Using this function for this check is preferred
         * because we expect to add support for items consuming multiple ammo types in
         * the future.  Users of this function will not need to be refactored when this
         * happens.
         *
         * @param[in] qty Number of uses
         * @returns true if ammo sufficent for number of uses is loaded, false otherwise
         */", args = { "int" } },
            { name = "ammo_sufficient", rval = "bool", comment = "/**
         * Check if sufficient ammo is loaded for given number of uses.
         *
         * Check if there is enough ammo loaded in a tool for the given number of uses
         * or given number of gun shots.  Using this function for this check is preferred
         * because we expect to add support for items consuming multiple ammo types in
         * the future.  Users of this function will not need to be refactored when this
         * happens.
         *
         * @param[in] qty Number of uses
         * @returns true if ammo sufficent for number of uses is loaded, false otherwise
         */", args = { } },
            { name = "ammo_unset", rval = "item&", comment = "/**
         * Filter removing all ammo from this instance
         * If the item is neither a tool, gun nor magazine is a no-op
         * For items reloading using magazines any empty magazine remains present.
         */", args = { } },
            { name = "amount_of", rval = "int", comment = "/**
         * Count items matching id including both this instance and any contained items
         * @param what ID of items to count
         * @param pseudo whether pseudo-items (from map/vehicle tiles, bionics etc) are considered
         * @param limit stop searching after this many matches
         * @note items must be empty to be considered a match
         */", args = { "std::string" } },
            { name = "amount_of", rval = "int", comment = "/**
         * Count items matching id including both this instance and any contained items
         * @param what ID of items to count
         * @param pseudo whether pseudo-items (from map/vehicle tiles, bionics etc) are considered
         * @param limit stop searching after this many matches
         * @note items must be empty to be considered a match
         */", args = { "std::string", "bool" } },
            { name = "amount_of", rval = "int", comment = "/**
         * Count items matching id including both this instance and any contained items
         * @param what ID of items to count
         * @param pseudo whether pseudo-items (from map/vehicle tiles, bionics etc) are considered
         * @param limit stop searching after this many matches
         * @note items must be empty to be considered a match
         */", args = { "std::string", "bool", "int" } },
            { name = "attack_time", rval = "int", comment = "/**
     * Base number of moves (@ref Creature::moves) that a single melee attack with this items
     * takes. The actual time depends heavily on the attacker, see melee.cpp.
     */", args = { } },
            { name = "base_volume", rval = "units::volume", comment = "/** Simplified, faster volume check for when processing time is important and exact volume is not. */", args = { } },
            { name = "bash_resist", rval = "int", args = { "bool" } },
            { name = "bash_resist", rval = "int", args = { } },
            { name = "brewing_results", rval = "std::vector<std::string>", comment = "/** The results of fermenting this item. */", args = { } },
            { name = "brewing_time", rval = "int", comment = "/** Turns for this item to be fully fermented. */", args = { } },
            { name = "calc_rot", rval = nil, comment = "/**
     * Accumulate rot of the item since last rot calculation.
     * This function works for non-rotting stuff, too - it increases the value
     * of rot.
     * @param p The absolute, global location (in map square coordinates) of the item to
     * check for temperature.
     */", args = { "tripoint" } },
            { name = "can_contain", rval = "bool", args = { "item" } },
            { name = "can_contain", rval = "bool", args = { "itype" } },
            { name = "can_holster", rval = "bool", comment = "/** Checks if item is a holster and currently capable of storing obj
         *  @param obj object that we want to holster
         *  @param ignore only check item is compatible and ignore any existing contents
         */", args = { "item" } },
            { name = "can_holster", rval = "bool", comment = "/** Checks if item is a holster and currently capable of storing obj
         *  @param obj object that we want to holster
         *  @param ignore only check item is compatible and ignore any existing contents
         */", args = { "item", "bool" } },
            { name = "can_revive", rval = "bool", comment = "/**
         * Whether this is a corpse that can be revived.
         */", args = { } },
            { name = "casings_count", rval = "int", comment = "/** How many spent casings are contained within this item? */", args = { } },
            { name = "charges_of", rval = "int", comment = "/**
         * Count maximum available charges from this instance and any contained items
         * @param what ID of item to count charges of
         * @param limit stop searching after this many charges have been found
         */", args = { "std::string" } },
            { name = "charges_of", rval = "int", comment = "/**
         * Count maximum available charges from this instance and any contained items
         * @param what ID of item to count charges of
         * @param limit stop searching after this many charges have been found
         */", args = { "std::string", "int" } },
            { name = "charges_per_volume", rval = "int", comment = "/**
         * Number of charges of this item that fit into the given volume.
         * May return 0 if not even one charge fits into the volume. Only depends on the *type*
         * of this item not on its current charge count.
         *
         * For items not counted by charges, this returns this->volume() / vol.
         */", args = { "units::volume" } },
            { name = "chip_resistance", rval = "int", comment = "/**
     * Returns resistance to being damaged by attack against the item itself.
     * Calculated from item's materials.
     * @param worst If this is true, the worst resistance is used. Otherwise the best one.
     */", args = { "bool" } },
            { name = "chip_resistance", rval = "int", comment = "/**
     * Returns resistance to being damaged by attack against the item itself.
     * Calculated from item's materials.
     * @param worst If this is true, the worst resistance is used. Otherwise the best one.
     */", args = { } },
            { name = "clear_vars", rval = nil, comment = "/** Removes all item variables. */", args = { } },
            { name = "color", rval = "int", comment = "/**
     * Returns the default color of the item (e.g. @ref itype::color).
     */", args = { } },
            { name = "color_in_inventory", rval = "int", comment = "/**
     * Returns the color of the item depending on usefulness for the player character,
     * e.g. differently if it its an unread book or a spoiling food item etc.
     * This should only be used for displaying data, it should not affect game play.
     */", args = { } },
            { name = "components_to_string", rval = "std::string", comment = "/** List of all @ref components in printable form, empty if this item has
     * no components */", args = { } },
            { name = "conductive", rval = "bool", comment = "/**
     * Whether the items is conductive.
     */", args = { } },
            { name = "contextualize_skill", rval = "skill_id", comment = "/** Puts the skill in context of the item */", args = { "skill_id" } },
            { name = "convert", rval = "item&", comment = "/**
         * Filter converting this instance to another type preserving all other aspects
         * @param new_type the type id to convert to
         * @return same instance to allow method chaining
         */", args = { "std::string" } },
            { name = "count_by_charges", rval = "bool", args = { } },
            { name = "count_by_charges", static = true, rval = "bool", comment = "/**
         * Whether the item is counted by charges, this is a static wrapper
         * around @ref count_by_charges, that does not need an items instance.
         */", args = { "std::string" } },
            { name = "covers", rval = "bool", comment = "/**
         * Whether this item (when worn) covers the given body part.
         */", args = { "body_part" } },
            { name = "craft_has_charges", rval = "bool", args = { } },
            { name = "cut_resist", rval = "int", args = { "bool" } },
            { name = "cut_resist", rval = "int", args = { } },
            { name = "damage", rval = "int", comment = "/** How much damage has the item sustained? */", args = { } },
            { name = "damage_color", rval = "int", comment = "/** Provide color for UI display dependent upon current item damage level */", args = { } },
            { name = "damage_symbol", rval = "std::string", comment = "/** Provide prefix symbol for UI display dependent upon current item damage level */", args = { } },
            { name = "deactivate", rval = "item&", comment = "/**
         * Filter converting this instance to the inactive type
         * If the item is either inactive or cannot be deactivated is a no-op
         * @param ch character currently possessing or acting upon the item (if any)
         * @param alert whether to display any messages
         * @return same instance to allow method chaining
         */", args = { "Character" } },
            { name = "deactivate", rval = "item&", comment = "/**
         * Filter converting this instance to the inactive type
         * If the item is either inactive or cannot be deactivated is a no-op
         * @param ch character currently possessing or acting upon the item (if any)
         * @param alert whether to display any messages
         * @return same instance to allow method chaining
         */", args = { "Character", "bool" } },
            { name = "deactivate", rval = "item&", comment = "/**
         * Filter converting this instance to the inactive type
         * If the item is either inactive or cannot be deactivated is a no-op
         * @param ch character currently possessing or acting upon the item (if any)
         * @param alert whether to display any messages
         * @return same instance to allow method chaining
         */", args = { } },
            { name = "deserialize", rval = nil, args = { "std::string" } },
            { name = "destroyed_at_zero_charges", rval = "bool", args = { } },
            { name = "detonate", rval = "bool", comment = "/**
     * Detonates the item and adds remains (if any) to drops.
     * Returns true if the item actually detonated,
     * potentially destroying other items and invalidating iterators.
     * Should NOT be called on an item on the map, but on a local copy.
     */", args = { "tripoint", "std::vector<item>" } },
            { name = "display_name", rval = "std::string", comment = "/**
     * Returns the item name and the charges or contained charges (if the item can have
     * charges at at all). Calls @ref tname with given quantity and with_prefix being true.
     */", args = { "int" } },
            { name = "display_name", rval = "std::string", comment = "/**
     * Returns the item name and the charges or contained charges (if the item can have
     * charges at at all). Calls @ref tname with given quantity and with_prefix being true.
     */", args = { } },
            { name = "engine_displacement", rval = "int", comment = "/** for combustion engines the displacement (cc) */", args = { } },
            { name = "erase_var", rval = nil, comment = "/** Erase the value of the given variable. */", args = { "std::string" } },
            { name = "fill_with", rval = nil, comment = "/**
     * Fill item with liquid up to its capacity. This works for guns and tools that accept
     * liquid ammo.
     * @param liquid Liquid to fill the container with.
     * @param amount Amount to fill item with, capped by remaining capacity
     */", args = { "item" } },
            { name = "fill_with", rval = nil, comment = "/**
     * Fill item with liquid up to its capacity. This works for guns and tools that accept
     * liquid ammo.
     * @param liquid Liquid to fill the container with.
     * @param amount Amount to fill item with, capped by remaining capacity
     */", args = { "item", "int" } },
            { name = "find_parent", rval = "item&", comment = "/**
         * Determine the immediate parent container (if any) for an item.
         * @param it item to search for which must be contained (at any depth) by this object
         * @return parent container or nullptr if the item is not within a container
         */", args = { "item" } },
            { name = "find_type", static = true, rval = "itype&", comment = "/**
         * Returns the item type of the given identifier. Never returns null.
         */", args = { "std::string" } },
            { name = "fire_resist", rval = "int", args = { "bool" } },
            { name = "fire_resist", rval = "int", args = { } },
            { name = "flammable", rval = "bool", comment = "/**
     * Whether the items is flammable. (Make sure to keep this in sync with
     * fire code in fields.cpp)
     * @param threshold Item is flammable if it provides more fuel than threshold.
     */", args = { "int" } },
            { name = "flammable", rval = "bool", comment = "/**
     * Whether the items is flammable. (Make sure to keep this in sync with
     * fire code in fields.cpp)
     * @param threshold Item is flammable if it provides more fuel than threshold.
     */", args = { } },
            { name = "fuel_energy", rval = "float", comment = "/** Returns energy of one charge of this item as fuel for an engine. */", args = { } },
            { name = "get_base_material", rval = "material_type&", comment = "/**
     * Get the basic (main) material of this item. May return the null-material.
     */", args = { } },
            { name = "get_cable_target", rval = "tripoint", comment = "/**
     * Gets the point (vehicle tile) the cable is connected to.
     * Returns tripoint_min if not connected to anything.
     */", args = { } },
            { name = "get_chapters", rval = "int", comment = "/**
         * How many chapters the book has (if any). Will be 0 if the item is not a book, or if it
         * has no chapters at all.
         * Each reading will "consume" a chapter, if the book has no unread chapters, it's less fun.
         */", args = { } },
            { name = "get_contained", rval = "item", comment = "/**
         * Return a contained item (if any and only one).
         */", args = { } },
            { name = "get_container_capacity", rval = "units::volume", comment = "/**
     * It returns the total capacity (volume) of the container.
     */", args = { } },
            { name = "get_coverage", rval = "int", comment = "/**
         * Returns the relative coverage that this item has when worn.
         * Values range from 0 (not covering anything, or no armor at all) to
         * 100 (covering the whole body part). Items that cover more are more likely to absorb
         * damage from attacks.
         */", args = { } },
            { name = "get_encumber", rval = "int", comment = "/**
         * Returns the encumbrance value that this item has when worn.
         * Returns 0 if this is can not be worn at all.
         */", args = { } },
            { name = "get_env_resist", rval = "int", comment = "/**
         * Returns the resistance to environmental effects (@ref islot_armor::env_resist) that this
         * item provides when worn. See @ref player::get_env_resist. Higher values are better.
         * For non-armor it returns 0.
         */", args = { } },
            { name = "get_free_mod_locations", rval = "int", comment = "/**
         * Number of mods that can still be installed into the given mod location,
         * for non-guns it always returns 0.
         */", args = { "std::string" } },
            { name = "get_gun_ups_drain", rval = "int", args = { } },
            { name = "get_layer", rval = "int", comment = "/**
         * Returns clothing layer for item which will always be 0 for non-wearable items.
         */", args = { } },
            { name = "get_mtype", rval = "mtype&", comment = "/**
         * @return The monster type associated with this item (@ref corpse). It is usually the
         * type that this item is made of (e.g. corpse, meat or blood of the monster).
         * May return a null-pointer.
         */", args = { } },
            { name = "get_plant_epoch", rval = "int", comment = "/**
         * Time (in turns) it takes to grow from one stage to another. There are 4 plant stages:
         * seed, seedling, mature and harvest. Non-seed items return 0.
         */", args = { } },
            { name = "get_plant_name", rval = "std::string", comment = "/**
         * The name of the plant as it appears in the various informational menus. This should be
         * translated. Returns an empty string for non-seed items.
         */", args = { } },
            { name = "get_property_long", rval = "int", args = { "std::string" } },
            { name = "get_property_long", rval = "int", args = { "std::string", "int" } },
            { name = "get_property_string", rval = "std::string", comment = "/**
          * Get typed property for item.
          * Return same type as the passed default value, or string where no default provided
          */", args = { "std::string" } },
            { name = "get_property_string", rval = "std::string", comment = "/**
          * Get typed property for item.
          * Return same type as the passed default value, or string where no default provided
          */", args = { "std::string", "std::string" } },
            { name = "get_quality", rval = "int", args = { "quality_id" } },
            { name = "get_random_material", rval = "material_type&", comment = "/**
     * Get a material reference to a random material that this item is made of.
     * This might return the null-material, you may check this with @ref material_type::ident.
     * Note that this may also return a different material each time it's invoked (if the
     * item is made from several materials).
     */", args = { } },
            { name = "get_relative_rot", rval = "float", comment = "/** Get @ref rot value relative to shelf life (or 0 if item does not spoil) */", args = { } },
            { name = "get_remaining_capacity_for_liquid", rval = "int", args = { "item", "Character" } },
            { name = "get_remaining_capacity_for_liquid", rval = "int", comment = "/**
     * How much more of this liquid (in charges) can be put in this container.
     * If this is not a container (or not suitable for the liquid), it returns 0.
     * Note that mixing different types of liquid is not possible.
     * Also note that this works for guns and tools that accept liquid ammo.
     * @param liquid Liquid to check capacity for
     * @param allow_bucket Allow filling non-sealable containers
     * @param err Message to print if no more material will fit
     */", args = { "item" } },
            { name = "get_remaining_capacity_for_liquid", rval = "int", comment = "/**
     * How much more of this liquid (in charges) can be put in this container.
     * If this is not a container (or not suitable for the liquid), it returns 0.
     * Note that mixing different types of liquid is not possible.
     * Also note that this works for guns and tools that accept liquid ammo.
     * @param liquid Liquid to check capacity for
     * @param allow_bucket Allow filling non-sealable containers
     * @param err Message to print if no more material will fit
     */", args = { "item", "bool" } },
            { name = "get_remaining_chapters", rval = "int", comment = "/**
         * Get the number of unread chapters. If the item is no book or has no chapters, it returns 0.
         * This is a per-character setting, different characters may have different number of
         * unread chapters.
         */", args = { "player" } },
            { name = "get_rot", rval = "int", args = { } },
            { name = "get_storage", rval = "units::volume", comment = "/**
         * Returns the storage amount (@ref islot_armor::storage) that this item provides when worn.
         * For non-armor it returns 0. The storage amount increases the volume capacity of the
         * character that wears the item.
         */", args = { } },
            { name = "get_techniques", rval = "std::set<matec_id>", comment = "/**
         * Returns all the martial art techniques that this items supports.
         */", args = { } },
            { name = "get_thickness", rval = "int", comment = "/**
         * Returns the @ref islot_armor::thickness value, or 0 for non-armor. Thickness is are
         * relative value that affects the items resistance against bash / cutting damage.
         */", args = { } },
            { name = "get_usable_item", rval = "item&", comment = "/**
         * Checks this item and its contents (recursively) for types that have
         * use_function with type use_name. Returns the first item that does have
         * such type or nullptr if none found.
         */", args = { "std::string" } },
            { name = "get_var", rval = "float", args = { "std::string", "float" } },
            { name = "get_var", rval = "std::string", args = { "std::string", "std::string" } },
            { name = "get_var", rval = "std::string", comment = "/** Get the variable, if it does not exists, returns an empty string. */", args = { "std::string" } },
            { name = "get_warmth", rval = "int", comment = "/**
         * Returns the warmth value that this item has when worn. See player class for temperature
         * related code, or @ref player::warmth. Returned values should be positive. A value
         * of 0 indicates no warmth from this item at all (this is also the default for non-armor).
         */", args = { } },
            { name = "getlight_emit", rval = "int", comment = "/**
         * How much light (see lightmap.cpp) the item emits (it's assumed to be circular).
         */", args = { } },
            { name = "goes_bad", rval = "bool", comment = "/** whether an item is perishable (can rot) */", args = { } },
            { name = "gun_cycle_mode", rval = nil, comment = "/** Switch to the next available firing mode */", args = { } },
            { name = "gun_damage", rval = "int", comment = "/**
         * Summed ranged damage of a gun, including values from mods. Returns 0 on non-gun items.
         */", args = { "bool" } },
            { name = "gun_damage", rval = "int", comment = "/**
         * Summed ranged damage of a gun, including values from mods. Returns 0 on non-gun items.
         */", args = { } },
            { name = "gun_dispersion", rval = "int", comment = "/**
         * Summed dispersion of a gun, including values from mods. Returns 0 on non-gun items.
         */", args = { "bool" } },
            { name = "gun_dispersion", rval = "int", comment = "/**
         * Summed dispersion of a gun, including values from mods. Returns 0 on non-gun items.
         */", args = { } },
            { name = "gun_get_mode_id", rval = "std::string", comment = "/** Get id of mode a gun is currently set to, eg. DEFAULT, AUTO, BURST */", args = { } },
            { name = "gun_pierce", rval = "int", comment = "/**
         * Summed ranged armor-piercing of a gun, including values from mods. Returns 0 on non-gun items.
         */", args = { "bool" } },
            { name = "gun_pierce", rval = "int", comment = "/**
         * Summed ranged armor-piercing of a gun, including values from mods. Returns 0 on non-gun items.
         */", args = { } },
            { name = "gun_range", rval = "int", comment = "/**
         * Summed range value of a gun, including values from mods. Returns 0 on non-gun items.
         */", args = { "bool" } },
            { name = "gun_range", rval = "int", comment = "/**
         * Summed range value of a gun, including values from mods. Returns 0 on non-gun items.
         */", args = { } },
            { name = "gun_range", rval = "int", comment = "/**
         * The weapons range in map squares. If the item has an active gunmod, it returns the range
         * of that gunmod, the guns range is returned only when the item has no active gunmod.
         * This function applies to guns and auxiliary gunmods. For other items, 0 is returned.
         * It includes the range given by the ammo.
         * @param u The player that uses the weapon, their strength might affect this.
         * It's optional and can be null.
         */", args = { "player" } },
            { name = "gun_recoil", rval = "int", comment = "/**
         *  Get effective recoil considering handling, loaded ammo and effects of attached gunmods
         *  @param p player stats such as STR can alter effective recoil
         *  @param bipod whether any bipods should be considered
         *  @return effective recoil (per shot) or zero if gun uses ammo and none is loaded
         */", args = { "player" } },
            { name = "gun_recoil", rval = "int", comment = "/**
         *  Get effective recoil considering handling, loaded ammo and effects of attached gunmods
         *  @param p player stats such as STR can alter effective recoil
         *  @param bipod whether any bipods should be considered
         *  @return effective recoil (per shot) or zero if gun uses ammo and none is loaded
         */", args = { "player", "bool" } },
            { name = "gun_set_mode", rval = "bool", comment = "/** Try to set the mode for a gun, returning false if no such mode is possible */", args = { "std::string" } },
            { name = "gun_skill", rval = "skill_id", comment = "/**
         * The skill used to operate the gun. Can be "null" if this is not a gun.
         */", args = { } },
            { name = "gun_type", rval = "std::string", comment = "/** Get the type of a ranged weapon (eg. "rifle", "crossbow"), or empty string if non-gun */", args = { } },
            { name = "gunmod_compatible", rval = "bool", args = { "item" } },
            { name = "gunmod_find", rval = "item&", comment = "/** Get first attached gunmod matching type or nullptr if no such mod or item is not a gun */", args = { "std::string" } },
            { name = "has_amount", rval = "bool", comment = "/** Check instance provides at least qty of an item (@see amount_of) */", args = { "std::string", "int" } },
            { name = "has_amount", rval = "bool", comment = "/** Check instance provides at least qty of an item (@see amount_of) */", args = { "std::string", "int", "bool" } },
            { name = "has_any_flag", rval = "bool", args = { "std::vector<std::string>" } },
            { name = "has_flag", rval = "bool", args = { "std::string" } },
            { name = "has_infinite_charges", rval = "bool", args = { } },
            { name = "has_item", rval = "bool", comment = "/** Returns true if this visitable instance contains the item */", args = { "item" } },
            { name = "has_label", rval = "bool", comment = "/**
        * Returns true if item has "item_label" itemvar
        */", args = { } },
            { name = "has_property", rval = "bool", args = { "std::string" } },
            { name = "has_quality", rval = "bool", comment = "/** Returns true if instance has amount (or more) items of at least quality level */", args = { "quality_id" } },
            { name = "has_quality", rval = "bool", comment = "/** Returns true if instance has amount (or more) items of at least quality level */", args = { "quality_id", "int" } },
            { name = "has_quality", rval = "bool", comment = "/** Returns true if instance has amount (or more) items of at least quality level */", args = { "quality_id", "int", "int" } },
            { name = "has_rotten_away", rval = "bool", comment = "/** at twice regular shelf life perishable items rot away completely */", args = { } },
            { name = "has_technique", rval = "bool", comment = "/**
         * Whether the item supports a specific martial art technique (either through its type, or
         * through its individual @ref techniques).
         */", args = { "matec_id" } },
            { name = "has_var", rval = "bool", comment = "/** Whether the variable is defined at all. */", args = { "std::string" } },
            { name = "in_container", rval = "item", args = { "std::string" } },
            { name = "in_its_container", rval = "item", comment = "/**
     * Returns this item into its default container. If it does not have a default container,
     * returns this. It's intended to be used like \code newitem = newitem.in_its_container();\endcode
     */", args = { } },
            { name = "inc_damage", rval = "bool", comment = "/**
     * Increment item damage constrained @ref max_damage
     * @param dt type of damage which may be passed to @ref on_damage callback
     * @return whether item should be destroyed
     */", args = { } },
            { name = "info", rval = "std::string", comment = "/**
     * Return all the information about the item and its type.
     *
     * This includes the different
     * properties of the @ref itype (if they are visible to the player). The returned string
     * is already translated and can be *very* long.
     * @param showtext If true, shows the item description, otherwise only the properties item type.
     * the vector can be used to compare them to properties of another item.
     */", args = { "bool" } },
            { name = "info", rval = "std::string", comment = "/**
     * Return all the information about the item and its type.
     *
     * This includes the different
     * properties of the @ref itype (if they are visible to the player). The returned string
     * is already translated and can be *very* long.
     * @param showtext If true, shows the item description, otherwise only the properties item type.
     * the vector can be used to compare them to properties of another item.
     */", args = { } },
            { name = "is_ammo", rval = "bool", args = { } },
            { name = "is_ammo_belt", rval = "bool", args = { } },
            { name = "is_ammo_container", rval = "bool", args = { } },
            { name = "is_armor", rval = "bool", args = { } },
            { name = "is_artifact", rval = "bool", args = { } },
            { name = "is_bandolier", rval = "bool", args = { } },
            { name = "is_bionic", rval = "bool", args = { } },
            { name = "is_book", rval = "bool", args = { } },
            { name = "is_brewable", rval = "bool", args = { } },
            { name = "is_bucket", rval = "bool", args = { } },
            { name = "is_bucket_nonempty", rval = "bool", args = { } },
            { name = "is_comestible", rval = "bool", args = { } },
            { name = "is_container", rval = "bool", comment = "/** Whether this is container. Note that container does not necessarily means it's
     * suitable for liquids. */", args = { } },
            { name = "is_container_empty", rval = "bool", comment = "/** Whether this item has no contents at all. */", args = { } },
            { name = "is_container_full", rval = "bool", comment = "/**
     * Whether this item has no more free capacity for its current content.
     * @param allow_bucket Allow filling non-sealable containers
     */", args = { "bool" } },
            { name = "is_container_full", rval = "bool", comment = "/**
     * Whether this item has no more free capacity for its current content.
     * @param allow_bucket Allow filling non-sealable containers
     */", args = { } },
            { name = "is_corpse", rval = "bool", comment = "/**
         * Whether this is a corpse item. Corpses always have valid monster type (@ref corpse)
         * associated (@ref get_mtype return a non-null pointer) and have been created
         * with @ref make_corpse.
         */", args = { } },
            { name = "is_dangerous", rval = "bool", args = { } },
            { name = "is_emissive", rval = "bool", comment = "/**
         * Whether the item emits any light at all.
         */", args = { } },
            { name = "is_engine", rval = "bool", args = { } },
            { name = "is_faulty", rval = "bool", args = { } },
            { name = "is_filthy", rval = "bool", comment = "/** Marks the item as filthy, so characters with squeamish trait can't wear it.
    */", args = { } },
            { name = "is_firearm", rval = "bool", comment = "/**
         * Does it require gunsmithing tools to repair.
         */", args = { } },
            { name = "is_food", rval = "bool", args = { } },
            { name = "is_food_container", rval = "bool", args = { } },
            { name = "is_fresh", rval = "bool", comment = "/** an item is fresh if it is capable of rotting but still has a long shelf life remaining */", args = { } },
            { name = "is_fuel", rval = "bool", args = { } },
            { name = "is_funnel_container", rval = "bool", comment = "/**
     * Funnel related functions. See weather.cpp for their usage.
     */", args = { "units::volume" } },
            { name = "is_going_bad", rval = "bool", comment = "/** an item is about to become rotten when shelf life has nearly elapsed */", args = { } },
            { name = "is_gun", rval = "bool", comment = "/**
         *  Can this item be used to perform a ranged attack?
         *  @see item::is_melee()
         *  @note an item can be both a gun and melee weapon concurrently
         */", args = { } },
            { name = "is_gunmod", rval = "bool", args = { } },
            { name = "is_magazine", rval = "bool", args = { } },
            { name = "is_medication", rval = "bool", args = { } },
            { name = "is_melee", rval = "bool", comment = "/**
     *  Is this item an effective melee weapon for any damage type?
     *  @see item::is_gun()
     *  @note an item can be both a gun and melee weapon concurrently
     */", args = { } },
            { name = "is_non_resealable_container", rval = "bool", comment = "/** Whether removing this item's contents will permanently alter it. */", args = { } },
            { name = "is_null", rval = "bool", args = { } },
            { name = "is_power_armor", rval = "bool", comment = "/**
         * Whether this is a power armor item. Not necessarily the main armor, it could be a helmet
         * or similar.
         */", args = { } },
            { name = "is_reloadable", rval = "bool", comment = "/**
         * Is it ever possible to reload this item?
         * Only the base item is considered with any mods ignored
         * @see player::can_reload()
         */", args = { } },
            { name = "is_reloadable_with", rval = "bool", comment = "/** Returns true if this item can be reloaded with specified ammo type. */", args = { "std::string" } },
            { name = "is_salvageable", rval = "bool", args = { } },
            { name = "is_seed", rval = "bool", comment = "/**
         * Whether this is actually a seed, the seed functions won't be of much use for non-seeds.
         */", args = { } },
            { name = "is_sided", rval = "bool", comment = "/**
          * Returns true if item is armor and can be worn on different sides of the body
          */", args = { } },
            { name = "is_silent", rval = "bool", comment = "/** Whether this is a (nearly) silent gun (a tiny bit of sound is allowed). Non-guns are always silent. */", args = { } },
            { name = "is_soft", rval = "bool", comment = "/**
         * Is this item flexible enough to be worn on body parts like antlers?
         */", args = { } },
            { name = "is_tainted", rval = "bool", comment = "/** Is item derived from a zombie? */", args = { } },
            { name = "is_tool", rval = "bool", args = { } },
            { name = "is_tool_reversible", rval = "bool", args = { } },
            { name = "is_toolmod", rval = "bool", args = { } },
            { name = "is_two_handed", rval = "bool", comment = "/**
     * Whether the character needs both hands to wield this item.
     */", args = { "player" } },
            { name = "is_var_veh_part", rval = "bool", args = { } },
            { name = "is_watertight_container", rval = "bool", comment = "/** Whether this is a container which can be used to store liquids. */", args = { } },
            { name = "is_wheel", rval = "bool", args = { } },
            { name = "is_worn_only_with", rval = "bool", comment = "/**
         * Returns true whether this item can be worn only when @param it is worn.
         */", args = { "item" } },
            { name = "label", rval = "std::string", comment = "/**
        * Returns label from "item_label" itemvar and quantity
        */", args = { "int" } },
            { name = "label", rval = "std::string", comment = "/**
        * Returns label from "item_label" itemvar and quantity
        */", args = { } },
            { name = "lift_strength", rval = "int", comment = "/** Required strength to be able to successfully lift the item unaided by equipment */", args = { } },
            { name = "load_info", rval = nil, args = { "std::string" } },
            { name = "made_of", rval = "bool", comment = "/**
     * Are we solid, liquid, gas, plasma?
     */", args = { "phase_id" } },
            { name = "made_of", rval = "bool", comment = "/**
     * Check we are made of this material (e.g. matches at least one
     * in our set.)
     */", args = { "material_id" } },
            { name = "made_of", rval = "std::vector<material_id>", comment = "/**
     * The ids of all the materials this is made of.
     * This may return an empty vector.
     * The returned vector does not contain the null id.
     */", args = { } },
            { name = "made_of_any", rval = "bool", comment = "/**
     * Check we are made of at least one of a set (e.g. true if at least
     * one item of the passed in set matches any material).
     * @param mat_idents Set of material ids.
     */", args = { "std::set<material_id>" } },
            { name = "magazine_compatible", rval = "std::set<std::string>", comment = "/** Get compatible magazines (if any) for this item
         *  @param conversion whether to include the effect of any flags or mods which convert item's ammo type
         *  @return magazine compatibility which is always empty if item has integral magazine
         *  @see item::magazine_integral
         */", args = { "bool" } },
            { name = "magazine_compatible", rval = "std::set<std::string>", comment = "/** Get compatible magazines (if any) for this item
         *  @param conversion whether to include the effect of any flags or mods which convert item's ammo type
         *  @return magazine compatibility which is always empty if item has integral magazine
         *  @see item::magazine_integral
         */", args = { } },
            { name = "magazine_convert", rval = "std::vector<item>", comment = "/** Normalizes an item to use the new magazine system. Indempotent if item already converted.
         *  @return items that were created as a result of the conversion (excess ammo or magazines) */", args = { } },
            { name = "magazine_current", rval = "item&", comment = "/** Currently loaded magazine (if any)
         *  @return current magazine or nullptr if either no magazine loaded or item has integral magazine
         *  @see item::magazine_integral
         */", args = { } },
            { name = "magazine_default", rval = "std::string", comment = "/** Get the default magazine type (if any) for the current effective ammo type
         *  @param conversion whether to include the effect of any flags or mods which convert item's ammo type
         *  @return magazine type or "null" if item has integral magazine or no magazines for current ammo type */", args = { "bool" } },
            { name = "magazine_default", rval = "std::string", comment = "/** Get the default magazine type (if any) for the current effective ammo type
         *  @param conversion whether to include the effect of any flags or mods which convert item's ammo type
         *  @return magazine type or "null" if item has integral magazine or no magazines for current ammo type */", args = { } },
            { name = "magazine_integral", rval = "bool", comment = "/** Does item have an integral magazine (as opposed to allowing detachable magazines) */", args = { } },
            { name = "make_corpse", static = true, rval = "item", args = { "mtype_id" } },
            { name = "make_corpse", static = true, rval = "item", args = { "mtype_id", "int" } },
            { name = "make_corpse", static = true, rval = "item", args = { "mtype_id", "int", "std::string" } },
            { name = "make_corpse", static = true, rval = "item", args = { } },
            { name = "mark_as_used_by_player", rval = nil, comment = "/**
     * Marks the item as being used by this specific player, it remains unmarked
     * for other players. The player is identified by its id.
     */", args = { "player" } },
            { name = "mark_chapter_as_read", rval = nil, comment = "/**
         * Mark one chapter of the book as read by the given player. May do nothing if the book has
         * no unread chapters. This is a per-character setting, see @ref get_remaining_chapters.
         */", args = { "player" } },
            { name = "max_damage", rval = "int", comment = "/** Maximum amount of damage to an item (state before destroyed) */", args = { } },
            { name = "max_quality", rval = "int", comment = "/** Return maximum tool quality level provided by instance or INT_MIN if not found */", args = { "quality_id" } },
            { name = "melee_skill", rval = "skill_id", comment = "/**
     * The most relevant skill used with this melee weapon. Can be "null" if this is not a weapon.
     * Note this function returns null if the item is a gun for which you can use gun_skill() instead.
     */", args = { } },
            { name = "merge_charges", rval = "bool", comment = "/**
         * Merge charges of the other item into this item.
         * @return true if the items have been merged, otherwise false.
         * Merging is only done for items counted by charges (@ref count_by_charges) and
         * items that stack together (@ref stacks_with).
         */", args = { "item" } },
            { name = "min_damage", rval = "int", comment = "/** Minimum amount of damage to an item (state of maximum repair) */", args = { } },
            { name = "mod_charges", rval = nil, comment = "/**
     * Modify the charges of this item, only use for items counted by charges!
     * The item must have enough charges for this (>= quantity) and be counted
     * by charges.
     * @param mod How many charges should be removed.
     */", args = { "int" } },
            { name = "mod_damage", rval = "bool", comment = "/**
     * Apply damage to item constrained by @ref min_damage and @ref max_damage
     * @param qty maximum amount by which to adjust damage (negative permissible)
     * @param dt type of damage which may be passed to @ref on_damage callback
     * @return whether item should be destroyed
     */", args = { "float" } },
            { name = "needs_processing", rval = "bool", comment = "/**
     * Whether the item should be processed (by calling @ref process).
     */", args = { } },
            { name = "nname", static = true, rval = "std::string", comment = "/**
         * Returns the translated item name for the item with given id.
         * The name is in the proper plural form as specified by the
         * quantity parameter. This is roughly equivalent to creating an item instance and calling
         * @ref tname, however this function does not include strings like "(fresh)".
         */", args = { "std::string" } },
            { name = "nname", static = true, rval = "std::string", comment = "/**
         * Returns the translated item name for the item with given id.
         * The name is in the proper plural form as specified by the
         * quantity parameter. This is roughly equivalent to creating an item instance and calling
         * @ref tname, however this function does not include strings like "(fresh)".
         */", args = { "std::string", "int" } },
            { name = "on_contents_changed", rval = nil, comment = "/**
         * Callback when contents of the item are affected in any way other than just processing.
         */", args = { } },
            { name = "on_drop", rval = "bool", comment = "/**
     * Invokes item type's @ref itype::drop_action.
     * This function can change the item.
     * @param pos Where is the item being placed. Note: the item isn't there yet.
     * @return true if the item was destroyed during placement.
     */", args = { "tripoint" } },
            { name = "on_pickup", rval = nil, comment = "/**
         * Callback when a player starts carrying the item. The item is already in the inventory
         * and is called from there. This is not called when the item is added to the inventory
         * from worn vector or weapon slot. The item is considered already carried.
         */", args = { "Character" } },
            { name = "on_takeoff", rval = nil, comment = "/**
         * Callback when a character takes off an item. The item is still in the worn items
         * vector but will be removed immediately after the function returns
         */", args = { "Character" } },
            { name = "on_wear", rval = nil, comment = "/**
         * Callback when a character starts wearing the item. The item is already in the worn
         * items vector and is called from there.
         */", args = { "Character" } },
            { name = "on_wield", rval = nil, comment = "/**
         * Callback when a player starts wielding the item. The item is already in the weapon
         * slot and is called from there.
         * @param p player that has started wielding item
         * @param mv number of moves *already* spent wielding the weapon
         */", args = { "player" } },
            { name = "on_wield", rval = nil, comment = "/**
         * Callback when a player starts wielding the item. The item is already in the weapon
         * slot and is called from there.
         * @param p player that has started wielding item
         * @param mv number of moves *already* spent wielding the weapon
         */", args = { "player", "int" } },
            { name = "only_made_of", rval = "bool", comment = "/**
     * Check we are made of only the materials (e.g. false if we have
     * one material not in the set or no materials at all).
     * @param mat_idents Set of material ids.
     */", args = { "std::set<material_id>" } },
            { name = "price", rval = "int", comment = "/**
         * Returns the monetary value of an item.
         * If `practical` is false, returns pre-cataclysm market value,
         * otherwise returns approximate post-cataclysm value.
         */", args = { "bool" } },
            { name = "process", rval = "bool", comment = "/**
     * This is called once each turn. It's usually only useful for active items,
     * but can be called for inactive items without problems.
     * It is recursive, and calls process on any contained items.
     * @param carrier The player / npc that carries the item. This can be null when
     * the item is not carried by anyone (laying on ground)!
     * @param pos The location of the item on the map, same system as
     * @ref player::pos used. If the item is carried, it should be the
     * location of the carrier.
     * @param activate Whether the item should be activated (true), or
     * processed as an active item.
     * @return true if the item has been destroyed by the processing. The caller
     * should than delete the item wherever it was stored.
     * Returns false if the item is not destroyed.
     */", args = { "player", "tripoint", "bool" } },
            { name = "process_artifact", rval = nil, comment = "/**
     * Process and apply artifact effects. This should be called exactly once each turn, it may
     * modify character stats (like speed, strength, ...), so call it after those have been reset.
     * @param carrier The character carrying the artifact, can be null.
     * @param pos The location of the artifact (should be the player location if carried).
     */", args = { "player", "tripoint" } },
            { name = "processing_speed", rval = "int", comment = "/**
     * The rate at which an item should be processed, in number of turns between updates.
     */", args = { } },
            { name = "put_in", rval = nil, comment = "/**
     * Puts the given item into this one, no checks are performed.
     */", args = { "item" } },
            { name = "reach_range", rval = "int", comment = "/** Max range weapon usable for melee attack accounting for player/NPC abilities */", args = { "player" } },
            { name = "ready_to_revive", rval = "bool", comment = "/**
         * Whether this corpse should revive now. Note that this function includes some randomness,
         * the return value can differ on successive calls.
         * @param pos The location of the item (see REVIVE_SPECIAL flag).
         */", args = { "tripoint" } },
            { name = "remove_item", rval = "item", comment = "/** Removes and returns the item which must be contained by this instance */", args = { "item" } },
            { name = "repaired_with", rval = "std::set<std::string>", comment = "/** If possible to repair this item what tools could potentially be used for this purpose? */", args = { } },
            { name = "reset_cable", rval = nil, comment = "/**
     * Helper to bring a cable back to its initial state.
     */", args = { "player" } },
            { name = "rotten", rval = "bool", comment = "/** returns true if item is now rotten after all shelf life has elapsed */", args = { } },
            { name = "serialize", rval = "std::string", args = { } },
            { name = "set_countdown", rval = nil, comment = "/**
     * Sets time until activation for an item that will self-activate in the future.
     **/", args = { "int" } },
            { name = "set_damage", rval = "item&", comment = "/**
         * Filter setting damage constrained by @ref min_damage and @ref max_damage
         * @note this method does not invoke the @ref on_damage callback
         * @return same instance to allow method chaining
         */", args = { "float" } },
            { name = "set_flag", rval = "item&", comment = "/** Idempotent filter setting an item specific flag. */", args = { "std::string" } },
            { name = "set_mtype", rval = nil, comment = "/**
         * Sets the monster type associated with this item (@ref corpse). You must not pass a
         * null pointer.
         * TODO: change this to take a reference instead.
         */", args = { "mtype" } },
            { name = "set_relative_rot", rval = nil, comment = "/** Set current item @ref rot relative to shelf life (no-op if item does not spoil) */", args = { "float" } },
            { name = "set_snippet", rval = nil, comment = "/**
     * Set the snippet text (description) of this specific item, using the snippet library.
     * @see snippet_library.
     */", args = { "std::string" } },
            { name = "set_var", rval = nil, args = { "std::string", "float" } },
            { name = "set_var", rval = nil, args = { "std::string", "int" } },
            { name = "set_var", rval = nil, args = { "std::string", "int" } },
            { name = "set_var", rval = nil, args = { "std::string", "std::string" } },
            { name = "sight_dispersion", rval = "int", comment = "/** Get lowest dispersion of either integral or any attached sights */", args = { } },
            { name = "spill_contents", rval = "bool", comment = "/**
         * Unloads the item's contents.
         * @param c Character who receives the contents.
         *          If c is the player, liquids will be handled, otherwise they will be spilled.
         * @return If the item is now empty.
         */", args = { "Character" } },
            { name = "spill_contents", rval = "bool", comment = "/**
         * Unloads the item's contents.
         * @param pos Position to dump the contents on.
         * @return If the item is now empty.
         */", args = { "tripoint" } },
            { name = "split", rval = "item", comment = "/**
         * Splits a count-by-charges item always leaving source item with minimum of 1 charge
         * @param qty number of required charges to split from source
         * @return new instance containing exactly qty charges or null item if splitting failed
         */", args = { "int" } },
            { name = "spoilage_sort_order", rval = "int", comment = "/**
     * Get time left to rot, ignoring fridge.
     * Returns time to rot if item is able to, max int - N otherwise,
     * where N is
     * 3 for food,
     * 2 for medication,
     * 1 for other comestibles,
     * 0 otherwise.
     */", args = { } },
            { name = "stab_resist", rval = "int", args = { "bool" } },
            { name = "stab_resist", rval = "int", args = { } },
            { name = "stacks_with", rval = "bool", args = { "item" } },
            { name = "swap_side", rval = "bool", comment = "/**
         * Swap the side on which the item is worn. Returns false if the item is not sided
         */", args = { } },
            { name = "symbol", rval = "std::string", args = { } },
            { name = "tname", rval = "std::string", comment = "/**
     * Return the (translated) item name.
     * @param quantity used for translation to the proper plural form of the name, e.g.
     * returns "rock" for quantity 1 and "rocks" for quantity > 0.
     * @param with_prefix determines whether to include more item properties, such as
     * the extent of damage and burning (was created to sort by name without prefix
     * in additional inventory)
     */", args = { "int" } },
            { name = "tname", rval = "std::string", comment = "/**
     * Return the (translated) item name.
     * @param quantity used for translation to the proper plural form of the name, e.g.
     * returns "rock" for quantity 1 and "rocks" for quantity > 0.
     * @param with_prefix determines whether to include more item properties, such as
     * the extent of damage and burning (was created to sort by name without prefix
     * in additional inventory)
     */", args = { "int", "bool" } },
            { name = "tname", rval = "std::string", comment = "/**
     * Return the (translated) item name.
     * @param quantity used for translation to the proper plural form of the name, e.g.
     * returns "rock" for quantity 1 and "rocks" for quantity > 0.
     * @param with_prefix determines whether to include more item properties, such as
     * the extent of damage and burning (was created to sort by name without prefix
     * in additional inventory)
     */", args = { } },
            { name = "typeId", rval = "std::string", comment = "/** return the unique identifier of the items underlying type */", args = { } },
            { name = "type_is_defined", static = true, rval = "bool", comment = "/**
         * Check whether the type id refers to a known type.
         * This should be used either before instantiating an item when it's possible
         * that the item type is unknown and the caller can do something about it (e.g. the
         * uninstall-bionics function checks this to see if there is a CBM item type and has
         * logic to handle the case when that item type does not exist).
         * Or one can use this to check that type ids from json refer to valid items types (e.g.
         * the items that make up the vehicle parts must be defined somewhere, or the result of
         * crafting recipes must be valid type ids).
         */", args = { "std::string" } },
            { name = "type_name", rval = "std::string", comment = "/**
         * Name of the item type (not the item), with proper plural.
         * This is only special when the item itself has a special name ("name" entry in
         * @ref item_tags) or is a named corpse.
         * It's effectively the same as calling @ref nname with the item type id. Use this when
         * the actual item is not meant, for example "The shovel" instead of "Your shovel".
         * Or "The jacket is too small", when it applies to all jackets, not just the one the
         * character tried to wear).
         */", args = { "int" } },
            { name = "type_name", rval = "std::string", comment = "/**
         * Name of the item type (not the item), with proper plural.
         * This is only special when the item itself has a special name ("name" entry in
         * @ref item_tags) or is a named corpse.
         * It's effectively the same as calling @ref nname with the item type id. Use this when
         * the actual item is not meant, for example "The shovel" instead of "Your shovel".
         * Or "The jacket is too small", when it applies to all jackets, not just the one the
         * character tried to wear).
         */", args = { } },
            { name = "units_remaining", rval = "int", comment = "/**
         * How many units (ammo or charges) are remaining?
         * @param ch character responsible for invoking the item
         * @param limit stop searching after this many units found
         * @note also checks availability of UPS charges if applicable
         */", args = { "Character" } },
            { name = "units_remaining", rval = "int", comment = "/**
         * How many units (ammo or charges) are remaining?
         * @param ch character responsible for invoking the item
         * @param limit stop searching after this many units found
         * @note also checks availability of UPS charges if applicable
         */", args = { "Character", "int" } },
            { name = "units_sufficient", rval = "bool", comment = "/**
         * Check if item has sufficient units (ammo or charges) remaining
         * @param ch Character to check (used if ammo is UPS charges)
         * @param qty units required, if unspecified use item default
         */", args = { "Character" } },
            { name = "units_sufficient", rval = "bool", comment = "/**
         * Check if item has sufficient units (ammo or charges) remaining
         * @param ch Character to check (used if ammo is UPS charges)
         * @param qty units required, if unspecified use item default
         */", args = { "Character", "int" } },
            { name = "unset_flag", rval = "item&", comment = "/** Idempotent filter removing an item specific flag */", args = { "std::string" } },
            { name = "unset_flags", rval = nil, comment = "/** Removes all item specific flags. */", args = { } },
            { name = "volume", rval = "units::volume", args = { "bool" } },
            { name = "volume", rval = "units::volume", args = { } },
            { name = "weight", rval = "int", args = { "bool" } },
            { name = "weight", rval = "int", args = { } },
            { name = "wheel_area", rval = "int", comment = "/** Returns the total area of this wheel or 0 if it isn't one. */", args = { } },
            { name = "will_explode_in_fire", rval = "bool", args = { } },
        }
    },
    point = {
        new = {
            { "int", "int" },
            { "point" },
            { },
        },
        by_value = true,
        attributes = {
            x = { type = "int", writable = true },
            y = { type = "int", writable = true },
        },
        functions = {
            { name = "deserialize", rval = nil, args = { "std::string" } },
            { name = "serialize", rval = "std::string", args = { } },
        }
    },
    tripoint = {
        new = {
            { "int", "int", "int" },
            { "point", "int" },
            { "tripoint" },
            { },
        },
        by_value = true,
        attributes = {
            x = { type = "int", writable = true },
            y = { type = "int", writable = true },
            z = { type = "int", writable = true },
        },
        functions = {
            { name = "deserialize", rval = nil, args = { "std::string" } },
            { name = "serialize", rval = "std::string", args = { } },
        }
    },
    uimenu = {
        attributes = {
            border = { type = "bool", writable = true },
            border_color = { type = "int", writable = true },
            centered_scroll = { type = "bool", writable = true },
            desc_enabled = { type = "bool", writable = true },
            desc_lines = { type = "int", writable = true },
            disabled_color = { type = "int", writable = true },
            filter = { type = "std::string", writable = true },
            filtering = { type = "bool", writable = true },
            filtering_nocase = { type = "bool", writable = true },
            fselected = { type = "int", writable = true },
            hilight_color = { type = "int", writable = true },
            hilight_disabled = { type = "bool", writable = true },
            hilight_full = { type = "bool", writable = true },
            hotkey_color = { type = "int", writable = true },
            keypress = { type = "int", writable = true },
            max_desc_len = { type = "int", writable = true },
            max_entry_len = { type = "int", writable = true },
            pad_left = { type = "int", writable = true },
            pad_right = { type = "int", writable = true },
            ret = { type = "int", writable = true },
            return_invalid = { type = "bool", writable = true },
            scrollbar_auto = { type = "bool", writable = true },
            scrollbar_nopage_color = { type = "int", writable = true },
            scrollbar_page_color = { type = "int", writable = true },
            scrollbar_side = { type = "int", writable = true },
            selected = { type = "int", writable = true },
            shift_retval = { type = "int", writable = true },
            text = { type = "std::string", writable = true },
            text_color = { type = "int", writable = true },
            textalign = { type = "int", writable = true },
            textformatted = { type = "std::vector<std::string>", writable = true },
            textwidth = { type = "int", writable = true },
            title = { type = "std::string", writable = true },
            title_color = { type = "int", writable = true },
            vmax = { type = "int", writable = true },
            vshift = { type = "int", writable = true },
            w_height = { type = "int", writable = true },
            w_width = { type = "int", writable = true },
            w_x = { type = "int", writable = true },
            w_y = { type = "int", writable = true },
        },
        functions = {
            { name = "addentry", rval = nil, args = { "int", "bool", "int", "std::string" } },
            { name = "addentry", rval = nil, args = { "std::string" } },
            { name = "addentry_desc", rval = nil, args = { "int", "bool", "int", "std::string", "std::string" } },
            { name = "addentry_desc", rval = nil, args = { "std::string", "std::string" } },
            { name = "apply_scrollbar", rval = nil, args = { } },
            { name = "filterlist", rval = nil, args = { } },
            { name = "init", rval = nil, args = { } },
            { name = "inputfilter", rval = "std::string", args = { } },
            { name = "query", rval = nil, args = { "bool" } },
            { name = "query", rval = nil, args = { } },
            { name = "redraw", rval = nil, args = { "bool" } },
            { name = "redraw", rval = nil, args = { } },
            { name = "refresh", rval = nil, args = { "bool" } },
            { name = "refresh", rval = nil, args = { } },
            { name = "reset", rval = nil, args = { } },
            { name = "scroll_amount_from_action", rval = "int", args = { "std::string" } },
            { name = "scroll_amount_from_key", rval = "int", args = { "int" } },
            { name = "scrollby", rval = "bool", args = { "int" } },
            { name = "settext", rval = nil, args = { "std::string" } },
            { name = "setup", rval = nil, args = { } },
            { name = "show", rval = nil, args = { } },
        }
    },
    field_entry = {
        functions = {
            { name = "decays_on_actualize", rval = "bool", args = { } },
            { name = "getFieldAge", rval = "int", args = { } },
            { name = "getFieldDensity", rval = "int", args = { } },
            { name = "getFieldType", rval = "field_id", args = { } },
            { name = "isAlive", rval = "bool", args = { } },
            { name = "is_dangerous", rval = "bool", args = { } },
            { name = "mod_age", rval = "int", comment = "/** Adds a number to current age. */", args = { "int" } },
            { name = "move_cost", rval = "int", args = { } },
            { name = "name", rval = "std::string", args = { } },
            { name = "setFieldAge", rval = "int", args = { "int" } },
            { name = "setFieldDensity", rval = "int", args = { "int" } },
            { name = "setFieldType", rval = "field_id", args = { "field_id" } },
        }
    },
    field = {
        functions = {
            { name = "addField", rval = "bool", comment = "/**
     * Inserts the given field_id into the field list for a given tile if it does not already exist.
     * If you wish to modify an already existing field use findField and modify the result.
     * Density defaults to 1, and age to 0 (permanent) if not specified.
     * The density is added to an existing field entry, but the age is only used for newly added entries.
     * @return false if the field_id already exists, true otherwise.
     */", args = { "field_id" } },
            { name = "addField", rval = "bool", comment = "/**
     * Inserts the given field_id into the field list for a given tile if it does not already exist.
     * If you wish to modify an already existing field use findField and modify the result.
     * Density defaults to 1, and age to 0 (permanent) if not specified.
     * The density is added to an existing field entry, but the age is only used for newly added entries.
     * @return false if the field_id already exists, true otherwise.
     */", args = { "field_id", "int" } },
            { name = "addField", rval = "bool", comment = "/**
     * Inserts the given field_id into the field list for a given tile if it does not already exist.
     * If you wish to modify an already existing field use findField and modify the result.
     * Density defaults to 1, and age to 0 (permanent) if not specified.
     * The density is added to an existing field entry, but the age is only used for newly added entries.
     * @return false if the field_id already exists, true otherwise.
     */", args = { "field_id", "int", "int" } },
            { name = "fieldCount", rval = "int", args = { } },
            { name = "fieldSymbol", rval = "field_id", comment = "/**
     * Returns the id of the field that should be drawn.
     */", args = { } },
            { name = "findField", rval = "field_entry&", comment = "/**
     * Returns a field entry corresponding to the field_id parameter passed in.
     * If no fields are found then nullptr is returned.
     */", args = { "field_id" } },
            { name = "findFieldc", rval = "field_entry&", comment = "/**
     * Returns a field entry corresponding to the field_id parameter passed in.
     * If no fields are found then nullptr is returned.
     */", args = { "field_id" } },
            { name = "move_cost", rval = "int", comment = "/**
     * Returns the total move cost from all fields.
     */", args = { } },
            { name = "removeField", rval = "bool", comment = "/**
     * Removes the field entry with a type equal to the field_id parameter.
     * Make sure to decrement the field counter in the submap if (and only if) the
     * function returns true.
     * @return True if the field was removed, false if it did not exist in the first place.
     */", args = { "field_id" } },
        }
    },
    map = {
        functions = {
            { name = "accessible_furniture", rval = "bool", comment = "/**
     * Like @ref accessible_items but checks for accessible furniture.
     * It ignores the furniture flags of the target square (ignores if target is SEALED).
     */", args = { "tripoint", "tripoint", "int" } },
            { name = "accessible_items", rval = "bool", comment = "/**
     * Check whether items in the target square are accessible from the source square
     * `f` and `t`.
     *
     * Checks two things:
     * 1. The `sees()` algorithm between `f` and `t` OR origin and target match.
     * 2. That the target location isn't sealed.
     */", args = { "tripoint", "tripoint", "int" } },
            { name = "add_camp", rval = nil, args = { "tripoint", "std::string" } },
            { name = "add_corpse", rval = nil, args = { "tripoint" } },
            { name = "add_field", rval = "bool", comment = "/**
         * Add field entry at point, or set density if present
         * @return false if the field could not be created (out of bounds), otherwise true.
         */", args = { "tripoint", "field_id", "int" } },
            { name = "add_field", rval = "bool", comment = "/**
         * Add field entry at point, or set density if present
         * @return false if the field could not be created (out of bounds), otherwise true.
         */", args = { "tripoint", "field_id", "int", "int" } },
            { name = "add_item", rval = "item&", comment = "/**
     * Place an item on the map, despite the parameter name, this is not necessaraly a new item.
     * WARNING: does -not- check volume or stack charges. player functions (drop etc) should use
     * map::add_item_or_charges
     *
     * @returns The item that got added, or nulitem.
     */", args = { "tripoint", "item" } },
            { name = "add_item", rval = nil, args = { "int", "int", "item" } },
            { name = "add_item_at", rval = "item&", comment = "/** Helper for map::add_item */", args = { "tripoint", "std::list<item>::iterator", "item" } },
            { name = "add_item_or_charges", rval = "item&", args = { "int", "int", "item" } },
            { name = "add_item_or_charges", rval = "item&", args = { "int", "int", "item", "bool" } },
            { name = "add_item_or_charges", rval = "item&", comment = "/**
     *  Adds an item to map tile or stacks charges
     *  @param pos Where to add item
     *  @param obj Item to add
     *  @param overflow if destination is full attempt to drop on adjacent tiles
     *  @return reference to dropped (and possibly stacked) item or null item on failure
     *  @warning function is relatively expensive and meant for user initiated actions, not mapgen
     */", args = { "tripoint", "item" } },
            { name = "add_item_or_charges", rval = "item&", comment = "/**
     *  Adds an item to map tile or stacks charges
     *  @param pos Where to add item
     *  @param obj Item to add
     *  @param overflow if destination is full attempt to drop on adjacent tiles
     *  @return reference to dropped (and possibly stacked) item or null item on failure
     *  @warning function is relatively expensive and meant for user initiated actions, not mapgen
     */", args = { "tripoint", "item", "bool" } },
            { name = "add_spawn", rval = nil, args = { "mtype_id", "int", "int", "int" } },
            { name = "add_spawn", rval = nil, args = { "mtype_id", "int", "int", "int", "bool" } },
            { name = "add_spawn", rval = nil, args = { "mtype_id", "int", "int", "int", "bool", "int" } },
            { name = "add_spawn", rval = nil, args = { "mtype_id", "int", "int", "int", "bool", "int", "int" } },
            { name = "add_spawn", rval = nil, args = { "mtype_id", "int", "int", "int", "bool", "int", "int", "std::string" } },
            { name = "add_splash", rval = nil, args = { "field_id", "tripoint", "int", "int" } },
            { name = "add_splatter", rval = nil, args = { "field_id", "tripoint" } },
            { name = "add_splatter", rval = nil, args = { "field_id", "tripoint", "int" } },
            { name = "add_splatter_trail", rval = nil, args = { "field_id", "tripoint", "tripoint" } },
            { name = "add_trap", rval = nil, args = { "tripoint", "trap_id" } },
            { name = "adjust_field_age", rval = "int", comment = "/**
         * Increment/decrement age of field entry at point.
         * @return resulting age or -1 if not present (does *not* create a new field).
         */", args = { "tripoint", "field_id", "int" } },
            { name = "adjust_field_strength", rval = "int", comment = "/**
         * Increment/decrement density of field entry at point, creating if not present,
         * removing if density becomes 0.
         * @return resulting density, or 0 for not present (either removed or not created at all).
         */", args = { "tripoint", "field_id", "int" } },
            { name = "adjust_radiation", rval = nil, args = { "int", "int", "int" } },
            { name = "adjust_radiation", rval = nil, comment = "/** Increment the radiation in the given tile by the given delta
    *  (decrement it if delta is negative)
    */", args = { "tripoint", "int" } },
            { name = "allow_camp", rval = "bool", args = { "tripoint" } },
            { name = "allow_camp", rval = "bool", args = { "tripoint", "int" } },
            { name = "ambient_light_at", rval = "float", args = { "tripoint" } },
            { name = "bash_rating", rval = "int", args = { "int", "int", "int" } },
            { name = "bash_rating", rval = "int", comment = "/** Returns a success rating from -1 to 10 for a given tile based on a set strength, used for AI movement planning
    *  Values roughly correspond to 10% increment chances of success on a given bash, rounded down. -1 means the square is not bashable */", args = { "int", "tripoint" } },
            { name = "bash_rating", rval = "int", comment = "/** Returns a success rating from -1 to 10 for a given tile based on a set strength, used for AI movement planning
    *  Values roughly correspond to 10% increment chances of success on a given bash, rounded down. -1 means the square is not bashable */", args = { "int", "tripoint", "bool" } },
            { name = "bash_resistance", rval = "int", args = { "int", "int" } },
            { name = "bash_resistance", rval = "int", comment = "/** Returns min_str of the furniture or terrain at p */", args = { "tripoint" } },
            { name = "bash_resistance", rval = "int", comment = "/** Returns min_str of the furniture or terrain at p */", args = { "tripoint", "bool" } },
            { name = "bash_strength", rval = "int", args = { "int", "int" } },
            { name = "bash_strength", rval = "int", comment = "/** Returns max_str of the furniture or terrain at p */", args = { "tripoint" } },
            { name = "bash_strength", rval = "int", comment = "/** Returns max_str of the furniture or terrain at p */", args = { "tripoint", "bool" } },
            { name = "board_vehicle", rval = nil, args = { "tripoint", "player" } },
            { name = "build_floor_cache", rval = nil, args = { "int" } },
            { name = "build_floor_caches", rval = nil, args = { } },
            { name = "build_map_cache", rval = nil, args = { "int" } },
            { name = "build_map_cache", rval = nil, args = { "int", "bool" } },
            { name = "build_outside_cache", rval = nil, args = { "int" } },
            { name = "can_move_furniture", rval = "bool", args = { "tripoint" } },
            { name = "can_move_furniture", rval = "bool", args = { "tripoint", "player" } },
            { name = "can_put_items", rval = "bool", args = { "tripoint" } },
            { name = "can_put_items_ter_furn", rval = "bool", args = { "int", "int" } },
            { name = "can_put_items_ter_furn", rval = "bool", args = { "tripoint" } },
            { name = "clear_path", rval = "bool", comment = "/**
     * Check whether there's a direct line of sight between `F` and
     * `T` with the additional movecost restraints.
     *
     * Checks two things:
     * 1. The `sees()` algorithm between `F` and `T`
     * 2. That moving over the line of sight would have a move_cost between
     *    `cost_min` and `cost_max`.
     */", args = { "tripoint", "tripoint", "int", "int", "int" } },
            { name = "clear_spawns", rval = nil, args = { } },
            { name = "clear_traps", rval = nil, args = { } },
            { name = "clear_vehicle_cache", rval = nil, args = { "int" } },
            { name = "clear_vehicle_list", rval = nil, args = { "int" } },
            { name = "climb_difficulty", rval = "int", comment = "/**
     * Checks 3x3 block centered on p for terrain to climb.
     * @return Difficulty of climbing check from point p.
     */", args = { "tripoint" } },
            { name = "clip_to_bounds", rval = nil, comment = "/** Clips the coords of p to fit the map bounds */", args = { "tripoint" } },
            { name = "close_door", rval = "bool", args = { "tripoint", "bool", "bool" } },
            { name = "collapse_at", rval = nil, comment = "/** Causes a collapse at p, such as from destroying a wall */", args = { "tripoint", "bool" } },
            { name = "collapse_check", rval = "int", comment = "/** Checks if a square should collapse, returns the X for the one_in(X) collapse chance */", args = { "tripoint" } },
            { name = "combined_movecost", rval = "int", comment = "/**
    * Cost to move out of one tile and into the next.
    *
    * @return The cost in turns to move out of tripoint `from` and into `to`
    */", args = { "tripoint", "tripoint" } },
            { name = "coord_to_angle", rval = "int", args = { "int", "int", "int", "int" } },
            { name = "could_see_items", rval = "bool", comment = "/**
     * Check if the creature could see items at p if there were
     * any items. This is similar to @ref sees_some_items, but it
     * does not check that there are actually any items.
     */", args = { "tripoint", "Creature" } },
            { name = "creature_in_field", rval = nil, comment = "/**
         * Apply field effects to the creature when it's on a square with fields.
         */", args = { "Creature" } },
            { name = "creature_on_trap", rval = nil, comment = "/**
         * Apply trap effects to the creature, similar to @ref creature_in_field.
         * If there is no trap at the creatures location, nothing is done.
         * If the creature can avoid the trap, nothing is done as well.
         * Otherwise the trap is triggered.
         * @param critter Creature that just got trapped
         * @param may_avoid If true, the creature tries to avoid the trap
         * (@ref Creature::avoid_trap). If false, the trap is always triggered.
         */", args = { "Creature" } },
            { name = "creature_on_trap", rval = nil, comment = "/**
         * Apply trap effects to the creature, similar to @ref creature_in_field.
         * If there is no trap at the creatures location, nothing is done.
         * If the creature can avoid the trap, nothing is done as well.
         * Otherwise the trap is triggered.
         * @param critter Creature that just got trapped
         * @param may_avoid If true, the creature tries to avoid the trap
         * (@ref Creature::avoid_trap). If false, the trap is always triggered.
         */", args = { "Creature", "bool" } },
            { name = "crush", rval = nil, args = { "tripoint" } },
            { name = "debug", rval = nil, args = { } },
            { name = "decay_fields_and_scent", rval = nil, comment = "/**
    * Moved here from weather.cpp for speed. Decays fire, washable fields and scent.
    * Washable fields are decayed only by 1/3 of the amount fire is.
    */", args = { "int" } },
            { name = "delete_graffiti", rval = nil, args = { "tripoint" } },
            { name = "delete_signage", rval = nil, args = { "tripoint" } },
            { name = "destroy", rval = nil, comment = "/** Keeps bashing a square until it can't be bashed anymore */", args = { "tripoint" } },
            { name = "destroy", rval = nil, comment = "/** Keeps bashing a square until it can't be bashed anymore */", args = { "tripoint", "bool" } },
            { name = "destroy_furn", rval = nil, comment = "/** Keeps bashing a square until there is no more furniture */", args = { "tripoint" } },
            { name = "destroy_furn", rval = nil, comment = "/** Keeps bashing a square until there is no more furniture */", args = { "tripoint", "bool" } },
            { name = "disarm_trap", rval = nil, args = { "tripoint" } },
            { name = "disp_name", rval = "std::string", args = { "tripoint" } },
            { name = "displace_water", rval = "bool", args = { "tripoint" } },
            { name = "draw_circle_furn", rval = nil, args = { "furn_id", "int", "int", "int" } },
            { name = "draw_circle_ter", rval = nil, args = { "ter_id", "float", "float", "float" } },
            { name = "draw_circle_ter", rval = nil, args = { "ter_id", "int", "int", "int" } },
            { name = "draw_fill_background", rval = nil, args = { "ter_id" } },
            { name = "draw_line_furn", rval = nil, args = { "furn_id", "int", "int", "int", "int" } },
            { name = "draw_line_ter", rval = nil, args = { "ter_id", "int", "int", "int", "int" } },
            { name = "draw_rough_circle_furn", rval = nil, args = { "furn_id", "int", "int", "int" } },
            { name = "draw_rough_circle_ter", rval = nil, args = { "ter_id", "int", "int", "int" } },
            { name = "draw_square_furn", rval = nil, args = { "furn_id", "int", "int", "int", "int" } },
            { name = "draw_square_ter", rval = nil, args = { "ter_id", "int", "int", "int", "int" } },
            { name = "drop_everything", rval = nil, args = { "tripoint" } },
            { name = "drop_fields", rval = nil, args = { "tripoint" } },
            { name = "drop_furniture", rval = nil, args = { "tripoint" } },
            { name = "drop_items", rval = nil, args = { "tripoint" } },
            { name = "drop_vehicle", rval = nil, args = { "tripoint" } },
            { name = "examine", rval = nil, comment = "/**
     * Calls the examine function of furniture or terrain at given tile, for given character.
     * Will only examine terrain if furniture had @ref iexamine::none as the examine function.
     */", args = { "Character", "tripoint" } },
            { name = "features", rval = "std::string", args = { "int", "int" } },
            { name = "features", rval = "std::string", args = { "tripoint" } },
            { name = "field_at", rval = "field&", comment = "/**
         * Gets fields that are here. Both for querying and edition.
         */", args = { "tripoint" } },
            { name = "find_clear_path", rval = "std::vector<tripoint>", comment = "/**
     * Iteratively tries bresenham lines with different biases
     * until it finds a clear line or decides there isn't one.
     * returns the line found, which may be the staright line, but blocked.
     */", args = { "tripoint", "tripoint" } },
            { name = "flammable_items_at", rval = "bool", comment = "/**
     * Checks if there are any flammable items on the tile.
     * @param p tile to check
     * @param threshold Fuel threshold (lower means worse fuels are accepted).
     */", args = { "tripoint" } },
            { name = "flammable_items_at", rval = "bool", comment = "/**
     * Checks if there are any flammable items on the tile.
     * @param p tile to check
     * @param threshold Fuel threshold (lower means worse fuels are accepted).
     */", args = { "tripoint", "int" } },
            { name = "free_volume", rval = "units::volume", args = { "tripoint" } },
            { name = "furn", rval = "furn_id", args = { "int", "int" } },
            { name = "furn", rval = "furn_id", args = { "tripoint" } },
            { name = "furn_set", rval = nil, args = { "int", "int", "furn_id" } },
            { name = "furn_set", rval = nil, args = { "tripoint", "furn_id" } },
            { name = "furnname", rval = "std::string", args = { "int", "int" } },
            { name = "furnname", rval = "std::string", args = { "tripoint" } },
            { name = "generate", rval = nil, args = { "int", "int", "int", "int" } },
            { name = "get_abs_sub", rval = "tripoint", comment = "/** return @ref abs_sub */", args = { } },
            { name = "get_dir_circle", rval = "std::vector<tripoint>", comment = "/**
  * Calculate next search points surrounding the current position.
  * Points closer to the target come first.
  * This method leads to straighter lines and prevents weird looking movements away from the target.
  */", args = { "tripoint", "tripoint" } },
            { name = "get_field", rval = "field_entry&", comment = "/**
         * Get field of specific type at point.
         * @return NULL if there is no such field entry at that place.
         */", args = { "tripoint", "field_id" } },
            { name = "get_field_age", rval = "int", comment = "/**
         * Get the age of a field entry (@ref field_entry::age), if there is no
         * field of that type, returns -1.
         */", args = { "tripoint", "field_id" } },
            { name = "get_field_strength", rval = "int", comment = "/**
         * Get the density of a field entry (@ref field_entry::density),
         * if there is no field of that type, returns 0.
         */", args = { "tripoint", "field_id" } },
            { name = "get_harvest_names", rval = "std::set<std::string>", comment = "/**
     * Returns names of the items that would be dropped.
     */", args = { "tripoint" } },
            { name = "get_radiation", rval = "int", args = { "tripoint" } },
            { name = "get_signage", rval = "std::string", args = { "tripoint" } },
            { name = "get_ter_transforms_into", rval = "ter_id", args = { "tripoint" } },
            { name = "getabs", rval = "point", args = { "point" } },
            { name = "getabs", rval = "point", comment = "/**
     * Translates local (to this map) coordinates of a square to
     * global absolute coordinates. (x,y) is in the system that
     * is used by the ter/furn/i_at functions.
     * Output is in the same scale, but in global system.
     */", args = { "int", "int" } },
            { name = "getabs", rval = "tripoint", comment = "/**
     * Translates tripoint in local coords (near player) to global,
     * just as the 2D variant of the function.
     * z-coord remains unchanged (it is always global).
     */", args = { "tripoint" } },
            { name = "getlocal", rval = "point", args = { "point" } },
            { name = "getlocal", rval = "point", comment = "/**
     * Inverse of @ref getabs
     */", args = { "int", "int" } },
            { name = "getlocal", rval = "tripoint", args = { "tripoint" } },
            { name = "getmapsize", rval = "int", args = { } },
            { name = "graffiti_at", rval = "std::string", args = { "tripoint" } },
            { name = "has_adjacent_furniture", rval = "bool", args = { "tripoint" } },
            { name = "has_flag", rval = "bool", args = { "std::string", "int", "int" } },
            { name = "has_flag", rval = "bool", args = { "std::string", "tripoint" } },
            { name = "has_flag_furn", rval = "bool", args = { "std::string", "int", "int" } },
            { name = "has_flag_furn", rval = "bool", args = { "std::string", "tripoint" } },
            { name = "has_flag_ter", rval = "bool", args = { "std::string", "int", "int" } },
            { name = "has_flag_ter", rval = "bool", args = { "std::string", "tripoint" } },
            { name = "has_flag_ter_or_furn", rval = "bool", args = { "std::string", "int", "int" } },
            { name = "has_flag_ter_or_furn", rval = "bool", args = { "std::string", "tripoint" } },
            { name = "has_floor", rval = "bool", args = { "tripoint" } },
            { name = "has_floor_or_support", rval = "bool", args = { "tripoint" } },
            { name = "has_furn", rval = "bool", args = { "int", "int" } },
            { name = "has_furn", rval = "bool", args = { "tripoint" } },
            { name = "has_graffiti_at", rval = "bool", args = { "tripoint" } },
            { name = "has_items", rval = "bool", comment = "/**
     * Checks for existence of items. Faster than i_at(p).empty
     */", args = { "tripoint" } },
            { name = "has_nearby_fire", rval = "bool", args = { "tripoint" } },
            { name = "has_nearby_fire", rval = "bool", args = { "tripoint", "int" } },
            { name = "has_zlevels", rval = "bool", args = { } },
            { name = "hit_with_acid", rval = "bool", args = { "tripoint" } },
            { name = "hit_with_fire", rval = "bool", args = { "tripoint" } },
            { name = "i_at", rval = "map_stack", args = { "int", "int" } },
            { name = "i_at", rval = "map_stack", args = { "tripoint" } },
            { name = "i_clear", rval = nil, args = { "int", "int" } },
            { name = "i_clear", rval = nil, args = { "tripoint" } },
            { name = "i_rem", rval = "int", args = { "int", "int", "int" } },
            { name = "i_rem", rval = "int", args = { "tripoint", "int" } },
            { name = "i_rem", rval = "std::list<item>::iterator", args = { "point", "std::list<item>::iterator" } },
            { name = "i_rem", rval = "std::list<item>::iterator", args = { "tripoint", "std::list<item>::iterator" } },
            { name = "i_rem", rval = nil, args = { "int", "int", "item" } },
            { name = "i_rem", rval = nil, args = { "tripoint", "item" } },
            { name = "impassable", rval = "bool", args = { "int", "int" } },
            { name = "impassable", rval = "bool", args = { "tripoint" } },
            { name = "impassable_ter_furn", rval = "bool", args = { "tripoint" } },
            { name = "inbounds", rval = "bool", args = { "int", "int" } },
            { name = "inbounds", rval = "bool", args = { "int", "int", "int" } },
            { name = "inbounds", rval = "bool", args = { "tripoint" } },
            { name = "inbounds_z", rval = "bool", args = { "int" } },
            { name = "is_bashable", rval = "bool", args = { "int", "int" } },
            { name = "is_bashable", rval = "bool", comment = "/** Returns true if there is a bashable vehicle part or the furn/terrain is bashable at p */", args = { "tripoint" } },
            { name = "is_bashable", rval = "bool", comment = "/** Returns true if there is a bashable vehicle part or the furn/terrain is bashable at p */", args = { "tripoint", "bool" } },
            { name = "is_bashable_furn", rval = "bool", args = { "int", "int" } },
            { name = "is_bashable_furn", rval = "bool", comment = "/** Returns true if the furniture at p is bashable */", args = { "tripoint" } },
            { name = "is_bashable_ter", rval = "bool", args = { "int", "int" } },
            { name = "is_bashable_ter", rval = "bool", comment = "/** Returns true if the terrain at p is bashable */", args = { "tripoint" } },
            { name = "is_bashable_ter", rval = "bool", comment = "/** Returns true if the terrain at p is bashable */", args = { "tripoint", "bool" } },
            { name = "is_bashable_ter_furn", rval = "bool", args = { "int", "int" } },
            { name = "is_bashable_ter_furn", rval = "bool", comment = "/** Returns true if the furniture or terrain at p is bashable */", args = { "tripoint" } },
            { name = "is_bashable_ter_furn", rval = "bool", comment = "/** Returns true if the furniture or terrain at p is bashable */", args = { "tripoint", "bool" } },
            { name = "is_divable", rval = "bool", args = { "int", "int" } },
            { name = "is_divable", rval = "bool", args = { "tripoint" } },
            { name = "is_harvestable", rval = "bool", comment = "/**
     * Returns true if point at pos is harvestable right now, with no extra tools.
     */", args = { "tripoint" } },
            { name = "is_outside", rval = "bool", args = { "int", "int" } },
            { name = "is_outside", rval = "bool", args = { "tripoint" } },
            { name = "item_from", rval = "item&", comment = "/**
  * Fetch an item from this map location, with sanity checks to ensure it still exists.
  */", args = { "tripoint", "int" } },
            { name = "light_transparency", rval = "float", args = { "tripoint" } },
            { name = "load", rval = nil, comment = "/**
     * Load submaps into @ref grid. This might create new submaps if
     * the @ref mapbuffer can not deliver the requested submap (as it does
     * not exist on disc).
     * This must be called before the map can be used at all!
     * @param wx global coordinates of the submap at grid[0]. This
     * is in submap coordinates.
     * @param wy see wx
     * @param wz see wx, this is the z-level
     * @param update_vehicles If true, add vehicles to the vehicle cache.
     */", args = { "int", "int", "int", "bool" } },
            { name = "make_rubble", rval = nil, args = { "tripoint" } },
            { name = "make_rubble", rval = nil, args = { "tripoint", "furn_id", "bool" } },
            { name = "make_rubble", rval = nil, comment = "/** Generates rubble at the given location, if overwrite is true it just writes on top of what currently exists
     *  floor_type is only used if there is a non-bashable wall at the location or with overwrite = true */", args = { "tripoint", "furn_id", "bool", "ter_id" } },
            { name = "make_rubble", rval = nil, comment = "/** Generates rubble at the given location, if overwrite is true it just writes on top of what currently exists
     *  floor_type is only used if there is a non-bashable wall at the location or with overwrite = true */", args = { "tripoint", "furn_id", "bool", "ter_id", "bool" } },
            { name = "max_volume", rval = "units::volume", args = { "tripoint" } },
            { name = "mop_spills", rval = "bool", comment = "/** Remove moppable fields/items at this location
     *  @param p the location
     *  @return true if anything moppable was there, false otherwise.
     */", args = { "tripoint" } },
            { name = "move_cost", rval = "int", args = { "int", "int" } },
            { name = "move_cost", rval = "int", comment = "/**
    * Calculate the cost to move past the tile at p.
    *
    * The move cost is determined by various obstacles, such
    * as terrain, vehicles and furniture.
    *
    * @note Movement costs for players and zombies both use this function.
    *
    * @return The return value is interpreted as follows:
    * Move Cost | Meaning
    * --------- | -------
    * 0         | Impassable. Use `passable`/`impassable` to check for this.
    * n > 0     | x*n turns to move past this
    */", args = { "tripoint" } },
            { name = "move_cost_ter_furn", rval = "int", args = { "int", "int" } },
            { name = "move_cost_ter_furn", rval = "int", comment = "/**
    * Similar behavior to `move_cost()`, but ignores vehicles.
    */", args = { "tripoint" } },
            { name = "name", rval = "std::string", args = { "int", "int" } },
            { name = "name", rval = "std::string", args = { "tripoint" } },
            { name = "need_draw_lower_floor", rval = "bool", args = { "tripoint" } },
            { name = "obstacle_name", rval = "std::string", comment = "/**
    * Returns the name of the obstacle at p that might be blocking movement/projectiles/etc.
    * Note that this only accounts for vehicles, terrain, and furniture.
    */", args = { "tripoint" } },
            { name = "on_vehicle_moved", rval = nil, comment = "/**
     * Callback invoked when a vehicle has moved.
     */", args = { "int" } },
            { name = "open_door", rval = "bool", args = { "tripoint", "bool" } },
            { name = "open_door", rval = "bool", args = { "tripoint", "bool", "bool" } },
            { name = "passable", rval = "bool", args = { "int", "int" } },
            { name = "passable", rval = "bool", args = { "tripoint" } },
            { name = "passable_ter_furn", rval = "bool", args = { "tripoint" } },
            { name = "pl_line_of_sight", rval = "bool", comment = "/**
         * Uses the map cache to tell if the player could see the given square.
         * pl_sees implies pl_line_of_sight
         * Used for infrared.
         */", args = { "tripoint", "int" } },
            { name = "pl_sees", rval = "bool", comment = "/**
         * Whether the player character (g->u) can see the given square (local map coordinates).
         * This only checks the transparency of the path to the target, the light level is not
         * checked.
         * @param t Target point to look at
         * @param max_range All squares that are further away than this are invisible.
         * Ignored if smaller than 0.
         */", args = { "tripoint", "int" } },
            { name = "place_gas_pump", rval = nil, args = { "int", "int", "int" } },
            { name = "place_gas_pump", rval = nil, args = { "int", "int", "int", "std::string" } },
            { name = "place_npc", rval = "int", args = { "int", "int", "std::string" } },
            { name = "place_spawns", rval = nil, args = { "mongroup_id", "int", "int", "int", "int", "int", "float" } },
            { name = "place_toilet", rval = nil, args = { "int", "int" } },
            { name = "place_toilet", rval = nil, args = { "int", "int", "int" } },
            { name = "place_vending", rval = nil, args = { "int", "int", "std::string" } },
            { name = "process_active_items", rval = nil, args = { } },
            { name = "process_falling", rval = nil, comment = "/**
     * Invoked @ref drop_everything on cached dirty tiles.
     */", args = { } },
            { name = "process_fields", rval = "bool", args = { } },
            { name = "propagate_field", rval = nil, args = { "tripoint", "field_id", "int" } },
            { name = "propagate_field", rval = nil, args = { "tripoint", "field_id", "int", "int" } },
            { name = "put_items_from_loc", rval = nil, comment = "/**
    * Place items from an item group at p. Places as much items as the item group says.
    * (Most item groups are distributions and will only create one item.)
    * @param loc Current location of items
    * @param p Destination of items
    * @param turn The birthday that the created items shall have.
    * @return Vector of pointers to placed items (can be empty, but no nulls).
    */", args = { "std::string", "tripoint" } },
            { name = "put_items_from_loc", rval = nil, comment = "/**
    * Place items from an item group at p. Places as much items as the item group says.
    * (Most item groups are distributions and will only create one item.)
    * @param loc Current location of items
    * @param p Destination of items
    * @param turn The birthday that the created items shall have.
    * @return Vector of pointers to placed items (can be empty, but no nulls).
    */", args = { "std::string", "tripoint", "int" } },
            { name = "random_outdoor_tile", rval = "point", args = { } },
            { name = "random_perimeter", rval = "tripoint", comment = "/** Get random tile on circumference of a circle */", args = { "tripoint", "int" } },
            { name = "ranged_target_size", rval = "float", comment = "/**
     * Size of map objects at `p` for purposes of ranged combat.
     * Size is in percentage of tile: if 1.0, all attacks going through tile
     * should hit map objects on it, if 0.0 there is nothing to be hit (air/water).
     */", args = { "tripoint" } },
            { name = "remove_field", rval = nil, comment = "/**
         * Remove field entry at xy, ignored if the field entry is not present.
         */", args = { "tripoint", "field_id" } },
            { name = "remove_trap", rval = nil, args = { "tripoint" } },
            { name = "reset_vehicle_cache", rval = nil, args = { "int" } },
            { name = "rotate", rval = nil, args = { "int" } },
            { name = "save", rval = nil, comment = "/**
     * Add currently loaded submaps (in @ref grid) to the @ref mapbuffer.
     * They will than be stored by that class and can be loaded from that class.
     * This can be called several times, the mapbuffer takes care of adding
     * the same submap several times. It should only be called after the map has
     * been loaded.
     * Submaps that have been loaded from the mapbuffer (and not generated) are
     * already stored in the mapbuffer.
     * TODO: determine if this is really needed? Submaps are already in the mapbuffer
     * if they have been loaded from disc and the are added by map::generate, too.
     * So when do they not appear in the mapbuffer?
     */", args = { } },
            { name = "sees", rval = "bool", comment = "/**
    * Returns whether `F` sees `T` with a view range of `range`.
    */", args = { "tripoint", "tripoint", "int" } },
            { name = "sees_some_items", rval = "bool", comment = "/**
     * Check if creature can see some items at p. Includes:
     * - check for items at this location (has_items(p))
     * - check for SEALED flag (sealed furniture/terrain makes
     * items not visible under any circumstances).
     * - check for CONTAINER flag (makes items only visible when
     * the creature is at p or at an adjacent square).
     */", args = { "tripoint", "Creature" } },
            { name = "set", rval = nil, args = { "int", "int", "ter_id", "furn_id" } },
            { name = "set", rval = nil, args = { "tripoint", "ter_id", "furn_id" } },
            { name = "set_field_age", rval = "int", comment = "/**
         * Set age of field entry at point.
         * @param p Location of field
         * @param t ID of field
         * @param age New age of specified field
         * @param isoffset If true, the given age value is added to the existing value,
         * if false, the existing age is ignored and overridden.
         * @return resulting age or -1 if not present (does *not* create a new field).
         */", args = { "tripoint", "field_id", "int" } },
            { name = "set_field_age", rval = "int", comment = "/**
         * Set age of field entry at point.
         * @param p Location of field
         * @param t ID of field
         * @param age New age of specified field
         * @param isoffset If true, the given age value is added to the existing value,
         * if false, the existing age is ignored and overridden.
         * @return resulting age or -1 if not present (does *not* create a new field).
         */", args = { "tripoint", "field_id", "int", "bool" } },
            { name = "set_field_strength", rval = "int", comment = "/**
         * Set density of field entry at point, creating if not present,
         * removing if density becomes 0.
         * @param p Location of field
         * @param t ID of field
         * @param str New strength of field
         * @param isoffset If true, the given str value is added to the existing value,
         * if false, the existing density is ignored and overridden.
         * @return resulting density, or 0 for not present (either removed or not created at all).
         */", args = { "tripoint", "field_id", "int" } },
            { name = "set_field_strength", rval = "int", comment = "/**
         * Set density of field entry at point, creating if not present,
         * removing if density becomes 0.
         * @param p Location of field
         * @param t ID of field
         * @param str New strength of field
         * @param isoffset If true, the given str value is added to the existing value,
         * if false, the existing density is ignored and overridden.
         * @return resulting density, or 0 for not present (either removed or not created at all).
         */", args = { "tripoint", "field_id", "int", "bool" } },
            { name = "set_floor_cache_dirty", rval = nil, args = { "int" } },
            { name = "set_graffiti", rval = nil, args = { "tripoint", "std::string" } },
            { name = "set_outside_cache_dirty", rval = nil, args = { "int" } },
            { name = "set_pathfinding_cache_dirty", rval = nil, args = { "int" } },
            { name = "set_radiation", rval = nil, args = { "int", "int", "int" } },
            { name = "set_radiation", rval = nil, args = { "tripoint", "int" } },
            { name = "set_signage", rval = nil, args = { "tripoint", "std::string" } },
            { name = "set_temperature", rval = nil, args = { "int", "int", "int" } },
            { name = "set_temperature", rval = nil, args = { "tripoint", "int" } },
            { name = "set_transparency_cache_dirty", rval = nil, args = { "int" } },
            { name = "shift", rval = nil, comment = "/**
     * Shift the map along the vector (sx,sy).
     * This is like loading the map with coordinates derived from the current
     * position of the map (@ref abs_sub) plus the shift vector.
     * Note: the map must have been loaded before this can be called.
     */", args = { "int", "int" } },
            { name = "smash_items", rval = nil, comment = "/** Tries to smash the items at the given tripoint. Used by the explosion code */", args = { "tripoint", "int" } },
            { name = "spawn_an_item", rval = "item&", args = { "tripoint", "item", "int", "int" } },
            { name = "spawn_an_item", rval = nil, args = { "int", "int", "item", "int", "int" } },
            { name = "spawn_artifact", rval = nil, args = { "tripoint" } },
            { name = "spawn_item", rval = nil, args = { "int", "int", "std::string" } },
            { name = "spawn_item", rval = nil, args = { "int", "int", "std::string", "int" } },
            { name = "spawn_item", rval = nil, args = { "int", "int", "std::string", "int", "int" } },
            { name = "spawn_item", rval = nil, args = { "int", "int", "std::string", "int", "int", "int" } },
            { name = "spawn_item", rval = nil, args = { "int", "int", "std::string", "int", "int", "int", "int" } },
            { name = "spawn_item", rval = nil, args = { "tripoint", "std::string" } },
            { name = "spawn_item", rval = nil, args = { "tripoint", "std::string", "int" } },
            { name = "spawn_item", rval = nil, args = { "tripoint", "std::string", "int", "int" } },
            { name = "spawn_item", rval = nil, args = { "tripoint", "std::string", "int", "int", "int" } },
            { name = "spawn_item", rval = nil, args = { "tripoint", "std::string", "int", "int", "int", "int" } },
            { name = "spawn_items", rval = nil, args = { "int", "int", "std::vector<item>" } },
            { name = "spawn_monsters", rval = nil, comment = "/**
     * Spawn monsters from submap spawn points and from the overmap.
     * @param ignore_sight If true, monsters may spawn in the view of the player
     * character (useful when the whole map has been loaded instead, e.g.
     * when starting a new game, or after teleportation or after moving vertically).
     * If false, monsters are not spawned in view of of player character.
     */", args = { "bool" } },
            { name = "stored_volume", rval = "units::volume", args = { "tripoint" } },
            { name = "supports_above", rval = "bool", comment = "/** Does this tile support vehicles and furniture above it */", args = { "tripoint" } },
            { name = "ter", rval = "ter_id", args = { "int", "int" } },
            { name = "ter", rval = "ter_id", args = { "tripoint" } },
            { name = "ter_set", rval = nil, args = { "int", "int", "ter_id" } },
            { name = "ter_set", rval = nil, args = { "tripoint", "ter_id" } },
            { name = "tername", rval = "std::string", args = { "int", "int" } },
            { name = "tername", rval = "std::string", args = { "tripoint" } },
            { name = "tr_at", rval = "trap&", args = { "tripoint" } },
            { name = "trans", rval = "bool", comment = "/**
     * Returns whether the tile at `p` is transparent(you can look past it).
     */", args = { "tripoint" } },
            { name = "translate", rval = nil, args = { "ter_id", "ter_id" } },
            { name = "translate_radius", rval = nil, args = { "ter_id", "ter_id", "float", "tripoint" } },
            { name = "trap_locations", rval = "std::vector<tripoint>", args = { "trap_id" } },
            { name = "trap_set", rval = nil, args = { "tripoint", "trap_id" } },
            { name = "trigger_rc_items", rval = nil, args = { "std::string" } },
            { name = "unboard_vehicle", rval = nil, args = { "tripoint" } },
            { name = "update_pathfinding_cache", rval = nil, args = { "int" } },
            { name = "update_visibility_cache", rval = nil, args = { "int" } },
            { name = "valid_move", rval = "bool", comment = "/**
     * Returns true if a creature could walk from `from` to `to` in one step.
     * That is, if the tiles are adjacent and either on the same z-level or connected
     * by stairs or (in case of flying monsters) open air with no floors.
     */", args = { "tripoint", "tripoint" } },
            { name = "valid_move", rval = "bool", comment = "/**
     * Returns true if a creature could walk from `from` to `to` in one step.
     * That is, if the tiles are adjacent and either on the same z-level or connected
     * by stairs or (in case of flying monsters) open air with no floors.
     */", args = { "tripoint", "tripoint", "bool" } },
            { name = "valid_move", rval = "bool", comment = "/**
     * Returns true if a creature could walk from `from` to `to` in one step.
     * That is, if the tiles are adjacent and either on the same z-level or connected
     * by stairs or (in case of flying monsters) open air with no floors.
     */", args = { "tripoint", "tripoint", "bool", "bool" } },
            { name = "veh_part_coordinates", rval = "point", comment = "/**
    * Vehicle-relative coordinates from reality bubble coordinates, if a vehicle
    * actually exists here.
    * Returns 0,0 if no vehicle exists there (use veh_at to check if it exists first)
    */", args = { "tripoint" } },
            { name = "vehmove", rval = nil, args = { } },
            { name = "vehproceed", rval = "bool", args = { } },
            { name = "vertical_shift", rval = nil, comment = "/**
     * Moves the map vertically to (not by!) newz.
     * Does not actually shift anything, only forces cache updates.
     * In the future, it will either actually shift the map or it will get removed
     *  after 3D migration is complete.
     */", args = { "int" } },
            { name = "water_from", rval = "item", args = { "tripoint" } },
        }
    },
    ter_t = {
        string_id = "ter_str_id",
        int_id = "ter_id",
        attributes = {
            close = { type = "ter_str_id", writable = true },
            connect_group = { type = "int", writable = true },
            description = { type = "std::string", writable = true },
            id = { type = "ter_str_id" },
            max_volume = { type = "units::volume", writable = true },
            movecost = { type = "int", writable = true },
            name = { type = "std::string", writable = true },
            open = { type = "ter_str_id", writable = true },
            roof = { type = "ter_str_id", writable = true },
            transforms_into = { type = "ter_str_id", writable = true },
            transparent = { type = "bool", writable = true },
            trap = { type = "trap_id", writable = true },
            trap_id_str = { type = "std::string", writable = true },
        },
        functions = {
            { name = "check", rval = nil, args = { } },
            { name = "color", rval = "int", args = { } },
            { name = "connects_to", rval = "bool", args = { "int" } },
            { name = "count", static = true, rval = "int", args = { } },
            { name = "extended_description", rval = "std::string", args = { } },
            { name = "get_harvest_names", rval = "std::set<std::string>", comment = "/**
     * Returns a set of names of the items that would be dropped.
     * Used for NPC whitelist checking.
     */", args = { } },
            { name = "has_flag", rval = "bool", args = { "std::string" } },
            { name = "set_connects", rval = nil, args = { "std::string" } },
            { name = "set_flag", rval = nil, args = { "std::string" } },
            { name = "symbol", rval = "int", args = { } },
        }
    },
    furn_t = {
        string_id = "furn_str_id",
        int_id = "furn_id",
        attributes = {
            close = { type = "furn_str_id", writable = true },
            connect_group = { type = "int", writable = true },
            crafting_pseudo_item = { type = "std::string", writable = true },
            description = { type = "std::string", writable = true },
            id = { type = "furn_str_id" },
            max_volume = { type = "units::volume", writable = true },
            move_str_req = { type = "int", writable = true },
            movecost = { type = "int", writable = true },
            name = { type = "std::string", writable = true },
            open = { type = "furn_str_id", writable = true },
            transparent = { type = "bool", writable = true },
        },
        functions = {
            { name = "check", rval = nil, args = { } },
            { name = "color", rval = "int", args = { } },
            { name = "connects_to", rval = "bool", args = { "int" } },
            { name = "count", static = true, rval = "int", args = { } },
            { name = "crafting_ammo_item_type", rval = "itype&", args = { } },
            { name = "crafting_pseudo_item_type", rval = "itype&", args = { } },
            { name = "extended_description", rval = "std::string", args = { } },
            { name = "get_harvest_names", rval = "std::set<std::string>", comment = "/**
     * Returns a set of names of the items that would be dropped.
     * Used for NPC whitelist checking.
     */", args = { } },
            { name = "has_flag", rval = "bool", args = { "std::string" } },
            { name = "set_connects", rval = nil, args = { "std::string" } },
            { name = "set_flag", rval = nil, args = { "std::string" } },
            { name = "symbol", rval = "int", args = { } },
        }
    },
    Creature = {
        attributes = {
            moves = { type = "int", writable = true },
            underwater = { type = "bool", writable = true },
        },
        functions = {
            { name = "add_effect", rval = nil, comment = "/** Adds or modifies an effect. If intensity is given it will set the effect intensity
            to the given value, or as close as max_intensity values permit. */", args = { "efftype_id", "int" } },
            { name = "add_effect", rval = nil, comment = "/** Adds or modifies an effect. If intensity is given it will set the effect intensity
            to the given value, or as close as max_intensity values permit. */", args = { "efftype_id", "int", "body_part" } },
            { name = "add_effect", rval = nil, comment = "/** Adds or modifies an effect. If intensity is given it will set the effect intensity
            to the given value, or as close as max_intensity values permit. */", args = { "efftype_id", "int", "body_part", "bool" } },
            { name = "add_effect", rval = nil, comment = "/** Adds or modifies an effect. If intensity is given it will set the effect intensity
            to the given value, or as close as max_intensity values permit. */", args = { "efftype_id", "int", "body_part", "bool", "int" } },
            { name = "add_effect", rval = nil, comment = "/** Adds or modifies an effect. If intensity is given it will set the effect intensity
            to the given value, or as close as max_intensity values permit. */", args = { "efftype_id", "int", "body_part", "bool", "int", "bool" } },
            { name = "add_env_effect", rval = "bool", comment = "/** Gives chance to save via environmental resist, returns false if resistance was successful. */", args = { "efftype_id", "body_part", "int", "int" } },
            { name = "add_env_effect", rval = "bool", comment = "/** Gives chance to save via environmental resist, returns false if resistance was successful. */", args = { "efftype_id", "body_part", "int", "int", "body_part" } },
            { name = "add_env_effect", rval = "bool", comment = "/** Gives chance to save via environmental resist, returns false if resistance was successful. */", args = { "efftype_id", "body_part", "int", "int", "body_part", "bool" } },
            { name = "add_env_effect", rval = "bool", comment = "/** Gives chance to save via environmental resist, returns false if resistance was successful. */", args = { "efftype_id", "body_part", "int", "int", "body_part", "bool", "int" } },
            { name = "add_env_effect", rval = "bool", comment = "/** Gives chance to save via environmental resist, returns false if resistance was successful. */", args = { "efftype_id", "body_part", "int", "int", "body_part", "bool", "int", "bool" } },
            { name = "apply_damage", rval = nil, args = { "Creature", "body_part", "int" } },
            { name = "avoid_trap", rval = "bool", comment = "/**
         * Called when a creature triggers a trap, returns true if they don't set it off.
         * @param tr is the trap that was triggered.
         * @param pos is the location of the trap (not necessarily of the creature) in the main map.
         */", args = { "tripoint", "trap" } },
            { name = "basic_symbol_color", rval = "int", args = { } },
            { name = "bleed", rval = nil, comment = "/** Adds an appropriate blood splatter. */", args = { } },
            { name = "bloodType", rval = "field_id", args = { } },
            { name = "check_dead_state", rval = nil, comment = "/**
         * This function checks the creatures @ref is_dead_state and (if true) calls @ref die.
         * You can either call this function after hitting this creature, or let the game
         * call it during @ref game::cleanup_dead.
         * As @ref die has many side effects (messages, on-death-triggers, ...), you should be
         * careful when calling this and expect that at least a "The monster dies!" message might
         * have been printed. If you want to print any message relating to the attack (e.g. how
         * much damage has been dealt, how the attack was performed, what has been blocked...), do
         * it *before* calling this function.
         */", args = { } },
            { name = "clear_effects", rval = nil, comment = "/** Remove all effects. */", args = { } },
            { name = "deal_melee_attack", rval = "int", args = { "Creature", "int" } },
            { name = "die", rval = nil, comment = "/** Empty function. Should always be overwritten by the appropriate player/NPC/monster version. */", args = { "Creature" } },
            { name = "digging", rval = "bool", args = { } },
            { name = "disp_name", rval = "std::string", args = { "bool" } },
            { name = "disp_name", rval = "std::string", args = { } },
            { name = "dodge_roll", rval = "float", args = { } },
            { name = "extended_description", rval = "std::string", args = { } },
            { name = "fall_damage_mod", rval = "float", comment = "/** Returns multiplier on fall damage at low velocity (knockback/pit/1 z-level, not 5 z-levels) */", args = { } },
            { name = "get_all_body_parts", rval = "std::vector<body_part>", comment = "/**
         * Returns body parts in order in which they should be displayed.
         * @param main If true, only displays parts that can have hit points
         */", args = { "bool" } },
            { name = "get_all_body_parts", rval = "std::vector<body_part>", comment = "/**
         * Returns body parts in order in which they should be displayed.
         * @param main If true, only displays parts that can have hit points
         */", args = { } },
            { name = "get_armor_bash", rval = "int", args = { "body_part" } },
            { name = "get_armor_bash_base", rval = "int", args = { "body_part" } },
            { name = "get_armor_bash_bonus", rval = "int", args = { } },
            { name = "get_armor_cut", rval = "int", args = { "body_part" } },
            { name = "get_armor_cut_base", rval = "int", args = { "body_part" } },
            { name = "get_armor_cut_bonus", rval = "int", args = { } },
            { name = "get_bash_bonus", rval = "int", args = { } },
            { name = "get_bash_mult", rval = "float", args = { } },
            { name = "get_block_bonus", rval = "int", args = { } },
            { name = "get_cut_bonus", rval = "int", args = { } },
            { name = "get_cut_mult", rval = "float", args = { } },
            { name = "get_dodge", rval = "float", args = { } },
            { name = "get_dodge_base", rval = "float", args = { } },
            { name = "get_dodge_bonus", rval = "float", args = { } },
            { name = "get_effect_dur", rval = "int", comment = "/** Returns the duration of the matching effect. Returns 0 if effect doesn't exist. */", args = { "efftype_id" } },
            { name = "get_effect_dur", rval = "int", comment = "/** Returns the duration of the matching effect. Returns 0 if effect doesn't exist. */", args = { "efftype_id", "body_part" } },
            { name = "get_effect_int", rval = "int", comment = "/** Returns the intensity of the matching effect. Returns 0 if effect doesn't exist. */", args = { "efftype_id" } },
            { name = "get_effect_int", rval = "int", comment = "/** Returns the intensity of the matching effect. Returns 0 if effect doesn't exist. */", args = { "efftype_id", "body_part" } },
            { name = "get_env_resist", rval = "int", args = { "body_part" } },
            { name = "get_grab_resist", rval = "int", args = { } },
            { name = "get_hit", rval = "float", args = { } },
            { name = "get_hit_base", rval = "float", args = { } },
            { name = "get_hit_bonus", rval = "float", args = { } },
            { name = "get_hp", rval = "int", args = { "hp_part" } },
            { name = "get_hp", rval = "int", args = { } },
            { name = "get_hp_max", rval = "int", args = { "hp_part" } },
            { name = "get_hp_max", rval = "int", args = { } },
            { name = "get_killer", rval = "Creature&", args = { } },
            { name = "get_melee", rval = "float", args = { } },
            { name = "get_melee_quiet", rval = "bool", args = { } },
            { name = "get_name", rval = "std::string", args = { } },
            { name = "get_num_blocks", rval = "int", args = { } },
            { name = "get_num_blocks_bonus", rval = "int", args = { } },
            { name = "get_num_dodges", rval = "int", args = { } },
            { name = "get_num_dodges_bonus", rval = "int", args = { } },
            { name = "get_pain", rval = "int", args = { } },
            { name = "get_path_avoid", rval = "std::set<tripoint>", comment = "/** Returns a set of points we do not want to path through. */", args = { } },
            { name = "get_perceived_pain", rval = "int", args = { } },
            { name = "get_random_body_part", rval = "body_part", args = { "bool" } },
            { name = "get_random_body_part", rval = "body_part", args = { } },
            { name = "get_size", rval = "m_size", args = { } },
            { name = "get_speed", rval = "int", args = { } },
            { name = "get_speed_base", rval = "int", args = { } },
            { name = "get_speed_bonus", rval = "int", args = { } },
            { name = "get_throw_resist", rval = "int", args = { } },
            { name = "get_value", rval = "std::string", args = { "std::string" } },
            { name = "get_weight", rval = "int", args = { } },
            { name = "gibType", rval = "field_id", args = { } },
            { name = "has_effect", rval = "bool", comment = "/** Check if creature has the matching effect. bp = num_bp means to check if the Creature has any effect
         *  of the matching type, targeted or untargeted. */", args = { "efftype_id" } },
            { name = "has_effect", rval = "bool", comment = "/** Check if creature has the matching effect. bp = num_bp means to check if the Creature has any effect
         *  of the matching type, targeted or untargeted. */", args = { "efftype_id", "body_part" } },
            { name = "has_grab_break_tec", rval = "bool", args = { } },
            { name = "has_trait", rval = "bool", comment = "/** Returns true if the player has the entered trait, returns false for non-humans */", args = { "std::string" } },
            { name = "has_weapon", rval = "bool", args = { } },
            { name = "hit_roll", rval = "float", comment = "/** Should always be overwritten by the appropriate player/NPC/monster version. */", args = { } },
            { name = "hp_percentage", rval = "int", args = { } },
            { name = "impact", rval = "int", comment = "/** Deals falling/collision damage with terrain/creature at pos */", args = { "int", "tripoint" } },
            { name = "in_sleep_state", rval = "bool", args = { } },
            { name = "is_dangerous_field", rval = "bool", comment = "/** Returns true if the given field entry is dangerous to us. */", args = { "field_entry" } },
            { name = "is_dangerous_fields", rval = "bool", comment = "/** Returns true if there is a field in the field set that is dangerous to us. */", args = { "field" } },
            { name = "is_dead_state", rval = "bool", args = { } },
            { name = "is_elec_immune", rval = "bool", args = { } },
            { name = "is_fake", rval = "bool", comment = "/** Returns true for non-real Creatures used temporarily; i.e. fake NPC's used for turret fire. */", args = { } },
            { name = "is_hallucination", rval = "bool", args = { } },
            { name = "is_immune_effect", rval = "bool", args = { "efftype_id" } },
            { name = "is_immune_field", rval = "bool", comment = "/** Returns true if we are immune to the field type with the given fid. Does not
         *  handle density, so this function should only be called through is_dangerous_field().
         */", args = { "field_id" } },
            { name = "is_monster", rval = "bool", args = { } },
            { name = "is_npc", rval = "bool", args = { } },
            { name = "is_on_ground", rval = "bool", args = { } },
            { name = "is_player", rval = "bool", args = { } },
            { name = "is_symbol_highlighted", rval = "bool", args = { } },
            { name = "is_underwater", rval = "bool", args = { } },
            { name = "is_warm", rval = "bool", args = { } },
            { name = "knock_back_from", rval = nil, args = { "tripoint" } },
            { name = "made_of", rval = "bool", args = { "material_id" } },
            { name = "melee_attack", rval = nil, comment = "/**
         * Calls the to other melee_attack function with an empty technique id (meaning no specific
         * technique should be used).
         */", args = { "Creature", "bool" } },
            { name = "melee_attack", rval = nil, comment = "/** Make a single melee attack with the currently equipped weapon against the targeted
         *  creature with prerolled hitspread. Should always be overwritten by the appropriate
         *  player/NPC/monster function. */", args = { "Creature", "bool", "matec_id", "int" } },
            { name = "melee_attack", rval = nil, comment = "/** Make a single melee attack with the currently equipped weapon against the targeted
         *  creature. Should always be overwritten by the appropriate player/NPC/monster function. */", args = { "Creature", "bool", "matec_id" } },
            { name = "mod_bash_bonus", rval = nil, args = { "int" } },
            { name = "mod_block_bonus", rval = nil, args = { "int" } },
            { name = "mod_cut_bonus", rval = nil, args = { "int" } },
            { name = "mod_dodge_bonus", rval = nil, args = { "float" } },
            { name = "mod_hit_bonus", rval = nil, args = { "float" } },
            { name = "mod_moves", rval = nil, args = { "int" } },
            { name = "mod_pain", rval = nil, args = { "int" } },
            { name = "mod_pain_noresist", rval = nil, args = { "int" } },
            { name = "mod_speed_bonus", rval = nil, args = { "int" } },
            { name = "mod_stat", rval = nil, args = { "std::string", "float" } },
            { name = "move_effects", rval = "bool", comment = "/** Processes move stopping effects. Returns false if movement is stopped. */", args = { "bool" } },
            { name = "normalize", rval = nil, comment = "/** Recreates the Creature from scratch. */", args = { } },
            { name = "on_dodge", rval = nil, comment = "/**
         * This creature just dodged an attack - possibly special/ranged attack - from source.
         * Players should train dodge, monsters may use some special defenses.
         */", args = { "Creature", "float" } },
            { name = "on_hit", rval = nil, comment = "/**
         * This creature just got hit by an attack - possibly special/ranged attack - from source.
         * Players should train dodge, possibly counter-attack somehow.
         */", args = { "Creature" } },
            { name = "on_hit", rval = nil, comment = "/**
         * This creature just got hit by an attack - possibly special/ranged attack - from source.
         * Players should train dodge, possibly counter-attack somehow.
         */", args = { "Creature", "body_part" } },
            { name = "on_hit", rval = nil, comment = "/**
         * This creature just got hit by an attack - possibly special/ranged attack - from source.
         * Players should train dodge, possibly counter-attack somehow.
         */", args = { "Creature", "body_part", "float" } },
            { name = "pos", rval = "tripoint", args = { } },
            { name = "posx", rval = "int", args = { } },
            { name = "posy", rval = "int", args = { } },
            { name = "posz", rval = "int", args = { } },
            { name = "power_rating", rval = "float", comment = "/** Returns an approximation of the creature's strength. */", args = { } },
            { name = "process_effects", rval = nil, comment = "/** Processes through all the effects on the Creature. */", args = { } },
            { name = "process_turn", rval = nil, comment = "/** Processes effects and bonuses and allocates move points based on speed. */", args = { } },
            { name = "projectile_attack_chance", rval = "float", comment = "/**
         * Probability that a projectile attack will hit with at least the given accuracy.
         *
         * @param total_dispersion nominal shot dispersion of gun + shooter
         * @param range range of the attack
         * @param accuracy the required accuracy, in the range [0..1]
         * @param target_size Ease of hitting target. 1.0 means target occupies entire tile and doesn't dodge.
         * @return the probability, in the range (0..1]
         */", args = { "float", "float", "float", "float" } },
            { name = "ranged_target_size", rval = "float", comment = "/**
         * Size of the target this creature presents to ranged weapons.
         * 0.0 means unhittable, 1.0 means all projectiles going through this creature's tile will hit it.
         */", args = { } },
            { name = "remove_effect", rval = "bool", comment = "/** Removes a listed effect, adding the removal memorial log if needed. bp = num_bp means to remove
         *  all effects of a given type, targeted or untargeted. Returns true if anything was removed. */", args = { "efftype_id" } },
            { name = "remove_effect", rval = "bool", comment = "/** Removes a listed effect, adding the removal memorial log if needed. bp = num_bp means to remove
         *  all effects of a given type, targeted or untargeted. Returns true if anything was removed. */", args = { "efftype_id", "body_part" } },
            { name = "remove_value", rval = nil, args = { "std::string" } },
            { name = "reset", rval = nil, comment = "/** Handles stat and bonus reset. */", args = { } },
            { name = "reset_bonuses", rval = nil, comment = "/** Resets the value of all bonus fields to 0. */", args = { } },
            { name = "reset_stats", rval = nil, comment = "/** Resets stats, and applies effects in an idempotent manner */", args = { } },
            { name = "sees", rval = "bool", args = { "Creature" } },
            { name = "sees", rval = "bool", args = { "int", "int" } },
            { name = "sees", rval = "bool", args = { "point" } },
            { name = "sees", rval = "bool", args = { "tripoint" } },
            { name = "sees", rval = "bool", args = { "tripoint", "bool" } },
            { name = "select_body_part", rval = "body_part", args = { "Creature", "int" } },
            { name = "set_armor_bash_bonus", rval = nil, args = { "int" } },
            { name = "set_armor_cut_bonus", rval = nil, args = { "int" } },
            { name = "set_bash_bonus", rval = nil, args = { "int" } },
            { name = "set_bash_mult", rval = nil, args = { "float" } },
            { name = "set_block_bonus", rval = nil, args = { "int" } },
            { name = "set_cut_bonus", rval = nil, args = { "int" } },
            { name = "set_cut_mult", rval = nil, args = { "float" } },
            { name = "set_dodge_bonus", rval = nil, args = { "float" } },
            { name = "set_fake", rval = nil, comment = "/** Sets a Creature's fake boolean. */", args = { "bool" } },
            { name = "set_grab_resist", rval = nil, args = { "int" } },
            { name = "set_hit_bonus", rval = nil, args = { "float" } },
            { name = "set_melee_quiet", rval = nil, args = { "bool" } },
            { name = "set_moves", rval = nil, args = { "int" } },
            { name = "set_num_blocks_bonus", rval = nil, args = { "int" } },
            { name = "set_num_dodges_bonus", rval = nil, args = { "int" } },
            { name = "set_pain", rval = nil, args = { "int" } },
            { name = "set_speed_base", rval = nil, args = { "int" } },
            { name = "set_speed_bonus", rval = nil, args = { "int" } },
            { name = "set_throw_resist", rval = nil, args = { "int" } },
            { name = "set_value", rval = nil, args = { "std::string", "std::string" } },
            { name = "setpos", rval = nil, args = { "tripoint" } },
            { name = "sight_range", rval = "int", comment = "/**
         * How far the creature sees under the given light. Places outside this range can
         * @param light_level See @ref game::light_level.
         */", args = { "int" } },
            { name = "skin_name", rval = "std::string", args = { } },
            { name = "speed_rating", rval = "float", comment = "/** Returns an approximate number of tiles this creature can travel per turn. */", args = { } },
            { name = "stability_roll", rval = "float", args = { } },
            { name = "symbol", rval = "std::string", args = { } },
            { name = "symbol_color", rval = "int", args = { } },
            { name = "uncanny_dodge", rval = "bool", args = { } },
            { name = "weight_capacity", rval = "int", args = { } },
        }
    },
    monster = {
        parent = "Creature",
        attributes = {
            anger = { type = "int", writable = true },
            friendly = { type = "int", writable = true },
            hallucination = { type = "bool", writable = true },
            ignoring = { type = "int", writable = true },
            inv = { type = "std::vector<item>", writable = true },
            last_updated = { type = "int", writable = true },
            made_footstep = { type = "bool", writable = true },
            mission_id = { type = "int", writable = true },
            morale = { type = "int", writable = true },
            no_corpse_quiet = { type = "bool", writable = true },
            no_extra_death_drops = { type = "bool", writable = true },
            staircount = { type = "int", writable = true },
            type = { type = "mtype", writable = true },
            unique_name = { type = "std::string", writable = true },
            wander_pos = { type = "tripoint", writable = true },
            wandf = { type = "int", writable = true },
        },
        functions = {
            { name = "add_item", rval = nil, args = { "item" } },
            { name = "attack_at", rval = "bool", comment = "/**
         * Attack any enemies at the given location.
         *
         * Attacks only if there is a creature at the given location towards
         * we are hostile.
         *
         * @return true if something was attacked, false otherwise
         */", args = { "tripoint" } },
            { name = "attack_target", rval = "Creature&", args = { } },
            { name = "bash_at", rval = "bool", comment = "/**
         * Try to smash/bash/destroy your way through the terrain at p.
         *
         * @return true if we destroyed something, false otherwise.
         */", args = { "tripoint" } },
            { name = "bash_estimate", rval = "int", args = { } },
            { name = "bash_skill", rval = "int", comment = "/** Returns innate monster bash skill, without calculating additional from helpers */", args = { } },
            { name = "calc_climb_cost", rval = "int", args = { "tripoint", "tripoint" } },
            { name = "calc_movecost", rval = "int", args = { "tripoint", "tripoint" } },
            { name = "can_act", rval = "bool", args = { } },
            { name = "can_drown", rval = "bool", args = { } },
            { name = "can_hear", rval = "bool", args = { } },
            { name = "can_move_to", rval = "bool", comment = "/**
         * Checks whether we can move to/through p. This does not account for bashing.
         *
         * This is used in pathfinding and ONLY checks the terrain. It ignores players
         * and monsters, which might only block this tile temporarily.
         */", args = { "tripoint" } },
            { name = "can_see", rval = "bool", args = { } },
            { name = "can_submerge", rval = "bool", args = { } },
            { name = "can_upgrade", rval = "bool", args = { } },
            { name = "color_with_effects", rval = "int", args = { } },
            { name = "deserialize", rval = nil, args = { "std::string" } },
            { name = "die_in_explosion", rval = nil, args = { "Creature" } },
            { name = "disable_special", rval = nil, comment = "/** Sets the enabled flag for the given special to false */", args = { "std::string" } },
            { name = "drop_items_on_death", rval = nil, args = { } },
            { name = "explode", rval = nil, args = { } },
            { name = "footsteps", rval = nil, args = { "tripoint" } },
            { name = "get_hp", rval = "int", args = { } },
            { name = "get_hp_max", rval = "int", args = { } },
            { name = "group_bash_skill", rval = "int", comment = "/** Returns ability of monster and any cooperative helpers to
         * bash the designated target.  **/", args = { "tripoint" } },
            { name = "hasten_upgrade", rval = nil, args = { } },
            { name = "heal", rval = "int", comment = "/**
         * Flat addition to the monsters @ref hp. If `overheal` is true, this is not capped by max hp.
         * Returns actually healed hp.
         */", args = { "int" } },
            { name = "heal", rval = "int", comment = "/**
         * Flat addition to the monsters @ref hp. If `overheal` is true, this is not capped by max hp.
         * Returns actually healed hp.
         */", args = { "int", "bool" } },
            { name = "hear_sound", rval = nil, comment = "/**
         * Makes monster react to heard sound
         *
         * @param from Location of the sound source
         * @param source_volume Volume at the center of the sound source
         * @param distance Distance to sound source (currently just rl_dist)
         */", args = { "tripoint", "int", "int" } },
            { name = "init_from_item", rval = nil, comment = "/**
         * Initialize values like speed / hp from data of an item.
         * This applies to robotic monsters that are spawned by invoking an item (e.g. turret),
         * and to reviving monsters that spawn from a corpse.
         */", args = { "item" } },
            { name = "is_dead", rval = "bool", args = { } },
            { name = "is_fleeing", rval = "bool", args = { "player" } },
            { name = "load_info", rval = nil, args = { "std::string" } },
            { name = "made_of", rval = "bool", args = { "phase_id" } },
            { name = "make_ally", rval = nil, comment = "/** Makes this monster an ally of the given monster. */", args = { "monster" } },
            { name = "make_friendly", rval = nil, args = { } },
            { name = "make_fungus", rval = "bool", comment = "/**
         * Makes this monster into a fungus version
         * Returns false if no such monster exists
         */", args = { } },
            { name = "move", rval = nil, args = { } },
            { name = "move_target", rval = "tripoint", args = { } },
            { name = "move_to", rval = "bool", comment = "/**
         * Attempt to move to p.
         *
         * If there's something blocking the movement, such as infinite move
         * costs at the target, an existing NPC or monster, this function simply
         * aborts and does nothing.
         *
         * @param p Destination of movement
         * @param force If this is set to true, the movement will happen even if
         *              there's currently something blocking the destination.
         *
         * @param stagger_adjustment is a multiplier for move cost to compensate for staggering.
         *
         * @return true if movement successful, false otherwise
         */", args = { "tripoint" } },
            { name = "move_to", rval = "bool", comment = "/**
         * Attempt to move to p.
         *
         * If there's something blocking the movement, such as infinite move
         * costs at the target, an existing NPC or monster, this function simply
         * aborts and does nothing.
         *
         * @param p Destination of movement
         * @param force If this is set to true, the movement will happen even if
         *              there's currently something blocking the destination.
         *
         * @param stagger_adjustment is a multiplier for move cost to compensate for staggering.
         *
         * @return true if movement successful, false otherwise
         */", args = { "tripoint", "bool" } },
            { name = "move_to", rval = "bool", comment = "/**
         * Attempt to move to p.
         *
         * If there's something blocking the movement, such as infinite move
         * costs at the target, an existing NPC or monster, this function simply
         * aborts and does nothing.
         *
         * @param p Destination of movement
         * @param force If this is set to true, the movement will happen even if
         *              there's currently something blocking the destination.
         *
         * @param stagger_adjustment is a multiplier for move cost to compensate for staggering.
         *
         * @return true if movement successful, false otherwise
         */", args = { "tripoint", "bool", "float" } },
            { name = "name", rval = "std::string", args = { "int" } },
            { name = "name", rval = "std::string", args = { } },
            { name = "name_with_armor", rval = "std::string", args = { } },
            { name = "on_load", rval = nil, comment = "/**
         * Retroactively update monster.
         */", args = { } },
            { name = "on_unload", rval = nil, comment = "/**
         * Do some cleanup and caching as monster is being unloaded from map.
         */", args = { } },
            { name = "poly", rval = nil, args = { "mtype_id" } },
            { name = "process_triggers", rval = nil, args = { } },
            { name = "push_to", rval = "bool", comment = "/**
         * Try to push away whatever occupies p, then step in.
         * May recurse and try to make the creature at p push further.
         *
         * @param p Location of pushed object
         * @param boost A bonus on the roll to represent a horde pushing from behind
         * @param depth Number of recursions so far
         *
         * @return True if we managed to push something and took its place, false otherwise.
         */", args = { "tripoint", "int", "int" } },
            { name = "rate_target", rval = "float", args = { "Creature", "float" } },
            { name = "rate_target", rval = "float", args = { "Creature", "float", "bool" } },
            { name = "reset_special", rval = nil, comment = "/** Resets a given special to its monster type cooldown value */", args = { "std::string" } },
            { name = "reset_special_rng", rval = nil, comment = "/** Resets a given special to a value between 0 and its monster type cooldown value. */", args = { "std::string" } },
            { name = "scent_move", rval = "tripoint", args = { } },
            { name = "serialize", rval = "std::string", args = { } },
            { name = "set_dest", rval = nil, args = { "tripoint" } },
            { name = "set_hp", rval = nil, comment = "/**
         * Directly set the current @ref hp of the monster (not capped at the maximal hp).
         * You might want to use @ref heal / @ref apply_damage or @ref deal_damage instead.
         */", args = { "int" } },
            { name = "set_special", rval = nil, comment = "/** Sets a given special to the given value */", args = { "std::string", "int" } },
            { name = "shift", rval = nil, args = { "int", "int" } },
            { name = "spawn", rval = nil, args = { "tripoint" } },
            { name = "stumble", rval = nil, args = { } },
            { name = "to_item", rval = "item", comment = "/**
         * Convert this monster into an item (see @ref mtype::revert_to_itype).
         * Only useful for robots and the like, the monster must have at least
         * a non-empty item id as revert_to_itype.
         */", args = { } },
            { name = "try_upgrade", rval = nil, args = { "bool" } },
            { name = "turns_to_reach", rval = "int", args = { "int", "int" } },
            { name = "unset_dest", rval = nil, args = { } },
            { name = "wander", rval = "bool", args = { } },
            { name = "wander_to", rval = nil, comment = "/**
         * Set p as wander destination.
         *
         * This will cause the monster to slowly move towards the destination,
         * unless there is an overriding smell or plan.
         *
         * @param p Destination of monster's wonderings
         * @param f The priority of the destination, as well as how long we should
         *          wander towards there.
         */", args = { "tripoint", "int" } },
            { name = "will_reach", rval = "bool", args = { "int", "int" } },
        }
    },
    martialart = {
        string_id = "matype_id",
        attributes = {
            arm_block = { type = "int", writable = true },
            arm_block_with_bio_armor_arms = { type = "bool", writable = true },
            description = { type = "std::string", writable = true },
            id = { type = "matype_id" },
            leg_block = { type = "int", writable = true },
            leg_block_with_bio_armor_legs = { type = "bool", writable = true },
            name = { type = "std::string", writable = true },
            onattack_buffs = { type = "std::vector<mabuff_id>", writable = true },
            onblock_buffs = { type = "std::vector<mabuff_id>", writable = true },
            ondodge_buffs = { type = "std::vector<mabuff_id>", writable = true },
            ongethit_buffs = { type = "std::vector<mabuff_id>", writable = true },
            onhit_buffs = { type = "std::vector<mabuff_id>", writable = true },
            onmove_buffs = { type = "std::vector<mabuff_id>", writable = true },
            static_buffs = { type = "std::vector<mabuff_id>", writable = true },
            strictly_unarmed = { type = "bool", writable = true },
            techniques = { type = "std::set<matec_id>", writable = true },
            weapons = { type = "std::set<std::string>", writable = true },
        },
        functions = {
            { name = "apply_onattack_buffs", rval = nil, args = { "player" } },
            { name = "apply_onblock_buffs", rval = nil, args = { "player" } },
            { name = "apply_ondodge_buffs", rval = nil, args = { "player" } },
            { name = "apply_ongethit_buffs", rval = nil, args = { "player" } },
            { name = "apply_onhit_buffs", rval = nil, args = { "player" } },
            { name = "apply_onmove_buffs", rval = nil, args = { "player" } },
            { name = "apply_static_buffs", rval = nil, args = { "player" } },
            { name = "has_technique", rval = "bool", args = { "player", "matec_id" } },
            { name = "has_weapon", rval = "bool", args = { "std::string" } },
            { name = "weapon_valid", rval = "bool", args = { "item" } },
        }
    },
    material_type = {
        string_id = "material_id",
        attributes = {
            id = { type = "material_id" },
        },
        functions = {
            { name = "acid_resist", rval = "int", args = { } },
            { name = "bash_dmg_verb", rval = "std::string", args = { } },
            { name = "bash_resist", rval = "int", args = { } },
            { name = "check", rval = nil, args = { } },
            { name = "chip_resist", rval = "int", args = { } },
            { name = "cut_dmg_verb", rval = "std::string", args = { } },
            { name = "cut_resist", rval = "int", args = { } },
            { name = "density", rval = "int", args = { } },
            { name = "dmg_adj", rval = "std::string", args = { "int" } },
            { name = "edible", rval = "bool", args = { } },
            { name = "elec_resist", rval = "int", args = { } },
            { name = "fire_resist", rval = "int", args = { } },
            { name = "ident", rval = "material_id", args = { } },
            { name = "name", rval = "std::string", args = { } },
            { name = "repaired_with", rval = "std::string", args = { } },
            { name = "salvaged_into", rval = "std::string", args = { } },
            { name = "soft", rval = "bool", args = { } },
        }
    },
    start_location = {
        string_id = "start_location_id",
        functions = {
            { name = "add_map_special", rval = nil, comment = "/**
         * Adds a map special, see mapgen.h and mapgen.cpp. Look at the namespace MapExtras.
         */", args = { "tripoint", "std::string" } },
            { name = "burn", rval = nil, comment = "/**
         * Burn random terrain / furniture with FLAMMABLE or FLAMMABLE_ASH tag.
         * Doors and windows are excluded.
         * @param omtstart Global overmap terrain coordinates where the player is to be spawned.
         * @param rad safe radius area to prevent player spawn next to burning wall.
         * @param count number of fire on the map.
         */", args = { "tripoint", "int", "int" } },
            { name = "find_player_initial_location", rval = "tripoint", comment = "/**
         * Find a suitable start location on the overmap.
         * @return Global, absolute overmap terrain coordinates where the player should spawn.
         * It may return `overmap::invalid_tripoint` if no suitable starting location could be found
         * in the world.
         */", args = { } },
            { name = "flags", rval = "std::set<std::string>", args = { } },
            { name = "handle_heli_crash", rval = nil, args = { "player" } },
            { name = "name", rval = "std::string", args = { } },
            { name = "place_player", rval = nil, comment = "/**
         * Place the player somewher ein th reality bubble (g->m).
         */", args = { "player" } },
            { name = "prepare_map", rval = nil, comment = "/**
         * Initialize the map at players start location using @ref prepare_map.
         * @param omtstart Global overmap terrain coordinates where the player is to be spawned.
         */", args = { "tripoint" } },
            { name = "target", rval = "std::string", args = { } },
        }
    },
    ma_buff = {
        string_id = "mabuff_id",
        attributes = {
            blocks_bonus = { type = "int", writable = true },
            buff_duration = { type = "int", writable = true },
            description = { type = "std::string", writable = true },
            dodges_bonus = { type = "int", writable = true },
            id = { type = "mabuff_id" },
            max_stacks = { type = "int", writable = true },
            melee_allowed = { type = "bool", writable = true },
            name = { type = "std::string", writable = true },
            quiet = { type = "bool", writable = true },
            strictly_unarmed = { type = "bool", writable = true },
            throw_immune = { type = "bool", writable = true },
        },
        functions = {
            { name = "apply_buff", rval = nil, args = { "player" } },
            { name = "apply_player", rval = nil, args = { "player" } },
            { name = "block_bonus", rval = "int", args = { "player" } },
            { name = "can_melee", rval = "bool", args = { } },
            { name = "dodge_bonus", rval = "int", args = { "player" } },
            { name = "get_effect_id", rval = "efftype_id", args = { } },
            { name = "hit_bonus", rval = "int", args = { "player" } },
            { name = "is_quiet", rval = "bool", args = { } },
            { name = "is_throw_immune", rval = "bool", args = { } },
            { name = "is_valid_player", rval = "bool", args = { "player" } },
            { name = "speed_bonus", rval = "int", args = { "player" } },
        }
    },
    ma_technique = {
        string_id = "matec_id",
        attributes = {
            aoe = { type = "std::string", writable = true },
            block_counter = { type = "bool", writable = true },
            crit_tec = { type = "bool", writable = true },
            defensive = { type = "bool", writable = true },
            disarms = { type = "bool", writable = true },
            dodge_counter = { type = "bool", writable = true },
            down_dur = { type = "int", writable = true },
            dummy = { type = "bool", writable = true },
            flags = { type = "std::set<std::string>", writable = true },
            goal = { type = "std::string", writable = true },
            grab_break = { type = "bool", writable = true },
            id = { type = "matec_id" },
            knockback_dist = { type = "int", writable = true },
            knockback_spread = { type = "float", writable = true },
            miss_recovery = { type = "bool", writable = true },
            name = { type = "std::string", writable = true },
            npc_message = { type = "std::string", writable = true },
            player_message = { type = "std::string", writable = true },
            stun_dur = { type = "int", writable = true },
            weighting = { type = "int", writable = true },
        },
        functions = {
            { name = "is_valid_player", rval = "bool", args = { "player" } },
            { name = "move_cost_multiplier", rval = "float", args = { "player" } },
            { name = "move_cost_penalty", rval = "float", args = { "player" } },
        }
    },
    Skill = {
        string_id = "skill_id",
        has_equal = true,
        functions = {
            { name = "description", rval = "std::string", args = { } },
            { name = "from_legacy_int", static = true, rval = "skill_id", args = { "int" } },
            { name = "get", static = true, rval = "Skill&", args = { "skill_id" } },
            { name = "ident", rval = "skill_id", args = { } },
            { name = "is_combat_skill", rval = "bool", args = { } },
            { name = "is_contextual_skill", rval = "bool", args = { } },
            { name = "name", rval = "std::string", args = { } },
            { name = "random_skill", static = true, rval = "skill_id", args = { } },
            { name = "skill_count", static = true, rval = "int", args = { } },
        }
    },
    quality = {
        string_id = "quality_id",
        attributes = {
            id = { type = "quality_id" },
            name = { type = "std::string", writable = true },
        },
    },
    species_type = {
        string_id = "species_id",
        attributes = {
            id = { type = "species_id" },
        },
    },
    MonsterGroup = {
        string_id = "mongroup_id",
        attributes = {
            defaultMonster = { type = "mtype_id", writable = true },
            is_safe = { type = "bool", writable = true },
            monster_group_time = { type = "int", writable = true },
            name = { type = "mongroup_id", writable = true },
            new_monster_group = { type = "mongroup_id", writable = true },
            replace_monster_group = { type = "bool", writable = true },
        },
        functions = {
            { name = "IsMonsterInGroup", rval = "bool", args = { "mtype_id" } },
        }
    },
    mtype = {
        string_id = "mtype_id",
        attributes = {
            agro = { type = "int", writable = true },
            armor_acid = { type = "int", writable = true },
            armor_bash = { type = "int", writable = true },
            armor_cut = { type = "int", writable = true },
            armor_fire = { type = "int", writable = true },
            armor_stab = { type = "int", writable = true },
            attack_cost = { type = "int", writable = true },
            bash_skill = { type = "int", writable = true },
            burn_into = { type = "mtype_id", writable = true },
            categories = { type = "std::set<std::string>", writable = true },
            color = { type = "int", writable = true },
            death_drops = { type = "std::string", writable = true },
            def_chance = { type = "int", writable = true },
            description = { type = "std::string", writable = true },
            difficulty = { type = "int", writable = true },
            half_life = { type = "int", writable = true },
            hp = { type = "int", writable = true },
            id = { type = "mtype_id" },
            luminance = { type = "float", writable = true },
            mat = { type = "std::vector<material_id>", writable = true },
            melee_dice = { type = "int", writable = true },
            melee_sides = { type = "int", writable = true },
            melee_skill = { type = "int", writable = true },
            morale = { type = "int", writable = true },
            phase = { type = "phase_id", writable = true },
            revert_to_itype = { type = "std::string", writable = true },
            size = { type = "m_size", writable = true },
            sk_dodge = { type = "int", writable = true },
            special_attacks_names = { type = "std::vector<std::string>", writable = true },
            species = { type = "std::set<species_id>", writable = true },
            speed = { type = "int", writable = true },
            sym = { type = "std::string", writable = true },
            upgrade_group = { type = "mongroup_id", writable = true },
            upgrade_into = { type = "mtype_id", writable = true },
            upgrades = { type = "bool", writable = true },
            vision_day = { type = "int", writable = true },
            vision_night = { type = "int", writable = true },
        },
        functions = {
            { name = "bloodType", rval = "field_id", args = { } },
            { name = "get_meat_chunks_count", rval = "int", args = { } },
            { name = "get_meat_itype", rval = "std::string", args = { } },
            { name = "gibType", rval = "field_id", args = { } },
            { name = "has_flag", rval = "bool", args = { "std::string" } },
            { name = "has_special_attack", rval = "bool", args = { "std::string" } },
            { name = "in_category", rval = "bool", args = { "std::string" } },
            { name = "in_species", rval = "bool", args = { "species_id" } },
            { name = "in_species", rval = "bool", args = { "species_type" } },
            { name = "made_of", rval = "bool", args = { "material_id" } },
            { name = "nname", rval = "std::string", args = { "int" } },
            { name = "nname", rval = "std::string", args = { } },
            { name = "same_species", rval = "bool", comment = "/**
         * Check if this type is of the same species as the other one, because
         * species is a set and can contain several species, one entry that is
         * in both monster types fulfills that test.
         */", args = { "mtype" } },
            { name = "set_flag", rval = nil, args = { "std::string", "bool" } },
        }
    },
    mongroup = {
        attributes = {
            diffuse = { type = "bool", writable = true },
            dying = { type = "bool", writable = true },
            horde = { type = "bool", writable = true },
            horde_behaviour = { type = "std::string", writable = true },
            interest = { type = "int", writable = true },
            population = { type = "int", writable = true },
            pos = { type = "tripoint", writable = true },
            radius = { type = "int", writable = true },
            target = { type = "tripoint", writable = true },
            type = { type = "mongroup_id", writable = true },
        },
        functions = {
            { name = "clear", rval = nil, args = { } },
            { name = "dec_interest", rval = nil, args = { "int" } },
            { name = "deserialize", rval = nil, args = { "std::string" } },
            { name = "empty", rval = "bool", args = { } },
            { name = "inc_interest", rval = nil, args = { "int" } },
            { name = "is_safe", rval = "bool", args = { } },
            { name = "serialize", rval = "std::string", args = { } },
            { name = "set_interest", rval = nil, args = { "int" } },
            { name = "set_target", rval = nil, args = { "int", "int" } },
            { name = "wander", rval = nil, args = { "overmap" } },
        }
    },
    overmap = {
        functions = {
            { name = "add_note", rval = nil, args = { "int", "int", "int", "std::string" } },
            { name = "clear", rval = nil, args = { } },
            { name = "clear_mon_groups", rval = nil, args = { } },
            { name = "delete_note", rval = nil, args = { "int", "int", "int" } },
            { name = "display_notes", static = true, rval = "point", comment = "/// @todo This one should be obsoleted
    /**
     * Display a list of all notes on this z-level. Let the user choose
     * one or none of them.
     * @returns The location of the chosen note (absolute overmap terrain
     * coordinates), or invalid_point if the user did not choose a note.
     */", args = { "int" } },
            { name = "draw_editor", static = true, rval = "tripoint", args = { } },
            { name = "draw_hordes", static = true, rval = "tripoint", comment = "/**
     * Draw overmap like with @ref draw_overmap() and display hordes.
     */", args = { } },
            { name = "draw_overmap", static = true, rval = "tripoint", comment = "/**
     * Interactive point choosing; used as the map screen.
     * The map is initially center at the players position.
     * @returns The absolute coordinates of the chosen point or
     * invalid_point if canceled with escape (or similar key).
     */", args = { } },
            { name = "draw_overmap", static = true, rval = "tripoint", comment = "/**
     * Same as @ref draw_overmap() but starts at select if set.
     * Otherwise on players location.
     */
    /**
     * Same as above but start at z-level z instead of players
     * current z-level, x and y are taken from the players position.
     */", args = { "int" } },
            { name = "draw_scents", static = true, rval = "tripoint", comment = "/**
     * Draw overmap like with @ref draw_overmap() and display scent traces.
     */", args = { } },
            { name = "draw_weather", static = true, rval = "tripoint", comment = "/**
     * Draw overmap like with @ref draw_overmap() and display the weather.
     */", args = { } },
            { name = "draw_zones", static = true, rval = "tripoint", comment = "/**
     * Draw overmap like with @ref draw_overmap() and display the given zone.
     */", args = { "tripoint", "tripoint", "int" } },
            { name = "find_notes", rval = "std::vector<point>", comment = "/**
     * Return a vector containing the absolute coordinates of
     * every matching note on the current z level of the current overmap.
     * @returns A vector of note coordinates (absolute overmap terrain
     * coordinates), or empty vector if no matching notes are found.
     */", args = { "int", "std::string" } },
            { name = "find_random_omt", rval = "tripoint", comment = "/**
     * @return The (local) overmap terrain coordinates of a randomly
     * chosen place on the overmap with the specific overmap terrain.
     * Returns @ref invalid_tripoint if no suitable place has been found.
     */", args = { "std::string" } },
            { name = "find_terrain", rval = "std::vector<point>", comment = "/**
     * Return a vector containing the absolute coordinates of
     * every matching terrain on the current z level of the current overmap.
     * @returns A vector of terrain coordinates (absolute overmap terrain
     * coordinates), or empty vector if no matching terrain is found.
     */", args = { "std::string", "int" } },
            { name = "global_base_point", rval = "point", comment = "/** Returns the (0, 0) corner of the overmap in the global coordinates. */", args = { } },
            { name = "has_note", rval = "bool", args = { "int", "int", "int" } },
            { name = "inbounds", static = true, rval = "bool", args = { "int", "int", "int" } },
            { name = "inbounds", static = true, rval = "bool", args = { "int", "int", "int", "int" } },
            { name = "inbounds", static = true, rval = "bool", comment = "/**
     * @returns Whether @param loc is within desired bounds of the overmap
     * @param clearance Minimal distance from the edges of the overmap
     */", args = { "tripoint" } },
            { name = "inbounds", static = true, rval = "bool", comment = "/**
     * @returns Whether @param loc is within desired bounds of the overmap
     * @param clearance Minimal distance from the edges of the overmap
     */", args = { "tripoint", "int" } },
            { name = "is_explored", rval = "bool", args = { "int", "int", "int" } },
            { name = "mongroup_check", rval = "bool", comment = "/** Unit test enablers to check if a given mongroup is present. */", args = { "mongroup" } },
            { name = "note", rval = "std::string", args = { "int", "int", "int" } },
            { name = "pos", rval = "point", args = { } },
            { name = "save", rval = nil, args = { } },
        }
    },
    itype = {
        attributes = {
            color = { type = "int", writable = true },
            countdown_destroy = { type = "bool", writable = true },
            countdown_interval = { type = "int", writable = true },
            damage_max = { type = "int", writable = true },
            damage_min = { type = "int", writable = true },
            default_container = { type = "std::string", writable = true },
            description = { type = "std::string", writable = true },
            explode_in_fire = { type = "bool", writable = true },
            integral_volume = { type = "units::volume", writable = true },
            item_tags = { type = "std::set<std::string>", writable = true },
            light_emission = { type = "int", writable = true },
            m_to_hit = { type = "int", writable = true },
            magazine_well = { type = "units::volume", writable = true },
            materials = { type = "std::vector<material_id>", writable = true },
            min_dex = { type = "int", writable = true },
            min_int = { type = "int", writable = true },
            min_per = { type = "int", writable = true },
            min_str = { type = "int", writable = true },
            phase = { type = "phase_id", writable = true },
            price = { type = "int", writable = true },
            price_post = { type = "int", writable = true },
            repair = { type = "std::set<std::string>", writable = true },
            rigid = { type = "bool", writable = true },
            snippet_category = { type = "std::string", writable = true },
            stack_size = { type = "int", writable = true },
            stackable = { type = "bool", writable = true },
            sym = { type = "std::string", writable = true },
            techniques = { type = "std::set<matec_id>", writable = true },
            volume = { type = "units::volume", writable = true },
            weight = { type = "int", writable = true },
        },
        functions = {
            { name = "can_use", rval = "bool", args = { "std::string" } },
            { name = "charges_default", rval = "int", args = { } },
            { name = "charges_to_use", rval = "int", args = { } },
            { name = "count_by_charges", rval = "bool", args = { } },
            { name = "get_id", rval = "std::string", args = { } },
            { name = "get_item_type_string", rval = "std::string", args = { } },
            { name = "has_use", rval = "bool", args = { } },
            { name = "invoke", rval = "int", args = { "player", "item", "tripoint" } },
            { name = "invoke", rval = "int", args = { "player", "item", "tripoint", "std::string" } },
            { name = "maximum_charges", rval = "int", args = { } },
            { name = "nname", rval = "std::string", args = { "int" } },
            { name = "tick", rval = "int", args = { "player", "item", "tripoint" } },
        }
    },
    trap = {
        string_id = "trap_str_id",
        int_id = "trap_id",
        attributes = {
            color = { type = "int", writable = true },
            id = { type = "trap_str_id" },
            loadid = { type = "trap_id" },
            name = { type = "std::string", writable = true },
            sym = { type = "int", writable = true },
        },
        functions = {
            { name = "can_see", rval = "bool", comment = "/**
         * Can player/npc p see this kind of trap, either by their memory (they known there is
         * the trap) or by the visibility of the trap (the trap is not hidden at all)?
         */", args = { "tripoint", "player" } },
            { name = "count", static = true, rval = "int", args = { } },
            { name = "detect_trap", rval = "bool", comment = "/** Player has not yet seen the trap and returns the variable chance, at this moment,
         of whether the trap is seen or not. */", args = { "tripoint", "player" } },
            { name = "funnel_turns_per_charge", rval = "float", args = { "float" } },
            { name = "get_avoidance", rval = "int", comment = "/**
         * Whether triggering the trap can be avoid (if greater than 0) and if so, this is
         * compared to dodge skill (with some adjustments). Smaller values means it's easier
         * to dodge.
         */", args = { } },
            { name = "get_difficulty", rval = "int", comment = "/**
         * This is used when disarming the trap. A value of 0 means disarming will always work
         * (e.g. for funnels), a values of 99 means it can not be disarmed at all. Smaller values
         * makes it easier to disarm the trap.
         */", args = { } },
            { name = "get_visibility", rval = "int", comment = "/**
         * How easy it is to spot the trap. Smaller values means it's easier to spot.
         */", args = { } },
            { name = "is_3x3_trap", rval = "bool", comment = "/**
         * Whether this kind of trap actually occupies a 3x3 area. Currently only blade traps
         * do so.
         */", args = { } },
            { name = "is_benign", rval = "bool", comment = "/**
         * If true, this is not really a trap and there won't be any safety queries before stepping
         * onto it (e.g. for funnels).
         */", args = { } },
            { name = "is_funnel", rval = "bool", comment = "/**
         * @name Funnels
         *
         * Traps can act as funnels, for this they need a @ref trap::funnel_radius_mm > 0.
         * Funnels are usual not hidden at all (@ref trap::visibility == 0), are @ref trap::benign and can
         * be picked up easily (@ref trap::difficulty == 0).
         * The funnel filling is handled in weather.cpp. is_funnel is used the check whether the
         * funnel specific code should be run for this trap.
         */", args = { } },
            { name = "is_null", rval = "bool", comment = "/**
         * Whether this is the null-traps, aka no trap at all.
         */", args = { } },
            { name = "on_disarmed", rval = nil, comment = "/**
         * Called when a trap at the given point in the main map has been disarmed.
         * It should spawn trap items (if any) and remove the trap from the map via
         * @ref map::remove_trap.
         */", args = { "tripoint" } },
            { name = "trigger", rval = nil, comment = "/**
         * Trigger trap effects.
         * @param creature The creature that triggered the trap, it does not necessarily have to
         * be on the place of the trap (traps can be triggered from adjacent, e.g. when disarming
         * them). This can also be a null pointer if the trap has been triggered by some thrown
         * item (which must have the @ref trigger_weight).
         * @param pos The location of the trap in the main map.
         */", args = { "tripoint", "Creature" } },
            { name = "triggered_by_item", rval = "bool", comment = "/**
         * If the given item is throw onto the trap, does it trigger the trap?
         */", args = { "item" } },
        }
    },
    w_point = {
        attributes = {
            acidic = { type = "bool", writable = true },
            humidity = { type = "float", writable = true },
            pressure = { type = "float", writable = true },
            temperature = { type = "float", writable = true },
            windpower = { type = "float", writable = true },
        },
    },
}

enums = {
    body_part = {
        "bp_torso",
        "bp_head",
        "bp_eyes",
        "bp_mouth",
        "bp_arm_l",
        "bp_arm_r",
        "bp_hand_l",
        "bp_hand_r",
        "bp_leg_l",
        "bp_leg_r",
        "bp_foot_l",
        "bp_foot_r",
        "num_bp",
    },
    hp_part = {
        "hp_head",
        "hp_torso",
        "hp_arm_l",
        "hp_arm_r",
        "hp_leg_l",
        "hp_leg_r",
        "num_hp_parts",
    },
    phase_id = {
        "PNULL",
        "SOLID",
        "LIQUID",
        "GAS",
        "PLASMA",
    },
    m_size = {
        "MS_TINY",
        "MS_SMALL",
        "MS_MEDIUM",
        "MS_LARGE",
        "MS_HUGE",
    },
    game_message_type = {
        "m_good",
        "m_bad",
        "m_mixed",
        "m_warning",
        "m_info",
        "m_neutral",
        "m_debug",
        "m_headshot",
        "m_critical",
        "m_grazing",
    },
    season_type = {
        "SPRING",
        "SUMMER",
        "AUTUMN",
        "WINTER",
    },
    add_type = {
        "ADD_NULL",
        "ADD_CAFFEINE",
        "ADD_ALCOHOL",
        "ADD_SLEEP",
        "ADD_PKILLER",
        "ADD_SPEED",
        "ADD_CIG",
        "ADD_COKE",
        "ADD_CRACK",
        "ADD_MUTAGEN",
        "ADD_DIAZEPAM",
        "ADD_MARLOSS_R",
        "ADD_MARLOSS_B",
        "ADD_MARLOSS_Y",
    },
    field_id = {
        "fd_null",
        "fd_blood",
        "fd_bile",
        "fd_gibs_flesh",
        "fd_gibs_veggy",
        "fd_web",
        "fd_slime",
        "fd_acid",
        "fd_sap",
        "fd_sludge",
        "fd_fire",
        "fd_rubble",
        "fd_smoke",
        "fd_toxic_gas",
        "fd_tear_gas",
        "fd_nuke_gas",
        "fd_gas_vent",
        "fd_fire_vent",
        "fd_flame_burst",
        "fd_electricity",
        "fd_fatigue",
        "fd_push_items",
        "fd_shock_vent",
        "fd_acid_vent",
        "fd_plasma",
        "fd_laser",
        "fd_spotlight",
        "fd_dazzling",
        "fd_blood_veggy",
        "fd_blood_insect",
        "fd_blood_invertebrate",
        "fd_gibs_insect",
        "fd_gibs_invertebrate",
        "fd_cigsmoke",
        "fd_weedsmoke",
        "fd_cracksmoke",
        "fd_methsmoke",
        "fd_bees",
        "fd_incendiary",
        "fd_relax_gas",
        "fd_fungal_haze",
        "fd_hot_air1",
        "fd_hot_air2",
        "fd_hot_air3",
        "fd_hot_air4",
        "fd_fungicidal_gas",
        "num_fields",
    },
}

make_list_class("item")
make_set_class("matec_id")
make_set_class("material_id")
make_set_class("species_id")
make_set_class("std::string")
make_set_class("tripoint")
make_vector_class("body_part")
make_vector_class("item")
make_vector_class("mabuff_id")
make_vector_class("matec_id")
make_vector_class("material_id")
make_vector_class("matype_id")
make_vector_class("point")
make_vector_class("std::string")
make_vector_class("tripoint")
