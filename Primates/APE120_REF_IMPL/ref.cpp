// ref.cpp : main project file.

#include "api.h"
#include <stdio.h>
#include <stdlib.h>
#include "crypto_aead.h"
#include <string.h>
#include <Windows.h>
#include <float.h>

#define u8 unsigned char
#define AdLength 40 

#define GibbonKey {0x0},	{0x00},	{0x00},	{0x00},	{0x00},	{0x00},	{0x00},	{0x00},	{0x0},	{0x0},	{0x0},	{0x0},	{0x0},	{0x0},	{0x0}
#define GibbonKeyLength 15

#define GibbonNonce	{0x00}, {0x00},	{0x00},	{0x00},	{0x00},	{0x00},	{0x00},	{0x00},	{0x0},	{0x0},	{0x0},	{0x0},	{0x0},	{0x0},	{0x0}
#define GibbonNonceLength 15

#define DataOnes	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF}
#define DataOnes40 DataOnes, DataOnes, DataOnes, DataOnes, DataOnes

int main()
{
	printf("APE-120 reference implementation: \n");

	if (1) {
		const unsigned char ad[AdLength] = { DataOnes40 };
		const unsigned char key[GibbonKeyLength] = { GibbonKey };
		const unsigned char nonce[GibbonNonceLength] = { GibbonNonce };

		LARGE_INTEGER start, finish;
		double cpu_frequency;
		QueryPerformanceFrequency(&start);
		cpu_frequency = (double)(start.QuadPart) / 1000.0; //Frequency: Ticks per milisecond on the system.

		int iterations_b = 20'000;
		int iterations_kb = 2'000;
		int iterations_mb = 40;

		int b_test_size = 40;
		int kb_test_size = 4000;
		int mb_test_size = 2'000'000;

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

		printf("Press to start...\n");
		getchar();

		//b loop
		for (int i = 0; i < iterations_b; i++) {
			unsigned char *msg = (unsigned char*)calloc(b_test_size + 120, sizeof(u8));
			unsigned char *decrypted_msg = (unsigned char*)calloc(b_test_size + 120, sizeof(u8)); //8mb (8421376 bytes)
			unsigned char *c = (unsigned char*)calloc(b_test_size + 120, sizeof(u8)); //8mb (8421376 bytes)
			unsigned long long clen;
			unsigned long long mlenT;

			QueryPerformanceCounter(&start);

			crypto_aead_encrypt(c, &clen, msg, b_test_size + 1, ad, AdLength + 1, NULL, nonce, key);
			int tagmatch = crypto_aead_decrypt(decrypted_msg, &mlenT, NULL, c, clen, ad, AdLength + 1, nonce, key);
			if (tagmatch)
				printf("Tag failed!! \n");

			QueryPerformanceCounter(&finish);

			if (i % (iterations_b / 10) == 0) {
				//One tenth through...
				printf("Progress: %i/10 through. \n", i / (iterations_b / 10));
			}

			double clocks_taken = (double)(finish.QuadPart - start.QuadPart);

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
			unsigned char *msg = (unsigned char*)calloc(kb_test_size + 120, sizeof(u8));
			unsigned char *decrypted_msg = (unsigned char*)calloc(kb_test_size + 120, sizeof(u8)); //8mb (8421376 bytes)
			unsigned char *c = (unsigned char*)calloc(kb_test_size + 120, sizeof(u8)); //8mb (8421376 bytes)
			unsigned long long clen;
			unsigned long long mlenT;

			QueryPerformanceCounter(&start);

			crypto_aead_encrypt(c, &clen, msg, kb_test_size + 1, ad, AdLength + 1, NULL, nonce, key);
			int tagmatch = crypto_aead_decrypt(decrypted_msg, &mlenT, NULL, c, clen, ad, AdLength + 1, nonce, key);
			if (tagmatch)
				printf("Tag failed!! \n");

			QueryPerformanceCounter(&finish);

			if (i % (iterations_kb / 10) == 0) {
				//One tenth through...
				printf("Progress: %i/10 through. \n", i / (iterations_kb / 10));
			}

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
			unsigned char *msg = (unsigned char*)calloc(mb_test_size + 120, sizeof(u8));
			unsigned char *decrypted_msg = (unsigned char*)calloc(mb_test_size + 120, sizeof(u8)); //8mb (8421376 bytes)
			unsigned char *c = (unsigned char*)calloc(mb_test_size + 120, sizeof(u8)); //8mb (8421376 bytes)
			unsigned long long clen;
			unsigned long long mlenT;

			QueryPerformanceCounter(&start);

			crypto_aead_encrypt(c, &clen, msg, mb_test_size + 1, ad, AdLength + 1, NULL, nonce, key);
			int tagmatch = crypto_aead_decrypt(decrypted_msg, &mlenT, NULL, c, clen, ad, AdLength + 1, nonce, key);
			if (tagmatch)
				printf("Tag failed!! \n");

			QueryPerformanceCounter(&finish);

			if (i % (iterations_mb / 10) == 0) {
				//One tenth through...
				printf("Progress: %i/10 through. \n", i / (iterations_mb / 10));
			}

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
		int cor;

		const unsigned char k[CRYPTO_KEYBYTES] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14 };
		const unsigned char npub[CRYPTO_NPUBBYTES] = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
		unsigned char *m = (unsigned char*)calloc(256, sizeof(u8));
		unsigned char ad[256];
		unsigned char c[256 + CRYPTO_ABYTES];
		unsigned long long clen;
		unsigned long long mlenT;

		unsigned char mT[256];

		/*
		unsigned long long mlen = 0;
		unsigned long long adlen = 0;
		crypto_aead_encrypt(c,&clen,m,mlen,ad,adlen,NULL,npub,k);
		for(unsigned long long i=0; i<clen; i++)
		printf("%.2x ",c[i]);
		printf("\n");


		cor = crypto_aead_decrypt(mT,&mlenT,NULL,c,clen,ad,adlen,npub,k);
		if(cor==0)
		{
		for(unsigned long long i=0; i<mlenT; i++)
		printf("%.2x ",m[i]);
		}
		else
		printf("authentication failed!!");
		*/





		printf("\nNow start the old code... \n");

		for (unsigned long long mlen = 0; mlen < 256; mlen++) {
			m[mlen] = mlen ^ 0x0c;
			for (unsigned long long adlen = 0; adlen < 256; adlen++) {
				ad[adlen] = adlen ^ 0x0f;

				crypto_aead_encrypt(c, &clen, m, mlen + 1, ad, adlen + 1, NULL, npub, k);
				//	    assert(clen == mlen + CRYPTO_ABYTES);
				/*
				for(unsigned long long i=0; i<clen; i++)
				printf("%.2x ",c[i]);
				printf("\n");
				*/

				cor = crypto_aead_decrypt(mT, &mlenT, NULL, c, clen, ad, adlen + 1, npub, k);
				if (cor == 0)
				{
					if (mlenT != (mlen + 1))
						printf("sizes don't match!");
					for (unsigned long long i = 0; i<mlenT; i++)
						if (mT[i] != m[i]) {
							printf("messages don't match"); break;
						}

					/*for(unsigned long long i=0; i<mlenT; i++)
					printf("%.2x ",mT[i]);*/
				}
				else
					printf("authentication failed!!");
			}
		}


	}


	getchar();
	return 0;

}
