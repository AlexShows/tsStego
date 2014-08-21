// tsStego.cpp
// Released under the MIT License
// 
// A simple implementation of stegonography in C++
// Uses the LodePNG library version 20140801 by Lode Vandevenne

#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <exception>
#include "lodepng.h"

#define MAP_BINARY_PATH 0x10

#define MAP_USING_XOR 0x11
#define MAP_USING_XOR_STR "using_xor"

#define MAP_OPERATION_TYPE 0x12
#define MAP_ENCODE_OPERATION_NAME "encode"
#define MAP_DECODE_OPERATION_NAME "decode"

#define MAP_PLAINTEXT_FILENAME 0x100
#define MAP_REF_IMAGE_FILENAME 0x200
#define MAP_CIPHER_IMAGE_FILENAME 0x210

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
// The using_XOR flag allows the merge to either overwrite the destination bits in the img data, or XOR
// the source bits (from the text data) with the destination
// Throws exception on error
void merge_text_into_img_data(std::vector<unsigned char>& text_data, std::vector<unsigned char>& img_data, bool using_XOR=false)
{
	try
	{
		// Bounds check
		// Using 4 * the plaintext size because each element in img_data is a color channel of a pixel, of which
		// there are 4 channels per pixel, and we're going to overwrite certain bits across three of the channels
		if (img_data.size() < 4 * text_data.size())
			throw std::exception("Exception in merge_text_into_img_data: image is too small to fit all the text");

	}
	catch (...)
	{
		throw std::exception("Exception in merge_text_into_img_data: possible invalid reference to image or text data");
	}

	// Add an EOF tag so the extraction knows when to stop
	text_data.push_back(0x1a);

	unsigned char tmp = 0;
	unsigned int img_index = 0;

	// Loop through all the characters
	for (auto& c : text_data)
	{
		// TODO: Test and fix the XOR paths of this function

		// First color channel is Red

		// First 3 MSBs from the character are put into the Red channel's 3 LSBs
		tmp = c >> 5; // shift those 3 bits to the right 5 to make them align with the 3 LSBs

		// Depending on the XOR state flag, either overwrite the data or XOR the text into it
		if (!using_XOR)
		{
			// merge the 5 bits from the original with the 3 shifted MSBs of the text
			img_data[img_index] = (img_data[img_index] & 0xF8) | (tmp & 0x7);
		}
		else // Otherwise XOR the bits in, requiring the original image's pixel data to extract
			img_data[img_index] ^= tmp;

		img_index++; // Next color channel (Green)

		// Next 2 bits from the character are put into the Green channel's 2 LSBs
		tmp = c << 3; // shave off the 3 MSBs
		tmp = tmp >> 6; // shift back 6 to make them align with the 2 LSBs
		// Depending on the XOR state flag, either overwrite the data or XOR the text into it
		if (!using_XOR)
		{
			// merge the 5 bits from the original with the 2 shifted MSBs of the text
			img_data[img_index] = (img_data[img_index] & 0xFC) | (tmp & 0x3);
		}
		else // Otherwise XOR the bits in, requiring the original image's pixel data to extract
			img_data[img_index] ^= tmp;

		img_index++; // Next color channel (Blue)

		// The 3 LSBs from the character are put into the Blue channel's 3 LSBs
		tmp = c;
		// Depending on the XOR state flag, either overwrite the data or XOR the text into it
		if (!using_XOR)
		{
			// merge the 5 bits from the original with the 3 shifted MSBs of the text
			img_data[img_index] = (img_data[img_index] & 0xF8) | (tmp & 0x7);
		}
		else // Otherwise XOR the bits in, requiring the original image's pixel data to extract
			img_data[img_index] ^= tmp;
		
		img_index++; // Next color channel (Alpha)
		img_index++; // Next color channel (Red of the next pixel...so we're ready for next char)
	}
}

// Given an image or images, extract the text found inside them
// See the merge function for more on how the text is embedded in the image
// If the using_XOR flag is set, ref_img_data must be valid, as it's required
// in order to extract the text properly
// ref_img_data can be NULL, but using_XOR must be false if it is
// text_data should be an empty vector, but if it isn't, the data will be appended to the end
// TODO: Need to fix the passing of NULL for ref_img_data
// Throws exception on error
void extract_text_from_img_data(std::vector<unsigned char>& img_data, std::vector<unsigned char>& ref_img_data, std::vector<unsigned char>& text_data, bool using_XOR = false)
{
	try
	{
		// Validate inputs
		// Just trying to trigger an exception here if these are NULL pointers
		unsigned int img_data_size = img_data.size();
		unsigned int text_data_size = text_data.size();
		unsigned int ref_img_data_size = 0;
		if (using_XOR)
			ref_img_data_size = ref_img_data.size();				
	}
	catch (...)
	{
		throw std::exception("Exception in extract_text_from_img_data: possible invalid reference to image or text data");
	}

	unsigned char tmp = 0;
	unsigned char reconstruct = 0;
	unsigned int index = 0;
	bool finished = false;

	// Loop through all the color channels of all the pixels
	for (auto& p : img_data)
	{
		if (finished)
			break;

		if (using_XOR)
			tmp = p ^ ref_img_data[index];
		else
			tmp = p;

		// To reconstruct the character, we need 3 bits of Red, 2 bits of Green, and 3 bits of Blue
		switch (index % 4)
		{
		case 0:
			tmp = tmp << 5; // Red Channel = shift left 5 bits
			reconstruct |= tmp;
			break;
		case 1: 
			tmp = tmp << 6; // Green Channel = shift left 6 bits...
			tmp = tmp >> 3; // ...then back right 3 bits
			reconstruct |= tmp;
			break;
		case 2:
			tmp = tmp << 5; // Blue Channel = shift left 5 bits...
			tmp = tmp >> 5; // ...then back right 5 bits
			reconstruct |= tmp;
			break;
		default: // Alpha channel, so commit the reconstructed character or exit the loop on EOF
			if (reconstruct == 0x1a) // EOF character
				finished = true;
			else
				text_data.push_back(reconstruct);
			reconstruct = 0;
			break;
		}
		
		index++;
	}
}

void capture_args(int argc, char** argv, std::map<unsigned char, std::string>& args_map)
{
	/******************************************************
	There are several usages that result in different argument counts:

		1. .exe encode text ref_img cipher_img
				This encodes the text file given into a reference image, producing a cipher image
		2. .exe encode using_xor text ref_img cipher_img
				This encodes the text file given into a reference image using XOR, producing a cipher image 
		3. .exe decode cipher_img text
				This decodes the cipher image, producing a text file 
		4. .exe decode using_xor cipher_img ref_img text
				This decodes the cipher image using XOR and the reference image, producing the text

	Thus there could be either 5 or 6 parameters in total, and the order varies depending on the op.
	********************************************************/ 

	try
	{
		// The first argument is always the absolutely path to the binary
		args_map[MAP_BINARY_PATH] = argv[0];

		// The second argument should always be the operation name 
		for (int i = 1; i < argc; i++)
		{
			if (argv[i] == MAP_USING_XOR_STR)
				args_map[MAP_USING_XOR] = argv[i];
			// TODO: Contiue working on this...or scratch the loop and do this 
			//		more explicitly
		}
	}
	catch (...)
	{
		throw std::exception("Exception in capture_args attempting to place the arguments into the argument list.");
	}
}

// TODO: 
//		- Load the PNG file into the image data structure [ DONE ] 
//		- Load the plain text file [ DONE ] 
//		- Save the image data structure as a new PNG file [ DONE ]
//		- Save the plain text as a new text file [ DONE ]
//		- Handle command line input [ IN PROGRESS ]
//		- Display usage information
//		- Encipher the plain text 
//		- Merge the cipher text into the image data structure [ DONE ]
//		- Load the enciphered PNG file into the enciphered image data structure [ DONE ] 
//		- Extract the cipher text from the enciphered image data structure [ DONE ]
//		- Decipher to plain text 
int main(int argc, char** argv)
{
	std::map<unsigned char, std::string>cmd_args;
	capture_args(argc, argv, cmd_args);

	// Testing the read file function
	std::vector<unsigned char> plain_text;
	read_text_file("example.txt", plain_text);
	/*
	plain_text[3] = 'F';
	plain_text[4] = 0x1a;
	write_text_file("output1.txt", plain_text);
	*/

	std::vector<unsigned char> img_data;
	unsigned int h, w;

	read_png_from_file("planet.png", img_data, w, h);

	merge_text_into_img_data(plain_text, img_data);

	write_png_to_file("output.png", img_data, w, h);

	// Testing the reading back of the text embedded into the image
	// TODO: Test the XOR feature
	std::vector<unsigned char> modified_img_data;
	std::vector<unsigned char> ref_img_data; // Not using this for now
	std::vector<unsigned char> modified_text_data;
	read_png_from_file("output.png", modified_img_data, w, h);
	extract_text_from_img_data(modified_img_data, ref_img_data, modified_text_data);
	write_text_file("output.txt", modified_text_data);
	
	return 0;
}