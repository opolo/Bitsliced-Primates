#include "Encrypt.h"
#include <stdio.h>
#include "Primate.h"
#include <time.h>
#include <float.h>
#include <Windows.h>
#include <stdlib.h>

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

#define Key		0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0

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

	printf("HANUMAN120-BS V1.00 \n");

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
	u8 tag[30];


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

		u64 start, mid;
		u64 cpu_frequency;

		printf("Estimating cycle counter frequency... ");
		cpu_frequency = cpucycles();
		Sleep(5000);
		cpu_frequency = (cpucycles() - cpu_frequency) / 5;
		printf("%f GHz\n", cpu_frequency / 1e9);


		int iterations_b = 20000;
		int iterations_kb = 2000;
		int iterations_mb = 40;

		int b_test_size = 40;
		int kb_test_size = 4000;
		int mb_test_size = 2000000;

		u64 *enc_results_b = calloc(iterations_b, sizeof(u64));
		u64 *enc_results_kb = calloc(iterations_kb, sizeof(u64));
		u64 *enc_results_mb = calloc(iterations_mb, sizeof(u64));

		u64 *dec_results_b = calloc(iterations_b, sizeof(u64));
		u64 *dec_results_kb = calloc(iterations_kb, sizeof(u64));
		u64 *dec_results_mb = calloc(iterations_mb, sizeof(u64));

		printf("Press to start...\n");
		getchar();

		//b loop encrypt
		for (int i = 0; i < iterations_b; i++) {
			msg = calloc(b_test_size, sizeof(u8));
			decrypted_msg = calloc(b_test_size + 40, sizeof(u8));
			c = calloc(b_test_size + 40, sizeof(u8));

			start = cpucycles();
			crypto_aead_encrypt(c, msg, b_test_size, ad, AdLength, nonce, key, tag);
			mid = cpucycles();
			int tagmatch = crypto_aead_decrypt(c, b_test_size, decrypted_msg, ad, AdLength, nonce, key, tag);
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
			msg = calloc(kb_test_size, sizeof(u8));
			decrypted_msg = calloc(kb_test_size + 40, sizeof(u8));
			c = calloc(kb_test_size + 40, sizeof(u8));

			start = cpucycles();
			crypto_aead_encrypt(c, msg, kb_test_size, ad, AdLength, nonce, key, tag);
			mid = cpucycles();
			int tagmatch = crypto_aead_decrypt(c, kb_test_size, decrypted_msg, ad, AdLength, nonce, key, tag);
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
			msg = calloc(mb_test_size, sizeof(u8));
			decrypted_msg = calloc(mb_test_size + 40, sizeof(u8));
			c = calloc(mb_test_size + 40, sizeof(u8));

			start = cpucycles();
			crypto_aead_encrypt(c, msg, mb_test_size, ad, AdLength, nonce, key, tag);
			mid = cpucycles();
			int tagmatch = crypto_aead_decrypt(c, mb_test_size, decrypted_msg, ad, AdLength, nonce, key, tag);
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

static int bench_cmp(const void *x, const void *y)
{
	const u64 *ix = (const u64 *)x;
	const u64 *iy = (const u64 *)y;
	return (int)*ix - *iy;
}