// tsStego.cpp
// Released under the MIT License
// 
// A simple implementation of stegonography in C++
// Uses the LodePNG library version 20140801 by Lode Vandevenne

#include <fstream>
#include <sstream>
#include <iostream>
#include <exception>
#include "lodepng.h"

// Read the plain text file in
void read_text_file(const char* filename, std::vector<unsigned char>& plaintext)
{
	try
	{
		std::fstream text_file(filename);
		while (text_file.good())
		{
			unsigned char c = text_file.get();
			if (text_file.good())
				plaintext.push_back(c);
		}
	}
	catch (...)
	{
		throw std::exception("Exception in read_text_file()");
		return;
	}
}

// Write the plain text file out
void write_text_file(const char* filename, std::vector<unsigned char>& plaintext)
{
	try
	{
		std::ofstream text_file(filename);
		for (auto& c : plaintext)
			text_file.put(c);
	}
	catch (...)
	{
		throw std::exception("Exception in write_text_file()");
		return;
	}
}

// Read the PNG file and save the data into the data structure
// Color values in the vector are 4 bytes per pixel, ordered RGBARGBA...
// On an error, throws an exception
void read_png_from_file(const char* filename, std::vector<unsigned char>& image, unsigned int& width, unsigned int& height)
{
	unsigned int error = 0;
	try
	{
		unsigned int error = lodepng::decode(image, width, height, filename);
	}
	catch (...)
	{
		throw std::exception("Exception in lodepng::decode()");
		return;
	}

	if (error)
	{
		std::stringstream err_desc;
		err_desc << "Decoder error " << error << ": " << lodepng_error_text(error);
		std::string s = err_desc.str();
		throw std::exception(s.c_str());
	}
}

// Write the PNG file from the data structure
// Color values in the vector are 4 bytes per pixel, ordered RGBARGBA...
// On an error, throws an exception
void write_png_to_file(const char* filename, std::vector<unsigned char>& image, unsigned int width, unsigned int height)
{
	unsigned int error = 0;

	try
	{
		unsigned error = lodepng::encode(filename, image, width, height);
	}
	catch (...)
	{
		throw std::exception("Exception in lodepng::decode()");
		return;
	}

	if (error) 
	{ 
		std::stringstream err_desc;
		err_desc << "Encoder error " << error << ": " << lodepng_error_text(error);
		std::string s = err_desc.str();
		throw std::exception(s.c_str());
	}
}

// Take the 8 bits per char and split them 3-2-3, putting 3 bits into the Red, 2 bits into the Green,
// 3 bits into the Blue, and nothing in Alpha
// The bits will be placed starting from the LSB of each byte so as to make the least impact to the 
// image when viewed by a human
// We put fewer bits into the Green channel because human eyes are more sensitive to a yellowish-green
void merge_plaintext_into_img_data(std::vector<unsigned char>& plaintext, std::vector<unsigned char>& img_data)
{
	// Bounds check
	// Using 4 * the plaintext size because each element in img_data is a color channel of a pixel, of which
	// there are 4 channels per pixel, and we're going to overwrite certain bits across three of the channels
	if (img_data.size() < 4 * plaintext.size())
		throw std::exception("Exception in merge_plaintext_into_img_data: image is too small to fit all the text");

	// An iterator for moving through the image data
	auto img_ptr = img_data.begin();
	unsigned char tmp = 0;

	// TODO: Fix this - the masking is WRONG

	// Loop through all the characters
	for (auto& c : plaintext)
	{
		// First 3 MSBs from the char are put into the Red channels 3 LSBs
		tmp = c & 0xE0; // mask off just the 3 MSBs
		tmp = tmp >> 5; // shift those 3 bits to the right 5 to make them align with the 3 LSBs
		c = ( c & 0x1F) | (tmp & 0x7); // merge the 5 bits from the original with the 3 shifted MSBs of the text

		c++; // Next character
		img_ptr++; // Next color channel (Blue)

		// Next 3 bits from the char are put into the Blue channels 3 LSBs
		tmp = c & 0x1C; // mask off just the 3 bits in the middle of the byte
		tmp = tmp >> 2; // shift those 3 bits to the right 2 to make them align with the 3 LSBs
		c = (c & 0x1F) | (tmp & 0x7); // merge the 5 bits from the original with the 3 shifted MSBs of the text

		c++; // Next character
		img_ptr++; // Next color channel (Green)

		// Next 2 bits from the char are put into the Green channels 2 LSBs
		tmp = c & 0x3; // mask off just the 2 LSBs
		c = (c & 0x1F) | (tmp & 0x7); // merge the 5 bits from the original with the 3 shifted MSBs of the text

		img_ptr++; // Next color channel (Alpha)
		img_ptr++; // Next color channel (Red of the next pixel)
	}
}

// TODO: 
//		- Load the PNG file into the image data structure [ DONE ] 
//		- Load the plain text file [ DONE ] 
//		- Save the image data structure as a new PNG file [ DONE ]
//		- Save the plain text as a new text file [ DONE ]
//		- Handle command line input
//		- Encipher the plain text 
//		- Merge the cipher text into the image data structure 
//		- Load the enciphered PNG file into the enciphered image data structure 
//		- Extract the cipher text from the enciphered image data structure
//		- Decipher to plain text 
int main(int argc, char** argv)
{
	// Testing the read file function
	std::vector<unsigned char>plain_text;
	read_text_file("example.txt", plain_text);

	plain_text[3] = 'F';
	write_text_file("output.txt", plain_text);

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

	merge_plaintext_into_img_data(plain_text, img_data);

	write_png_to_file("output.png", img_data, w, h);

	return 0;
}