#include "look_around.h"

#include "game.h"
#include "monster.h"
#include "npc.h"
#include "map.h"
#include "item.h"
#include "player.h"
#include "options.h"
#include "input.h"
#include "output.h"
#include "mapsharing.h"
#include "debug.h"
#include "messages.h"
#include "map_item_stack.h"
#include "line.h"
#include "uistate.h"
#include "safemode_ui.h"
#include "string_input_popup.h"
#include "game_inventory.h"

#include <map>

#define dbg(x) DebugLog((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

static const efftype_id effect_boomered( "boomered" );
extern const int LOOK_AROUND_HEIGHT;
bool is_valid_in_w_terrain( int x, int y );

tripoint look_around_t::invoke()
{
    bVMonsterLookFire = false;

    //@todo use those directly.
    map &m = g->m;
    player &u = g->u;
    catacurses::WINDOW *const w_terrain = g->w_terrain;


    // TODO: Make this `true`
    const bool allow_zlev_move = m.has_zlevels() && ( debug_mode || fov_3d ||
                                 u.has_trait( trait_id( "DEBUG_NIGHTVISION" ) ) );

    g->temp_exit_fullscreen();

    const int offset_x = ( u.posx() + u.view_offset.x ) - getmaxx( w_terrain ) / 2;
    const int offset_y = ( u.posy() + u.view_offset.y ) - getmaxy( w_terrain ) / 2;

    tripoint lp = u.pos() + u.view_offset;
    int &lx = lp.x;
    int &ly = lp.y;
    int &lz = lp.z;

    if( select_zone && has_first_point ) {
        lp = start_point;
    }

    g->draw_ter( lp );

    //change player location to peek location temporarily for minimap update
    tripoint current_pos = u.pos();
    u.setpos( lp );
    g->draw_pixel_minimap();
    u.setpos( current_pos );

    int soffset = get_option<int>( "MOVE_VIEW_OFFSET" );
    bool fast_scroll = false;
    bool blink = false;

    int lookWidth, lookY, lookX;
    g->get_lookaround_dimensions( lookWidth, lookY, lookX );

    bool bNewWindow = false;
    if( w_info == nullptr ) {
        w_info = catacurses::newwin( LOOK_AROUND_HEIGHT, lookWidth, lookY, lookX );
        bNewWindow = true;
    }

    dbg( D_PEDANTIC_INFO ) << ": calling handle_input()";

    std::string action;
    input_context ctxt( "LOOK" );
    ctxt.set_iso( true );
    ctxt.register_directions();
    ctxt.register_action( "COORDINATE" );
    ctxt.register_action( "LEVEL_UP" );
    ctxt.register_action( "LEVEL_DOWN" );
    ctxt.register_action( "TOGGLE_FAST_SCROLL" );
    ctxt.register_action( "EXTENDED_DESCRIPTION" );
    if( select_zone ) {
        ctxt.register_action( "SELECT" );
    } else {
        ctxt.register_action( "TRAVEL_TO" );
        ctxt.register_action( "LIST_ITEMS" );
        ctxt.register_action( "MOUSE_MOVE" );
    }

    ctxt.register_action( "debug_scent" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    const int old_levz = g->get_levz();

    m.update_visibility_cache( old_levz );
    const visibility_variables &cache = m.get_visibility_variables_cache();

    do {
        if( bNewWindow ) {
            werase( w_info );
            draw_border( w_info );

            if( !select_zone ) {
                nc_color clr = c_white;
                std::string colored_key = string_format( "<color_light_green>%s</color>",
                                          ctxt.get_desc( "EXTENDED_DESCRIPTION", 1 ).c_str() );
                print_colored_text( w_info, getmaxy( w_info ) - 2, 2, clr, clr,
                                    string_format( _( "Press %s to view extended description" ), colored_key.c_str() ) );
                colored_key = string_format( "<color_light_green>%s</color>", ctxt.get_desc( "LIST_ITEMS",
                                             1 ).c_str() );
                print_colored_text( w_info, getmaxy( w_info ) - 1, 2, clr, clr,
                                    string_format( _( "Press %s to list items and monsters" ), colored_key.c_str() ) );
            }
        }

        if( select_zone ) {
            //Select Zone
            if( has_first_point ) {
                blink = !blink;

                const int dx = start_point.x - offset_x + u.posx() - lx;
                const int dy = start_point.y - offset_y + u.posy() - ly;

                if( blink ) {
                    const tripoint start = tripoint( std::min( dx, POSX ), std::min( dy, POSY ), lz );
                    const tripoint end = tripoint( std::max( dx, POSX ), std::max( dy, POSY ), lz );

                    tripoint offset = tripoint( 0, 0, 0 ); //ASCII/SDL
#ifdef TILES
                    if( use_tiles ) {
                        offset = tripoint( offset_x + lx - u.posx(), offset_y + ly - u.posy(), 0 ); //TILES
                    }
#endif

                    g->draw_zones( start, end, offset );

                } else {
#ifdef TILES
                    if( !use_tiles ) {
#endif
                        for( int iY = std::min( start_point.y, ly ); iY <= std::max( start_point.y, ly ); ++iY ) {
                            for( int iX = std::min( start_point.x, lx ); iX <= std::max( start_point.x, lx ); ++iX ) {
                                if( u.sees( iX, iY ) ) {
                                    m.drawsq( w_terrain, u, tripoint( iX, iY, lp.z ), false, false, tripoint( lx, ly, u.posz() ) );
                                } else {
                                    if( u.has_effect( effect_boomered ) ) {
                                        mvwputch( w_terrain, iY - offset_y - ly + u.posy(), iX - offset_x - lx + u.posx(), c_magenta, '#' );

                                    } else {
                                        mvwputch( w_terrain, iY - offset_y - ly + u.posy(), iX - offset_x - lx + u.posx(), c_black, ' ' );
                                    }
                                }
                            }
                        }
#ifdef TILES
                    }
#endif
                }

                //Draw first point
                mvwputch_inv( w_terrain, dy, dx, c_light_green, 'X' );
            }

            //Draw select cursor
            mvwputch_inv( w_terrain, POSY, POSX, c_light_green, 'X' );

        } else {
            //Look around
            int first_line = 1;
            const int last_line = LOOK_AROUND_HEIGHT - 2;
            g->print_all_tile_info( lp, w_info, 1, first_line, last_line, !is_draw_tiles_mode(), cache );

            if( fast_scroll ) {
                //~ "Fast Scroll" mark below the top right corner of the info window
                right_print( w_info, 1, 0, c_light_green, _( "F" ) );
            }

            wrefresh( w_info );
        }

        if( !is_draw_tiles_mode() && action != "MOUSE_MOVE" ) {
            // When tiles are disabled, this refresh is required to update the
            // selected terrain square with highlighted ascii coloring. When
            // tiles are enabled, the selected square isn't highlighted using
            // this function, and it is too CPU-intensive to call repeatedly
            // in a mouse event loop. If we did want to highlight the tile
            // selected by the mouse, we could call wrefresh when the mouse
            // hovered over a new tile (rather than on every mouse move
            // event).
            wrefresh( w_terrain );
        }

        if( select_zone && has_first_point ) {
            inp_mngr.set_timeout( BLINK_SPEED );
        }

        //Wait for input
        action = ctxt.handle_input();
        if( action == "LIST_ITEMS" ) {
            list_items_monsters();
            g->draw_ter( lp, true );

        } else if( action == "TOGGLE_FAST_SCROLL" ) {
            fast_scroll = !fast_scroll;
        } else if( action == "LEVEL_UP" || action == "LEVEL_DOWN" ) {
            if( !allow_zlev_move ) {
                continue;
            }

            int new_levz = lp.z + ( action == "LEVEL_UP" ? 1 : -1 );
            if( new_levz > OVERMAP_HEIGHT ) {
                new_levz = OVERMAP_HEIGHT;
            } else if( new_levz < -OVERMAP_DEPTH ) {
                new_levz = -OVERMAP_DEPTH;
            }

            add_msg( m_debug, "levx: %d, levy: %d, levz :%d", g->get_levx(), g->get_levy(), new_levz );
            u.view_offset.z = new_levz - u.posz();
            lp.z = new_levz;
            g->refresh_all();
            g->draw_ter( lp, true );
        } else if( action == "TRAVEL_TO" ) {
            if( !u.sees( lp ) ) {
                add_msg( _( "You can't see that destination." ) );
                continue;
            }

            auto route = m.route( u.pos(), lp, u.get_pathfinding_settings(), u.get_path_avoid() );
            if( route.size() > 1 ) {
                route.pop_back();
                u.set_destination( route );
            } else {
                add_msg( m_info, _( "You can't travel there." ) );
                continue;
            }
            return { INT_MIN, INT_MIN, INT_MIN };
        } else if( action == "debug_scent" ) {
            if( !MAP_SHARING::isCompetitive() || MAP_SHARING::isDebugger() ) {
                g->display_scent();
            }
        } else if( action == "EXTENDED_DESCRIPTION" ) {
            g->extended_description( lp );
            g->draw_sidebar();
            g->draw_ter( lp, true );
        } else if( !ctxt.get_coordinates( w_terrain, lx, ly ) && action != "MOUSE_MOVE" ) {
            int dx, dy;
            ctxt.get_direction( dx, dy, action );

            if( dx == -2 ) {
                dx = 0;
                dy = 0;
            } else {
                if( fast_scroll ) {
                    dx *= soffset;
                    dy *= soffset;
                }
            }

            lx += dx;
            ly += dy;

            //Keep cursor inside the reality bubble
            if( lx < 0 ) {
                lx = 0;
            } else if( lx > MAPSIZE * SEEX ) {
                lx = MAPSIZE * SEEX;
            }

            if( ly < 0 ) {
                ly = 0;
            } else if( ly > MAPSIZE * SEEY ) {
                ly = MAPSIZE * SEEY;
            }

            g->draw_ter( lp, true );
        }
    } while( action != "QUIT" && action != "CONFIRM" && action != "SELECT" );

    if( m.has_zlevels() && lp.z != old_levz ) {
        m.build_map_cache( old_levz );
        u.view_offset.z = 0;
    }

    inp_mngr.reset_timeout();

    if( bNewWindow ) {
        werase( w_info );
        delwin( w_info );
    }
    g->reenter_fullscreen();
    bVMonsterLookFire = true;

    if( action == "CONFIRM" || action == "SELECT" ) {
        return lp;
    }

    return tripoint( INT_MIN, INT_MIN, INT_MIN );
}

std::vector<map_item_stack> look_around_t::find_nearby_items( int iRadius )
{
    std::map<std::string, map_item_stack> temp_items;
    std::vector<map_item_stack> ret;
    std::vector<std::string> item_order;

    player &u = g->u;
    map &m = g->m;

    if( u.is_blind() ) {
        return ret;
    }

    std::vector<tripoint> points = closest_tripoints_first( iRadius, u.pos() );

    tripoint last_pos;

    for( auto &points_p_it : points ) {
        if( points_p_it.y >= u.posy() - iRadius && points_p_it.y <= u.posy() + iRadius &&
            u.sees( points_p_it ) &&
            m.sees_some_items( points_p_it, u ) ) {

            for( auto &elem : m.i_at( points_p_it ) ) {
                const std::string name = elem.tname();
                const tripoint relative_pos = points_p_it - u.pos();

                if( std::find( item_order.begin(), item_order.end(), name ) == item_order.end() ) {
                    item_order.push_back( name );
                    temp_items[name] = map_item_stack( &elem, relative_pos );
                } else {
                    temp_items[name].add_at_pos( &elem, relative_pos );
                }
            }
        }
    }

    for( auto &elem : item_order ) {
        ret.push_back( temp_items[elem] );
    }

    return ret;
}

void look_around_t::reset_item_list_state( WINDOW *window, int height, bool bRadiusSort )
{
    const int width = use_narrow_sidebar() ? 45 : 55;
    for( int i = 1; i < TERMX; i++ ) {
        if( i < width ) {
            mvwputch( window, 0, i, c_light_gray, LINE_OXOX ); // -
            mvwputch( window, TERMY - height - 1 - VIEW_OFFSET_Y * 2, i, c_light_gray, LINE_OXOX ); // -
        }

        if( i < TERMY - height - VIEW_OFFSET_Y * 2 ) {
            mvwputch( window, i, 0, c_light_gray, LINE_XOXO ); // |
            mvwputch( window, i, width - 1, c_light_gray, LINE_XOXO ); // |
        }
    }

    mvwputch( window, 0, 0, c_light_gray, LINE_OXXO ); // |^
    mvwputch( window, 0, width - 1, c_light_gray, LINE_OOXX ); // ^|

    mvwputch( window, TERMY - height - 1 - VIEW_OFFSET_Y * 2, 0, c_light_gray, LINE_XXXO ); // |-
    mvwputch( window, TERMY - height - 1 - VIEW_OFFSET_Y * 2, width - 1, c_light_gray,
              LINE_XOXX ); // -|

    mvwprintz( window, 0, 2, c_light_green, "<Tab> " );
    wprintz( window, c_white, _( "Items" ) );

    std::string sSort;
    if( bRadiusSort ) {
        //~ Sort type: distance.
        sSort = _( "<s>ort: dist" );
    } else {
        //~ Sort type: category.
        sSort = _( "<s>ort: cat" );
    }

    int letters = utf8_width( sSort );

    shortcut_print( window, 0, getmaxx( window ) - letters, c_white, c_light_green, sSort );

    std::vector<std::string> tokens;
    if( !sFilter.empty() ) {
        tokens.emplace_back( _( "<R>eset" ) );
    }

    tokens.emplace_back( _( "<E>xamine" ) );
    tokens.emplace_back( _( "<C>ompare" ) );
    tokens.emplace_back( _( "<F>ilter" ) );
    tokens.emplace_back( _( "<+/->Priority" ) );

    int gaps = tokens.size() + 1;
    letters = 0;
    int n = tokens.size();
    for( int i = 0; i < n; i++ ) {
        letters += utf8_width( tokens[i] ) - 2; //length ignores < >
    }

    int usedwidth = letters;
    const int gap_spaces = ( width - usedwidth ) / gaps;
    usedwidth += gap_spaces * gaps;
    int xpos = gap_spaces + ( width - usedwidth ) / 2;
    const int ypos = TERMY - height - 1 - VIEW_OFFSET_Y * 2;

    for( int i = 0; i < n; i++ ) {
        xpos += shortcut_print( window, ypos, xpos, c_white, c_light_green, tokens[i] ) + gap_spaces;
    }

    g->refresh_all();
}

void centerlistview( const tripoint &active_item_position )
{
    player &u = g->u;
    if( get_option<std::string>( "SHIFT_LIST_ITEM_VIEW" ) != "false" ) {
        u.view_offset.z = active_item_position.z;
        int xpos = POSX + active_item_position.x;
        int ypos = POSY + active_item_position.y;
        if( get_option<std::string>( "SHIFT_LIST_ITEM_VIEW" ) == "centered" ) {
            int xOffset = TERRAIN_WINDOW_WIDTH / 2;
            int yOffset = TERRAIN_WINDOW_HEIGHT / 2;
            if( !is_valid_in_w_terrain( xpos, ypos ) ) {
                if( xpos < 0 ) {
                    u.view_offset.x = xpos - xOffset;
                } else {
                    u.view_offset.x = xpos - ( TERRAIN_WINDOW_WIDTH - 1 ) + xOffset;
                }

                if( xpos < 0 ) {
                    u.view_offset.y = ypos - yOffset;
                } else {
                    u.view_offset.y = ypos - ( TERRAIN_WINDOW_HEIGHT - 1 ) + yOffset;
                }
            } else {
                u.view_offset.x = 0;
                u.view_offset.y = 0;
            }
        } else {
            if( xpos < 0 ) {
                u.view_offset.x = xpos;
            } else if( xpos >= TERRAIN_WINDOW_WIDTH ) {
                u.view_offset.x = xpos - ( TERRAIN_WINDOW_WIDTH - 1 );
            } else {
                u.view_offset.x = 0;
            }

            if( ypos < 0 ) {
                u.view_offset.y = ypos;
            } else if( ypos >= TERRAIN_WINDOW_HEIGHT ) {
                u.view_offset.y = ypos - ( TERRAIN_WINDOW_HEIGHT - 1 );
            } else {
                u.view_offset.y = 0;
            }
        }
    }

}

void look_around_t::list_items_monsters()
{
    player &u = g->u;

    std::vector<Creature *> mons = u.get_visible_creatures( DAYLIGHT_LEVEL );
    ///\EFFECT_PER increases range of interacting with items on the ground from a list
    const std::vector<map_item_stack> items = find_nearby_items( 2 * u.per_cur + 12 );

    if( mons.empty() && items.empty() ) {
        add_msg( m_info, _( "You don't see any items or monsters around you!" ) );
        return;
    }

    std::sort( mons.begin(), mons.end(), [&]( const Creature * lhs, const Creature * rhs ) {
        const auto att_lhs = lhs->attitude_to( u );
        const auto att_rhs = rhs->attitude_to( u );

        return att_lhs < att_rhs || ( att_lhs == att_rhs &&
                                      rl_dist( u.pos(), lhs->pos() ) < rl_dist( u.pos(), rhs->pos() ) );
    } );

    // If the current list is empty, switch to the non-empty list
    if( uistate.vmenu_show_items ) {
        if( items.empty() ) {
            uistate.vmenu_show_items = false;
        }
    } else if( mons.empty() ) {
        uistate.vmenu_show_items = true;
    }

    g->temp_exit_fullscreen();
    vmenu_ret ret;
    while( true ) {
        ret = uistate.vmenu_show_items ? list_items( items ) : list_monsters( mons );
        if( ret == vmenu_ret::CHANGE_TAB ) {
            uistate.vmenu_show_items = !uistate.vmenu_show_items;
        } else {
            break;
        }
    }

    g->refresh_all();
    if( ret == vmenu_ret::FIRE ) {
        g->plfire( u.weapon );
    }
    g->reenter_fullscreen();
}

look_around_t::vmenu_ret look_around_t::list_items( const std::vector<map_item_stack> &item_list )
{
    player &u = g->u;

    int iInfoHeight = std::min( 25, TERMY / 2 );
    const int width = use_narrow_sidebar() ? 45 : 55;
    const int offsetX = g->right_sidebar ? TERMX - VIEW_OFFSET_X - width : VIEW_OFFSET_X;

    catacurses::window w_items = catacurses::newwin( TERMY - 2 - iInfoHeight - VIEW_OFFSET_Y * 2,
                                 width - 2, VIEW_OFFSET_Y + 1, offsetX + 1 );
    WINDOW_PTR w_itemsptr( w_items );

    catacurses::window w_items_border = catacurses::newwin( TERMY - iInfoHeight - VIEW_OFFSET_Y * 2,
                                        width, VIEW_OFFSET_Y, offsetX );
    WINDOW_PTR w_items_borderptr( w_items_border );

    catacurses::window w_item_info = catacurses::newwin( iInfoHeight, width,
                                     TERMY - iInfoHeight - VIEW_OFFSET_Y, offsetX );
    WINDOW_PTR w_item_infoptr( w_item_info );

    // use previously selected sorting method
    bool sort_radius = uistate.list_item_sort != 2;
    bool addcategory = !sort_radius;

    // reload filter/priority settings on the first invocation, if they were active
    if( !uistate.list_item_init ) {
        if( uistate.list_item_filter_active ) {
            sFilter = uistate.list_item_filter;
        }
        if( uistate.list_item_downvote_active ) {
            list_item_downvote = uistate.list_item_downvote;
        }
        if( uistate.list_item_priority_active ) {
            list_item_upvote = uistate.list_item_priority;
        }
        uistate.list_item_init = true;
    }

    std::vector<map_item_stack> ground_items = item_list;
    //this stores only those items that match our filter
    std::vector<map_item_stack> filtered_items = !sFilter.empty() ? filter_item_stacks( ground_items,
            sFilter ) : ground_items;
    int highPEnd = list_filter_high_priority( filtered_items, list_item_upvote );
    int lowPStart = list_filter_low_priority( filtered_items, highPEnd, list_item_downvote );
    int iItemNum = ground_items.size();

    const tripoint stored_view_offset = u.view_offset;

    u.view_offset = tripoint_zero;

    int iActive = 0; // Item index that we're looking at
    const int iMaxRows = TERMY - iInfoHeight - 2 - VIEW_OFFSET_Y * 2;
    int iStartPos = 0;
    tripoint active_pos;
    tripoint iLastActive = tripoint_min;
    bool reset = true;
    bool refilter = true;
    int page_num = 0;
    int iCatSortNum = 0;
    int iScrollPos = 0;
    map_item_stack *activeItem = nullptr;
    std::map<int, std::string> mSortCategory;

    std::string action;
    input_context ctxt( "LIST_ITEMS" );
    ctxt.register_action( "UP", _( "Move cursor up" ) );
    ctxt.register_action( "DOWN", _( "Move cursor down" ) );
    ctxt.register_action( "LEFT", _( "Previous item" ) );
    ctxt.register_action( "RIGHT", _( "Next item" ) );
    ctxt.register_action( "PAGE_DOWN" );
    ctxt.register_action( "PAGE_UP" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "FILTER" );
    ctxt.register_action( "RESET_FILTER" );
    ctxt.register_action( "EXAMINE" );
    ctxt.register_action( "COMPARE" );
    ctxt.register_action( "PRIORITY_INCREASE" );
    ctxt.register_action( "PRIORITY_DECREASE" );
    ctxt.register_action( "SORT" );
    ctxt.register_action( "TRAVEL_TO" );

    do {
        if( action == "COMPARE" ) {
            game_menus::inv::compare( u, active_pos );
            reset = true;
            g->refresh_all();
        } else if( action == "FILTER" ) {
            draw_item_filter_rules( w_item_info, 0, iInfoHeight - 1, item_filter_type::FILTER );
            string_input_popup()
            .title( _( "Filter:" ) )
            .width( 55 )
            .description( _( "UP: history, CTRL-U: clear line, ESC: abort, ENTER: save" ) )
            .identifier( "item_filter" )
            .max_length( 256 )
            .edit( sFilter );
            reset = true;
            refilter = true;
            addcategory = !sort_radius;
            uistate.list_item_filter_active = !sFilter.empty();
        } else if( action == "RESET_FILTER" ) {
            sFilter.clear();
            filtered_items = ground_items;
            iLastActive = tripoint_min;
            reset = true;
            refilter = true;
            uistate.list_item_filter_active = false;
            addcategory = !sort_radius;
        } else if( action == "EXAMINE" && !filtered_items.empty() ) {
            std::vector<iteminfo> vThisItem, vDummy;
            int dummy = 0; // draw_item_info needs an int &
            activeItem->example->info( true, vThisItem );
            draw_item_info( 0, width - 5, 0, TERMY - VIEW_OFFSET_Y * 2, activeItem->example->tname(),
                            activeItem->example->type_name(), vThisItem, vDummy, dummy, false, false, true );
            // wait until the user presses a key to wipe the screen
            iLastActive = tripoint_min;
            reset = true;
        } else if( action == "PRIORITY_INCREASE" ) {
            draw_item_filter_rules( w_item_info, 0, iInfoHeight - 1, item_filter_type::HIGH_PRIORITY );
            list_item_upvote = string_input_popup()
                               .title( _( "High Priority:" ) )
                               .width( 55 )
                               .text( list_item_upvote )
                               .description( _( "UP: history, CTRL-U clear line, ESC: abort, ENTER: save" ) )
                               .identifier( "list_item_priority" )
                               .max_length( 256 )
                               .query_string();
            refilter = true;
            reset = true;
            addcategory = !sort_radius;
            uistate.list_item_priority_active = !list_item_upvote.empty();
        } else if( action == "PRIORITY_DECREASE" ) {
            draw_item_filter_rules( w_item_info, 0, iInfoHeight - 1, item_filter_type::LOW_PRIORITY );
            list_item_downvote = string_input_popup()
                                 .title( _( "Low Priority:" ) )
                                 .width( 55 )
                                 .text( list_item_downvote )
                                 .description( _( "UP: history, CTRL-U clear line, ESC: abort, ENTER: save" ) )
                                 .identifier( "list_item_downvote" )
                                 .max_length( 256 )
                                 .query_string();
            refilter = true;
            reset = true;
            addcategory = !sort_radius;
            uistate.list_item_downvote_active = !list_item_downvote.empty();
        } else if( action == "SORT" ) {
            if( sort_radius ) {
                sort_radius = false;
                addcategory = true;
                uistate.list_item_sort = 2; // list is sorted by category
            } else {
                sort_radius = true;
                uistate.list_item_sort = 1; // list is sorted by distance
            }
            highPEnd = -1;
            lowPStart = -1;
            iCatSortNum = 0;

            mSortCategory.clear();
            refilter = true;
            reset = true;
        } else if( action == "TRAVEL_TO" ) {
            if( !u.sees( u.pos() + active_pos ) ) {
                add_msg( _( "You can't see that destination." ) );
            }
            auto route = g->m.route( u.pos(), u.pos() + active_pos, u.get_pathfinding_settings(),
                                     u.get_path_avoid() );
            if( route.size() > 1 ) {
                route.pop_back();
                u.set_destination( route );
                break;
            } else {
                add_msg( m_info, _( "You can't travel there." ) );
            }
        }
        if( uistate.list_item_sort == 1 ) {
            ground_items = item_list;
        } else if( uistate.list_item_sort == 2 ) {
            std::sort( ground_items.begin(), ground_items.end(), map_item_stack::map_item_stack_sort );
        }

        if( refilter ) {
            refilter = false;
            filtered_items = filter_item_stacks( ground_items, sFilter );
            highPEnd = list_filter_high_priority( filtered_items, list_item_upvote );
            lowPStart = list_filter_low_priority( filtered_items, highPEnd, list_item_downvote );
            iActive = 0;
            page_num = 0;
            iLastActive = tripoint_min;
            iItemNum = filtered_items.size();
        }

        if( addcategory ) {
            addcategory = false;
            iCatSortNum = 0;
            mSortCategory.clear();
            if( highPEnd > 0 ) {
                mSortCategory[0] = _( "HIGH PRIORITY" );
                iCatSortNum++;
            }
            std::string last_cat_name;
            for( int i = std::max( 0, highPEnd ); i < std::min( lowPStart, ( int )filtered_items.size() );
                 i++ ) {
                const std::string &cat_name = filtered_items[i].example->get_category().name;
                if( cat_name != last_cat_name ) {
                    mSortCategory[i + iCatSortNum++] = cat_name;
                    last_cat_name = cat_name;
                }
            }
            if( lowPStart < ( int )filtered_items.size() ) {
                mSortCategory[lowPStart + iCatSortNum++] = _( "LOW PRIORITY" );
            }
            if( !mSortCategory[0].empty() ) {
                iActive++;
            }
            iItemNum = int( filtered_items.size() ) + iCatSortNum;
        }

        if( reset ) {
            reset_item_list_state( w_items_border, iInfoHeight, sort_radius );
            reset = false;
            iScrollPos = 0;
        }

        if( action == "UP" ) {
            do {
                iActive--;
            } while( !mSortCategory[iActive].empty() );
            iScrollPos = 0;
            page_num = 0;
            if( iActive < 0 ) {
                iActive = iItemNum - 1;
            }
        } else if( action == "DOWN" ) {
            do {
                iActive++;
            } while( !mSortCategory[iActive].empty() );
            iScrollPos = 0;
            page_num = 0;
            if( iActive >= iItemNum ) {
                iActive = mSortCategory[0].empty() ? 0 : 1;
            }
        } else if( action == "RIGHT" ) {
            if( !filtered_items.empty() && ++page_num >= ( int )activeItem->vIG.size() ) {
                page_num = activeItem->vIG.size() - 1;
            }
        } else if( action == "LEFT" ) {
            page_num = std::max( 0, page_num - 1 );
        } else if( action == "PAGE_UP" ) {
            iScrollPos--;
        } else if( action == "PAGE_DOWN" ) {
            iScrollPos++;
        } else if( action == "NEXT_TAB" || action == "PREV_TAB" ) {
            u.view_offset = stored_view_offset;
            return vmenu_ret::CHANGE_TAB;
        }

        if( ground_items.empty() ) {
            reset_item_list_state( w_items_border, iInfoHeight, sort_radius );
            wrefresh( w_items_border );
            mvwprintz( w_items, 10, 2, c_white, _( "You don't see any items around you!" ) );
        } else {
            werase( w_items );
            calcStartPos( iStartPos, iActive, iMaxRows, iItemNum );
            int iNum = 0;
            active_pos = tripoint_zero;
            bool high = false;
            bool low = false;
            int index = 0;
            int iCatSortOffset = 0;

            for( int i = 0; i < iStartPos; i++ ) {
                if( !mSortCategory[i].empty() ) {
                    iNum++;
                }
            }
            for( auto iter = filtered_items.begin(); iter != filtered_items.end(); ++index ) {
                if( highPEnd > 0 && index < highPEnd + iCatSortOffset ) {
                    high = true;
                    low = false;
                } else if( index >= lowPStart + iCatSortOffset ) {
                    high = false;
                    low = true;
                } else {
                    high = false;
                    low = false;
                }

                if( iNum >= iStartPos && iNum < iStartPos + ( iMaxRows > iItemNum ? iItemNum : iMaxRows ) ) {
                    int iThisPage = 0;
                    if( !mSortCategory[iNum].empty() ) {
                        iCatSortOffset++;
                        mvwprintz( w_items, iNum - iStartPos, 1, c_magenta, "%s", mSortCategory[iNum].c_str() );
                    } else {
                        if( iNum == iActive ) {
                            iThisPage = page_num;
                            active_pos = iter->vIG[iThisPage].pos;
                            activeItem = &( *iter );
                        }
                        std::stringstream sText;
                        if( iter->vIG.size() > 1 ) {
                            sText << "[" << iThisPage + 1 << "/" << iter->vIG.size() << "] (" << iter->totalcount << ") ";
                        }
                        sText << iter->example->tname();
                        if( iter->vIG[iThisPage].count > 1 ) {
                            sText << " [" << iter->vIG[iThisPage].count << "]";
                        }

                        nc_color col = c_light_green;
                        if( iNum != iActive ) {
                            if( high ) {
                                col = c_yellow;
                            } else if( low ) {
                                col = c_red;
                            } else {
                                col = iter->example->color_in_inventory();
                            }
                        }
                        trim_and_print( w_items, iNum - iStartPos, 1, width - 9, col, "%s", sText.str().c_str() );
                        const int numw = iItemNum > 9 ? 2 : 1;
                        const int x = iter->vIG[iThisPage].pos.x;
                        const int y = iter->vIG[iThisPage].pos.y;
                        mvwprintz( w_items, iNum - iStartPos, width - 6 - numw,
                                   iNum == iActive ? c_light_green : c_light_gray, "%*d %s", numw, rl_dist( 0, 0, x, y ),
                                   direction_name_short( direction_from( 0, 0, x, y ) ).c_str() );
                        ++iter;
                    }
                } else {
                    ++iter;
                }
                iNum++;
            }
            iNum = 0;
            for( int i = 0; i < iActive; i++ ) {
                if( !mSortCategory[i].empty() ) {
                    iNum++;
                }
            }
            mvwprintz( w_items_border, 0, ( width - 9 ) / 2 + ( iItemNum > 9 ? 0 : 1 ), c_light_green, " %*d",
                       iItemNum > 9 ? 2 : 1, iItemNum > 0 ? iActive - iNum + 1 : 0 );
            wprintz( w_items_border, c_white, " / %*d ", iItemNum > 9 ? 2 : 1, iItemNum - iCatSortNum );
            werase( w_item_info );

            if( iItemNum > 0 ) {
                std::vector<iteminfo> vThisItem, vDummy;
                activeItem->example->info( true, vThisItem );
                draw_item_info( w_item_info, "", "", vThisItem, vDummy, iScrollPos, true, true );
                //Only redraw trail/terrain if x/y position changed
                if( active_pos != iLastActive ) {
                    iLastActive = active_pos;
                    centerlistview( active_pos );
                    g->draw_trail_to_square( active_pos, true );
                }
            }
            draw_scrollbar( w_items_border, iActive, iMaxRows, iItemNum, 1 );
            wrefresh( w_items_border );
        }

        const bool bDrawLeft = ground_items.empty() || filtered_items.empty();
        draw_custom_border( w_item_info, bDrawLeft, true, false, true, LINE_XOXO, LINE_XOXO, true, true );
        wrefresh( w_items );
        wrefresh( w_item_info );
        refresh();
        action = ctxt.handle_input();
    } while( action != "QUIT" );

    u.view_offset = stored_view_offset;
    return vmenu_ret::QUIT;
}

look_around_t::vmenu_ret look_around_t::list_monsters( const std::vector<Creature *> &monster_list )
{
    player &u = g->u;

    int iInfoHeight = 12;
    const int width = use_narrow_sidebar() ? 45 : 55;
    const int offsetX = g->right_sidebar ? TERMX - VIEW_OFFSET_X - width : VIEW_OFFSET_X;

    catacurses::window w_monsters = catacurses::newwin( TERMY - 2 - iInfoHeight - VIEW_OFFSET_Y * 2,
                                    width - 2, VIEW_OFFSET_Y + 1, offsetX + 1 );
    WINDOW_PTR w_monstersptr( w_monsters );
    catacurses::window w_monsters_border = catacurses::newwin( TERMY - iInfoHeight - VIEW_OFFSET_Y * 2,
                                           width, VIEW_OFFSET_Y, offsetX );
    WINDOW_PTR w_monsters_borderptr( w_monsters_border );
    catacurses::window w_monster_info = catacurses::newwin( iInfoHeight - 1, width - 2,
                                        TERMY - iInfoHeight - VIEW_OFFSET_Y, offsetX + 1 );
    WINDOW_PTR w_monster_infoptr( w_monster_info );
    catacurses::window w_monster_info_border = catacurses::newwin( iInfoHeight, width,
            TERMY - iInfoHeight - VIEW_OFFSET_Y, offsetX );
    WINDOW_PTR w_monster_info_borderptr( w_monster_info_border );

    const int max_gun_range = u.weapon.gun_range( &u );

    const tripoint stored_view_offset = u.view_offset;
    u.view_offset = tripoint_zero;

    int iActive = 0; // monster index that we're looking at
    const int iMaxRows = TERMY - iInfoHeight - 2 - VIEW_OFFSET_Y * 2 - 1;
    int iStartPos = 0;
    tripoint iActivePos;
    tripoint iLastActivePos = tripoint_min;
    Creature *cCurMon = nullptr;

    for( int i = 1; i < TERMX; i++ ) {
        if( i < width ) {
            mvwputch( w_monsters_border, 0, i, BORDER_COLOR, LINE_OXOX ); // -
            mvwputch( w_monsters_border, TERMY - iInfoHeight - 1 - VIEW_OFFSET_Y * 2, i, BORDER_COLOR,
                      LINE_OXOX ); // -
        }

        if( i < TERMY - iInfoHeight - VIEW_OFFSET_Y * 2 ) {
            mvwputch( w_monsters_border, i, 0, BORDER_COLOR, LINE_XOXO ); // |
            mvwputch( w_monsters_border, i, width - 1, BORDER_COLOR, LINE_XOXO ); // |
        }
    }

    mvwputch( w_monsters_border, 0, 0, BORDER_COLOR, LINE_OXXO ); // |^
    mvwputch( w_monsters_border, 0, width - 1, BORDER_COLOR, LINE_OOXX ); // ^|

    mvwputch( w_monsters_border, TERMY - iInfoHeight - 1 - VIEW_OFFSET_Y * 2, 0, BORDER_COLOR,
              LINE_XXXO ); // |-
    mvwputch( w_monsters_border, TERMY - iInfoHeight - 1 - VIEW_OFFSET_Y * 2, width - 1, BORDER_COLOR,
              LINE_XOXX ); // -|

    mvwprintz( w_monsters_border, 0, 2, c_light_green, "<Tab> " );
    wprintz( w_monsters_border, c_white, _( "Monsters" ) );

    std::string action;
    input_context ctxt( "LIST_MONSTERS" );
    ctxt.register_action( "UP", _( "Move cursor up" ) );
    ctxt.register_action( "DOWN", _( "Move cursor down" ) );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "SAFEMODE_BLACKLIST_ADD" );
    ctxt.register_action( "SAFEMODE_BLACKLIST_REMOVE" );
    ctxt.register_action( "QUIT" );
    if( bVMonsterLookFire ) {
        ctxt.register_action( "look" );
        ctxt.register_action( "fire" );
    }
    ctxt.register_action( "HELP_KEYBINDINGS" );


    // first integer is the row the attitude category string is printed in the menu
    std::map<int, Creature::Attitude> mSortCategory;

    for( int i = 0, last_attitude = -1; i < ( int )monster_list.size(); i++ ) {
        const auto attitude = monster_list[i]->attitude_to( u );
        if( attitude != last_attitude ) {
            mSortCategory[i + mSortCategory.size()] = attitude;
            last_attitude = attitude;
        }
    }

    do {
        if( action == "UP" ) {
            iActive--;
            if( iActive < 0 ) {
                iActive = int( monster_list.size() ) - 1;
            }
        } else if( action == "DOWN" ) {
            iActive++;
            if( iActive >= int( monster_list.size() ) ) {
                iActive = 0;
            }
        } else if( action == "NEXT_TAB" || action == "PREV_TAB" ) {
            u.view_offset = stored_view_offset;
            return vmenu_ret::CHANGE_TAB;
        } else if( action == "SAFEMODE_BLACKLIST_REMOVE" ) {
            const auto m = dynamic_cast<monster *>( cCurMon );
            const std::string monName = ( m != nullptr ) ? m->name() : "human";

            if( get_safemode().has_rule( monName, Creature::A_ANY ) ) {
                get_safemode().remove_rule( monName, Creature::A_ANY );
            }
        } else if( action == "SAFEMODE_BLACKLIST_ADD" ) {
            if( !get_safemode().empty() ) {
                const auto m = dynamic_cast<monster *>( cCurMon );
                const std::string monName = ( m != nullptr ) ? m->name() : "human";

                get_safemode().add_rule( monName, Creature::A_ANY, get_option<int>( "SAFEMODEPROXIMITY" ),
                                         RULE_BLACKLISTED );
            }
        } else if( action == "look" ) {
            tripoint recentered = g->look_around();
            iLastActivePos = recentered;
        } else if( action == "fire" ) {
            if( cCurMon != nullptr && rl_dist( u.pos(), cCurMon->pos() ) <= max_gun_range ) {
                g->last_target = g->shared_from( *cCurMon );
                u.view_offset = stored_view_offset;
                return vmenu_ret::FIRE;
            }
        }

        if( monster_list.empty() ) {
            wrefresh( w_monsters_border );
            mvwprintz( w_monsters, 10, 2, c_white, _( "You don't see any monsters around you!" ) );
        } else {
            werase( w_monsters );
            const int iNumMonster = monster_list.size();
            const int iMenuSize = monster_list.size() + mSortCategory.size();

            const int numw = iNumMonster > 999 ? 4 :
                             iNumMonster > 99  ? 3 :
                             iNumMonster > 9   ? 2 : 1;

            // given the currently selected monster iActive. get the selected row
            int iSelPos = iActive;
            for( auto &ia : mSortCategory ) {
                int index = ia.first;
                if( index <= iSelPos ) {
                    ++iSelPos;
                } else {
                    break;
                }
            }
            // use selected row get the start row
            calcStartPos( iStartPos, iSelPos, iMaxRows, iMenuSize );

            // get first visible monster and category
            int iCurMon = iStartPos;
            auto CatSortIter = mSortCategory.cbegin();
            while( CatSortIter != mSortCategory.cend() && CatSortIter->first < iStartPos ) {
                ++CatSortIter;
                --iCurMon;
            }

            const auto endY = std::min<int>( iMaxRows, iMenuSize );
            for( int y = 0; y < endY; ++y ) {
                if( CatSortIter != mSortCategory.cend() ) {
                    const int iCurPos = iStartPos + y;
                    const int iCatPos = CatSortIter->first;
                    if( iCurPos == iCatPos ) {
                        std::string const &cat_name = Creature::get_attitude_ui_data( CatSortIter->second ).first;
                        mvwprintz( w_monsters, y, 1, c_magenta, "%s", cat_name.c_str() );
                        ++CatSortIter;
                        continue;
                    }
                }
                // select current monster
                const auto critter = monster_list[iCurMon];
                const bool selected = iCurMon == iActive;
                ++iCurMon;
                if( critter->sees( g->u ) ) {
                    mvwprintz( w_monsters, y, 0, c_yellow, "!" );
                }
                bool is_npc = false;
                const monster *m = dynamic_cast<monster *>( critter );
                const npc     *p = dynamic_cast<npc *>( critter );

                if( m != nullptr ) {
                    mvwprintz( w_monsters, y, 1, selected ? c_light_green : c_white, "%s", m->name().c_str() );
                } else {
                    mvwprintz( w_monsters, y, 1, selected ? c_light_green : c_white, "%s",
                               critter->disp_name().c_str() );
                    is_npc = true;
                }

                if( selected && !get_safemode().empty() ) {
                    for( int i = 1; i < width - 2; i++ ) {
                        mvwputch( w_monsters_border, TERMY - iInfoHeight - 1 - VIEW_OFFSET_Y * 2, i, BORDER_COLOR,
                                  LINE_OXOX ); // -
                    }
                    const std::string monName = is_npc ? get_safemode().npc_type_name() : m->name();

                    std::string sSafemode;
                    if( get_safemode().has_rule( monName, Creature::A_ANY ) ) {
                        sSafemode = _( "<R>emove from safemode Blacklist" );
                    } else {
                        sSafemode = _( "<A>dd to safemode Blacklist" );
                    }

                    shortcut_print( w_monsters_border, TERMY - iInfoHeight - 1 - VIEW_OFFSET_Y * 2, 3, c_white,
                                    c_light_green, sSafemode );
                }

                nc_color color = c_white;
                std::string sText;

                if( m != nullptr ) {
                    m->get_HP_Bar( color, sText );
                } else {
                    std::tie( sText, color ) = ::get_hp_bar( critter->get_hp(), critter->get_hp_max(), false );
                }
                mvwprintz( w_monsters, y, 22, color, "%s", sText.c_str() );

                if( m != nullptr ) {
                    const auto att = m->get_attitude();
                    sText = att.first;
                    color = att.second;
                } else if( p != nullptr ) {
                    sText = npc_attitude_name( p->attitude );
                    color = p->symbol_color();
                }
                mvwprintz( w_monsters, y, 28, color, "%s", sText.c_str() );

                mvwprintz( w_monsters, y, width - ( 6 + numw ), ( selected ? c_light_green : c_light_gray ),
                           "%*d %s", numw, rl_dist( u.pos(), critter->pos() ), direction_name_short( direction_from( u.pos(),
                                   critter->pos() ) ).c_str() );
            }

            mvwprintz( w_monsters_border, 0, ( width / 2 ) - numw - 2, c_light_green, " %*d", numw,
                       iActive + 1 );
            wprintz( w_monsters_border, c_white, " / %*d ", numw, int( monster_list.size() ) );

            cCurMon = monster_list[iActive];

            werase( w_monster_info );
            cCurMon->print_info( w_monster_info, 1, 11, 1 );

            if( bVMonsterLookFire ) {
                mvwprintz( w_monsters, getmaxy( w_monsters ) - 1, 1, c_light_green, "%s",
                           ctxt.press_x( "look" ).c_str() );
                wprintz( w_monsters, c_light_gray, " %s", _( "to look around" ) );

                if( rl_dist( u.pos(), cCurMon->pos() ) <= max_gun_range ) {
                    wprintz( w_monsters, c_light_gray, "%s", " " );
                    wprintz( w_monsters, c_light_green, "%s", ctxt.press_x( "fire" ).c_str() );
                    wprintz( w_monsters, c_light_gray, " %s", _( "to shoot" ) );
                }
            }

            //Only redraw trail/terrain if x/y position changed
            iActivePos = cCurMon->pos() - u.pos();
            if( iActivePos != iLastActivePos ) {
                iLastActivePos = iActivePos;
                centerlistview( iActivePos );
                g->draw_trail_to_square( iActivePos, false );
            }

            draw_scrollbar( w_monsters_border, iActive, iMaxRows, int( monster_list.size() ), 1 );
            wrefresh( w_monsters_border );
        }

        for( int j = 0; j < iInfoHeight - 1; j++ ) {
            mvwputch( w_monster_info_border, j, 0, c_light_gray, LINE_XOXO );
            mvwputch( w_monster_info_border, j, width - 1, c_light_gray, LINE_XOXO );
        }

        for( int j = 0; j < width - 1; j++ ) {
            mvwputch( w_monster_info_border, iInfoHeight - 1, j, c_light_gray, LINE_OXOX );
        }

        mvwputch( w_monster_info_border, iInfoHeight - 1, 0, c_light_gray, LINE_XXOO );
        mvwputch( w_monster_info_border, iInfoHeight - 1, width - 1, c_light_gray, LINE_XOOX );

        wrefresh( w_monsters );
        wrefresh( w_monster_info_border );
        wrefresh( w_monster_info );

        refresh();

        action = ctxt.handle_input();
    } while( action != "QUIT" );

    u.view_offset = stored_view_offset;

    return vmenu_ret::QUIT;
}
