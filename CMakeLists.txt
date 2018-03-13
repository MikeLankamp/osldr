cmake_minimum_required(VERSION 2.8)

project(osldr)

enable_language(ASM-ATT)

add_executable(osldr
    # start.s has to come first in the link order; it contains the entry point
    # which has to be at the beginning of the file.
    src/start.s

    src/asm.s
    src/config.c
    src/conio.c
    src/ctype.c
    src/drive.c
    src/fat.c
    src/io.c
    src/loader.c
    src/main.c
    src/mem.c
    src/messages.c
    src/multiboot.c
    src/raw.c
    src/stdio.c
    src/stdlib.c
    src/string.s
    src/time.c
    src/video.c
)

target_include_directories(osldr
  PRIVATE
    src
)

target_compile_options(osldr
  PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-O2 -Wall -pedantic-errors -ffreestanding -fno-builtin -nostdinc -m32>
    $<$<COMPILE_LANGUAGE:ASM-ATT>:-32>
)

target_link_libraries(osldr
  PRIVATE
    gcc         # We need this for 64-bit arithmetic
)

set_target_properties(osldr
  PROPERTIES
    LINK_FLAGS "-nostdlib -m32 -T ${CMAKE_CURRENT_SOURCE_DIR}/osldr.djl"
)
