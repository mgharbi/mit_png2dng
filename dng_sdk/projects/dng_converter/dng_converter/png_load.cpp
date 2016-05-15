#include <iostream>
#include "lodepng.hpp"

std::vector<unsigned char> png_load(const char* filename, unsigned &width, unsigned &height)
{
    std::vector<unsigned char> image; //the raw pixels
    
    //decode
    unsigned error = lodepng::decode(image, width, height, filename);
    
    //if there's an error, display it
    if(error) std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
    
    
    //the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...
    return image;
}
