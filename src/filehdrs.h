#ifndef FILEHDRS_H
#define FILEHDRS_H

#include <types.h>

/*
 * Executable and Linkable Format
 */

#define EI_NIDENT 16

typedef struct _ELF32_HDR
{
    UCHAR  e_ident[EI_NIDENT];
    USHORT e_type;
    USHORT e_machine;
    ULONG  e_version;
    ULONG  e_entry;
    ULONG  e_phoff;
    ULONG  e_shoff;
    ULONG  e_flags;
    USHORT e_ehsize;
    USHORT e_phentsize;
    USHORT e_phnum;
    USHORT e_shentsize;
    USHORT e_shnum;
    USHORT e_shstrndx;
} ELF32_HDR;

/* Identification indeces */
#define EI_MAG0     0      /* File identification */
#define EI_MAG1     1      /* File identification */
#define EI_MAG2     2      /* File identification */
#define EI_MAG3     3      /* File identification */
#define EI_CLASS    4      /* File class    */
#define EI_DATA     5      /* Data encoding */
#define EI_VERSION  6      /* File version  */
#define EI_PAD      7      /* Start of padding bytes */

/* Magic number */
#define ELFMAG0     0x7F   /* e_ident[EI_MAG0] */
#define ELFMAG1     'E'    /* e_ident[EI_MAG1] */
#define ELFMAG2     'L'    /* e_ident[EI_MAG2] */
#define ELFMAG3     'F'    /* e_ident[EI_MAG3] */

/* Class */
#define ELFCLASSNONE 0     /* Invalid class  */
#define ELFCLASS32   1     /* 32-bit objects */
#define ELFCLASS64   2     /* 64-bit objects */

/* Data encoding */
#define ELFDATANONE  0     /* Invalid data encoding */
#define ELFDATA2LSB  1     /* Least Significant Byte first */
#define ELFDATA2MSB  2     /* Most Significant Byte first  */

/* Object file versions */
#define EV_NONE     0      /* Invalid version */
#define EV_CURRENT  1      /* Current version */

/* Object file types */
#define ET_NONE   0        /* No file type       */
#define ET_REL    1        /* Relocatable file   */
#define ET_EXEC   2        /* Executable file    */
#define ET_DYN    3        /* Shared object file */
#define ET_CORE   4        /* Core file          */
#define ET_LOPROC 0xFF00   /* Processor-specific (lower bound) */
#define ET_HIPROC 0xFFFF   /* Processor-specific (upper bound)  */

/* Machine types */
#define EM_NONE  0         /* No machine     */
#define EM_M32   1         /* AT&T WE 32100  */
#define EM_SPARC 2         /* SPARC          */
#define EM_386   3         /* Intel 80386    */
#define EM_68K   4         /* Motorola 68000 */
#define EM_88K   5         /* Motorola 88000 */
#define EM_860   7         /* Intel 80860    */
#define EM_MIPS  8         /* MIPS RS3000    */

typedef struct _ELF32_PHDR
{
    ULONG p_type;
    ULONG p_offset;
    ULONG p_vaddr;
    ULONG p_paddr;
    ULONG p_filesz;
    ULONG p_memsz;
    ULONG p_flags;
    ULONG p_align;
} ELF32_PHDR;

/* Segment types */
#define PT_NULL     0          /* Unused segment */
#define PT_LOAD     1          /* Loadable segment */
#define PT_DYNAMIC  2          /* Dynamic linking info */
#define PT_INTERP   3          /* Interpreter path */
#define PT_NOTE     4          /* Auxiliary info */
#define PT_SHLIB    5          /* Reserved */
#define PT_PHDR     6          /* Program header table */
#define PT_LOPROC   0x70000000 /* Processor-specific (lower bound) */
#define PT_HIPROC   0x7FFFFFFF /* Processor-specific (upper bound) */

/* Segment flags */
#define PF_X        1   /* Segment is executable */
#define PF_W        2   /* Segment is writable */
#define PF_R        4   /* Segment is readable */

typedef struct _ELF32_SHDR
{
    ULONG sh_name;
    ULONG sh_type;
    ULONG sh_flags;
    ULONG sh_addr;
    ULONG sh_offset;
    ULONG sh_size;
    ULONG sh_link;
    ULONG sh_info;
    ULONG sh_addralign;
    ULONG sh_entsize;
} ELF32_SHDR;

/* Section types */
#define SHT_NULL        0
#define SHT_PROGBITS    1
#define SHT_SYMTAB      2
#define SHT_STRTAB      3
#define SHT_RELA        4
#define SHT_HASH        5
#define SHT_DYNAMIC     6
#define SHT_NOTE        7
#define SHT_NOBITS      8
#define SHT_REL         9
#define SHT_SHLIB       10
#define SHT_DYNSYM      11
#define SHT_LOPROC      0x70000000
#define SHT_HIPROC      0x7fffffff
#define SHT_LOUSER      0x80000000
#define SHT_HIUSER      0xffffffff

/* Section flags */
#define SHF_WRITE       0x1
#define SHF_ALLOC       0x2
#define SHF_EXECINSTR   0x4
#define SHF_MASKPROC    0xf0000000
/*
 * Portable Executable / Common Object File Format
 */
#define IMAGE_NT_SIGNATURE 0x00004550  /* PE00 */

typedef struct _COFF_FILE_HEADER
{
    USHORT  Machine;
    USHORT  NumberOfSections;
    ULONG   TimeDateStamp;
    ULONG   PointerToSymbolTable;
    ULONG   NumberOfSymbols;
    USHORT  SizeOfOptionalHeader;
    USHORT  Characteristics;
} COFF_FILE_HEADER;

#define IMAGE_FILE_RELOCS_STRIPPED           0x0001  /* Relocation info stripped from file */
#define IMAGE_FILE_EXECUTABLE_IMAGE          0x0002  /* File is executable  (i.e. no unresolved externel references) */
#define IMAGE_FILE_LINE_NUMS_STRIPPED        0x0004  /* Line nunbers stripped from file */
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED       0x0008  /* Local symbols stripped from file */
#define IMAGE_FILE_AGGRESIVE_WS_TRIM         0x0010  /* Agressively trim working set */
#define IMAGE_FILE_LARGE_ADDRESS_AWARE       0x0020  /* App can handle >2gb addresses */
#define IMAGE_FILE_BYTES_REVERSED_LO         0x0080  /* Bytes of machine word are reversed */
#define IMAGE_FILE_32BIT_MACHINE             0x0100  /* 32 bit word machine */
#define IMAGE_FILE_DEBUG_STRIPPED            0x0200  /* Debugging info stripped from file in .DBG file */
#define IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP   0x0400  /* If Image is on removable media, copy and run from the swap file */
#define IMAGE_FILE_NET_RUN_FROM_SWAP         0x0800  /* If Image is on Net, copy and run from the swap file */
#define IMAGE_FILE_SYSTEM                    0x1000  /* System File */
#define IMAGE_FILE_DLL                       0x2000  /* File is a DLL */
#define IMAGE_FILE_UP_SYSTEM_ONLY            0x4000  /* File should only be run on a UP machine */
#define IMAGE_FILE_BYTES_REVERSED_HI         0x8000  /* Bytes of machine word are reversed */

#define IMAGE_FILE_MACHINE_UNKNOWN           0
#define IMAGE_FILE_MACHINE_I386              0x014c  /* Intel 386 */
#define IMAGE_FILE_MACHINE_R3000             0x0162  /* MIPS little-endian, 0x160 big-endian */
#define IMAGE_FILE_MACHINE_R4000             0x0166  /* MIPS little-endian */
#define IMAGE_FILE_MACHINE_R10000            0x0168  /* MIPS little-endian */
#define IMAGE_FILE_MACHINE_WCEMIPSV2         0x0169  /* MIPS little-endian WCE v2 */
#define IMAGE_FILE_MACHINE_ALPHA             0x0184  /* Alpha_AXP */
#define IMAGE_FILE_MACHINE_POWERPC           0x01F0  /* IBM PowerPC Little-Endian */
#define IMAGE_FILE_MACHINE_SH3               0x01a2  /* SH3 little-endian */
#define IMAGE_FILE_MACHINE_SH3E              0x01a4  /* SH3E little-endian */
#define IMAGE_FILE_MACHINE_SH4               0x01a6  /* SH4 little-endian */
#define IMAGE_FILE_MACHINE_ARM               0x01c0  /* ARM Little-Endian */
#define IMAGE_FILE_MACHINE_THUMB             0x01c2
#define IMAGE_FILE_MACHINE_IA64              0x0200  /* Intel 64 */
#define IMAGE_FILE_MACHINE_MIPS16            0x0266  /* MIPS */
#define IMAGE_FILE_MACHINE_MIPSFPU           0x0366  /* MIPS */
#define IMAGE_FILE_MACHINE_MIPSFPU16         0x0466  /* MIPS */
#define IMAGE_FILE_MACHINE_ALPHA64           0x0284  /* ALPHA64 */
#define IMAGE_FILE_MACHINE_AXP64             IMAGE_FILE_MACHINE_ALPHA64

typedef struct _COFF_OPTIONAL_HEADER
{
    USHORT  Magic;
    CHAR    MajorLinkerVersion;
    CHAR    MinorLinkerVersion;
    ULONG   SizeOfCode;
    ULONG   SizeOfInitializedData;
    ULONG   SizeOfUninitializedData;
    ULONG   AddressOfEntryPoint;
    ULONG   BaseOfCode;
    ULONG   BaseOfData;
} COFF_OPTIONAL_HEADER;

#define IMAGE_NT_OPTIONAL_HDR_MAGIC  0x10b

typedef struct _COFF_DATA_DIRECTORY
{
    ULONG   VirtualAddress;
    ULONG   Size;
} COFF_DATA_DIRECTORY;

typedef struct _COFF_OPTIONAL_NT_HEADER
{
    ULONG   ImageBase;
    ULONG   SectionAlignment;
    ULONG   FileAlignment;
    USHORT  MajorOperatingSystemVersion;
    USHORT  MinorOperatingSystemVersion;
    USHORT  MajorImageVersion;
    USHORT  MinorImageVersion;
    USHORT  MajorSubsystemVersion;
    USHORT  MinorSubsystemVersion;
    ULONG   Win32VersionValue;
    ULONG   SizeOfImage;
    ULONG   SizeOfHeaders;
    ULONG   CheckSum;
    USHORT  Subsystem;
    USHORT  DllCharacteristics;
    ULONG   SizeOfStackReserve;
    ULONG   SizeOfStackCommit;
    ULONG   SizeOfHeapReserve;
    ULONG   SizeOfHeapCommit;
    ULONG   LoaderFlags;
    ULONG   NumberOfRvaAndSizes;
    COFF_DATA_DIRECTORY DataDirectory[16];
} COFF_OPTIONAL_NT_HEADER;

typedef struct _COFF_SECTION_HEADER
{
    CHAR    Name[8];
    ULONG   VirtualSize;
    ULONG   VirtualAddress;
    ULONG   SizeOfRawData;
    ULONG   PointerToRawData;
    ULONG   PointerToRelocations;
    ULONG   PointerToLinenumbers;
    USHORT  NumberOfRelocations;
    USHORT  NumberOfLinenumbers;
    ULONG   Flags;
} COFF_SECTION_HEADER;

#define CSF_TEXT 0x20 /* Section contains executable code */
#define CSF_DATA 0x40 /* Section contains initialized data */
#define CSF_BSS  0x80 /* Section contains uninitialized data */

#endif
