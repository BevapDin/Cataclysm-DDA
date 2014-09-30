#if ((!defined TILES) && (!defined SDLTILES) && (defined _WIN32 || defined WINDOWS))
#define UNICODE 1
#define _UNICODE 1

#include "catacurse.h"
#include "options.h"
#include "output.h"
#include "color.h"
#include "catacharset.h"
#include "get_version.h"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include "init.h"
#include "path_info.h"
#include "file_wrapper.h"
#include "debug.h"

#define dbg(x) DebugLog( x, D_WINDOWS ) << __FILE__ << ":" << __LINE__ << ": "

//***********************************
//Globals                           *
//***********************************

const wchar_t *szWindowClass = L"CataCurseWindow";    //Class name :D
HINSTANCE WindowINST;   //the instance of the window
HWND WindowHandle;      //the handle of the window
HDC WindowDC;           //Device Context of the window, used for backbuffer
int WindowWidth;        //Width of the actual window, not the curses window
int WindowHeight;       //Height of the actual window, not the curses window
int lastchar;          //the last character that was pressed, resets in getch
int inputdelay;         //How long getch will wait for a character to be typed
HDC backbuffer;         //an off-screen DC to prevent flickering, lower cpu
HBITMAP backbit;        //the bitmap that is used in conjunction wth the above
int fontwidth;          //the width of the font, background is always this size
int fontheight;         //the height of the font, background is always this size
int halfwidth;          //half of the font width, used for centering lines
int halfheight;          //half of the font height, used for centering lines
HFONT font;             //Handle to the font created by CreateFont
// The color palette, 16 colors emulates a terminal
std::array<RGBQUAD, 16> windowsPalette;
unsigned char *dcbits;  //the bits of the screen image, for direct access
bool CursorVisible = true; // Showcursor is a somewhat weird function

//***********************************
//Non-curses, Window functions      *
//***********************************

// declare this locally, because it's not generally cross-compatible in catacurse.h
LRESULT CALLBACK ProcessMessages(HWND__ *hWnd,u_int32_t Msg,WPARAM wParam, LPARAM lParam);

std::wstring widen( const std::string &s )
{
    if( s.empty() ) {
        return std::wstring(); // MultiByteToWideChar can not handle this case
    }
    std::vector<wchar_t> buffer( s.length() );
    const int newlen = MultiByteToWideChar( CP_UTF8, 0, s.c_str(), s.length(),
                                            buffer.data(), buffer.size() );
    // on failure, newlen is 0, returns an empty strings.
    return std::wstring( buffer.data(), newlen );
}

//Registers, creates, and shows the Window!!
bool WinCreate()
{
    WindowINST = GetModuleHandle(0); // Get current process handle
    std::string title = string_format("Cataclysm: Dark Days Ahead - %s", getVersionString());

    // Register window class
    WNDCLASSEXW WindowClassType   = WNDCLASSEXW();
    WindowClassType.cbSize        = sizeof(WNDCLASSEXW);
    WindowClassType.lpfnWndProc   = ProcessMessages;//the procedure that gets msgs
    WindowClassType.hInstance     = WindowINST;// hInstance
    WindowClassType.hIcon         = LoadIcon(WindowINST, MAKEINTRESOURCE(0)); // Get first resource
    WindowClassType.hIconSm       = LoadIcon(WindowINST, MAKEINTRESOURCE(0));
    WindowClassType.hCursor       = LoadCursor(NULL, IDC_ARROW);
    WindowClassType.lpszClassName = szWindowClass;
    if( RegisterClassExW( &WindowClassType ) == 0 ) {
        dbg( D_ERROR ) << "RegisterClassEx failed: " << GetLastError();
        return false;
    }

    // Adjust window size
    uint32_t WndStyle = WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_VISIBLE; // Basic window, show on creation
    RECT WndRect;
    WndRect.left   = WndRect.top = 0;
    WndRect.right  = WindowWidth;
    WndRect.bottom = WindowHeight;
    AdjustWindowRect(&WndRect, WndStyle, false);

    // Center window
    RECT WorkArea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &WorkArea, 0);
    int WindowX = WorkArea.right/2 - (WndRect.right - WndRect.left)/2;
    int WindowY = WorkArea.bottom/2 - (WndRect.bottom - WndRect.top)/2;

    // Magic
    WindowHandle = CreateWindowExW(0, szWindowClass , widen(title).c_str(), WndStyle,
                                   WindowX, WindowY,
                                   WndRect.right - WndRect.left,
                                   WndRect.bottom - WndRect.top,
                                   0, 0, WindowINST, NULL);
    if( WindowHandle == 0 ) {
        dbg( D_ERROR ) << "CreateWindowEx failed: " << GetLastError();
        return false;
    }

    return true;
};

//Unregisters, releases the DC if needed, and destroys the window.
void WinDestroy()
{
    if ((WindowDC != NULL) && (ReleaseDC(WindowHandle, WindowDC) == 0)){
        WindowDC = 0;
    }
    if ((!WindowHandle == 0) && (!(DestroyWindow(WindowHandle)))){
        WindowHandle = 0;
    }
    if (!(UnregisterClassW(szWindowClass, WindowINST))){
        WindowINST = 0;
    }
};

// Copied from sdlcurses.cpp
#define ALT_BUFFER_SIZE 8
static char alt_buffer[ALT_BUFFER_SIZE] = {};
static int alt_buffer_len = 0;
static bool alt_down = false;

static void begin_alt_code()
{
    alt_buffer[0] = '\0';
    alt_down = true;
    alt_buffer_len = 0;
}

void add_alt_code(char c)
{
    // not exactly how it works, but acceptable
    if(c>='0' && c<='9')
    {
        if(alt_buffer_len<ALT_BUFFER_SIZE-1)
        {
            alt_buffer[alt_buffer_len] = c;
            alt_buffer[++alt_buffer_len] = '\0';
        }
    }
}

static int end_alt_code()
{
    alt_down = false;
    return atoi(alt_buffer);
}

//This function processes any Windows messages we get. Keyboard, OnClose, etc
LRESULT CALLBACK ProcessMessages(HWND__ *hWnd,unsigned int Msg,
                                 WPARAM wParam, LPARAM lParam)
{
    uint16_t MouseOver;
    switch (Msg)
    {
    case WM_DEADCHAR:
    case WM_CHAR:
        lastchar = (int)wParam;
        switch (lastchar){
            case VK_RETURN: //Reroute ENTER key for compatilbity purposes
                lastchar=10;
                break;
            case VK_BACK: //Reroute BACKSPACE key for compatilbity purposes
                lastchar=127;
                break;
        }
        return 0;

    case WM_KEYDOWN:                //Here we handle non-character input
        switch (wParam){
            case VK_LEFT:
                lastchar = KEY_LEFT;
                break;
            case VK_RIGHT:
                lastchar = KEY_RIGHT;
                break;
            case VK_UP:
                lastchar = KEY_UP;
                break;
            case VK_DOWN:
                lastchar = KEY_DOWN;
                break;
            case VK_NEXT:
                lastchar = KEY_NPAGE;
                break;
            case VK_PRIOR:
                lastchar = KEY_PPAGE;
                break;
            case VK_F1:
                lastchar = KEY_F(1);
                break;
            case VK_F2:
                lastchar = KEY_F(2);
                break;
            case VK_F3:
                lastchar = KEY_F(3);
                break;
            case VK_F4:
                lastchar = KEY_F(4);
                break;
            case VK_F5:
                lastchar = KEY_F(5);
                break;
            case VK_F6:
                lastchar = KEY_F(6);
                break;
            case VK_F7:
                lastchar = KEY_F(7);
                break;
            case VK_F8:
                lastchar = KEY_F(8);
                break;
            case VK_F9:
                lastchar = KEY_F(9);
                break;
            case VK_F10:
                lastchar = KEY_F(10);
                break;
            case VK_F11:
                lastchar = KEY_F(11);
                break;
            case VK_F12:
                lastchar = KEY_F(12);
                break;
            default:
                break;
        };
        return 0;

    case WM_KEYUP:
        if (!GetAsyncKeyState(VK_LMENU) && alt_down){ // LeftAlt hack
            if (int code = end_alt_code())
                lastchar = code;
        }
        return 0;

    case WM_SYSCHAR:
        add_alt_code((char)wParam);
        return 0;

    case WM_SYSKEYDOWN:
        if (GetAsyncKeyState(VK_LMENU) && !alt_down){ // LeftAlt hack
            begin_alt_code();
        }
        break;

    case WM_SETCURSOR:
        MouseOver = LOWORD(lParam);
        if (OPTIONS["HIDE_CURSOR"] == "hide")
        {
            if (MouseOver==HTCLIENT && CursorVisible)
            {
                CursorVisible = false;
                ShowCursor(false);
            }
            else if (MouseOver!=HTCLIENT && !CursorVisible)
            {
                CursorVisible = true;
                ShowCursor(true);
            }
        }
        else if (!CursorVisible)
        {
            CursorVisible = true;
            ShowCursor(true);
        }
        break;

    case WM_ERASEBKGND:
        return 1; // Don't erase background

    case WM_PAINT:
        BitBlt(WindowDC, 0, 0, WindowWidth, WindowHeight, backbuffer, 0, 0,SRCCOPY);
        ValidateRect(WindowHandle,NULL);
        return 0;

    case WM_DESTROY:
        exit(0); // A messy exit, but easy way to escape game loop
    };

    return DefWindowProcW(hWnd, Msg, wParam, lParam);
}

//The following 3 methods use mem functions for fast drawing
inline void VertLineDIB(int x, int y, int y2,int thickness, unsigned char color)
{
    int j;
    for (j=y; j<y2; j++)
        memset(&dcbits[x+j*WindowWidth],color,thickness);
}
inline void HorzLineDIB(int x, int y, int x2,int thickness, unsigned char color)
{
    int j;
    for (j=y; j<y+thickness; j++)
        memset(&dcbits[x+j*WindowWidth],color,x2-x);
}
inline void FillRectDIB(int x, int y, int width, int height, unsigned char color)
{
    int j;
    for (j=y; j<y+height; j++)
        //NOTE TO FUTURE: this breaks if j is negative. Apparently it doesn't break if j is too large, though?
        memset(&dcbits[x+j*WindowWidth],color,width);
}

void curses_drawwindow(WINDOW *win)
{
    int i,j,drawx,drawy;
    unsigned tmp;
    RECT update = {win->x * fontwidth, -1,
                   (win->x + win->width) * fontwidth, -1};

    for (j=0; j<win->height; j++){
        if (win->line[j].touched)
        {
            update.bottom = (win->y+j+1)*fontheight;
            if (update.top == -1)
            {
                update.top = update.bottom - fontheight;
            }

            win->line[j].touched=false;

            for (i=0; i<win->width; i++){
                const cursecell &cell = win->line[j].chars[i];
                if( cell.ch.empty() ) {
                    continue; // second cell of a multi-cell character
                }
                drawx=((win->x+i)*fontwidth);
                drawy=((win->y+j)*fontheight);//-j;
                if( drawx + fontwidth > WindowWidth || drawy + fontheight > WindowHeight ) {
                    // Outside of the display area, would not render anyway
                    continue;
                }
                const char* utf8str = cell.ch.c_str();
                int len = cell.ch.length();
                tmp = UTF8_getch(&utf8str, &len);
                int FG = cell.FG;
                int BG = cell.BG;
                FillRectDIB(drawx,drawy,fontwidth,fontheight,BG);

                if (tmp != UNKNOWN_UNICODE) {

                    int color = RGB(windowsPalette[FG].rgbRed,windowsPalette[FG].rgbGreen,windowsPalette[FG].rgbBlue);
                    SetTextColor(backbuffer,color);

                    int cw = mk_wcwidth(tmp);
                    if (cw > 1) {
                        FillRectDIB(drawx+fontwidth*(cw-1), drawy, fontwidth, fontheight, BG);
                        i += cw - 1;
                    }
                    if (tmp) {
                        const std::wstring utf16 = widen(cell.ch);
                        ExtTextOutW( backbuffer, drawx, drawy, 0, NULL, utf16.c_str(), utf16.length(), NULL );
                    }
                } else {
                    switch ((unsigned char)win->line[j].chars[i].ch[0]) {
                    case LINE_OXOX_C://box bottom/top side (horizontal line)
                        HorzLineDIB(drawx,drawy+halfheight,drawx+fontwidth,1,FG);
                        break;
                    case LINE_XOXO_C://box left/right side (vertical line)
                        VertLineDIB(drawx+halfwidth,drawy,drawy+fontheight,2,FG);
                        break;
                    case LINE_OXXO_C://box top left
                        HorzLineDIB(drawx+halfwidth,drawy+halfheight,drawx+fontwidth,1,FG);
                        VertLineDIB(drawx+halfwidth,drawy+halfheight,drawy+fontheight,2,FG);
                        break;
                    case LINE_OOXX_C://box top right
                        HorzLineDIB(drawx,drawy+halfheight,drawx+halfwidth,1,FG);
                        VertLineDIB(drawx+halfwidth,drawy+halfheight,drawy+fontheight,2,FG);
                        break;
                    case LINE_XOOX_C://box bottom right
                        HorzLineDIB(drawx,drawy+halfheight,drawx+halfwidth,1,FG);
                        VertLineDIB(drawx+halfwidth,drawy,drawy+halfheight+1,2,FG);
                        break;
                    case LINE_XXOO_C://box bottom left
                        HorzLineDIB(drawx+halfwidth,drawy+halfheight,drawx+fontwidth,1,FG);
                        VertLineDIB(drawx+halfwidth,drawy,drawy+halfheight+1,2,FG);
                        break;
                    case LINE_XXOX_C://box bottom north T (left, right, up)
                        HorzLineDIB(drawx,drawy+halfheight,drawx+fontwidth,1,FG);
                        VertLineDIB(drawx+halfwidth,drawy,drawy+halfheight,2,FG);
                        break;
                    case LINE_XXXO_C://box bottom east T (up, right, down)
                        VertLineDIB(drawx+halfwidth,drawy,drawy+fontheight,2,FG);
                        HorzLineDIB(drawx+halfwidth,drawy+halfheight,drawx+fontwidth,1,FG);
                        break;
                    case LINE_OXXX_C://box bottom south T (left, right, down)
                        HorzLineDIB(drawx,drawy+halfheight,drawx+fontwidth,1,FG);
                        VertLineDIB(drawx+halfwidth,drawy+halfheight,drawy+fontheight,2,FG);
                        break;
                    case LINE_XXXX_C://box X (left down up right)
                        HorzLineDIB(drawx,drawy+halfheight,drawx+fontwidth,1,FG);
                        VertLineDIB(drawx+halfwidth,drawy,drawy+fontheight,2,FG);
                        break;
                    case LINE_XOXX_C://box bottom east T (left, down, up)
                        VertLineDIB(drawx+halfwidth,drawy,drawy+fontheight,2,FG);
                        HorzLineDIB(drawx,drawy+halfheight,drawx+halfwidth,1,FG);
                        break;
                    default:
                        break;
                    };//switch (tmp)
                }//(tmp < 0)
            };//for (i=0;i<_windows[w].width;i++)
        }
    };// for (j=0;j<_windows[w].height;j++)
    win->draw=false;                //We drew the window, mark it as so
    if (update.top != -1)
    {
        RedrawWindow(WindowHandle, &update, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
    }
}

//Check for any window messages (keypress, paint, mousemove, etc)
void CheckMessages()
{
    MSG msg;
    while (PeekMessage(&msg, 0 , 0, 0, PM_REMOVE)){
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

// Calculates the new width of the window, given the number of columns.
int projected_window_width(int)
{
    return OPTIONS["TERMINAL_X"] * fontwidth;
}

// Calculates the new height of the window, given the number of rows.
int projected_window_height(int)
{
    return OPTIONS["TERMINAL_Y"] * fontheight;
}

//***********************************
//Psuedo-Curses Functions           *
//***********************************

//Basic Init, create the font, backbuffer, etc
WINDOW *curses_init(void)
{
    lastchar=-1;
    inputdelay=-1;

    int fontsize = 16;
    std::string typeface;
    int map_fontwidth = 8;
    int map_fontheight = 16;
    int map_fontsize = 16;
    std::string map_typeface;
    bool fontblending;

    std::ifstream jsonstream(FILENAMES["fontdata"].c_str(), std::ifstream::binary);
    if (jsonstream.good()) {
        JsonIn json(jsonstream);
        JsonObject config = json.get_object();
        // fontsize, fontblending, map_* are ignored in wincurse.
        fontwidth = config.get_int("fontwidth", fontwidth);
        fontheight = config.get_int("fontheight", fontheight);
        typeface = config.get_string("typeface", typeface);
        jsonstream.close();
    } else { // User fontdata is missed. Try to load legacy fontdata.
        // Get and save all values. With unused.
        std::ifstream InStream(FILENAMES["legacy_fontdata"].c_str(), std::ifstream::binary);
        if(InStream.good()) {
            JsonIn jIn(InStream);
            JsonObject config = jIn.get_object();
            fontwidth = config.get_int("fontwidth", fontwidth);
            fontheight = config.get_int("fontheight", fontheight);
            fontsize = config.get_int("fontsize", fontsize);
            typeface = config.get_string("typeface", typeface);
            map_fontwidth = config.get_int("map_fontwidth", fontwidth);
            map_fontheight = config.get_int("map_fontheight", fontheight);
            map_fontsize = config.get_int("map_fontsize", fontsize);
            map_typeface = config.get_string("map_typeface", typeface);
            InStream.close();
            // Save legacy as user fontdata.
            assure_dir_exist(FILENAMES["config_dir"]);
            std::ofstream OutStream(FILENAMES["fontdata"].c_str(), std::ofstream::binary);
            if(!OutStream.good()) {
                dbg( D_ERROR ) << "Can't save user fontdata file.\n"
                << "Check permissions for: " << FILENAMES["fontdata"].c_str();
                return NULL;
            }
            JsonOut jOut(OutStream, true); // pretty-print
            jOut.start_object();
            jOut.member("fontblending", fontblending);
            jOut.member("fontwidth", fontwidth);
            jOut.member("fontheight", fontheight);
            jOut.member("fontsize", fontsize);
            jOut.member("typeface", typeface);
            jOut.member("map_fontwidth", map_fontwidth);
            jOut.member("map_fontheight", map_fontheight);
            jOut.member("map_fontsize", map_fontsize);
            jOut.member("map_typeface", map_typeface);
            jOut.end_object();
            OutStream << "\n";
            OutStream.close();
        } else {
            dbg( D_ERROR ) << "Can't load fontdata files.\n"
            << "Check permissions for:\n" << FILENAMES["legacy_fontdata"].c_str() << "\n"
            << FILENAMES["fontdata"].c_str() << "\n";
            return NULL;
        }
    }

    halfwidth=fontwidth / 2;
    halfheight=fontheight / 2;
    WindowWidth= OPTIONS["TERMINAL_X"] * fontwidth;
    WindowHeight = OPTIONS["TERMINAL_Y"] * fontheight;

    if( !WinCreate() ) {
        return NULL;
    }
    timeBeginPeriod(1); // Set Sleep resolution to 1ms
    CheckMessages();    //Let the message queue handle setting up the window

    WindowDC   = GetDC(WindowHandle);
    if( WindowDC == NULL ) {
        dbg( D_ERROR ) << "GetDC returned NULL";
        return NULL;
    }
    backbuffer = CreateCompatibleDC(WindowDC);
    if( backbuffer == NULL ) {
        dbg( D_ERROR ) << "CreateCompatibleDC returned NULL";
        return NULL;
    }
    BITMAPINFO bmi = BITMAPINFO();
    bmi.bmiHeader.biSize         = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth        = WindowWidth;
    bmi.bmiHeader.biHeight       = -WindowHeight;
    bmi.bmiHeader.biPlanes       = 1;
    bmi.bmiHeader.biBitCount     = 8;
    bmi.bmiHeader.biCompression  = BI_RGB; // Raw RGB
    bmi.bmiHeader.biSizeImage    = WindowWidth * WindowHeight * 1;
    bmi.bmiHeader.biClrUsed      = 16; // Colors in the palette
    bmi.bmiHeader.biClrImportant = 16; // Colors in the palette
    backbit = CreateDIBSection(0, &bmi, DIB_RGB_COLORS, (void**)&dcbits, NULL, 0);
    if( backbit == NULL ) {
        dbg( D_ERROR ) << "CreateDIBSection returned NULL";
        return NULL;
    }
    auto oldobj = SelectObject( backbuffer, backbit ); // load the buffer into DC
    if( oldobj == NULL ) {
        dbg( D_ERROR ) << "SelectObject(backbuffer,backbit) returned NULL";
        return NULL;
    }
    DeleteObject( oldobj );

    // Load private fonts
    if (SetCurrentDirectoryW(L"data\\font")){
        WIN32_FIND_DATA findData;
        for (HANDLE findFont = FindFirstFileW(L".\\*", &findData); findFont != INVALID_HANDLE_VALUE; )
        {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){ // Skip folders
                if( AddFontResourceExW( findData.cFileName, FR_PRIVATE, NULL ) == 0 ) {
                    // stupid wide character string findData.cFileName, does not work with std::ostream, needs to be narrowed
                    dbg( D_ERROR ) << "AddFontResourceEx failed";
                }
            }
            if (!FindNextFile(findFont, &findData)){
                FindClose(findFont);
                break;
            }
        }
        SetCurrentDirectoryW(L"..\\..");
    } else {
        dbg( D_ERROR ) << "SetCurrentDirectory(data\\font) failed";
    }

    // Use desired font, if possible
    font = CreateFontW(fontheight, fontwidth, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
                      PROOF_QUALITY, FF_MODERN, widen(typeface).c_str());
    if( font == NULL ) {
        dbg( D_ERROR ) << "CreateFont returned NULL";
    }

    if( SetBkMode( backbuffer, TRANSPARENT ) == 0 ) { // Transparent font backgrounds
        dbg( D_ERROR ) << "SetBkMode failed";
    }
    if( font != NULL ) {
        auto oldobj = SelectObject( backbuffer, font ); // Load our font into the DC
        if( oldobj == NULL ) {
            dbg( D_ERROR ) << "SelectObject(backbit,font) returned NULL";
        } else {
            DeleteObject( oldobj );
        }
    }

    init_colors();

    mainwin = newwin(OPTIONS["TERMINAL_Y"],OPTIONS["TERMINAL_X"],0,0);
    return mainwin;   //create the 'stdscr' window and return its ref
}

// A very accurate and responsive timer (NEVER use GetTickCount)
uint64_t GetPerfCount(){
    uint64_t Count;
    QueryPerformanceCounter((PLARGE_INTEGER)&Count);
    return Count;
}

//Not terribly sure how this function is suppose to work,
//but jday helped to figure most of it out
int curses_getch(WINDOW* win)
{
    // standards note: getch is sometimes required to call refresh
    // see, e.g., http://linux.die.net/man/3/getch
    // so although it's non-obvious, that refresh() call (and maybe InvalidateRect?) IS supposed to be there
    uint64_t Frequency;
    QueryPerformanceFrequency((PLARGE_INTEGER)&Frequency);
    wrefresh(win);
    InvalidateRect(WindowHandle,NULL,true);
    lastchar = ERR;
    if (inputdelay < 0)
    {
        for (; lastchar==ERR; Sleep(1))
            CheckMessages();
    }
    else if (inputdelay > 0)
    {
        for (uint64_t t0=GetPerfCount(), t1=0; t1 < (t0 + inputdelay*Frequency/1000); t1=GetPerfCount())
        {
            CheckMessages();
            if (lastchar!=ERR) break;
            Sleep(1);
        }
    }
    else
    {
        CheckMessages();
    };

    if (lastchar!=ERR && OPTIONS["HIDE_CURSOR"] == "hidekb" && CursorVisible) {
        CursorVisible = false;
        ShowCursor(false);
    }

    return lastchar;
}


//Ends the terminal, destroy everything
int curses_destroy(void)
{
    if( font != NULL ) {
        DeleteObject( font );
    }
    WinDestroy();
    // TODO: should remove all fonts in the data/font directy, just as all of them have been loaded, see above
    RemoveFontResourceExA("data\\termfont",FR_PRIVATE,NULL);//Unload it
    return 1;
}

// translate color entry in consolecolors to RGBQUAD
inline RGBQUAD ccolor( const std::string &color )
{
    const auto it = consolecolors.find( color );
    if( it == consolecolors.end() ) {
        dbg( D_ERROR ) << "requested non-existing color " << color << "\n";
        return RGBQUAD { 0, 0, 0, 0 };
    }
    // rgbBlue, rgbGreen, rgbRed, rgbReserved
    return RGBQUAD { BYTE( it->second[0] ), BYTE( it->second[1] ), BYTE( it->second[2] ), 0 };
}

// This function mimics the ncurses interface. It must not throw.
// Instead it should return ERR or OK, see man curs_color
int curses_start_color(void)
{
    if( !load_colors_from_json() ) {
        return ERR;
    }
    for( size_t c = 0; c < main_color_names.size(); c++ ) {
        windowsPalette[c] = ccolor( main_color_names[c] );
    }
    if( SetDIBColorTable( backbuffer, 0, windowsPalette.size(), windowsPalette.data() ) == 0 ) {
        dbg( D_ERROR ) << "SetDIBColorTable failed";
        return ERR;
    }
    return OK;
}

void curses_timeout(int t)
{
    inputdelay = t;
}

#endif
