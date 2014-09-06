// crypto.cpp
// Released under the MIT License
// 
// A separate source file for linking to the OpenSSL crypto library

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <openssl/aes.h>

#define IVEC_STRING "o3Fc3WlpA3BdiZbx"

// Using openssl-for-windows binaries available here: 
// https://code.google.com/p/openssl-for-windows/

/* IMPORTANT: For consistency, output vector will be cleared and if this function succeeds, it will
			contain the number of bytes as found in input (TEST this with various lengths to be sure) */
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
	
	try
	{
		unsigned int key_string_size_bytes = key_string.size();

		unsigned char* key_array = new unsigned char[key_string_size_bytes];
		memset(key_array, 0, key_string_size_bytes);
		memcpy(key_array, key_string.c_str(), key_string_size_bytes);
		
		// TODO: For added security, the initialization vector shouldn't be the 
		//		same every time. It's okay if it's simple, but I really need to 
		//		consider embedding the IVEC_STRING somewhere in the cypher result
		//		so that it can vary with each cypher created. The question is, 
		//		how best to embed it. 
		unsigned char ivec[] = IVEC_STRING;

		AES_KEY key;
		AES_set_encrypt_key(key_array, 128, &key);
		int num = 0;

		unsigned char* input_array = new unsigned char[input.size()];
		unsigned char* output_array = new unsigned char[input.size()];
		memset(input_array, 0, input.size());
		memset(output_array, 0, input.size());

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

/* IMPORTANT: For consistency, output vector will be cleared and if this function succeeds, it will
			contain the number of bytes as found in input (TEST this with various lengths to be sure) */
void openssl_aes_decrypt(std::string key_string, 
							  std::vector<unsigned char>& input,
							  std::vector<unsigned char>& output)
{
	try
	{
		output.clear();
	}
	catch (...)
	{
		std::cout << "Exception caught in openssl_aes_decrypt when attempting to clear the output vector." << std::endl;
		return;
	}

	try
	{
		unsigned int key_string_size_bytes = key_string.size();

		unsigned char* key_array = new unsigned char[key_string_size_bytes];
		memset(key_array, 0, key_string_size_bytes);
		memcpy(key_array, key_string.c_str(), key_string_size_bytes);

		// TODO: For added security, it might make sense to embed the IVEC_STRING
		//		into the cypher, and then extract it here. Probably some clever
		//		way to spread the bytes around, or just put them at the beginning
		//		or end. A random-looking set of 128 bits should be indistinguishable
		//		from cypher text, but need to read more about the security of this.
		unsigned char ivec[] = IVEC_STRING;

		AES_KEY key;
		AES_set_encrypt_key(key_array, 128, &key);
		int num = 0;

		unsigned char* input_array = new unsigned char[input.size()];
		unsigned char* output_array = new unsigned char[input.size()];
		memset(input_array, 0, input.size());
		memset(output_array, 0, input.size());

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
	catch (...)
	{
		std::cout << "Exception caught in openssl_aes_decrypt when attempting to decrypt the input vector." << std::endl;
	}
}

