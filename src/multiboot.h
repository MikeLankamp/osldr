#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <io.h>

typedef struct _MMAP_ENTRY
{
    ULONG     size;
    ULONGLONG Start;
    ULONGLONG Length;
    ULONG     Type;
} MMAP_ENTRY;

/* Multiboot magic value */
#define MULTIBOOT_MAGIC 0x1BADB002

#define MULTIBOOT_REQUIRED_MASK    0x0000FFFF
#define MULTIBOOT_SUPPORTED_FLAGS (MIF_WANT_PAGE_ALIGN | MIF_WANT_SIMPLE_MEMORY | MIF_WANT_GRAPHICS)

/* Values for MULTIBOOT_IMAGE.Flags */
#define MIF_WANT_PAGE_ALIGN    0x00001
#define MIF_WANT_SIMPLE_MEMORY 0x00002
#define MIF_WANT_GRAPHICS      0x00004
#define MIF_HAS_ADDRESS        0x10000

typedef struct _MULTIBOOT_HEADER
{
    ULONG Magic;
    ULONG Flags;
    ULONG Checksum;

    /* Valid if flags & MIF_ADDRESS */
    ULONG HeaderAddr;
    ULONG LoadAddr;
    ULONG LoadEndAddr;
    ULONG BssEndAddr;
    ULONG EntryAddr;

    /* Valid if flags & MIF_GRAPHICS */
    ULONG ModeType;
    ULONG Width;
    ULONG Height;
    ULONG Depth;
} PACKED MULTIBOOT_HEADER;

typedef struct _APM_TABLE
{
    USHORT Version;         /* Version = (Version >> 8).(Version & 0xFF) */
    USHORT CSeg;            /* Protected mode 32-bit code segment */
    ULONG  Offset;          /* Entry point offset */
    USHORT CSeg16;          /* Protected mode 16-bit code segment */
    USHORT DSeg;            /* Protected mode 16-bit data segment */
    USHORT Flags;           /* Flags */
    USHORT CSegLen;         /* Protected mode 32-bit code segment length */
    USHORT CSeg16Len;       /* Protected mode 16-bit code segment length  */
    USHORT DSegLen;         /* Protected mode 16-bit data segment length  */
} PACKED APM_TABLE;

typedef struct _VBE_INFO_BLOCK
{
    CHAR   VbeSignature[4];   /* VBE Signature ("VESA") */
    USHORT VbeVersion;        /* VBE Version */
    ULONG  OemStringPtr;      /* VbeFarPtr to OEM String */
    ULONG  Capabilities;      /* Capabilities of graphics controller */
    ULONG  VideoModePtr;      /* VbeFarPtr to VideoModeList */
    USHORT TotalMemory;       /* Number of 64kb memory blocks */
                              /* Added for VBE 2.0+ */
    USHORT OemSoftwareRe;     /* VBE implementation Software revision */
    ULONG  OemVendorNamePt;   /* VbeFarPtr to Vendor Name String */
    ULONG  OemProductNamePtr; /* VbeFarPtr to Product Name String */
    ULONG  OemProductRevPtr;  /* VbeFarPtr to Product Revision String */
    UCHAR  Reserved[222];     /* Reserved for VBE implementation scratch area */
    UCHAR  OemData[ 256 ];    /* Data Area for OEM Strings */
} PACKED VBE_INFO_BLOCK;

typedef struct _VBE_MODE_INFO
{
    /* Mandatory information for all VBE revisions */
    USHORT ModeAttributes;        /* mode attributes */
    UCHAR  WinAAttributes;        /* window A attributes */
    UCHAR  WinBAttributes;        /* window B attributes */
    USHORT WinGranularity;        /* window granularity */
    USHORT WinSize;               /* window size */
    USHORT WinASegment;           /* window A start segment */
    USHORT WinBSegment;           /* window B start segment */
    ULONG  WinFuncPtr;            /* real mode pointer to window function */
    USHORT BytesPerScanLine;      /* bytes per scan line */

    /* Mandatory information for VBE 1.2 and above */
    USHORT XResolution;           /* horizontal resolution in pixels or characters */
    USHORT YResolution;           /* vertical resolution in pixels or characters */
    UCHAR  XCharSize;             /* character cell width in pixels */
    UCHAR  YCharSize;             /* character cell height in pixels */
    UCHAR  NumberOfPlanes;        /* number of memory planes */
    UCHAR  BitsPerPixel;          /* bits per pixel */
    UCHAR  NumberOfBanks;         /* number of banks */
    UCHAR  MemoryModel;           /* memory model type */
    UCHAR  BankSize;              /* bank size in KB */
    UCHAR  NumberOfImagePages;    /* number of images */
    UCHAR  Reserved1;             /* reserved for page function */

    /* Direct Color fields (required for direct/6 and YUV/7 memory models) */
    UCHAR  RedMaskSize;           /* size of direct color red mask in bits */
    UCHAR  RedFieldPosition;      /* bit position of lsb of red mask */
    UCHAR  GreenMaskSize;         /* size of direct color green mask in bits */
    UCHAR  GreenFieldPosition;    /* bit position of lsb of green mask */
    UCHAR  BlueMaskSize;          /* size of direct color blue mask in bits */
    UCHAR  BlueFieldPosition;     /* bit position of lsb of blue mask */
    UCHAR  RsvdMaskSize;          /* size of direct color reserved mask in bits */
    UCHAR  RsvdFieldPosition;     /* bit position of lsb of reserved mask */
    UCHAR  DirectColorModeInfo;   /* direct color mode attributes */

    /* Mandatory information for VBE 2.0 and above */
    ULONG  PhysBasePtr;           /* physical address for flat memory frame buffer */
    ULONG  Reserved2;             /* Reserved - always set to 0 */
    USHORT Reserved3;             /* Reserved - always set to 0 */

    /* Mandatory information for VBE 3.0 and above */
    USHORT LinBytesPerScanLine;   /* bytes per scan line for linear modes */
    UCHAR  BnkNumberOfImagePages; /* number of images for banked modes */
    UCHAR  LinNumberOfImagePages; /* number of images for linear modes */
    UCHAR  LinRedMaskSize;        /* size of direct color red mask (linear modes) */
    UCHAR  LinRedFieldPosition;   /* bit position of lsb of red mask (linear modes) */
    UCHAR  LinGreenMaskSize;      /* size of direct color green mask (linear modes) */
    UCHAR  LinGreenFieldPosition; /* bit position of lsb of green mask (linear modes) */
    UCHAR  LinBlueMaskSize;       /* size of direct color blue mask (linear modes) */
    UCHAR  LinBlueFieldPosition;  /* bit position of lsb of blue mask (linear modes) */
    UCHAR  LinRsvdMaskSize;       /* size of direct color reserved mask (linear modes) */
    UCHAR  LinRsvdFieldPosition;  /* bit position of lsb of reserved mask (linear modes) */
    ULONG  MaxPixelClock;         /* maximum pixel clock (in Hz) for graphics mode */
    UCHAR  Reserved4[190];        /* remainder of ModeInfoBlock */
} PACKED VBE_MODE_INFO;

typedef struct _VBE_INFO
{
    VBE_INFO_BLOCK* VbeControlInfo;
    VBE_MODE_INFO*  VbeModeInfo;
    USHORT          VbeMode;
    USHORT          VbeInterfaceSeg;
    USHORT          VbeInterfaceOff;
    USHORT          VbeInterfaceLen;
} PACKED VBE_INFO;

/* Values for MULTIBOOT_INFO.Flags */
#define MIF_SIMPLE_MEMORY   0x0001
#define MIF_BOOT_DEVICE     0x0002
#define MIF_CMDLINE         0x0004
#define MIF_MODULES         0x0008
#define MIF_SYMBOLS         0x0010
#define MIF_SECTIONS        0x0020
#define MIF_MEMORY_MAP      0x0040
#define MIF_DRIVES          0x0080
#define MIF_CONFIG          0x0100
#define MIF_LOADER_NAME     0x0200
#define MIF_APM             0x0400
#define MIF_GRAPHICS        0x0800

typedef struct _MODULE
{
    ULONG ModStart;
    ULONG ModEnd;
    CHAR* String;
    ULONG Reserved;
} PACKED MODULE;

typedef struct _MULTIBOOT_INFO
{
    ULONG Flags;

    ULONG MemLower;
    ULONG MemUpper;

    ULONG BootDevice;

    CHAR* CmdLine;

    ULONG   ModuleCount;
    MODULE* ModuleAddress;

    ULONG Syms[4];

    ULONG MemoryMapLength;
    VOID* MemoryMapAddress;

    ULONG DrivesLength;
    VOID* DrivesAddress;

    VOID* ConfigTable;

    CHAR* BootLoaderName;

    APM_TABLE* ApmTable;

    VBE_INFO VbeInfo;

    /* Other stuff to be added */

} PACKED MULTIBOOT_INFO;

BOOL GetSystemInformation( MULTIBOOT_INFO* mbi, MULTIBOOT_HEADER* mbhdr );
VOID LoadModules( MULTIBOOT_INFO* mbi, MODULE* Modules, ULONG nModules, BOOL PageAlign );
VOID CallAsMultiboot( ULONG EntryAddr, MULTIBOOT_INFO* mbi );

#endif

