#ifndef MESSAGES_H
#define MESSAGES_H

#include <types.h>

/*
 * This is the list of messages.
 * The corresponding strings are requested based on the language.
 */

/* Errors */
#define MSG_NO_ERROR                     0
#define MSG_OUT_OF_MEMORY                1
#define MSG_NO_SUCH_FILE                 2
#define MSG_OUT_OF_RANGE                 3
#define MSG_UNKNOWN_ERROR                4
#define MSG_NO_SUCH_DEVICE               5
#define MSG_NO_SUCH_PARTITION            6
#define MSG_NO_SUCH_FILESYSTEM           7
#define MSG_IO_ERROR                     8
#define MSG_CORRUPT_FILE                 9
#define MSG_UNKNOWN_FILE_TYPE           10

/* Configuration */
#define MSG_CONF_DIR_INSIDE_SECTION     11
#define MSG_CONF_INVALID_INTEGER        12
#define MSG_CONF_DIR_OUTSIDE_SECTION    13
#define MSG_CONF_MULTIPLE_DEFAULTS      14
#define MSG_CONF_MISSING_PROPERTY       15
#define MSG_CONF_ERROR_AT_LINE          16

/* Boot menu */
#define MSG_MENU_TITLE                  17
#define MSG_MENU_INSTR1                 18
#define MSG_MENU_INSTR2                 19
#define MSG_MENU_REBOOT                 20
#define MSG_MENU_TIME_LEFT              21
#define MSG_MENU_SECOND                 22
#define MSG_MENU_SECONDS                23

/* Main */
#define MSG_MAIN_PRESS_TO_REBOOT        24
#define MSG_MAIN_NO_MEMORY_INFO         25
#define MSG_MAIN_CANT_OPEN_CONF_FILE    26
#define MSG_MAIN_NO_ENTRIES_IN_FILE     27

/* Loader */
#define MSG_LOADER_CANT_LOAD_OS         28
#define MSG_LOADER_CANT_ENABLE_A20      29
#define MSG_LOADER_UNSUPPORTED_REQS     30
#define MSG_LOADER_WRONG_HARDWARE       31

INT         PrintError( ULONG MsgId, ... );
INT         PrintMessage( ULONG MsgId, ... );
CONST CHAR* GetMessage( ULONG MsgId );

#endif
