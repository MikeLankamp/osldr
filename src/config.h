#ifndef CONFIG_H
#define CONFIG_H

#include <io.h>
#include <multiboot.h>

/* Values for Image.Type */
#define IT_AUTO        0  /* Try to determine the type */
#define IT_BOOTSECTOR  1  /* A bootsector */
#define IT_MULTIBOOT   2  /* A multiboot-compliant image */
#define IT_RELOCATABLE 3  /* A relocatable executable (ELF, PE/COFF) */
#define IT_BINARY      4  /* A binary kernel */

typedef struct _IMAGE IMAGE;
struct _IMAGE
{
    CHAR*  Name;      /* Image name (shown in boot menu)          */
    CHAR*  Command;   /* Command line (first token is image path) */

    UINT   Type;      /* Type of the image (ignored for devices)  */
    ULONG  Address;   /* Only valid if Type is IT_BINARY          */
    ULONG  Drive;     /* Only valid is Type is IT_BOOTSECTOR      */

    ULONG   nModules;
    MODULE* Modules;  /* Modules */

    IMAGE* Next;

    /* Dynamic stuff. Filled in by various code */
    FILE*            File;        /* The opened image file */
    ULONGLONG        mbhdrOffset; /* Offset in file where header was found */
    MULTIBOOT_HEADER mbhdr;       /* The multiboot header in the file */
};

typedef struct _CONFIG
{
    ULONG  Timeout;   /* Wait this many seconds before booting default */
    IMAGE* Default;   /* Default image    */
    ULONG  nImages;   /* Number of images */

    IMAGE* Images;
} CONFIG;

BOOL ParseConfigFile( FILE* file, CONFIG* config );

#endif
