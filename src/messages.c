#include <stdarg.h>
#include <stdio.h>

#include "messages.h"

/* Language defines */
#define LANG_AFRIKAANS                   0x36
#define LANG_ALBANIAN                    0x1c
#define LANG_ARABIC                      0x01
#define LANG_ARMENIAN                    0x2b
#define LANG_ASSAMESE                    0x4d
#define LANG_AZERI                       0x2c
#define LANG_BASQUE                      0x2d
#define LANG_BELARUSIAN                  0x23
#define LANG_BENGALI                     0x45
#define LANG_BULGARIAN                   0x02
#define LANG_CATALAN                     0x03
#define LANG_CHINESE                     0x04
#define LANG_CROATIAN                    0x1a
#define LANG_CZECH                       0x05
#define LANG_DANISH                      0x06
#define LANG_DIVEHI                      0x65
#define LANG_DUTCH                       0x13
#define LANG_ENGLISH                     0x09
#define LANG_ESTONIAN                    0x25
#define LANG_FAEROESE                    0x38
#define LANG_FARSI                       0x29
#define LANG_FINNISH                     0x0b
#define LANG_FRENCH                      0x0c
#define LANG_GALICIAN                    0x56
#define LANG_GEORGIAN                    0x37
#define LANG_GERMAN                      0x07
#define LANG_GREEK                       0x08
#define LANG_GUJARATI                    0x47
#define LANG_HEBREW                      0x0d
#define LANG_HINDI                       0x39
#define LANG_HUNGARIAN                   0x0e
#define LANG_ICELANDIC                   0x0f
#define LANG_INDONESIAN                  0x21
#define LANG_ITALIAN                     0x10
#define LANG_JAPANESE                    0x11
#define LANG_KANNADA                     0x4b
#define LANG_KASHMIRI                    0x60
#define LANG_KAZAK                       0x3f
#define LANG_KONKANI                     0x57
#define LANG_KOREAN                      0x12
#define LANG_KYRGYZ                      0x40
#define LANG_LATVIAN                     0x26
#define LANG_LITHUANIAN                  0x27
#define LANG_MACEDONIAN                  0x2f
#define LANG_MALAY                       0x3e
#define LANG_MALAYALAM                   0x4c
#define LANG_MANIPURI                    0x58
#define LANG_MARATHI                     0x4e
#define LANG_MONGOLIAN                   0x50
#define LANG_NEPALI                      0x61
#define LANG_NORWEGIAN                   0x14
#define LANG_ORIYA                       0x48
#define LANG_POLISH                      0x15
#define LANG_PORTUGUESE                  0x16
#define LANG_PUNJABI                     0x46
#define LANG_ROMANIAN                    0x18
#define LANG_RUSSIAN                     0x19
#define LANG_SANSKRIT                    0x4f
#define LANG_SERBIAN                     0x1a
#define LANG_SINDHI                      0x59
#define LANG_SLOVAK                      0x1b
#define LANG_SLOVENIAN                   0x24
#define LANG_SPANISH                     0x0a
#define LANG_SWAHILI                     0x41
#define LANG_SWEDISH                     0x1d
#define LANG_SYRIAC                      0x5a
#define LANG_TAMIL                       0x49
#define LANG_TATAR                       0x44
#define LANG_TELUGU                      0x4a
#define LANG_THAI                        0x1e
#define LANG_TURKISH                     0x1f
#define LANG_UKRAINIAN                   0x22
#define LANG_URDU                        0x20
#define LANG_UZBEK                       0x43
#define LANG_VIETNAMESE                  0x2a

/* Default to english */
#ifndef LANG
#define LANG LANG_ENGLISH
#endif

#define N_MESSAGES 32
#define N_ERRORS   11

/* LANG_ENGLISH */
static CONST CHAR* Messages[ N_MESSAGES ] =
{
#if LANG == LANG_DUTCH
    /* Errors */
    "Geen fout",
    "Niet genoeg geheugen",
    "Bestand niet gevonden",
    "Resultaat te groot",
    "Onbekende fout",
    "Apparaat bestaat niet",
    "Partitie bestaat niet",
    "Onbekend bestandssysteem",
    "I/O fout",
    "Bestand is corrupt",
    "Onbekend bestandsformaat",

    /* Configuration */
    "Fout: '%s' opdracht niet toegestaan in een sectie\n",
    "Fout: '%s' waarde is geen geldige geheel getal\n",
    "Fout: '%s' opdracht niet toegestaan buiten een sectie\n",
    "Fout: configuratiebestand bevat meerdere standaard besturingssystemen\n",
    "Fout: '%s' sectie mist vereiste '%s' waarde\n",
    "Fout in configuratiebestand op regel %d\n",

    /* Boot menu */
    "  OSLDR opstartmenu",
    " Selecteer een besturingssysteem uit de onderstaande lijst en druk op\n",
    " Enter om dat besturingssysteem op te starten.\n",
    "Opnieuw opstarten",
    "Resterende tijd",
    "seconde ",
    "seconden ",

    /* Main */
    "Druk op een toets om de computer opnieuw op te starten\n",
    "Kan geen geheugeninformatie verkrijgen\n",
    "Kan configuratiebestand niet openen\n",
    "Er staan geen besturingssystemen in het configuratiebestand\n",
    "Kan besturingssysteem niet laden",

    /* Loader */
    "Kan de A20 poort niet activeren\n",
    "Kernel heeft niet-ondersteunde multiboot eisen\n"
    "Onverenigbare of oude hardware\n"
#else
    /* Errors */
    "No error",
    "Not enough memory",
    "File not found",
    "Result too large",
    "Unknown error",
    "No such device",
    "No such partition",
    "Unknown filesystem",
    "I/O error",
    "File is corrupt",
    "Unknown filetype",

    /* Configuration */
    "Error: '%s' directive not allowed within a section\n",
    "Error: '%s' value is not a valid integer\n",
    "Error: '%s' directive not allowed outside a section\n",
    "Error: configuration file contains multiple default operating systems\n",
    "Error: '%s' section misses required '%s' property\n",
    "Error in configuration file at line %d\n",

    /* Boot menu */
    "  OSLDR boot menu",
    " Select an operating system from the list below and press Enter to boot\n",
    " that operating system.\n",
    "Reboot",
    "Time remaining",
    "second ",
    "seconds ",

    /* Main */
    "Press any key to reset the computer\n",
    "Couldn't get memory information\n",
    "Unable to open configuration file\n",
    "There are no operating systems in the configuration file\n",
    "Unable to load operating system",

    /* Loader */
    "Unable to enable the A20 gate\n",
    "Kernel has unsupported multiboot requirements\n",
    "Incompatible or old hardware\n"
#endif
};

INT errno;

INT PrintError( ULONG MsgId, ... )
{
    if (MsgId < N_MESSAGES)
    {
        va_list args;
        INT     retval;

        va_start( args, MsgId );
        retval = vprintf( Messages[ MsgId ], args );
        va_end( args );

        if ((errno >= 0) && (errno < N_ERRORS))
        {
            retval += printf(": %s\n", Messages[ errno ]);
        }

        return retval;
    }
    return -1;
}

INT PrintMessage( ULONG MsgId, ... )
{
    if (MsgId < N_MESSAGES)
    {
        va_list args;
        INT     retval;

        va_start( args, MsgId );
        retval = vprintf( Messages[ MsgId ], args );
        va_end( args );
        return retval;
    }
    return -1;
}

CONST CHAR* GetMessage( ULONG MsgId )
{
    if (MsgId < N_MESSAGES)
    {
        return Messages[ MsgId ];
    }
    return NULL;
}
