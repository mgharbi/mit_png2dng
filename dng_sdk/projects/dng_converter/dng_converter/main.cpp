//
//  main.cpp
//  dng_converter
//
//  Created by mgharbi on 5/14/16.
//  Copyright © 2016 mgharbi. All rights reserved.
//


#include <iostream>
#include <cassert>
#include "XMP.incl_cpp"
#include "dng_converter.hpp"

using namespace std;

int main(int argc, const char * argv[]) {
    assert(argc == 3);
    string i_path(argv[1]);
    string o_path(argv[2]);

    cout << i_path << endl;
    cout << o_path << endl;
    
    make_dng(i_path, o_path);
    return 0;
}
