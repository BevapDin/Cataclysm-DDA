#ifndef SCROLLINGCOMBATTEXT_H
#define SCROLLINGCOMBATTEXT_H

#include "output.h"

#include <string>
#include <vector>

enum direction : unsigned;
enum game_message_type : int;

class scrollingcombattext
{
    public:
        enum : int { iMaxSteps = 8 };

        scrollingcombattext() = default;

        class cSCT
        {
            private:
                int iPosX;
                int iPosY;
                direction oDir;
                direction oUp, oUpRight, oRight, oDownRight, oDown, oDownLeft, oLeft, oUpLeft;
                int iDirX;
                int iDirY;
                int iStep;
                int iStepOffset;
                std::string sText;
                game_message_type gmt;
                std::string sText2;
                game_message_type gmt2;
                std::string sType;
                bool iso_mode;

            public:
                cSCT( const int p_iPosX, const int p_iPosY, direction p_oDir,
                      const std::string p_sText, const game_message_type p_gmt,
                      const std::string p_sText2 = "", const game_message_type p_gmt2 = m_neutral,
                      const std::string p_sType = "" );

                int getStep() const {
                    return iStep;
                }
                int getStepOffset() const {
                    return iStepOffset;
                }
                int advanceStep() {
                    return ++iStep;
                }
                int advanceStepOffset() {
                    return ++iStepOffset;
                }
                int getPosX() const;
                int getPosY() const;
                direction getDirecton() const {
                    return oDir;
                }
                int getInitPosX() const {
                    return iPosX;
                }
                int getInitPosY() const {
                    return iPosY;
                }
                std::string getType() const {
                    return sType;
                }
                std::string getText( std::string const &type = "full" ) const;
                game_message_type getMsgType( std::string const &type = "first" ) const;
        };

        std::vector<cSCT> vSCT;

        void add( const int p_iPosX, const int p_iPosY, const direction p_oDir,
                  const std::string p_sText, const game_message_type p_gmt,
                  const std::string p_sText2 = "", const game_message_type p_gmt2 = m_neutral,
                  const std::string p_sType = "" );
        void advanceAllSteps();
        void removeCreatureHP();
};

extern scrollingcombattext SCT;

#endif
