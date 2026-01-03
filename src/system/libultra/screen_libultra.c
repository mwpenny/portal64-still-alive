#include "system/screen.h"

#include <ultra64.h>

uint16_t* screenGetCurrentFramebuffer() {
    return osViGetCurrentFramebuffer();
}
