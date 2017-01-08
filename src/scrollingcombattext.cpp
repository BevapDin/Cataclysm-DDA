#include "scrollingcombattext.h"

#include "line.h"
#include "rng.h"
#include "options.h"

scrollingcombattext SCT;

scrollingcombattext::cSCT::cSCT( const int p_iPosX, const int p_iPosY, const direction p_oDir,
                                 const std::string p_sText, const game_message_type p_gmt,
                                 const std::string p_sText2, const game_message_type p_gmt2,
                                 const std::string p_sType )
{
    iPosX = p_iPosX;
    iPosY = p_iPosY;
    sType = p_sType;
    oDir = p_oDir;

    // translate from player relative to screen relative direction
    iso_mode = false;
#ifdef TILES
    iso_mode = tile_iso && use_tiles;
#endif
    oUp = iso_mode ? NORTHEAST : NORTH;
    oUpRight = iso_mode ? EAST : NORTHEAST;
    oRight = iso_mode ? SOUTHEAST : EAST;
    oDownRight = iso_mode ? SOUTH : SOUTHEAST;
    oDown = iso_mode ? SOUTHWEST : SOUTH;
    oDownLeft = iso_mode ? WEST : SOUTHWEST;
    oLeft = iso_mode ? NORTHWEST : WEST;
    oUpLeft = iso_mode ? NORTH : NORTHWEST;

    point pairDirXY = direction_XY( oDir );

    iDirX = pairDirXY.x;
    iDirY = pairDirXY.y;

    if( iDirX == 0 && iDirY == 0 ) {
        // This would cause infinite loop otherwise
        oDir = WEST;
        iDirX = -1;
    }

    iStep = 0;
    iStepOffset = 0;

    sText = p_sText;
    gmt = p_gmt;

    sText2 = p_sText2;
    gmt2 = p_gmt2;

}

void scrollingcombattext::add( const int p_iPosX, const int p_iPosY, direction p_oDir,
                               const std::string p_sText, const game_message_type p_gmt,
                               const std::string p_sText2, const game_message_type p_gmt2,
                               const std::string p_sType )
{
    if( get_option<bool>( "ANIMATION_SCT" ) ) {

        int iCurStep = 0;

        bool tiled = false;
        bool iso_mode = false;
#ifdef TILES
        tiled = use_tiles;
        iso_mode = tile_iso && use_tiles;
#endif

        if( p_sType == "hp" ) {
            //Remove old HP bar
            removeCreatureHP();

            if( p_oDir == WEST || p_oDir == NORTHWEST || p_oDir == ( iso_mode ? NORTH : SOUTHWEST ) ) {
                p_oDir = ( iso_mode ? NORTHWEST : WEST );
            } else {
                p_oDir = ( iso_mode ? SOUTHEAST : EAST );
            }

        } else {
            //reserve Left/Right for creature hp display
            if( p_oDir == ( iso_mode ? SOUTHEAST : EAST ) ) {
                p_oDir = ( one_in( 2 ) ) ? ( iso_mode ? EAST : NORTHEAST ) : ( iso_mode ? SOUTH : SOUTHEAST );

            } else if( p_oDir == ( iso_mode ? NORTHWEST : WEST ) ) {
                p_oDir = ( one_in( 2 ) ) ? ( iso_mode ? NORTH : NORTHWEST ) : ( iso_mode ? WEST : SOUTHWEST );
            }
        }

        // in tiles, SCT that scroll downwards are inserted at the beginning of the vector to prevent
        // oversize ASCII tiles overdrawing messages below them.
        if( tiled && ( p_oDir == SOUTHWEST || p_oDir == SOUTH ||
                       p_oDir == ( iso_mode ? WEST : SOUTHEAST ) ) ) {

            //Message offset: multiple impacts in the same direction in short order overriding prior messages (mostly turrets)
            for( std::vector<cSCT>::iterator iter = vSCT.begin(); iter != vSCT.end(); ++iter ) {
                if( iter->getDirecton() == p_oDir && ( iter->getStep() + iter->getStepOffset() ) == iCurStep ) {
                    ++iCurStep;
                    iter->advanceStepOffset();
                }
            }
            vSCT.insert( vSCT.begin(), cSCT( p_iPosX, p_iPosY, p_oDir, p_sText, p_gmt, p_sText2, p_gmt2,
                                             p_sType ) );

        } else {
            //Message offset: this time in reverse.
            for( std::vector<cSCT>::reverse_iterator iter = vSCT.rbegin(); iter != vSCT.rend(); ++iter ) {
                if( iter->getDirecton() == p_oDir && ( iter->getStep() + iter->getStepOffset() ) == iCurStep ) {
                    ++iCurStep;
                    iter->advanceStepOffset();
                }
            }
            vSCT.push_back( cSCT( p_iPosX, p_iPosY, p_oDir, p_sText, p_gmt, p_sText2, p_gmt2, p_sType ) );
        }

    }
}

std::string scrollingcombattext::cSCT::getText( std::string const &type ) const
{
    if( !sText2.empty() ) {
        if( oDir == oUpLeft || oDir == oDownLeft || oDir == oLeft ) {
            if( type == "first" ) {
                return sText2 + " ";

            } else if( type == "full" ) {
                return sText2 + " " + sText;
            }
        } else {
            if( type == "second" ) {
                return " " + sText2;
            } else if( type == "full" ) {
                return sText + " " + sText2;
            }
        }
    } else if( type == "second" ) {
        return {};
    }

    return sText;
}

game_message_type scrollingcombattext::cSCT::getMsgType( std::string const &type ) const
{
    if( !sText2.empty() ) {
        if( oDir == oUpLeft || oDir == oDownLeft || oDir == oLeft ) {
            if( type == "first" ) {
                return gmt2;
            }
        } else {
            if( type == "second" ) {
                return gmt2;
            }
        }
    }

    return gmt;
}

int scrollingcombattext::cSCT::getPosX() const
{
    if( getStep() > 0 ) {
        int iDirOffset = ( oDir == oRight ) ? 1 : ( ( oDir == oLeft ) ? -1 : 0 );

        if( oDir == oUp || oDir == oDown ) {

            if( iso_mode ) {
                iDirOffset = ( oDir == oUp ) ? 1 : -1;
            }

            //Center text
            iDirOffset -= ( getText().length() / 2 );

        } else if( oDir == oLeft || oDir == oDownLeft || oDir == oUpLeft ) {
            //Right align text
            iDirOffset -= getText().length() - 1;
        }

        return iPosX + iDirOffset + ( iDirX * ( ( sType == "hp" ) ? ( getStepOffset() + 1 ) :
                                                ( getStepOffset() * ( iso_mode ? 2 : 1 ) + getStep() ) ) );
    }

    return 0;
}

int scrollingcombattext::cSCT::getPosY() const
{
    if( getStep() > 0 ) {
        int iDirOffset = ( oDir == oDown ) ? 1 : ( ( oDir == oUp ) ? -1 : 0 );

        if( iso_mode ) {
            if( oDir == oLeft || oDir == oRight ) {
                iDirOffset = ( oDir == oRight ) ? 1 : -1;
            }

            if( oDir == oUp || oDir == oDown ) {
                //Center text
                iDirOffset -= ( getText().length() / 2 );

            } else if( oDir == oLeft || oDir == oDownLeft || oDir == oUpLeft ) {
                //Right align text
                iDirOffset -= getText().length() - 1;
            }

        }

        return iPosY + iDirOffset + ( iDirY * ( ( iso_mode && sType == "hp" ) ? ( getStepOffset() + 1 ) :
                                                ( getStepOffset() * ( iso_mode ? 2 : 1 ) + getStep() ) ) );
    }

    return 0;
}

void scrollingcombattext::advanceAllSteps()
{
    std::vector<cSCT>::iterator iter = vSCT.begin();

    while( iter != vSCT.end() ) {
        if( iter->advanceStep() > this->iMaxSteps ) {
            iter = vSCT.erase( iter );
        } else {
            ++iter;
        }
    }
}

void scrollingcombattext::removeCreatureHP()
{
    //check for previous hp display and delete it
    for( std::vector<cSCT>::iterator iter = vSCT.begin(); iter != vSCT.end(); ++iter ) {
        if( iter->getType() == "hp" ) {
            vSCT.erase( iter );
            break;
        }
    }
}
