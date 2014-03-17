/* Main Loop for cataclysm
 * Linux only I guess
 * But maybe not
 * Who knows
 */

#include "cursesdef.h"
#include <ctime>
#include "game.h"
#include "color.h"
#include "options.h"
#include "mapbuffer.h"
#include "debug.h"
#include "item_factory.h"
#include "profession.h"
#include "monstergenerator.h"
#include <sys/stat.h>
#include <cstdlib>
#include <signal.h>
#ifdef LOCALIZE
#include <libintl.h>
#endif
#include "translations.h"
#if (defined OSX_SDL_FW)
#include "SDL.h"
#elif (defined OSX_SDL_LIBS)
#include "SDL/SDL.h"
#endif

#if (defined _WIN32 || defined WINDOWS)
#else
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include <sstream>

void exit_handler(int s);
void setup_coredump();

#ifdef USE_WINMAIN
int APIENTRY WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    int argc = __argc;
    char **argv = __argv;
#else
int main(int argc, char *argv[])
{
#endif
#ifdef ENABLE_LOGGING
    setupDebug();
#endif
    int seed = time(NULL);
    bool verifyexit = false;
    bool check_all_mods = false;
    // set locale to system default
    setlocale(LC_ALL, "");
#ifdef LOCALIZE
    bindtextdomain("cataclysm-dda", "lang/mo");
    bind_textdomain_codeset("cataclysm-dda", "UTF-8");
    textdomain("cataclysm-dda");
#endif

    //args: world seeding only.
    argc--;
    argv++;
    while (argc) {
        if(std::string(argv[0]) == "--seed") {
            argc--;
            argv++;
            if(argc) {
                seed = djb2_hash((unsigned char *)argv[0]);
                argc--;
                argv++;
            }
        } else if(std::string(argv[0]) == "--jsonverify") {
            argc--;
            verifyexit = true;
        } else if(std::string(argv[0]) == "--check-mods") {
            argc--;
            check_all_mods = true;
        } else { // ignore unknown args.
            argc--;
        }
        argv++;
    }

    // ncurses stuff
    initOptions();
    load_options(); // For getting size options
    if (initscr() == NULL) { // Initialize ncurses
        DebugLog() << "initscr failed!\n";
        return 1;
    }
    init_interface();
    noecho();  // Don't echo keypresses
    cbreak();  // C-style breaks (e.g. ^C to SIGINT)
    keypad(stdscr, true); // Numpad is numbers
#if !(defined TILES || defined _WIN32 || defined WINDOWS)
    // For tiles or windows, this is handled already in initscr().
    init_colors();
#endif
    // curs_set(0); // Invisible cursor
    set_escdelay(10); // Make escape actually responsive

    std::srand(seed);

    g = new game;
    // First load and initialize everything that does not
    // depend on the mods.
    try {
        g->load_static_data();
        if (verifyexit) {
            if(g->game_error()) {
                exit_handler(-999);
            }
            exit_handler(0);
        }
        if (check_all_mods) {
            // Here we load all the mods and check their
            // consistency (both is done in check_all_mod_data).
            g->check_all_mod_data();
            if(g->game_error()) {
                exit_handler(-999);
            }
            // At this stage, the mods (and core game data)
            // are find and we could start playing, but this
            // is only for verifying that stage, so we exit.
            exit_handler(0);
        }
    } catch(std::string &error_message) {
        if(!error_message.empty()) {
            debugmsg("%s", error_message.c_str());
        }
        exit_handler(-999);
    }

    // Now we do the actuall game

    g->init_ui();
    if(g->game_error()) {
        exit_handler(-999);
    }

    curs_set(0); // Invisible cursor here, because MAPBUFFER.load() is crash-prone

#if (!(defined _WIN32 || defined WINDOWS))
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = exit_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
#endif

//    setup_coredump();

    bool quit_game = false;
    do {
        if(!g->opening_screen()) {
            quit_game = true;
        }
        while (!quit_game && !g->do_turn()) ;
        if (g->game_quit() || g->game_error()) {
            quit_game = true;
        }
    } while (!quit_game);


    exit_handler(-999);

    return 0;
}

#if (defined _WIN32 || defined WINDOWS)
LONG WINAPI exception_filter(LPEXCEPTION_POINTERS info);
void setup_coredump() {
    SetUnhandledExceptionFilter(exception_filter);
}
#else
void abort_handler(int s);
void setup_coredump() {
    // Prepare handlers for some common errors
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = abort_handler;
    sigaction(SIGABRT, &sigIntHandler, NULL);
    sigaction(SIGILL, &sigIntHandler, NULL);
    sigaction(SIGFPE, &sigIntHandler, NULL);
    sigaction(SIGSEGV, &sigIntHandler, NULL);
    // Set limit for coredumps to infinite
    struct rlimit ulimit;
    ulimit.rlim_cur = RLIM_INFINITY;
    ulimit.rlim_max = RLIM_INFINITY;
    if(setrlimit(RLIMIT_CORE, &ulimit) != 0) {
        debugmsg("setrlimit(RLIMIT_CORE, INFINIT) failed, errno is %d", (int) errno);
    }
}
#endif

void force_clean_screen() {
    erase(); // Clear screen
    endwin(); // End ncurses
    #if (defined _WIN32 || defined WINDOWS)
        system("cls"); // Tell the terminal to clear itself
        system("color 07");
    #else
        system("clear"); // Tell the terminal to clear itself
    #endif
}

static const char *DEFAULT_BACKTRACE_PATH = "crash.log";

// Write the backtrace and if that succeeds clear the stream,
// otherwise return false
bool write_backtrace(std::ostringstream &backtrace) {
    // Try to write it to a log file
    std::ofstream logfilestream(DEFAULT_BACKTRACE_PATH, std::ios::out);
    logfilestream << backtrace.str();
    logfilestream.close();
    if(logfilestream) {
        // Writing the log file was successful, no need to display
        // the backtrace, instead refer to the log file
        backtrace.str(std::string());
        backtrace << "backtrace written to " << DEFAULT_BACKTRACE_PATH << "\n";
        return true;
    } else {
        backtrace << "failed to write backtrace to " << DEFAULT_BACKTRACE_PATH << "\n";
    }
    return false;
}

#if (defined _WIN32 || defined WINDOWS)
#include <imagehlp.h>
#include <windows.h>
#include <excpt.h>
#include <imagehlp.h>
#include <psapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

#define MAX_SYMBOL_NAME_SIZE 255

// format an address as 8-digit hex-number with "0x"-prefix
static const char *address_to_hex(int addr) {
    static char address_buffer[14];
    sprintf(address_buffer, "0x%08X", addr);
    return address_buffer;
}

#define EX_CASE(code) case code: return #code;
LPCSTR GetExceptionName(DWORD code) {
    switch(code) {
        EX_CASE(EXCEPTION_ACCESS_VIOLATION);
        EX_CASE(EXCEPTION_DATATYPE_MISALIGNMENT);
        EX_CASE(EXCEPTION_BREAKPOINT);
        EX_CASE(EXCEPTION_SINGLE_STEP);
        EX_CASE(EXCEPTION_ARRAY_BOUNDS_EXCEEDED);
        EX_CASE(EXCEPTION_FLT_DENORMAL_OPERAND);
        EX_CASE(EXCEPTION_FLT_DIVIDE_BY_ZERO);
        EX_CASE(EXCEPTION_FLT_INEXACT_RESULT);
        EX_CASE(EXCEPTION_FLT_INVALID_OPERATION);
        EX_CASE(EXCEPTION_FLT_OVERFLOW);
        EX_CASE(EXCEPTION_FLT_STACK_CHECK);
        EX_CASE(EXCEPTION_FLT_UNDERFLOW);
        EX_CASE(EXCEPTION_INT_DIVIDE_BY_ZERO);
        EX_CASE(EXCEPTION_INT_OVERFLOW);
        EX_CASE(EXCEPTION_PRIV_INSTRUCTION);
        EX_CASE(EXCEPTION_IN_PAGE_ERROR);
        EX_CASE(EXCEPTION_ILLEGAL_INSTRUCTION);
        EX_CASE(EXCEPTION_NONCONTINUABLE_EXCEPTION);
        EX_CASE(EXCEPTION_STACK_OVERFLOW);
        EX_CASE(EXCEPTION_INVALID_DISPOSITION);
        EX_CASE(EXCEPTION_GUARD_PAGE);
        EX_CASE(EXCEPTION_INVALID_HANDLE);
        case 0xE06D7363: return "C++ Exception";
        default: return "Unknown exception";
    }
}
#undef EX_CASE

static void create_backtrace(LPEXCEPTION_POINTERS &info, std::ostream &output) {
    // Cache some values that are often used
    const HANDLE process = GetCurrentProcess();
    const HANDLE thread = GetCurrentThread();
    CONTEXT &context = *info->ContextRecord;
    EXCEPTION_RECORD &record = *info->ExceptionRecord;
    STACKFRAME frame;
    memset(&frame, 0, sizeof(frame));
    // Print infomrations about the exception itself
    output << "Exception " << GetExceptionName(record.ExceptionCode) <<
    " @ " << address_to_hex((int) record.ExceptionAddress) <<
    " (flags: " << record.ExceptionFlags << ")\n";
    // Setup current frame for walking along the stack
#if defined(_M_AMD64) || defined(_M_X64)
    static const DWORD machine = IMAGE_FILE_MACHINE_AMD64;
    frame.AddrPC.Offset = context.Rip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context.Rbp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context.Rsp;
    frame.AddrStack.Mode = AddrModeFlat;
#elif defined(_M_IX86)
    static const DWORD machine = IMAGE_FILE_MACHINE_I386;
    frame.AddrPC.Offset = context.Eip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context.Ebp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context.Esp;
    frame.AddrStack.Mode = AddrModeFlat;
#elif defined(_M_IA64)
    static const DWORD machine = IMAGE_FILE_MACHINE_IA64;
    frame.AddrPC.Offset = context.StIIP;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context.IntSp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrBStore.Offset = context.RsBSP;
    frame.AddrBStore.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context.IntSp;
    frame.AddrStack.Mode = AddrModeFlat;
#else
    #error "unknwon platform!"
#endif
    // Some buffers for function name and module name
    char symbolBuffer[sizeof(IMAGEHLP_SYMBOL) + MAX_SYMBOL_NAME_SIZE];
    IMAGEHLP_SYMBOL* const symbol = (IMAGEHLP_SYMBOL*) symbolBuffer;
    char module_name[MAX_PATH];
    size_t frame_counter = 0;

    SymInitialize(process, 0, true);
    while(StackWalk(machine, process, thread, &frame, &context, 0, SymFunctionTableAccess, SymGetModuleBase, 0)) {
        // First, raw address of function in hex
        output << address_to_hex(frame.AddrPC.Offset);
        // Second, the function name (if any)
        memset(symbolBuffer, 0, sizeof(IMAGEHLP_SYMBOL) + MAX_SYMBOL_NAME_SIZE);
        symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL) + MAX_SYMBOL_NAME_SIZE;
        symbol->MaxNameLength = MAX_SYMBOL_NAME_SIZE - 1;
        DWORD displacement = 0;
        if(SymGetSymFromAddr(process, frame.AddrPC.Offset, &displacement, symbol)) {
            output << ": " << symbol->Name;
        } else {
            output << ": unknown function";
            const int err = GetLastError();
            if(err != 0) {
                output << " (error " << err << ")";
            }
        }
        // Third, module name
        const DWORD module_base = SymGetModuleBase(process, frame.AddrPC.Offset);
        if(module_base) {
            if(GetModuleFileNameA((HINSTANCE) module_base, module_name, MAX_PATH)) {
                output << " in " << module_name;
            } else {
                output << " in unknown module @ " << address_to_hex(module_base);
            }
        }
        output << "\n";
        if(frame_counter++ > 20) {
            // Prevent endless recursion if stack is damaged
            break;
        }
    }
    SymCleanup(process);
}

#else // (defined _WIN32 || defined WINDOWS)

#define MAX_STACK_FRAME_COUNT 40
static void create_backtrace(std::ostream &buffer) {
    void *array[MAX_STACK_FRAME_COUNT];
    const size_t size = backtrace(array, MAX_STACK_FRAME_COUNT);
    char** const strings = backtrace_symbols(array, size);
    if(strings == NULL) {
        buffer << "backtrace_symbols returned NULL\n";
    } else {
        buffer << "backtrace:\n";
        for (size_t i = 0; i < size; i++) {
            buffer << (strings[i] == NULL ? "NULL" : strings[i]) << "\n";
        }
        free(strings);
    }
}
#endif

#if (defined _WIN32 || defined WINDOWS)
// see abort_handler, both functions have the same structure
LONG WINAPI exception_filter(LPEXCEPTION_POINTERS info) {
    std::ostringstream buffer;
    buffer << "Got an unhandled exception!\n";
    create_backtrace(info, buffer);
    if(write_backtrace(buffer)) {
        buffer << "Got an unhandled exception!\n";
    }
    buffer << "Going to exit the program!\n";
    popup("%s", buffer.str().c_str());
    force_clean_screen();
    exit_handler(0);
    return 0;
}
#else
void abort_handler(int s) {
    std::ostringstream buffer;
    buffer << "Received " << strsignal(s) << " (" << s << ")!\n";
    create_backtrace(buffer);
    if(write_backtrace(buffer)) {
        buffer << "Received " << strsignal(s) << " (" << s << ")!\n";
    }
    // Display some information for the user
    buffer << "Going to write a core dump and exit the program!\n";
    popup("%s", buffer.str().c_str());
    // Prepare for exit
    force_clean_screen();
    // send ourself a SIGQUIT, which is not caught and therefor
    // produces a core dump
    raise(SIGQUIT);
    // This should actually not execute at all, as SIGQUIT should terminate
    exit_handler(0);
}
#endif

void exit_handler(int s) {
    if (s != 2 || query_yn(_("Really Quit? All unsaved changes will be lost."))) {
        erase(); // Clear screen
        endwin(); // End ncurses
        int ret;
        #if (defined _WIN32 || defined WINDOWS)
            ret = system("cls"); // Tell the terminal to clear itself
            ret = system("color 07");
        #else
            ret = system("clear"); // Tell the terminal to clear itself
        #endif
        if (ret != 0) {
            DebugLog() << "main.cpp:exit_handler(): system(\"clear\"): error returned\n";
        }

        if(g != NULL) {
            if(g->game_error()) {
                delete g;
                exit(1);
            } else {
                delete g;
                exit(0);
            }
        }
        exit(0);
    }
}
