// ref.cpp : main project file.

#include "api.h"
#include <stdio.h>
#include <stdlib.h>
#include "crypto_aead.h"
#include <string.h>
#include <Windows.h>
#include <float.h>

typedef unsigned long long u64;
static int bench_cmp(const void *x, const void *y);

#if (_MSC_VER == 1900)
static unsigned long long cpucycles(void)
{
	return __rdtsc();
}
#elif defined(__x86_64__)
static unsigned long long cpucycles(void)
{
	unsigned long long result;
	__asm__ __volatile__
	(
		".byte 15;.byte 49\n"
		"shlq $32,%%rdx\n"
		"orq %%rdx,%%rax"
		: "=a" (result) ::  "%rdx"
	);
	return result;
}
#endif

#define u8 unsigned char
#define AdLength 40 

#define GibbonKey 0x0,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0
#define GibbonKeyLength 15

#define GibbonNonce	0x00, 0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0
#define GibbonNonceLength 15

#define DataOnes	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF,	0xFF
#define DataOnes40 DataOnes, DataOnes, DataOnes, DataOnes, DataOnes

int cmpfunc(const void * a, const void * b)
{
	return (int)(*(u64*)a - *(u64*)b);
}

int main()
{
	printf("GIBBON-120 reference implementation v1.00 \n");

	if (1) {
		const unsigned char ad[AdLength] = { DataOnes40 };
		const unsigned char key[GibbonKeyLength] = { GibbonKey };
		const unsigned char nonce[GibbonNonceLength] = { GibbonNonce };


		//Run only on one core
		SetThreadAffinityMask(GetCurrentThread(), 0x00000008); //Run on fourth core

		u64 start, mid;
		u64 cpu_frequency;

		printf("Estimating cycle counter frequency... ");
		cpu_frequency = cpucycles();
		Sleep(5000);
		cpu_frequency = (cpucycles() - cpu_frequency) / 5;
		printf("%f GHz\n", cpu_frequency / 1e9);

		int iterations_b = 10000;
		int iterations_kb = 1000;
		int iterations_mb = 10;

		int b_test_size = 40;
		int kb_test_size = 4000;
		int mb_test_size = 2000000;

		u64 *enc_results_b = (u64*)calloc(iterations_b, sizeof(u64));
		u64 *enc_results_kb = (u64*)calloc(iterations_kb, sizeof(u64));
		u64 *enc_results_mb = (u64*)calloc(iterations_mb, sizeof(u64));

		u64 *dec_results_b = (u64*)calloc(iterations_b, sizeof(u64));
		u64 *dec_results_kb = (u64*)calloc(iterations_kb, sizeof(u64));
		u64 *dec_results_mb = (u64*)calloc(iterations_mb, sizeof(u64));

		printf("Press to start...\n");
		getchar();

		//b loop encrypt
		for (int i = 0; i < iterations_b; i++) {
			unsigned char *msg = (unsigned char*)calloc(b_test_size + 120, sizeof(u8));
			unsigned char *decrypted_msg = (unsigned char*)calloc(b_test_size + 120, sizeof(u8)); //8mb (8421376 bytes)
			unsigned char *c = (unsigned char*)calloc(b_test_size + 120, sizeof(u8)); //8mb (8421376 bytes)
			unsigned long long clen;
			unsigned long long mlenT;

			start = cpucycles();
			crypto_aead_encrypt(c, &clen, msg, b_test_size + 1, ad, AdLength + 1, NULL, nonce, key);
			mid = cpucycles();
			int tagmatch = crypto_aead_decrypt(decrypted_msg, &mlenT, NULL, c, clen, ad, AdLength + 1, nonce, key);
			if (tagmatch)
				printf("Tag failed!! \n");

			dec_results_b[i] = cpucycles() - mid;
			enc_results_b[i] = mid - start;

			if (i % (iterations_b / 10) == 0) {
				//One tenth through...
				printf("Progress: %i/10 through. \n", (i / (iterations_b / 10) + 1));
			}

			free(msg);
			free(decrypted_msg);
			free(c);
		}
		qsort(enc_results_b, iterations_b, sizeof(u64), bench_cmp);
		qsort(dec_results_b, iterations_b, sizeof(u64), bench_cmp);
		u64 enc_medianSpeed_b = enc_results_b[iterations_b / 2]; //median amount of cycles for 40 bytes.
		u64 dec_medianSpeed_b = dec_results_b[iterations_b / 2]; //median amount of cycles for 40 bytes.
		double enc_cycles_per_byte_b = 1 / (double)(((double)b_test_size + AdLength) / (double)enc_medianSpeed_b);
		double dec_cycles_per_byte_b = 1 / (double)(((double)b_test_size + AdLength) / (double)dec_medianSpeed_b);

		//Output results:
		printf("*** byte-loop: *** \n");
		printf("Iterations: %i \n", iterations_b);
		printf("Test size data + AD (in bytes): %i + %i \n", b_test_size, AdLength);
		printf("Median cycles/byte encrypting: %f \n", enc_cycles_per_byte_b);
		printf("Median cycles/byte decrypting: %f \n", dec_cycles_per_byte_b);
		printf("Median encrypting clocks: %llu \n", enc_medianSpeed_b);
		printf("Median decrypting clocks: %llu \n", dec_medianSpeed_b);
		printf("\n\n");

		//kb loop
		for (int i = 0; i < iterations_kb; i++) {
			unsigned char *msg = (unsigned char*)calloc(kb_test_size + 120, sizeof(u8));
			unsigned char *decrypted_msg = (unsigned char*)calloc(kb_test_size + 120, sizeof(u8)); //8mb (8421376 bytes)
			unsigned char *c = (unsigned char*)calloc(kb_test_size + 120, sizeof(u8)); //8mb (8421376 bytes)
			unsigned long long clen;
			unsigned long long mlenT;

			start = cpucycles();
			crypto_aead_encrypt(c, &clen, msg, kb_test_size + 1, ad, AdLength + 1, NULL, nonce, key);
			mid = cpucycles();
			int tagmatch = crypto_aead_decrypt(decrypted_msg, &mlenT, NULL, c, clen, ad, AdLength + 1, nonce, key);
			if (tagmatch)
				printf("Tag failed!! \n");

			dec_results_kb[i] = cpucycles() - mid;
			enc_results_kb[i] = mid - start;

			if (i % (iterations_kb / 10) == 0) {
				//One tenth through...
				printf("Progress: %i/10 through. \n", (i / (iterations_kb / 10) + 1));
			}

			free(msg);
			free(decrypted_msg);
			free(c);
		}
		qsort(enc_results_kb, iterations_kb, sizeof(u64), bench_cmp);
		qsort(dec_results_kb, iterations_kb, sizeof(u64), bench_cmp);
		u64 enc_medianSpeed_kb = enc_results_kb[iterations_kb / 2]; //median amount of cycles for 40 bytes.
		u64 dec_medianSpeed_kb = dec_results_kb[iterations_kb / 2]; //median amount of cycles for 40 bytes.
		double enc_cycles_per_byte_kb = 1 / (double)(((double)kb_test_size + AdLength) / (double)enc_medianSpeed_kb);
		double dec_cycles_per_byte_kb = 1 / (double)(((double)kb_test_size + AdLength) / (double)dec_medianSpeed_kb);

		//Output results:
		printf("*** kilobyte-loop: *** \n");
		printf("Iterations: %i \n", iterations_kb);
		printf("Test size data + AD (in bytes): %i + %i \n", kb_test_size, AdLength);
		printf("Median cycles/byte encrypting: %f \n", enc_cycles_per_byte_kb);
		printf("Median cycles/byte decrypting: %f \n", dec_cycles_per_byte_kb);
		printf("Median encrypting clocks: %llu \n", enc_medianSpeed_kb);
		printf("Median decrypting clocks: %llu \n", dec_medianSpeed_kb);
		printf("\n\n");



		//mb loop
		for (int i = 0; i < iterations_mb; i++) {
			unsigned char *msg = (unsigned char*)calloc(mb_test_size + 120, sizeof(u8));
			unsigned char *decrypted_msg = (unsigned char*)calloc(mb_test_size + 120, sizeof(u8)); //8mb (8421376 bytes)
			unsigned char *c = (unsigned char*)calloc(mb_test_size + 120, sizeof(u8)); //8mb (8421376 bytes)
			unsigned long long clen;
			unsigned long long mlenT;

			start = cpucycles();
			crypto_aead_encrypt(c, &clen, msg, mb_test_size + 1, ad, AdLength + 1, NULL, nonce, key);
			mid = cpucycles();
			int tagmatch = crypto_aead_decrypt(decrypted_msg, &mlenT, NULL, c, clen, ad, AdLength + 1, nonce, key);
			if (tagmatch)
				printf("Tag failed!! \n");

			dec_results_mb[i] = cpucycles() - mid;
			enc_results_mb[i] = mid - start;

			if (i % (iterations_mb / 10) == 0) {
				//One tenth through...
				printf("Progress: %i/10 through. \n", (i / (iterations_mb / 10) + 1));
			}

			free(msg);
			free(decrypted_msg);
			free(c);
		}
		qsort(enc_results_mb, iterations_mb, sizeof(u64), bench_cmp);
		qsort(dec_results_mb, iterations_mb, sizeof(u64), bench_cmp);
		u64 enc_medianSpeed_mb = enc_results_mb[iterations_mb / 2]; //median amount of cycles for 40 bytes.
		u64 dec_medianSpeed_mb = dec_results_mb[iterations_mb / 2]; //median amount of cycles for 40 bytes.
		double enc_cycles_per_byte_mb = 1 / (double)(((double)mb_test_size + AdLength) / (double)enc_medianSpeed_mb);
		double dec_cycles_per_byte_mb = 1 / (double)(((double)mb_test_size + AdLength) / (double)dec_medianSpeed_mb);

		//Output results:
		printf("*** megabyte-loop: *** \n");
		printf("Iterations: %i \n", iterations_mb);
		printf("Test size data + AD (in bytes): %i + %i \n", mb_test_size, AdLength);
		printf("Median cycles/byte encrypting: %f \n", enc_cycles_per_byte_mb);
		printf("Median cycles/byte decrypting: %f \n", dec_cycles_per_byte_mb);
		printf("Median encrypting clocks: %llu \n", enc_medianSpeed_mb);
		printf("Median decrypting clocks: %llu \n", dec_medianSpeed_mb);
		printf("\n\n");

		free(enc_results_b);
		free(enc_results_kb);
		free(enc_results_mb);
		free(dec_results_b);
		free(dec_results_kb);
		free(dec_results_mb);
	}

	else {
		int cor;

		const unsigned char k[CRYPTO_KEYBYTES] = { 0,1,2,3,4,5,6,7,8,9};
		const unsigned char npub[CRYPTO_NPUBBYTES] = { 1,2,3,4,5,6,7,8,9,10};
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


static int bench_cmp(const void *x, const void *y)
{
	const u64 *ix = (const u64 *)x;
	const u64 *iy = (const u64 *)y;
	return (int)*ix - *iy;
}