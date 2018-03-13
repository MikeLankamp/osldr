#ifndef LOADER_H
#define LOADER_H

#include <multiboot.h>

#include "config.h"

VOID LoadImage( IMAGE* image, MULTIBOOT_INFO* mbi );

#endif
