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

#define STEGO_VERSION_STRING "0.2.1"

#define MAP_BINARY_PATH 0x1

#define MAP_USING_XOR 0x2
#define MAP_USING_XOR_STR "using_xor"

#define MAP_OPERATION_TYPE 0x4
#define MAP_ENCODE_OPERATION_NAME "encode"
#define MAP_DECODE_OPERATION_NAME "decode"

#define MAP_PLAINTEXT_FILENAME 0x8
#define MAP_REF_IMAGE_FILENAME 0x10
#define MAP_CIPHER_IMAGE_FILENAME 0x20
#define MAP_PASSWORD_STRING 0x40

void display_usage_info();

// From crypto.cpp:
void openssl_aes_encrypt(std::string key_string,
	std::vector<unsigned char>& input,
	std::vector<unsigned char>& output);
void openssl_aes_decrypt(std::string key_string,
	std::vector<unsigned char>& input,
	std::vector<unsigned char>& output);

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
// Throws std::exception on error
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

	// The first 4 bytes of the encoded data become the size of that data
	// so we perform that insertion here
	unsigned int text_size_bytes = text_data.size();
	unsigned char text_size_bytes_uc[4] = { 0 };
	memcpy(text_size_bytes_uc, &text_size_bytes, 4);
	text_data.insert(text_data.begin(), text_size_bytes_uc, text_size_bytes_uc+4);

	unsigned char tmp = 0;
	unsigned int img_index = 0;

	// Loop through all the characters
	for (auto& c : text_data)
	{
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
		{
			img_data[img_index] ^= (tmp & 0x7);
		}

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
			img_data[img_index] ^= (tmp & 0x7);

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
			img_data[img_index] ^= (tmp & 0x7);
		
		img_index++; // Next color channel (Alpha)
		img_index++; // Next color channel (Red of the next pixel...so we're ready for next char)
	}
}

// PARAMETERS: Image Data, Reference Image Data, Text Data, Using_XOR
// Given an image or images, extract the text found inside them
// See the merge function for more on how the text is embedded in the image
// If the using_XOR flag is set, ref_img_data must be valid, as it's required
// in order to extract the text properly
// ref_img_data can be NULL, but using_XOR must be false if it is
// text_data should be an empty vector, but if it isn't, the data will be appended to the end
// Throws std::exception on error
void extract_text_from_img_data(std::vector<unsigned char>& img_data, 
								std::vector<unsigned char>& ref_img_data, 
								std::vector<unsigned char>& text_data, 
								bool using_XOR = false)
{
	try
	{
		// Validate inputs
		// Just trying to trigger an exception here if these are NULL pointers
		unsigned int img_data_size = img_data.size();
		unsigned int text_data_size = text_data.size();
		unsigned int ref_img_data_size = ref_img_data.size();
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
	unsigned int size_in_bytes = 0;
	bool finished = false;

	// Loop through all the color channels of all the pixels
	for (auto& p : img_data)
	{
		// Once we have the first 4 bytes, then we can determine the actual size of the output
		// text file
		if (text_data.size() == 4 && !size_in_bytes)
		{
			unsigned char sz[4] = { 0 };
			std::vector<unsigned char>::iterator it = text_data.begin();
			for (unsigned int j = 0; j < 4; j++)
			{
				sz[j] = *it;
				it++;
			}
			memcpy(&size_in_bytes, sz, 4);
			text_data.clear();
		}

		// If we know our size, and we're using XOR, and our ref img doesn't have enough bytes,
		// abort
		if (size_in_bytes && size_in_bytes > ref_img_data.size())
			throw std::exception("Exception in extract_text_from_img_data: reference image is too small.");

		// If we know our size, and we've reached our size, we're done
		if (size_in_bytes && size_in_bytes == text_data.size())
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
		default: // Alpha channel, so commit the reconstructed character
			text_data.push_back(reconstruct);
			reconstruct = 0;
			break;
		}
		
		index++;
	}
}

// Interpret and store arguments
// Throws a std::exception on an error, or may throw an exception if no further processing is needed
void capture_args(int argc, char** argv, 
				std::map<unsigned char, 
				std::string>& args_map)
{
	/******************************************************
	There are several usages that result in different argument counts:

		1. .exe encode text ref_img cipher_img optional_password_string
				This encodes the text file given into a reference image, producing a cipher image
		2. .exe encode using_xor text ref_img cipher_img optional_password_string
				This encodes the text file given into a reference image using XOR, producing a cipher image 
		3. .exe decode cipher_img text optional_password_string
				This decodes the cipher image, producing a text file 
		4. .exe decode using_xor cipher_img ref_img text optional_password_string
				This decodes the cipher image using XOR and the reference image, producing the text

	Thus there could be 4 to 6 parameters in total, and the order varies depending on the op.
	If there are no parameters provided or just one, the user might be requesting help.
	********************************************************/ 
	int n = 0;

	// If invoked with no parameters...
	if (argc < 2)
	{
		display_usage_info();
		throw std::exception("In capture_args: help requested. No further processing required.");
	}

	// Or if the second parameter is asking for help...
	if (argv[n + 1] == "help" || argv[n + 1] == "?" ||
		argv[n + 1] == "/help" || argv[n + 1] == "/?")
	{
		display_usage_info();
		throw std::exception("In capture_args: help requested. No further processing required.");
	}

	if (argc < 4)
	{
		std::cout << "Too few arguments provided. See usage info." << std::endl;
		std::cout << std::endl;
		display_usage_info();
		throw std::exception("In capture_args: too few arguments provided.");
	}

	try
	{		
		// The first argument is always the absolute path to the binary
		args_map[MAP_BINARY_PATH] = argv[n];
		
		// The second argument should always be the operation name 
		args_map[MAP_OPERATION_TYPE] = argv[n + 1];

		// The third _could_ be the xor flag, or it could be a filename
		if (!_stricmp(argv[n + 2],MAP_USING_XOR_STR))
		{
			args_map[MAP_USING_XOR] = MAP_USING_XOR_STR;
			n++;
		}
		// ...else we do nothing and the xor flag isn't put into the map

		// The next sequence depends on the operation
		if (args_map[MAP_OPERATION_TYPE] == MAP_ENCODE_OPERATION_NAME)
		{
			// If using XOR...the arg count should be 6 for the encode
			if (args_map[MAP_USING_XOR] == MAP_USING_XOR_STR && (argc < n+5))
			{
				std::cout << "Too few arguments provided. See usage info." << std::endl;
				throw std::exception("In capture_args: too few arguments provided.");
			}
			
			args_map[MAP_PLAINTEXT_FILENAME] = argv[n + 2];
			args_map[MAP_REF_IMAGE_FILENAME] = argv[n + 3];
			args_map[MAP_CIPHER_IMAGE_FILENAME] = argv[n + 4];
			if (argc == n+6) // Optional password
				args_map[MAP_PASSWORD_STRING] = argv[n + 5];	
		}
		else if (args_map[MAP_OPERATION_TYPE] == MAP_DECODE_OPERATION_NAME)
		{
			if (args_map[MAP_USING_XOR] == MAP_USING_XOR_STR && (argc < n + 5))
			{
				std::cout << "Too few arguments provided. See usage info." << std::endl;
				throw std::exception("In capture_args: too few arguments provided.");
			}

			args_map[MAP_CIPHER_IMAGE_FILENAME] = argv[n + 2];
			if (args_map[MAP_USING_XOR] == MAP_USING_XOR_STR)
			{
				args_map[MAP_REF_IMAGE_FILENAME] = argv[n + 3];
				n++;
			}
			args_map[MAP_PLAINTEXT_FILENAME] = argv[n + 3];
			if (argc == n+5) // optional password
				args_map[MAP_PASSWORD_STRING] = argv[n + 4];
		}
	}
	catch (...)
	{
		throw std::exception("Exception in capture_args attempting to place the arguments into the argument list.");
	}
}

void display_usage_info()
{
	std::cout << std::endl;
	std::cout << "USAGE EXAMPLES" << std::endl;
	std::cout << "--------------" << std::endl;
	std::cout << "Encode a text file into an image (without using XOR):" << std::endl;
	std::cout << "\ttsStego.exe encode textfile ref_img cipher_img" << std::endl;
	std::cout << std::endl;
	std::cout << "Encode a text file into an image (using XOR):" << std::endl;
	std::cout << "\ttsStego.exe encode using_xor textfile ref_img cipher_img" << std::endl;
	std::cout << std::endl;
	std::cout << "Decode a text file from an image (without using XOR):" << std::endl;
	std::cout << "\ttsStego.exe decode cipher_img textfile" << std::endl;
	std::cout << std::endl;
	std::cout << "Decode a text file from an image (using XOR):" << std::endl;
	std::cout << "\ttsStego.exe decode using_xor cipher_img ref_img textfile" << std::endl;
	std::cout << std::endl;
	std::cout << "GLOSSARY" << std::endl;
	std::cout << "--------" << std::endl;
	std::cout << "\"encode\" means take the text from the text file and create a new cipher" << std::endl;
	std::cout << "\timage from a reference image with the text embedded in it." << std::endl;
	std::cout << std::endl;
	std::cout << "\"decode\" means extract text from a cipher image and create a text file" << std::endl;
	std::cout << std::endl;
	std::cout << "\"using_xor\" determines whether or not the encoded bits replace the" << std::endl;
	std::cout << "\toriginal bits or if they are a bitwise logical XOR with the original" << std::endl;
	std::cout << "\tbits. Using XOR provides an additional level of obscurity, but it" << std::endl;
	std::cout << "\trequires the original image in addition to the cipher image in order" << std::endl;
	std::cout << "\tto decode the text." << std::endl;
	std::cout << std::endl;
	std::cout << "\"textfile\" is the filename of a text file for encode or decode to/from" << std::endl;
	std::cout << std::endl;
	std::cout << "\"ref_img\" is the filename of a PNG image to be used as a reference" << std::endl;
	std::cout << std::endl;
	std::cout << "\"cipher_img\" is the filename of a PNG image for encode or decode to/from" << std::endl;
	std::cout << std::endl;
}

void display_about_info()
{
	std::cout << "tsStego version " << STEGO_VERSION_STRING << std::endl;
	std::cout << "Written by Alex Shows" << std::endl;
	std::cout << "PNG support provided by Lode Vandevenne" << std::endl;
}

//		- Load the PNG file into the image data structure [ DONE ] 
//		- Load the plain text file [ DONE ] 
//		- Save the image data structure as a new PNG file [ DONE ]
//		- Save the plain text as a new text file [ DONE ]
//		- Handle command line input [ DONE ]
//		- Display usage information [ DONE ]
//		- Merge the cipher text into the image data structure [ DONE ]
//		- Load the enciphered PNG file into the enciphered image data structure [ DONE ] 
//		- Extract the cipher text from the enciphered image data structure [ DONE ]
//		- Encipher from plain text [ DONE ]
//		- Decipher to plain text [ DONE ]
//		- Add command line option to pass the encryption key to be used [ DONE ]
int main(int argc, char** argv)
{
	display_about_info();

	std::map<unsigned char, std::string>cmd_args;

	try
	{
		capture_args(argc, argv, cmd_args);
	}
	catch (std::exception const& e)
	{
		//std::cout << e.what() << std::endl;
		return -1;
	}
	
	std::vector<unsigned char> plain_text;
	std::vector<unsigned char> cypher_text;
	std::vector<unsigned char> img_data;
	unsigned int h, w;

	std::vector<unsigned char> modified_img_data;
	std::vector<unsigned char> ref_img_data;
	std::vector<unsigned char> modified_text_data;
	
	if (cmd_args[MAP_OPERATION_TYPE] == MAP_ENCODE_OPERATION_NAME)
	{
		std::cout << std::endl;
		std::cout << "Encoding " << cmd_args[MAP_PLAINTEXT_FILENAME].c_str() << " into ";
		std::cout << cmd_args[MAP_REF_IMAGE_FILENAME].c_str() << std::endl;
		std::cout << "to produce the output file: " << cmd_args[MAP_CIPHER_IMAGE_FILENAME].c_str() << std::endl;
		std::cout << std::endl;

		try
		{
			read_text_file(cmd_args[MAP_PLAINTEXT_FILENAME].c_str(), plain_text);
			if (cmd_args[MAP_PASSWORD_STRING].size() == 0)
				cmd_args[MAP_PASSWORD_STRING] = "mysupersecretpasswordthatnobodywouldguess";
			openssl_aes_encrypt(cmd_args[MAP_PASSWORD_STRING].c_str(), plain_text, cypher_text);
			read_png_from_file(cmd_args[MAP_REF_IMAGE_FILENAME].c_str(), img_data, w, h);
			if (cmd_args[MAP_USING_XOR] == MAP_USING_XOR_STR)
				merge_text_into_img_data(cypher_text, img_data, true); // Using XOR flag
			else
				merge_text_into_img_data(cypher_text, img_data); // Not using XOR
			write_png_to_file(cmd_args[MAP_CIPHER_IMAGE_FILENAME].c_str(), img_data, w, h);
		}
		catch (std::exception const& e)
		{
			std::cout << e.what() << std::endl;
		}
		
	}
	else if (cmd_args[MAP_OPERATION_TYPE] == MAP_DECODE_OPERATION_NAME)
	{
		std::cout << std::endl;
		std::cout << "Decoding " << cmd_args[MAP_CIPHER_IMAGE_FILENAME].c_str() << std::endl;
		std::cout << "to produce the output file: " << cmd_args[MAP_PLAINTEXT_FILENAME].c_str() << std::endl;
		if (cmd_args[MAP_USING_XOR] == MAP_USING_XOR_STR)
			std::cout << "Using XOR with this image: " << cmd_args[MAP_REF_IMAGE_FILENAME].c_str() << std::endl;
		std::cout << std::endl;

		try
		{
			read_png_from_file(cmd_args[MAP_CIPHER_IMAGE_FILENAME].c_str(), modified_img_data, w, h);
			if (cmd_args[MAP_USING_XOR] == MAP_USING_XOR_STR)
			{
				read_png_from_file(cmd_args[MAP_REF_IMAGE_FILENAME].c_str(), ref_img_data, w, h);
				extract_text_from_img_data(modified_img_data, ref_img_data, cypher_text, true);
			}
				
			else
				extract_text_from_img_data(modified_img_data, ref_img_data, cypher_text);
			if (cmd_args[MAP_PASSWORD_STRING].size() == 0)
				cmd_args[MAP_PASSWORD_STRING] = "mysupersecretpasswordthatnobodywouldguess";
			openssl_aes_decrypt(cmd_args[MAP_PASSWORD_STRING].c_str(), cypher_text, modified_text_data);
			write_text_file(cmd_args[MAP_PLAINTEXT_FILENAME].c_str(), modified_text_data);
		}
		catch (std::exception const& e)
		{
			std::cout << e.what() << std::endl;
		}
	}
	
	std::cout << "End of program execution." << std::endl;
	return 0;
}