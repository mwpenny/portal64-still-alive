#ifndef __SERIALIZER_H__
#define __SERIALIZER_H__

struct Serializer {
    void* curr;
};

typedef void* Reader;

typedef void (*SerializeAction)(struct Serializer* serializer, void* target, int size);
typedef void (*SerializeCallback)(struct Serializer* serializer, SerializeAction action, void* data);

void serializeCount(struct Serializer* serializer, void* src, int size);
void serializeWrite(struct Serializer* serializer, void* src, int size);
void serializeRead(struct Serializer* serializer, void* dst, int size);

#endif