#ifndef UNFL_DEBUG_H
#define UNFL_DEBUG_H

    /*********************************
             Settings macros
    *********************************/

    // Enable/Disable debug
    #ifndef DEBUG_MODE
        #define DEBUG_MODE    1   // Enable/Disable debug mode
    #endif

    // Settings
    #define DEBUG_INIT_MSG    1   // Print a message when debug mode has initialized
    #define AUTOPOLL_ENABLED  1   // Automatically poll the USB on a timer
    #define AUTOPOLL_TIME     200 // Time (in milliseconds) between auto polls
    #define USE_FAULTTHREAD   1   // Create a fault detection thread (libultra only)
    #define USE_RDBTHREAD     1   // Create a remote debugger thread
    #define OVERWRITE_OSPRINT 0   // Replaces osSyncPrintf calls with debug_printf (libultra only)
    #define MAX_COMMANDS      25  // The max amount of user defined commands possible

    // USB thread definitions (libultra only)
    #define USB_THREAD_ID    14
    #define USB_THREAD_PRI   126
    #define USB_THREAD_STACK 0x2000

    // Fault thread definitions (libultra only)
    #define FAULT_THREAD_ID    13
    #define FAULT_THREAD_PRI   125
    #define FAULT_THREAD_STACK 0x2000

    // Remote debugger thread definitions (libultra only)
    #define RDB_THREAD_ID    15
    #define RDB_THREAD_PRI   124
    #define RDB_THREAD_STACK 0x2000


    /*********************************
             Debug Functions
    *********************************/

    #if DEBUG_MODE

        /*==============================
            debug_initialize
            Initializes the debug and USB library.
            Should be called last during game initialization.
        ==============================*/

        extern void debug_initialize();


        /*==============================
            debug_printf
            Prints a formatted message to the developer's command prompt.
            Supports up to 256 characters.
            @param A string to print
            @param variadic arguments to print as well
        ==============================*/

        extern void debug_printf(const char* message, ...);


        /*==============================
            debug_dumpbinary
            Dumps a binary file through USB
            @param The file to dump
            @param The size of the file
        ==============================*/

        extern void debug_dumpbinary(void* file, int size);


        /*==============================
            debug_screenshot
            Sends the currently displayed framebuffer through USB.
            DOES NOT PAUSE DRAWING THREAD! Using outside the drawing
            thread may lead to a screenshot with visible tearing
        ==============================*/

        extern void debug_screenshot();


        /*==============================
            debug_assert
            Halts the program if the expression fails.
            @param The expression to test
        ==============================*/

        #define debug_assert(expr) (expr) ? ((void)0) : _debug_assert(#expr, __FILE__, __LINE__)


        /*==============================
            debug_64drivebutton
            Assigns a function to be executed when the 64drive button is pressed.
            @param The function pointer to execute
            @param Whether or not to execute the function only on pressing (ignore holding the button down)
        ==============================*/

        extern void debug_64drivebutton(void(*execute)(), char onpress);


        /*==============================
            debug_pollcommands
            Check the USB for incoming commands.
        ==============================*/

        extern void debug_pollcommands();


        /*==============================
            debug_addcommand
            Adds a command for the USB to read.
            @param The command name
            @param The command description
            @param The function pointer to execute
        ==============================*/

        extern void debug_addcommand(char* command, char* description, char*(*execute)());


        /*==============================
            debug_parsecommand
            Stores the next part of the incoming command into the provided buffer.
            Make sure the buffer can fit the amount of data from debug_sizecommand!
            If you pass NULL, it skips this command.
            @param The buffer to store the data in
        ==============================*/

        extern void debug_parsecommand(void* buffer);


        /*==============================
            debug_sizecommand
            Returns the size of the data from this part of the command.
            @return The size of the data in bytes, or 0
        ==============================*/

        extern int debug_sizecommand();


        /*==============================
            debug_printcommands
            Prints a list of commands to the developer's command prompt.
        ==============================*/

        extern void debug_printcommands();


        // Ignore this, use the macro instead
        extern void _debug_assert(const char* expression, const char* file, int line);

        // Include usb.h automatically
        #include "usb.h"

    #else

        // Overwrite library functions with useless macros if debug mode is disabled
        #define debug_initialize()
        #define debug_printf (void)
        #define debug_screenshot(a, b, c)
        #define debug_assert(a)
        #define debug_pollcommands()
        #define debug_addcommand(a, b, c)
        #define debug_parsecommand(a) NULL
        #define debug_sizecommand() 0
        #define debug_printcommands()
        #define debug_64drivebutton(a, b)
        #define usb_initialize() 0
        #define usb_getcart() 0
        #define usb_write(a, b, c)
        #define usb_poll() 0
        #define usb_read(a, b)
        #define usb_skip(a)
        #define usb_rewind(a)
        #define usb_purge()

    #endif

#endif