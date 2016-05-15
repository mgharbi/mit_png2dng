//
//  dng_converter.hpp
//  dng_converter
//
//  Created by mgharbi on 5/14/16.
//  Copyright © 2016 mgharbi. All rights reserved.
//

#ifndef dng_converter_hpp
#define dng_converter_hpp

#include <stdio.h>
#include "dng_errors.h"
#include <iostream>

dng_error_code make_dng(std::string m_szInputFile, std::string output_file);

#endif /* dng_converter_hpp */
