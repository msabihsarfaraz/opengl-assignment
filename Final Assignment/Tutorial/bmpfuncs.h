#ifndef __BMPFUNCS__
#define __BMPFUNCS__

#include <iostream>
#include <fstream>
#include <string>
using namespace std;

// reads the contents of a 24-bit RGB bitmap file
unsigned char* readBitmapRGBImage(const char *filename, int* widthOut, int* heightOut);

// write to a 24-bit RGB bitmap file
void writeBitmapRGBImage(const char *filename, char* imageData, int width, int height);

#endif
