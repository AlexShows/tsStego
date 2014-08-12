// tsStego.cpp
// Released under the MIT License
// 
// A simple implementation of stegonography in C++
// Uses the LodePNG library version 20140801 by Lode Vandevenne

#include <iostream>
#include "lodepng.h"

// Read the PNG file and save the data into the data structure
// Color values in the vector are 4 bytes per pixel, ordered RGBARGBA...
void read_png_from_file(const char* filename, std::vector<unsigned char>& image, unsigned int& width, unsigned int& height)
{
	unsigned error = lodepng::decode(image, width, height, filename);

	if (error) 
		std::cout << "Decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
}

// Write the PNG file from the data structure
// Color values in the vector are 4 bytes per pixel, ordered RGBARGBA...
void write_png_to_file(const char* filename, std::vector<unsigned char>& image, unsigned int width, unsigned int height)
{
	unsigned error = lodepng::encode(filename, image, width, height);

	if (error) 
		std::cout << "Encoder error " << error << ": " << lodepng_error_text(error) << std::endl;
}

// Just getting started...
// TODO: 
//		- Handle command line input
//		- Load the PNG file into the image data structure
//		- Load the plain text file
//		- Encipher the plain text
//		- Merge the cipher text into the image data structure
//		- Save the image data structure as a new PNG file
//		- Load the enciphered PNG file into the enciphered image data structure
//		- Extract the cipher text from the enciphered image data structure
//		- Decipher to plain text
//		- Save the plain text as a new text file (or display it?)
int main(int argc, char** argv)
{
	// Just testing the LodePNG library for now...input, modification, and output
	std::vector<unsigned char> img_data;
	unsigned int h, w;

	read_png_from_file("planet.png", img_data, w, h);

	unsigned int index = 0;

	for (auto& color_val : img_data)
	{
		if (index % 4 == 2)
		{
			if (color_val >= 255)
				color_val = 0;
		}
		
		index++;
	}

	write_png_to_file("output.png", img_data, w, h);

	return 0;
}