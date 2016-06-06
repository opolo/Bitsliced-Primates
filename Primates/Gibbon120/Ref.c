#include "Encrypt.h"
#include <stdio.h>
#include "Primate.h"
#include <time.h>
#include <float.h>
#include <Windows.h>

//120bit = 15 bytes
//#define GibbonKey {0x00},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0x0},	{0x0},	{0x0},	{0x0},	{0x0},	{0x0},	{0x0}
//#define GibbonKey {0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF}
#define GibbonKey {0x0},	{0x00},	{0x00},	{0x00},	{0x00},	{0x00},	{0x00},	{0x00},	{0x0},	{0x0},	{0x0},	{0x0},	{0x0},	{0x0},	{0x0}
//#define GibbonKey {0x02},	{0x02},	{0x02},	{0x02},	{0x02},	{0x02},	{0x02},	{0x02},	{0x02},	{0x02},	{0x02},	{0x02},	{0x02},	{0x02},	{0x02}
#define GibbonKeyLength 15

//120bit = 15 bytes
#define GibbonNonce	{0x00}, {0x00},	{0x00},	{0x00},	{0x00},	{0x00},	{0x00},	{0x00},	{0x0},	{0x0},	{0x0},	{0x0},	{0x0},	{0x0},	{0x0}
//#define GibbonNonce		{0x02}, {0x02},	{0x02},	{0x02},	{0x02},	{0x02},	{0x02},	{0x02},	{0x02},	{0x02},	{0x02},	{0x02},	{0x02},	{0x02},	{0x02}
//#define GibbonNonce	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF}
#define GibbonNonceLength 15

//Tag 120 bit = 15 bytes
#define GibbonTagLength 15

//Data. 40 bytes for this example
#define DataOnes	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF}
#define DataOnes40 DataOnes, DataOnes, DataOnes, DataOnes, DataOnes


#define DataMix {0x00},	{0xFF},	{0x00},	{0xFF},	{0x00},	{0xFF},	{0x00},	{0xFF}
#define DataMix40 DataMix, DataMix, DataMix, DataMix, DataMix

#define DataAscend7 {0x01}, {0x02}, {0x03}, {0x04}, {0x05}, {0x06}, {0x07}
#define DataAscend8 {0x00}, {0x01}, {0x02}, {0x03}, {0x04}, {0x05}, {0x06}, {0x07}

#define DataZeroes	{0x00},	{0x00},	{0x00},	{0x00},	{0x00},	{0x00},	{0x00},	{0x00}
#define DataZeroes40 DataZeroes, DataZeroes, DataZeroes, DataZeroes, DataZeroes

//Constants for datalength. Change as needed
//#define MsgLength 80
#define MsgLength 40
#define AdLength 40 

void main() {
	
	//data
	unsigned char *msg = calloc(MsgLength, sizeof(u8)); 
	unsigned char *decrypted_msg = calloc(MsgLength + 40, sizeof(u8)); //8mb (8421376 bytes)
	unsigned char *c = calloc(MsgLength + 40, sizeof(u8)); //8mb (8421376 bytes)

	//data
	//const unsigned char msg[MsgLength] = { DataMix40 , DataAscend8 };
	//const unsigned char decrypted_msg[MsgLength];
	//unsigned char *c = calloc(100, sizeof(u8)); //Make room for message + 40 extra byte due to how implementation works.. could be done more tidy

	//constant length data
	const unsigned char key[GibbonKeyLength] = { GibbonKey };
	const unsigned char nonce[GibbonNonceLength] = { GibbonNonce };
	const unsigned char ad[AdLength] = { DataOnes40 };
	unsigned char tag[GibbonTagLength];

	
	Initialize();

	//Test whether primates are working as intended
	if (Debug) {
		test_primates();
	}
		

	if (Benchmark) {
		free(msg);
		free(decrypted_msg);
		free(c);

		LARGE_INTEGER start, finish;
		double cpu_frequency; 
		QueryPerformanceFrequency(&start);
		cpu_frequency = (double) (start.QuadPart)/1000.0; //Frequency: Ticks per milisecond on the system.

		printf("CPU frequency: %f \n", cpu_frequency);

		int iterations_b = 2000;
		int iterations_kb = 2000;
		int iterations_mb = 500;

		int b_test_size = 40;
		int kb_test_size = 4000;
		int mb_test_size = 4'000'000;

		//Data for storing benchmark results for
		//byte-loop
		double fastestEncryption_b = DBL_MAX;
		double slowestEncryption_b = DBL_MIN;
		double averageSpeed_b = 0;

		//kb-loop
		double fastestEncryption_kb = DBL_MAX;
		double slowestEncryption_kb = DBL_MIN;
		double averageSpeed_kb = 0;

		//mb-loop
		double fastestEncryption_mb = DBL_MAX;
		double slowestEncryption_mb = DBL_MIN;
		double averageSpeed_mb = 0;

		//b loop
		for (int i = 0; i < iterations_b; i++) {
			msg			= calloc(b_test_size, sizeof(u8));
			decrypted_msg	= calloc(b_test_size + 40, sizeof(u8));
			c				= calloc(b_test_size + 40, sizeof(u8));

			QueryPerformanceCounter(&start);

			crypto_aead_encrypt(c, msg, b_test_size, ad, AdLength, nonce, key, tag);
			int tagmatch = crypto_aead_decrypt(c, b_test_size, decrypted_msg, ad, AdLength, nonce, key, tag);
			if(tagmatch)
				printf("Tag failed!! \n");
			
			QueryPerformanceCounter(&finish);

			double clocks_taken = (double) (finish.QuadPart - start.QuadPart);

			if (clocks_taken < fastestEncryption_b)
				fastestEncryption_b = clocks_taken;
			if (clocks_taken > slowestEncryption_b)
				slowestEncryption_b = clocks_taken;
			averageSpeed_b += clocks_taken;

			free(msg);
			free(decrypted_msg);
			free(c);
		}
		averageSpeed_b = averageSpeed_b / iterations_b;
		double cycles_per_byte_b = 1 / (b_test_size / averageSpeed_b);

		//Output results:
		printf("*** byte-loop: *** \n");
		printf("Iterations: %i \n", iterations_b);
		printf("Test size (bytes): %i \n", b_test_size);
		printf("Average cycles/byte: %f", cycles_per_byte_b);
		printf("\n");
		printf("Speed in clocks: \n");
		printf("Fastest enc- & decryption: %f \n", fastestEncryption_b);
		printf("Slowest enc- & decryption: %f \n", slowestEncryption_b);
		printf("Average enc- & decryption speed: %f \n", averageSpeed_b);
		printf("Speed in milliseconds: \n");
		printf("Fastest enc- & decryption: %f \n", fastestEncryption_b / cpu_frequency);
		printf("Slowest enc- & decryption: %f \n", slowestEncryption_b / cpu_frequency);
		printf("Average enc- & decryption speed: %f \n", averageSpeed_b / cpu_frequency);
		printf("\n\n");


		//kb loop
		for (int i = 0; i < iterations_kb; i++) {
			msg = calloc(kb_test_size, sizeof(u8));
			decrypted_msg = calloc(kb_test_size + 40, sizeof(u8));
			c = calloc(kb_test_size + 40, sizeof(u8));

			QueryPerformanceCounter(&start);

			crypto_aead_encrypt(c, msg, kb_test_size, ad, AdLength, nonce, key, tag);
			if (crypto_aead_decrypt(c, kb_test_size, decrypted_msg, ad, AdLength, nonce, key, tag))
				printf("Tag failed!! \n");

			QueryPerformanceCounter(&finish);

			double clocks_taken = (double)(finish.QuadPart - start.QuadPart);

			if (clocks_taken < fastestEncryption_kb)
				fastestEncryption_kb = clocks_taken;
			if (clocks_taken > slowestEncryption_kb)
				slowestEncryption_kb = clocks_taken;
			averageSpeed_kb += clocks_taken;

			free(msg);
			free(decrypted_msg);
			free(c);
		}
		averageSpeed_kb = averageSpeed_kb / iterations_kb;
		double cycles_per_byte_kb = 1 / (kb_test_size / averageSpeed_kb);

		//Output results:
		printf("*** kilobyte-loop: *** \n");
		printf("Iterations: %i \n", iterations_kb);
		printf("Test size (bytes): %i \n", kb_test_size);
		printf("Average cycles/byte: %f", cycles_per_byte_kb);
		printf("\n");
		printf("Speed in clocks: \n");
		printf("Fastest enc- & decryption: %f \n", fastestEncryption_kb);
		printf("Slowest enc- & decryption: %f \n", slowestEncryption_kb);
		printf("Average enc- & decryption speed: %f \n", averageSpeed_kb);
		printf("Speed in milliseconds: \n");
		printf("Fastest enc- & decryption: %f \n", fastestEncryption_kb / cpu_frequency);
		printf("Slowest enc- & decryption: %f \n", slowestEncryption_kb / cpu_frequency);
		printf("Average enc- & decryption speed: %f", averageSpeed_kb / cpu_frequency);
		printf("\n\n");

		//mb loop
		for (int i = 0; i < iterations_mb; i++) {
			msg = calloc(mb_test_size, sizeof(u8));
			decrypted_msg = calloc(mb_test_size + 40, sizeof(u8));
			c = calloc(mb_test_size + 40, sizeof(u8));

			QueryPerformanceCounter(&start);

			crypto_aead_encrypt(c, msg, mb_test_size, ad, AdLength, nonce, key, tag);
			if (crypto_aead_decrypt(c, mb_test_size, decrypted_msg, ad, AdLength, nonce, key, tag))
				printf("Tag failed!! \n");
			
			QueryPerformanceCounter(&finish);

			double clocks_taken = (double)(finish.QuadPart - start.QuadPart);

			if (clocks_taken < fastestEncryption_mb)
				fastestEncryption_mb = clocks_taken;
			if (clocks_taken > slowestEncryption_mb)
				slowestEncryption_mb = clocks_taken;
			averageSpeed_mb += clocks_taken;

			free(msg);
			free(decrypted_msg);
			free(c);
		}
		averageSpeed_mb = averageSpeed_mb / iterations_mb;
		double cycles_per_byte_mb = 1 / (mb_test_size / averageSpeed_mb);

		//Output results:
		printf("*** megabyte-loop: *** \n");
		printf("Iterations: %i \n", iterations_mb);
		printf("Test size (bytes): %i \n", mb_test_size);
		printf("Average cycles/byte: %f", cycles_per_byte_mb);
		printf("\n");
		printf("Speed in clocks: \n");
		printf("Fastest enc- & decryption: %f \n", fastestEncryption_mb);
		printf("Slowest enc- & decryption: %f \n", slowestEncryption_mb);
		printf("Average enc- & decryption speed: %f \n", averageSpeed_mb);
		printf("Speed in milliseconds: \n");
		printf("Fastest enc- & decryption: %f \n", fastestEncryption_mb / cpu_frequency);
		printf("Slowest enc- & decryption: %f \n", slowestEncryption_mb / cpu_frequency);
		printf("Average enc- & decryption speed: %f", averageSpeed_mb / cpu_frequency);
		printf("\n\n");
	}
	else {
		//Just do a normal encryption/decryption with given #define-parameters.
		crypto_aead_encrypt(c, msg, MsgLength, ad, AdLength, nonce, key, tag);
		if (crypto_aead_decrypt(c, MsgLength, decrypted_msg, ad, AdLength, nonce, key, tag))
			printf("Tag did not match!");

		//Output result of encryption and used data for it
		if (OutputData) {
			printf("Key: \n");
			for (int i = 0; i < GibbonKeyLength; i++) {
				if ((i + 1) % 8 == 1 && i != 0)
					printf("\t");
				printf("%02x ", key[i]);
			}
			printf("\n\n");

			printf("Nonce: \n");
			for (int i = 0; i < GibbonNonceLength; i++) {
				if ((i + 1) % 8 == 1 && i != 0)
					printf("\t");
				printf("%02x ", nonce[i]);
			}
			printf("\n\n");

			printf("Tag: \n");
			for (int i = 0; i < GibbonKeyLength; i++) {
				if ((i + 1) % 8 == 1 && i != 0)
					printf("\t");
				printf("%02x ", tag[i]);
			}
			printf("\n\n");

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