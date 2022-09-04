#ifndef __TEXTURE_DEFINITION_H__
#define __TEXTURE_DEFINITION_H__

#include <vector>
#include <inttypes.h>

#include "TextureFormats.h"

#include "../definitions/DataChunk.h"
#include "../definitions/FileDefinition.h"

class DataChunkStream {
public:
    DataChunkStream();
    void WriteBytes(const char* data, int byteCount);
    void WriteBits(int from, int bitCount);

    const std::vector<uint64_t>& GetData();
private:
    void FlushBuffer();

    int mCurrentBufferPos;
    uint64_t mCurrentBuffer;
    std::vector<uint64_t> mData;
};

struct PixelRGBAu8 {
    PixelRGBAu8();
    PixelRGBAu8(uint8_t rVal, uint8_t gVal, uint8_t bVal, uint8_t aVal);

    bool operator==(const PixelRGBAu8& other) const;
    
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;

    bool WriteToStream(DataChunkStream& output, G_IM_SIZ size);
};

struct PixelIu8 {
    PixelIu8(uint8_t i);
    uint8_t i;

    bool WriteToStream(DataChunkStream& output, G_IM_SIZ size);
};

struct PixelIAu8 {
    PixelIAu8(uint8_t i, uint8_t a);
    uint8_t i;
    uint8_t a;

    bool WriteToStream(DataChunkStream& output, G_IM_SIZ size);
};

enum class TextureDefinitionEffect {
    TwoToneGrayscale = (1 << 0),
    NormalMap = (1 << 1),
    Invert = (1 << 2),
    SelectR = (1 << 3),
    SelectG = (1 << 4),
    SelectB = (1 << 5),
};

class PalleteDefinition {
public:
    PalleteDefinition(const std::string& filename);

    PixelIu8 FindIndex(PixelRGBAu8 color) const;

    std::unique_ptr<FileDefinition> GenerateDefinition(const std::string& name, const std::string& location) const;

    const std::string& Name() const;

    int LoadBlockSize() const;
    int DTX() const;
    unsigned ColorCount() const;
private:
    std::string mName;
    std::vector<PixelRGBAu8> mColors;
    std::vector<unsigned long long> mData;
};

class TextureDefinition {
public:
    TextureDefinition(const std::string& filename, G_IM_FMT fmt, G_IM_SIZ siz, TextureDefinitionEffect effects, std::shared_ptr<PalleteDefinition> pallete);

    static void DetermineIdealFormat(const std::string& filename, G_IM_FMT& fmt, G_IM_SIZ& siz);

    std::unique_ptr<FileDefinition> GenerateDefinition(const std::string& name, const std::string& location) const;

    int Width() const;
    int Height() const;

    G_IM_FMT Format() const;
    G_IM_SIZ Size() const;

    bool GetLine(int& line) const;
    int LoadBlockSize() const;
    int DTX() const;

    const std::string& Name() const;

    bool HasEffect(TextureDefinitionEffect effect) const;

    PixelRGBAu8 GetTwoToneMin() const;
    PixelRGBAu8 GetTwoToneMax() const;

    std::shared_ptr<PalleteDefinition> GetPallete() const;
private:
    std::string mName;
    G_IM_FMT mFmt;
    G_IM_SIZ mSiz;
    int mWidth;
    int mHeight;
    std::vector<unsigned long long> mData;
    std::shared_ptr<PalleteDefinition> mPallete;
    TextureDefinitionEffect mEffects;

    PixelRGBAu8 mTwoToneMin;
    PixelRGBAu8 mTwoToneMax;
};

#endif