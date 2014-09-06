// crypto.cpp
// Released under the MIT License
// 
// A separate source file for linking to the OpenSSL crypto library

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <openssl/aes.h>

// Using openssl-for-windows binaries available here: 
// https://code.google.com/p/openssl-for-windows/

// IMPORTANT: For consistency, output vector will be cleared and if this function succeeds, it will
//			contain the number of bytes as found in input (TEST this with various lengths to be sure)
//			Unsure of the behavior of BF_cfb64_encrypt, though the ECB and CBC modes are better understood
void openssl_aes_encrypt(std::string key_string,
	std::vector<unsigned char>& input,
	std::vector<unsigned char>& output)
{
	try
	{
		output.clear();
	}
	catch (...)
	{
		std::cout << "Exception caught in openssl_aes_encrypt when attempting to clear the output vector." << std::endl;
		return;
	}

	unsigned char ckey[] = "thiskeyisverybad";
	unsigned char ivec[] = "dontusethisinput";

	try
	{
		AES_KEY key;
		AES_set_encrypt_key(ckey, 128, &key);
		int num = 0;

		unsigned char* input_array = new unsigned char[input.size()];
		unsigned char* output_array = new unsigned char[input.size()];

		std::vector<unsigned char>::iterator it = input.begin();
		int i = 0;

		while (it != input.end())
		{
			input_array[i] = *it;
			i++;
			it++;
		}

		AES_cfb128_encrypt(input_array, output_array, input.size(), &key, ivec, &num, AES_ENCRYPT);
		for (int j = 0; j < input.size(); j++)
			output.push_back(output_array[j]);

		delete input_array;
		delete output_array;
	}
	catch (...)
	{
		std::cout << "Exception caught in openssl_aes_encrypt when attempting to encrypt the input vector." << std::endl;
	}
}

void openssl_aes_decrypt(std::string key_string, 
							  std::vector<unsigned char>& input,
							  std::vector<unsigned char>& output)
{
	unsigned char ckey[] = "thiskeyisverybad";
	unsigned char ivec[] = "dontusethisinput";

	AES_KEY key;
	AES_set_encrypt_key(ckey, 128, &key);
	int num = 0;

	unsigned char* input_array = new unsigned char[input.size()];
	unsigned char* output_array = new unsigned char[input.size()];

	std::vector<unsigned char>::iterator it = input.begin();
	int i = 0;

	while (it != input.end())
	{
		input_array[i] = *it;
		i++;
		it++;
	}

	AES_cfb128_encrypt(input_array, output_array, input.size(), &key, ivec, &num, AES_DECRYPT);
	for (int j = 0; j < input.size(); j++)
		output.push_back(output_array[j]);

	delete input_array;
	delete output_array;
}

