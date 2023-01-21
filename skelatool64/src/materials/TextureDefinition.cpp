#define cimg_display 0
#define cimg_use_png
#define cimg_use_tiff
#include "../../cimg/CImg.h"

#include "TextureDefinition.h"
#include "../FileUtils.h"
#include "../math/LeastSquares.h"

#include <iomanip>
#include <algorithm>
#include <iomanip>
#include <assimp/vector3.h>
#include <assimp/vector3.inl>

DataChunkStream::DataChunkStream() :
    mCurrentBufferPos(0),
    mCurrentBuffer(0) {}

void DataChunkStream::WriteBytes(const char* data, int byteCount) {
    for (int i = 0; i < byteCount; ++i) {
        WriteBits(data[i], 8);
    }
}

void DataChunkStream::WriteBits(int from, int bitCount) {
    if (!bitCount) {
        return;
    } else if (bitCount + mCurrentBufferPos > 64) {
        int firstChunkSize = 64 - mCurrentBufferPos;
        int secondChunkSize = bitCount - firstChunkSize;

        WriteBits(from >> secondChunkSize, firstChunkSize);
        FlushBuffer();
        WriteBits(from, secondChunkSize);
    } else {
        uint64_t mask = ~(~(uint64_t)0 << bitCount);
        mCurrentBuffer |= (from & mask) << (64 - mCurrentBufferPos - bitCount);
        mCurrentBufferPos += bitCount;
    }
}

const std::vector<uint64_t>& DataChunkStream::GetData() {
    FlushBuffer();
    return mData;
}

void DataChunkStream::FlushBuffer() {
    if (mCurrentBufferPos == 0) {
        return;
    }

    mData.push_back(mCurrentBuffer);

    mCurrentBufferPos = 0;
    mCurrentBuffer = 0;
}

PixelRGBAu8::PixelRGBAu8() : r(0), g(0), b(0), a(0) {}
PixelRGBAu8::PixelRGBAu8(uint8_t rVal, uint8_t gVal, uint8_t bVal, uint8_t aVal) : 
    r(rVal), g(gVal), b(bVal), a(aVal) {}

bool PixelRGBAu8::operator==(const PixelRGBAu8& other) const {
    return r == other.r && g == other.g && b == other.b && a == other.a;
}

bool PixelRGBAu8::WriteToStream(DataChunkStream& output, G_IM_SIZ size) {
    switch (size) {
        case G_IM_SIZ::G_IM_SIZ_32b:
            output.WriteBytes((const char*)this, sizeof(PixelRGBAu8));
            return true;
        case G_IM_SIZ::G_IM_SIZ_16b:
            output.WriteBits(r >> 3, 5);
            output.WriteBits(g >> 3, 5);
            output.WriteBits(b >> 3, 5);
            output.WriteBits(a >> 7, 1);
            return true;
        default:
            return false;

    }
}

PixelIu8::PixelIu8(uint8_t i) : i(i) {}

bool PixelIu8::WriteToStream(DataChunkStream& output, G_IM_SIZ size) {
    switch (size) {
        case G_IM_SIZ::G_IM_SIZ_8b:
            output.WriteBits(i, 8);
            return true;
        case G_IM_SIZ::G_IM_SIZ_4b:
            output.WriteBits(i >> 4, 4);
            return true;
        default:
            return false;
    }
}

PixelIAu8::PixelIAu8(uint8_t i, uint8_t a) : i(i), a(a) {}

bool PixelIAu8::WriteToStream(DataChunkStream& output, G_IM_SIZ size) {
    switch (size) {
        case G_IM_SIZ::G_IM_SIZ_16b:
            output.WriteBits(i, 8);
            output.WriteBits(a, 8);
            return true;
        case G_IM_SIZ::G_IM_SIZ_8b:
            output.WriteBits(i >> 4, 4);
            output.WriteBits(a >> 4, 4);
            return true;
        case G_IM_SIZ::G_IM_SIZ_4b:
            output.WriteBits(i >> 5, 3);
            output.WriteBits(a >> 7, 1);
            return true;
        default:
            return false;
    }
}

struct PixelRGBAu8 readRGBAPixel(cimg_library_suffixed::CImg<unsigned char>& input, int x, int y) {
    struct PixelRGBAu8 result;

    result.r = 0;
    result.g = 0;
    result.b = 1;
    result.a = 0xFF;
    
    switch (input.spectrum()) {
        case 4:
            result.a = input(x, y, 0, 3);
        case 3:
            result.r = input(x, y, 0, 0);
            result.g = input(x, y, 0, 1);
            result.b = input(x, y, 0, 2);
            break;
        case 2:
            result.a = input(x, y, 0, 1);
        case 1:
            result.r = input(x, y, 0, 0);
            result.g = input(x, y, 0, 0);
            result.b = input(x, y, 0, 0);
            break;
    }
    
    return result;
}

struct PixelIu8 readIPixel(cimg_library_suffixed::CImg<unsigned char>& input, int x, int y) {
    switch (input.spectrum()) {
        case 4:
        case 3:
            return PixelIu8((
                input(x, y, 0, 0) * 85 +
                input(x, y, 0, 1) * 86 +
                input(x, y, 0, 2) * 85
            ) >> 8);
        case 2:
        case 1:
            return PixelIu8(input(x, y, 0, 0));
    }

    return PixelIu8(0);
}

struct PixelIAu8 readIAPixel(cimg_library_suffixed::CImg<unsigned char>& input, int x, int y) {
    uint8_t alpha = 0xFF;

    switch (input.spectrum()) {
        case 4:
            alpha = input(x, y, 0, 3);
        case 3:
            return PixelIAu8((
                input(x, y, 0, 0) * 85 +
                input(x, y, 0, 1) * 86 +
                input(x, y, 0, 2) * 85
            ) >> 8, alpha);
        case 2:
            alpha = input(x, y, 0, 1);
        case 1:
            return PixelIAu8(input(x, y, 0, 0), alpha);
    }

    return PixelIAu8(0, alpha);
}

void writeRGBAPixel(cimg_library_suffixed::CImg<unsigned char>& input, int x, int y, struct PixelRGBAu8 value) {
    switch (input.spectrum()) {
        case 4:
            input(x, y, 0, 3) = value.a;
        case 3:
            input(x, y, 0, 0) = value.r;
            input(x, y, 0, 1) = value.g;
            input(x, y, 0, 2) = value.b;
            break;
        case 2:
            input(x, y, 0, 1) = value.a;
        case 1:
            input(x, y, 0, 0) = value.r;
            break;
    }
}

void writeIAPixel(cimg_library_suffixed::CImg<unsigned char>& input, int x, int y, struct PixelIAu8 value) {
    switch (input.spectrum()) {
        case 4:
            input(x, y, 0, 3) = value.a;
        case 3:
            input(x, y, 0, 0) = value.i;
            input(x, y, 0, 1) = value.i;
            input(x, y, 0, 2) = value.i;
            break;
        case 2:
            input(x, y, 0, 1) = value.a;
        case 1:
            input(x, y, 0, 0) = value.i;
            break;
    }
}

bool convertPixel(cimg_library_suffixed::CImg<unsigned char>& input, int x, int y, DataChunkStream& output, G_IM_FMT fmt, G_IM_SIZ siz, const std::shared_ptr<PalleteDefinition>& pallete) {
    switch (fmt) {
        case G_IM_FMT::G_IM_FMT_RGBA: {
            PixelRGBAu8 pixel = readRGBAPixel(input, x, y);
            return pixel.WriteToStream(output, siz);
        }
        case G_IM_FMT::G_IM_FMT_I: {
            PixelIu8 pixel = readIPixel(input, x, y);
            return pixel.WriteToStream(output, siz);
        }
        case G_IM_FMT::G_IM_FMT_IA: {
            PixelIAu8 pixel = readIAPixel(input, x, y);
            return pixel.WriteToStream(output, siz);
        }
        case G_IM_FMT::G_IM_FMT_CI: {
            PixelIu8 pixel = pallete ? pallete->FindIndex(readRGBAPixel(input, x, y)) : readIPixel(input, x, y);
            return pixel.WriteToStream(output, siz);
        }
        default:
            return false;
    }
}

const char* gFormatShortName[] = {
    "rgba",
    "yuv",
    "ci",
    "i",
    "ia",
};

const char* gSizeName[] = {
    "4b",
    "8b",
    "16b",
    "32b",
};

uint8_t interpolateGrayscale(int min, int max, uint8_t input) {
    if (input <= min) {
        return 0;
    }

    int result = 0x100 * (input - min + 1) / (max - min + 1) - 1;

    if (result > 0xFF) {
        return 0xFF;
    }

    return result;
}

uint8_t floatToByte(float input) {
    int result = (int)(input + 0.5f);

    if (result < 0) {
        return 0;
    }

    if (result > 0xFF) {
        return 0xFF;
    }

    return result;
}

void applyTwoToneEffect(cimg_library_suffixed::CImg<unsigned char>& input, PixelRGBAu8& maxColor, PixelRGBAu8& minColor) {
    LinearLeastSquares r;
    LinearLeastSquares g;
    LinearLeastSquares b;
    LinearLeastSquares a;

    int minGray = 0xFF;
    int maxGray = 0;

    int minAlpha = 0xFF;
    int maxAlpha = 0;

    for (int y = 0; y < input.height(); ++y) {
        for (int x = 0; x < input.width(); ++x) {
            PixelRGBAu8 colorValue = readRGBAPixel(input, x, y);
            PixelIAu8 grayScaleValue = readIAPixel(input, x, y);

            r.AddDataPoint(grayScaleValue.i, colorValue.r);
            g.AddDataPoint(grayScaleValue.i, colorValue.g);
            b.AddDataPoint(grayScaleValue.i, colorValue.b);
            a.AddDataPoint(grayScaleValue.a, colorValue.a);

            minGray = std::min((int)grayScaleValue.i, minGray);
            maxGray = std::max((int)grayScaleValue.i, maxGray);
            minAlpha = std::min((int)grayScaleValue.i, minAlpha);
            maxAlpha = std::max((int)grayScaleValue.i, maxAlpha);
        }
    }


    for (int y = 0; y < input.height(); ++y) {
        for (int x = 0; x < input.width(); ++x) {
            PixelIAu8 grayScaleValue = readIAPixel(input, x, y);
            grayScaleValue.i = interpolateGrayscale(minGray, maxGray, grayScaleValue.i);
            grayScaleValue.a = interpolateGrayscale(minAlpha, maxAlpha, grayScaleValue.a);
            writeIAPixel(input, x, y, grayScaleValue);
        }
    }

    maxColor.r = floatToByte(r.PredictY(maxGray));
    maxColor.g = floatToByte(g.PredictY(maxGray));
    maxColor.b = floatToByte(b.PredictY(maxGray));
    maxColor.a = floatToByte(a.PredictY(maxAlpha));

    minColor.r = floatToByte(r.PredictY(minGray));
    minColor.g = floatToByte(g.PredictY(minGray));
    minColor.b = floatToByte(b.PredictY(minGray));
    minColor.a = floatToByte(a.PredictY(minAlpha));
}

#define NORMAL_45_STEEPNESS 16

void calculateNormalMap(cimg_library_suffixed::CImg<unsigned char>& input) {
    cimg_library_suffixed::CImg<unsigned char> result(input.width(), input.height(), 1, 3);

    for (int y = 0; y < input.height(); ++y) {
        for (int x = 0; x < input.width(); ++x) {
            PixelIAu8 colorValue = readIAPixel(input, x, y);
            PixelIAu8 nextX = readIAPixel(input, (x + 1) % input.width(), y);
            PixelIAu8 nextY = readIAPixel(input, x, (y + 1) % input.height());
            
            aiVector3D xDir(NORMAL_45_STEEPNESS, 0, (nextX.i - colorValue.i) * colorValue.a * (1.0f / 256.0f));
            aiVector3D yDir(0, NORMAL_45_STEEPNESS, (nextY.i - colorValue.i) * colorValue.a * (1.0f / 256.0f));

            aiVector3D normal = xDir ^ yDir;
            normal.Normalize();

            writeRGBAPixel(result, x, y, PixelRGBAu8(
                (uint8_t)(127.0f * normal.x + 127.0f),
                (uint8_t)(127.0f * normal.y + 127.0f),
                (uint8_t)(127.0f * normal.z + 127.0f),
                255
            ));
        }
    }

    input = result;
}

void invertImage(cimg_library_suffixed::CImg<unsigned char>& input) {
    for (int y = 0; y < input.height(); ++y) {
        for (int x = 0; x < input.width(); ++x) {
            PixelRGBAu8 colorValue = readRGBAPixel(input, x, y);
            writeRGBAPixel(input, x, y, PixelRGBAu8(0xFF - colorValue.r, 0xFF - colorValue.g, 0xFF - colorValue.b, colorValue.a));
        }
    }
}

void selectChannel(cimg_library_suffixed::CImg<unsigned char>& input, TextureDefinitionEffect effects) {
    for (int y = 0; y < input.height(); ++y) {
        for (int x = 0; x < input.width(); ++x) {
            PixelRGBAu8 colorValue = readRGBAPixel(input, x, y);

            if ((int)effects & (int)TextureDefinitionEffect::SelectR) {
                writeIAPixel(input, x, y, PixelIAu8(colorValue.r, colorValue.a));
            } else if ((int)effects & (int)TextureDefinitionEffect::SelectG) {
                writeIAPixel(input, x, y, PixelIAu8(colorValue.g, colorValue.a));
            } else if ((int)effects & (int)TextureDefinitionEffect::SelectB) {
                writeIAPixel(input, x, y, PixelIAu8(colorValue.b, colorValue.a));
            }
        }
    }
}

PalleteDefinition::PalleteDefinition(const std::string& filename):
    mName(getBaseName(replaceExtension(filename, "")) + "_tlut") {
    cimg_library_suffixed::CImg<unsigned char> imageData(filename.c_str());

    DataChunkStream dataStream;
    
    for (int y = 0; y < imageData.height(); ++y) {
        for (int x = 0; x < imageData.width(); ++x) {
            PixelRGBAu8 colorValue = readRGBAPixel(imageData, x, y);
            mColors.push_back(colorValue);

            colorValue.WriteToStream(dataStream, G_IM_SIZ::G_IM_SIZ_16b);
        }
    }

    auto data = dataStream.GetData();
    mData.resize(data.size());

    std::copy(data.begin(), data.end(), mData.begin());
}

PixelIu8 PalleteDefinition::FindIndex(PixelRGBAu8 color) const {
    unsigned result = 0;
    unsigned distance = ~0;

    for (unsigned i = 0; i < mColors.size(); ++i) {
        auto& colorAtIndex = mColors[i];
        int rOffset = (int)colorAtIndex.r - (int)color.r;
        int gOffset = (int)colorAtIndex.g - (int)color.g;
        int bOffset = (int)colorAtIndex.b - (int)color.b;

        unsigned currentDistance = rOffset * rOffset + gOffset * gOffset + bOffset * bOffset;

        if (currentDistance < distance) {
            distance = currentDistance;
            result = i;
        }
    }

    return PixelIu8(result);
}


std::unique_ptr<FileDefinition> PalleteDefinition::GenerateDefinition(const std::string& name, const std::string& location) const {
    std::unique_ptr<StructureDataChunk> dataChunk(new StructureDataChunk());

    for (unsigned chunkIndex = 0; chunkIndex < mData.size(); ++chunkIndex) {
        std::ostringstream stream;
        stream << "0x" << std::hex << std::setw(16) << std::setfill('0') << mData[chunkIndex];
        dataChunk->AddPrimitive(stream.str());
    }

    return std::unique_ptr<FileDefinition>(new DataFileDefinition("u64", name, true, location, std::move(dataChunk), this));
}

const std::string& PalleteDefinition::Name() const {
    return mName;
}


int gSizeInc[] = {3, 1, 0, 0};
int gSizeShift[] = {2, 1, 0, 0};

int PalleteDefinition::LoadBlockSize() const {
    return mColors.size() - 1;
}

#define	G_TX_DTX_FRAC	11

int PalleteDefinition::DTX() const {
    int lineSize = mColors.size() / 4;

    if (!lineSize) {
        lineSize = 1;
    }
    return ((1 << G_TX_DTX_FRAC) + lineSize - 1) / lineSize;
}

int PalleteDefinition::NBytes() const {
    return mColors.size() * 2;
}

unsigned PalleteDefinition::ColorCount() const {
    return mColors.size();
}

TextureDefinition::TextureDefinition(const std::string& filename, G_IM_FMT fmt, G_IM_SIZ siz, TextureDefinitionEffect effects, std::shared_ptr<PalleteDefinition> pallete) :
    mName(getBaseName(replaceExtension(filename, "")) + "_" + gFormatShortName[(int)fmt] + "_" + gSizeName[(int)siz]),
    mFmt(fmt),
    mSiz(siz),
    mPallete(pallete),
    mEffects(effects) {

    cimg_library_suffixed::CImg<unsigned char> imageData(filename.c_str());

    if (HasEffect(TextureDefinitionEffect::TwoToneGrayscale)) {
        applyTwoToneEffect(imageData, mTwoToneMax, mTwoToneMin);
    }

    if (HasEffect(TextureDefinitionEffect::NormalMap)) {
        calculateNormalMap(imageData);
    }

    if (HasEffect(TextureDefinitionEffect::Invert)) {
        invertImage(imageData);
    }

    if (HasEffect(TextureDefinitionEffect::SelectR) || 
        HasEffect(TextureDefinitionEffect::SelectG) || 
        HasEffect(TextureDefinitionEffect::SelectB)) {
        selectChannel(imageData, mEffects);
    }

    mWidth = imageData.width();
    mHeight = imageData.height();

    DataChunkStream dataStream;

    for (int y = 0; y < mHeight; ++y) {
        for (int x = 0; x < mWidth; ++x) {
            convertPixel(imageData, x, y, dataStream, fmt, siz, pallete);
        }
    }

    auto data = dataStream.GetData();
    mData.resize(data.size());

    std::copy(data.begin(), data.end(), mData.begin());

    if (pallete) {
        mFmt = G_IM_FMT::G_IM_FMT_CI;
        mSiz = pallete->ColorCount() <= 16 ? G_IM_SIZ::G_IM_SIZ_4b : G_IM_SIZ::G_IM_SIZ_8b;
    }
}

bool isGrayscale(cimg_library_suffixed::CImg<unsigned char>& input, int x, int y) {
    switch (input.spectrum()) {
        case 1:
        case 2:
            return true;
        case 3:
        case 4:
            return input(x, y, 0, 0) == input(x, y, 0, 1) && input(x, y, 0, 1) == input(x, y, 0, 2);
    }

    return false;
}

int colorHash(cimg_library_suffixed::CImg<unsigned char>& input, int x, int y) {
    switch (input.spectrum()) {
        case 1:
        case 2:
            return input(x, y, 0, 0);
        case 3:
        case 4:
            return (input(x, y, 0, 0) << 24) | (input(x, y, 0, 1) << 16) | (input(x, y, 0, 2) << 8);
    }

    return 0;
}

void TextureDefinition::DetermineIdealFormat(const std::string& filename, G_IM_FMT& fmt, G_IM_SIZ& siz) {
    cimg_library_suffixed::CImg<unsigned char> imageData(filename.c_str());

    bool hasColor = false;
    bool hasFullTransparency = false;
    bool hasPartialTransparency = false;
    std::set<int> colorCount;

    for (int y = 0; y < imageData.height(); ++y) {
        for (int x = 0; x < imageData.width(); ++x) {
            colorCount.insert(colorHash(imageData, x, y));
            bool isPixelGrayscale = isGrayscale(imageData, x, y);
            hasColor = hasColor || !isPixelGrayscale;
            unsigned char alpha = imageData.spectrum() == 4 ? imageData(x, y, 0, 3) : 0xFF;

            hasPartialTransparency = hasPartialTransparency || (alpha != 0 && alpha != 0xFF);
            hasFullTransparency = hasFullTransparency || alpha == 0;
        }
    }

    if (hasColor) {
        if (hasPartialTransparency) {
            fmt = G_IM_FMT::G_IM_FMT_RGBA;
            siz = G_IM_SIZ::G_IM_SIZ_32b;
        } else {
            fmt = G_IM_FMT::G_IM_FMT_RGBA;
            siz = G_IM_SIZ::G_IM_SIZ_16b;
        }
    } else {
        if (hasPartialTransparency || hasFullTransparency) {
            fmt = G_IM_FMT::G_IM_FMT_IA;
            siz = G_IM_SIZ::G_IM_SIZ_16b;
        } else {
            fmt = G_IM_FMT::G_IM_FMT_I;
            siz = G_IM_SIZ::G_IM_SIZ_8b;
        }
    }
}

std::unique_ptr<FileDefinition> TextureDefinition::GenerateDefinition(const std::string& name, const std::string& location) const {
    std::unique_ptr<StructureDataChunk> dataChunk(new StructureDataChunk());

    int line;
    int index = 0;

    GetLine(line);

    for (int y = 0; y < mHeight; ++y) {
        std::ostringstream stream;

        for (int lineIndex = 0; lineIndex < line; ++lineIndex) {
            uint64_t data = mData[index];

            if (lineIndex != 0) {
                stream << ", ";
            }

            stream << "0x" << std::hex << std::setw(16) << std::setfill('0') << data;

            ++index;
        }

        dataChunk->AddPrimitive(stream.str());
    }

    return std::unique_ptr<FileDefinition>(new DataFileDefinition("u64", name, true, location, std::move(dataChunk), this));
}

int TextureDefinition::Width() const {
    return mWidth;
}

int TextureDefinition::Height() const {
    return mHeight;
}

G_IM_FMT TextureDefinition::Format() const {
    return mFmt;
}

G_IM_SIZ TextureDefinition::Size() const {
    return mSiz;
}

int TextureDefinition::LoadBlockSize() const {
    return ((Height() * Width() + gSizeInc[(int)mSiz]) >> gSizeShift[(int)mSiz]) - 1;
}

int TextureDefinition::DTX() const {
    int lineSize;

    if (mSiz == G_IM_SIZ::G_IM_SIZ_4b) {
        lineSize = Width() / 16;
    } else {
        GetLine(lineSize);
    }
    if (!lineSize) {
        lineSize = 1;
    }
    return ((1 << G_TX_DTX_FRAC) + lineSize - 1) / lineSize;
}

int TextureDefinition::NBytes() const {
    int line;
    GetLine(line);
    return mHeight * line * 8;
}

bool TextureDefinition::GetLine(int& line) const {
    int bitLine = bitSizeforSiz(mSiz) * mWidth;
    line = bitLine / 64;
    return bitLine % 64 == 0;
}

bool TextureDefinition::GetLineForTile(int& line) const {
    int bitLine = lineSizeForSize(mSiz) * mWidth;
    line = bitLine / 64;
    return bitLine % 64 == 0;
}

const std::string& TextureDefinition::Name() const {
    return mName;
}

bool TextureDefinition::HasEffect(TextureDefinitionEffect effect) const {
    return (int)mEffects & (int)effect;
}

PixelRGBAu8 TextureDefinition::GetTwoToneMin() const {
    return mTwoToneMin;
}

PixelRGBAu8 TextureDefinition::GetTwoToneMax() const {
    return mTwoToneMax;
}

std::shared_ptr<PalleteDefinition> TextureDefinition::GetPallete() const {
    return mPallete;
}