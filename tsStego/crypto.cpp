// crypto.cpp
// Released under the MIT License
// 
// A separate source file for linking to the OpenSSL crypto library

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <openssl/blowfish.h>

// Sloppy dev environment HACK for now (using Indy Project's OpenSSL for Win64 binaries, but need to migrate
// this over to my own 64b-bit binaries later - not these files are "32" in name but internally 64-bit)
#pragma comment(lib, "D:\\openssl-0.9.8g\\ssleay32.lib")
#pragma comment(lib, "D:\\openssl-0.9.8g\\libeay32.lib")

// Borrowing this from tsStego for now; just using for test function in this object
void read_text_file(const char* filename, std::vector<unsigned char>& plaintext);

#define BF_CHUNK_SIZE 8 // 8 bytes per chunk
#define BF_IVECTOR_SIZE 8 // 8 bytes in the initialization vector

// IMPORTANT: For consistency, output vector will be cleared and if this function succeeds, it will
//			contain the number of bytes as found in input (TEST this with various lengths to be sure)
//			Unsure of the behavior of BF_cfb64_encrypt, though the ECB and CBC modes are better understood
void openssl_blowfish_encrypt(std::string key_string,
	std::vector<unsigned char>& input,
	std::vector<unsigned char>& output)
{
	// TODO: Exceptions

	// Initialization vector
	unsigned char iv[BF_IVECTOR_SIZE];
	for (int s = 0; s < BF_IVECTOR_SIZE; s++) 
		iv[s] = 'i'; // TODO: make this more secure; for now using "i"s

	// Type matching exercise
	const unsigned char * cucp_key = new unsigned char[key_string.size()];

	// Init the blowfish key based on the string provided
	BF_KEY key = { 0 };
	BF_set_key(&key, key_string.size(), cucp_key);

	delete cucp_key;

	// Working on 64-bit chunks at a time
	unsigned int input_size = input.size(); // might need padding
	unsigned int input_chunk_count = input_size / BF_CHUNK_SIZE; // 64-bit chunks
	unsigned int last_chunk_byte_count = input_size % BF_CHUNK_SIZE;
	unsigned char input_chunk[BF_CHUNK_SIZE] = { 0 };
	unsigned char output_chunk[BF_CHUNK_SIZE] = { 0 };
	int num = 0; // Required for BF_cfb64_encrypt function; specs says just set it to 0,
				 // because internally this is used to streo the current offset into the 
				 // the initialization vector between successive calls

	if (!input_size)
		return;

	// For each chunk, store the BF_CHUNK_SIZE
	for (int i = 0; i < input_chunk_count; i++)
	{
		for (int j = 0; j < BF_CHUNK_SIZE; j++)
			input_chunk[j] = input[i + j];

		BF_cbc_encrypt(input_chunk, output_chunk, BF_CHUNK_SIZE, &key, iv, BF_ENCRYPT);
		//BF_cfb64_encrypt(input_chunk, output_chunk, BF_CHUNK_SIZE, &key, iv, &num, BF_ENCRYPT);

		for (int k = 0; k < BF_CHUNK_SIZE; k++)
			output.push_back(output_chunk[k]);

	}

	// Now if we have any input leftover, encrypt it as well
	if (last_chunk_byte_count)
	{
		// TODO: This
	}

	std::cout << "Encrypted " << input_chunk_count << " 8-byte chunks and " 
		<< last_chunk_byte_count << " additional bytes using BF_cfb64_encrypt." << std::endl;

}

void openssl_blowfish_decrypt(std::string key_string, 
							  std::vector<unsigned char>& input,
							  std::vector<unsigned char>& output)
{
	// Initialization vector
	unsigned char iv[BF_IVECTOR_SIZE];
	for (int s = 0; s < BF_IVECTOR_SIZE; s++)
		iv[s] = 'i'; // TODO: make this more secure; for now using "i"s


	// Type matching exercise
	const unsigned char * cucp_key = new unsigned char[key_string.size()];

	// Init the blowfish key based on the string provided
	BF_KEY key = { 0 };
	BF_set_key(&key, key_string.size(), cucp_key);

	delete cucp_key;

	// Working on 64-bit chunks at a time
	unsigned int input_size = input.size(); // might need padding
	unsigned int input_chunk_count = input_size / BF_CHUNK_SIZE; // 64-bit chunks
	unsigned int last_chunk_byte_count = input_size % BF_CHUNK_SIZE;
	unsigned char input_chunk[BF_CHUNK_SIZE] = { 0 };
	unsigned char output_chunk[BF_CHUNK_SIZE] = { 0 };
	int num = 0; // Required for BF_cfb64_encrypt function; specs says just set it to 0,
				 // because internally this is used to streo the current offset into the 
				 // the initialization vector between successive calls

	if (!input_size)
		return;

	// For each chunk, store the BF_CHUNK_SIZE
	for (int i = 0; i < input_chunk_count; i++)
	{
		for (int j = 0; j < BF_CHUNK_SIZE; j++)
			input_chunk[j] = input[i + j];

		BF_cbc_encrypt(input_chunk, output_chunk, BF_CHUNK_SIZE, &key, iv, BF_DECRYPT);
		//BF_cfb64_encrypt(input_chunk, output_chunk, BF_CHUNK_SIZE, &key, iv, &num, BF_DECRYPT);

		for (int k = 0; k < BF_CHUNK_SIZE; k++)
			output.push_back(output_chunk[k]);

	}

	// Now if we have any input leftover, encrypt it as well
	if (last_chunk_byte_count)
	{
		// TODO: This
	}

	std::cout << "Decrypted " << input_chunk_count << " 8-byte chunks and "
		<< last_chunk_byte_count << " additional bytes using BF_cfb64_encrypt." << std::endl;
}

void BF_test()
{
	std::vector<unsigned char> message;
	read_text_file("example.txt", message);

	std::vector<unsigned char> cipher_text;

	openssl_blowfish_encrypt("supersecretkeythatnobodywouldguess", message, cipher_text);

	std::vector<unsigned char> deciphered_text;

	openssl_blowfish_decrypt("supersecretkeythatnobodywouldguess", cipher_text, deciphered_text);
}
