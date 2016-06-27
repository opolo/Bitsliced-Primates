#include "Encrypt.h"
#include <stdio.h>
#include "Primate.h"
#include <time.h>
#include <float.h>
#include <Windows.h>
#include <stdlib.h>

int cmpfunc(const void * a, const void * b);

#define Key		0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0, \
					0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0

#define Nonce	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0

//Data. 40 bytes for this example
#define DataOnes	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF
#define DataOnes40 DataOnes, DataOnes, DataOnes, DataOnes, DataOnes


#define DataMix {0x00},	{0xFF},	{0x00},	{0xFF},	{0x00},	{0xFF},	{0x00},	{0xFF}
#define DataMix40 DataMix, DataMix, DataMix, DataMix, DataMix

#define DataAscend7 {0x01}, {0x02}, {0x03}, {0x04}, {0x05}, {0x06}, {0x07}
#define DataAscend8 {0x00}, {0x01}, {0x02}, {0x03}, {0x04}, {0x05}, {0x06}, {0x07}

#define DataZeroes	{0x00},	{0x00},	{0x00},	{0x00},	{0x00},	{0x00},	{0x00},	{0x00}
#define DataZeroes40 DataZeroes, DataZeroes, DataZeroes, DataZeroes, DataZeroes

//Constants for datalength. Change as needed
//#define MsgLength 80
#define MsgLength 80
#define AdLength 40 

void main() {

	//data
	unsigned char *msg = calloc(MsgLength + 40, sizeof(u8));
	unsigned char *decrypted_msg = calloc(MsgLength + 40, sizeof(u8)); //8mb (8421376 bytes)
	unsigned char *c = calloc(MsgLength + 40, sizeof(u8)); //8mb (8421376 bytes)

	//data
	//const unsigned char msg[MsgLength] = { DataMix40 , DataAscend8 };
	//const unsigned char decrypted_msg[MsgLength];
	//unsigned char *c = calloc(100, sizeof(u8)); //Make room for message + 40 extra byte due to how implementation works.. could be done more tidy

	//constant length data
	const unsigned char key[size_key_bytes] = { Key };
	const unsigned char nonce[size_nonce_bytes] = { Nonce };
	const unsigned char ad[AdLength] = { DataOnes40 };
	u64 tag[30];


	Initialize();

	//Test whether primates are working as intended
	if (Debug) {
		test_primates();
	}


	if (Benchmark) {
		free(msg);
		free(decrypted_msg);
		free(c);

		//Run only on one core
		SetThreadAffinityMask(GetCurrentThread(), 0x00000008); //Run on fourth core

		u64 start, finish, cpu_frequency;
		
		start = __rdtsc();
		Sleep(5000);
		finish = __rdtsc();
		cpu_frequency = (finish - start)/5;

		printf("CPU frequency: %llu \n", cpu_frequency);
		
		int iterations_b = 20000;
		int iterations_kb = 2000;
		int iterations_mb = 40;

		int b_test_size = 40;
		int kb_test_size = 4000;
		int mb_test_size = 2000000;
		
		u64 *results_b = calloc(iterations_b, sizeof(u64));
		u64 *results_kb = calloc(iterations_kb, sizeof(u64));
		u64 *results_mb = calloc(iterations_mb, sizeof(u64));
		
		printf("Press to start...\n");
		getchar();

		//b loop
		for (int i = 0; i < iterations_b; i++) {
			msg = calloc(b_test_size, sizeof(u8));
			decrypted_msg = calloc(b_test_size + 40, sizeof(u8));
			c = calloc(b_test_size + 40, sizeof(u8));

			start = __rdtsc();

			crypto_aead_encrypt(c, msg, b_test_size, ad, AdLength, nonce, key, tag);
			int tagmatch = crypto_aead_decrypt(c, b_test_size, decrypted_msg, ad, AdLength, nonce, key, tag);
			if (tagmatch)
				printf("Tag failed!! \n");

			finish = __rdtsc();

			results_b[i] = (finish - start);

			if (i % (iterations_b / 10) == 0) {
				//One tenth through...
				printf("Progress: %i/10 through. \n", (i / (iterations_b / 10) + 1));
			}

			free(msg);
			free(decrypted_msg);
			free(c);
		}
		qsort(results_b, iterations_b, sizeof(u64), cmpfunc);
		u64 medianSpeed_b = results_b[iterations_b/2]; //median amount of cycles for 40 bytes.
		double cycles_per_byte_b = 1 / (double)(((double)b_test_size + AdLength) / (double)medianSpeed_b);


		//Output results:
		printf("*** byte-loop: *** \n");
		printf("Iterations: %i \n", iterations_b);
		printf("Test size (bytes): %i \n", b_test_size);
		printf("Median cycles/byte: %f", cycles_per_byte_b);
		printf("\n");
		printf("Speed in clocks: \n");
		printf("Fastest enc- & decryption: %llu \n", results_b[0]);
		printf("Slowest enc- & decryption: %llu \n", results_b[iterations_b-1]);
		printf("Median enc- & decryption speed: %llu \n", medianSpeed_b);
		printf("Speed in seconds: \n");
		printf("Fastest enc- & decryption: %f \n", results_b[0] / (double)cpu_frequency);
		printf("Slowest enc- & decryption: %f \n", results_b[iterations_b - 1] / (double)cpu_frequency);
		printf("Median enc- & decryption speed: %f \n", medianSpeed_b / (double)cpu_frequency);
		printf("\n\n");
		

		u64 s, f;
		//kb loop
		for (int i = 0; i < iterations_kb; i++) {
			msg = calloc(kb_test_size, sizeof(u8));
			decrypted_msg = calloc(kb_test_size + 40, sizeof(u8));
			c = calloc(kb_test_size + 40, sizeof(u8));

			s = __rdtsc();

			crypto_aead_encrypt(c, msg, kb_test_size, ad, AdLength, nonce, key, tag);
			if (crypto_aead_decrypt(c, kb_test_size, decrypted_msg, ad, AdLength, nonce, key, tag))
				printf("Tag failed!! \n");

			f = __rdtsc();

			results_kb[i] = (f - s);

			if (i % (iterations_kb / 10) == 0) {
				//One tenth through...
				printf("Progress: %i/10 through. \n", (i / (iterations_kb / 10) + 1));
			}

			free(msg);
			free(decrypted_msg);
			free(c);
		}
		qsort(results_kb, iterations_kb, sizeof(u64), cmpfunc);
		u64 medianSpeed_kb = results_kb[iterations_kb / 2];
		double cycles_per_byte_kb = 1 / (double) (((double)kb_test_size + AdLength) / (double) medianSpeed_kb);

		//Output results:
		printf("*** kilobyte-loop: *** \n");
		printf("Iterations: %i \n", iterations_kb);
		printf("Test size (bytes): %i \n", kb_test_size);
		printf("Median cycles/byte: %f", cycles_per_byte_kb);
		printf("\n");
		printf("Speed in clocks: \n");
		printf("Fastest enc- & decryption: %llu \n", results_kb[0]);
		printf("Slowest enc- & decryption: %llu \n", results_kb[iterations_kb - 1]);
		printf("Median enc- & decryption speed: %llu \n", medianSpeed_kb);
		printf("Speed in seconds: \n");
		printf("Fastest enc- & decryption: %f \n", results_kb[0] / (double)cpu_frequency);
		printf("Slowest enc- & decryption: %f \n", results_kb[iterations_kb - 1] / (double)cpu_frequency);
		printf("Median enc- & decryption speed: %f \n", medianSpeed_kb / (double)cpu_frequency);
		printf("\n\n");

		
		
		//mb loop
		for (int i = 0; i < iterations_mb; i++) {
			msg = calloc(mb_test_size, sizeof(u8));
			decrypted_msg = calloc(mb_test_size + 40, sizeof(u8));
			c = calloc(mb_test_size + 40, sizeof(u8));

			s = __rdtsc();

			crypto_aead_encrypt(c, msg, mb_test_size, ad, AdLength, nonce, key, tag);
			if (crypto_aead_decrypt(c, mb_test_size, decrypted_msg, ad, AdLength, nonce, key, tag))
				printf("Tag failed!! \n");

			f = __rdtsc();

			results_mb[i] = (f - s);

			if (i % (iterations_mb / 10) == 0) {
				//One tenth through...
				printf("Progress: %i/10 through. \n", (i / (iterations_mb / 10) + 1));
			}

			free(msg);
			free(decrypted_msg);
			free(c);
		}
		qsort(results_mb, iterations_mb, sizeof(u64), cmpfunc);
		u64 medianSpeed_mb = results_mb[iterations_mb / 2];
		double cycles_per_byte_mb = 1 / (double)(((double)mb_test_size + AdLength)/ (double)medianSpeed_mb);

		//Output results:
		printf("*** megabyte-loop: *** \n");
		printf("Iterations: %i \n", iterations_mb);
		printf("Test size (bytes): %i \n", mb_test_size);
		printf("Median cycles/byte: %f", cycles_per_byte_mb);
		printf("\n");
		printf("Speed in clocks: \n");
		printf("Fastest enc- & decryption: %llu \n", results_mb[0]);
		printf("Slowest enc- & decryption: %llu \n", results_mb[iterations_mb - 1]);
		printf("Median enc- & decryption speed: %llu \n", medianSpeed_mb);
		printf("Speed in seconds: \n");
		printf("Fastest enc- & decryption: %f \n", results_mb[0] / (double)cpu_frequency);
		printf("Slowest enc- & decryption: %f \n", results_mb[iterations_mb - 1] / (double)cpu_frequency);
		printf("Median enc- & decryption speed: %f \n", medianSpeed_mb / (double)cpu_frequency);
		printf("\n\n");

		free(results_b);
		free(results_kb);
		free(results_mb);

	}
	else {
		//Just do a normal encryption/decryption with given #define-parameters.
		crypto_aead_encrypt(c, msg, MsgLength, ad, AdLength, nonce, key, tag);

		if (crypto_aead_decrypt(c, MsgLength, decrypted_msg, ad, AdLength, nonce, key, tag))
			printf("Tag did not match! \n");

		//Output result of encryption and used data for it
		if (OutputData) {
			printf("Key: \n");
			for (int i = 0; i < size_key_bytes; i++) {
				if ((i + 1) % 8 == 1 && i != 0)
					printf("\t");
				printf("%02x ", key[i]);
			}
			printf("\n\n");

			printf("Nonce: \n");
			for (int i = 0; i < size_nonce_bytes; i++) {
				if ((i + 1) % 8 == 1 && i != 0)
					printf("\t");
				printf("%02x ", nonce[i]);
			}
			printf("\n\n");

			/*
			printf("Tag: \n");
			for (int i = 0; i < size_tag_u64 * 8; i++) {
				if ((i + 1) % 8 == 1 && i != 0)
					printf("\t");
				printf("%02x ", tag[i]);
			}
			printf("\n\n");
			*/

			printf("Plaintext: \n");
			for (int i = 0; i < MsgLength; i++) {
				if ((i + 1) % 8 == 1 && i != 0)
					printf("\t");
				printf("%02x ", msg[i]);
			}
			printf("\n\n");

			printf("Ciphertext: \n");
			for (int i = 0; i < MsgLength; i++) {
				if ((i + 1) % 8 == 1 && i != 0)
					printf("\t");
				printf("%02x ", c[i]);
			}
			printf("\n\n");

			printf("Decrypted plaintext: \n");
			for (int i = 0; i < MsgLength; i++) {
				if ((i + 1) % 8 == 1 && i != 0)
					printf("\t");
				printf("%02x ", decrypted_msg[i]);
			}
			printf("\n\n");
		}

		free(msg);
		free(decrypted_msg);
		free(c);
	}

	getchar();
}

int cmpfunc(const void * a, const void * b)
{
	return (int)(*(u64*)a - *(u64*)b);
}