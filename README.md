# osldr
`osldr` is a stage-2 bootloader that is able to boot operating systems from files in the ELF, PE/COFF, Multiboot file format, or from disk directly by loading as a bootsector. It only supports the FAT file format for reading from disks. It features an editable boot.ini file and a boot menu where the user can select one of the configured operating systems.

## Prerequisites
* Before building, please make sure `gcc` and GNU binutils (`as` and `ld`) are installed. No other compilers or toolchains have been tested.
* If building on a 64-bit operating system, please make sure a 32-bit `libgcc` is available, e.g. by doing `apt-get install gcc-multilib`.

## Building
To build, run `cmake` and `make`:
```
mkdir build
cd build
cmake ..
make
```

## Running
Take the `osldr` raw binary produced in the build and place it in the root directory of a FAT-formatted partition or disk, and load it with a stage-1 bootloader. An example `boot.ini` file is provided that should be placed in the root directory of the partition or disk as well.
