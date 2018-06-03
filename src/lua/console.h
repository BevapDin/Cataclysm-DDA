#pragma once
#ifndef CONSOLE_H
#define CONSOLE_H

#include "output.h"
#include "cursesdef.h"

#include <string>
#include <vector>
#include <utility>

class nc_color;

namespace catalua {

class console
{
    public:
        console();
        ~console();
        void run();
    private:
        const int width = TERMX;
        const int lines = 10;

        catacurses::window cWin;
        catacurses::window iWin;

        std::vector<std::pair<std::string, nc_color>> text_stack;
        std::string get_input();
        void print( std::string text );
        void draw();

        bool done = false;

        int scroll = 0;

        void read_stream( std::stringstream &stream, nc_color text_color );

        void quit();
        void scroll_up();
        void scroll_down();
};

} // namespace catalua

#endif
