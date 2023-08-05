#ifndef __CIMG_U8_H__
#define __CIMG_U8_H__

#define cimg_display 0
#define cimg_use_png
#define cimg_use_tiff
// This is a massive header file and I don't
// want it to drive my compile times up by being
// included in every file. That is whey I created
// The CImgu8 class. You should not include this header
// file from other header files
#include "../../cimg/CImg.h"

#include <string>

class CImgu8 {
public:
    CImgu8(const std::string& filename);
    CImgu8(const cimg_library_suffixed::CImg<unsigned char>& img);
    cimg_library_suffixed::CImg<unsigned char> mImg;
};

#endif