#include "CImgu8.h"


CImgu8::CImgu8(const std::string& filename) : mImg(filename.c_str()) {
    
}

CImgu8::CImgu8(const cimg_library::CImg<unsigned char>& img) : mImg(img) {

}