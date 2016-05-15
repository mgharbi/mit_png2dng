//
//  png_load.hpp
//  dng_converter
//
//  Created by mgharbi on 5/15/16.
//  Copyright © 2016 mgharbi. All rights reserved.
//

#ifndef png_load_hpp
#define png_load_hpp

#include <vector>

std::vector<unsigned char> png_load(const char* filename, unsigned &width, unsigned &height);

#endif /* png_load_hpp */
