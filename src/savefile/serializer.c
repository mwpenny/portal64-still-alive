#include "serializer.h"

#include "../scene/signals.h"
#include "../util/memory.h"

void serializeCount(struct Serializer* serializer, void* src, int size) {
    serializer->curr = (char*)serializer->curr + size;
}

void serializeWrite(struct Serializer* serializer, void* src, int size) {
    memCopy(serializer->curr, src, size);
    serializer->curr = (char*)serializer->curr + size;
}

void serializeRead(struct Serializer* serializer, void* dst, int size) {
    memCopy(dst, serializer->curr, size);
    serializer->curr = (char*)serializer->curr + size;
}