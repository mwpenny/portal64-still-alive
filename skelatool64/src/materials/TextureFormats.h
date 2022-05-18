#ifndef __TEXTURE_FORMATS_H__
#define __TEXTURE_FORMATS_H__

enum class G_IM_FMT {
    G_IM_FMT_RGBA,
    G_IM_FMT_YUV,
    G_IM_FMT_CI,
    G_IM_FMT_I,
    G_IM_FMT_IA,
};

const char* nameForImageFormat(G_IM_FMT format);

enum class G_IM_SIZ {
    G_IM_SIZ_4b,
    G_IM_SIZ_8b,
    G_IM_SIZ_16b,
    G_IM_SIZ_32b,
};

const char* nameForImageSize(G_IM_SIZ size);

bool isImageFormatSupported(G_IM_FMT format, G_IM_SIZ size);

int bitSizeforSiz(G_IM_SIZ input);

#endif