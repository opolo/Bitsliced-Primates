#include "Encrypt.h"
#include <stdio.h>
#include "Primate.h"
#include <time.h>
#include <float.h>
#include <Windows.h>
#include <stdlib.h>

//Signatures
static int bench_cmp(const void *x, const void *y);
void testScheme();
void verboseEncryption();

//Needed since the different compilers (and platforms...) have different ways of reading the CPU cycle counter,  which we use for benchmarks.
#if (_MSC_VER == 1900)
static unsigned long long cpucycles(void)
{
	return __rdtsc();
}
#else
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

#define Key		0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0, \
				0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0

#define Nonce	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0

#define KeySize 30 //u8
#define TagSize 30 //u64
#define NonceSize 15 //u8

//Test a few different input scenarious and prints if the tags failed the tests.
void testScheme() {

	//Allocating enough space for the tests plus some more.
	unsigned char *msg = calloc(400, sizeof(u8));
	unsigned char *decrypted_msg = calloc(400, sizeof(u8)); //8mb (8421376 bytes)
	unsigned char *c = calloc(400, sizeof(u8)); //8mb (8421376 bytes)
	const unsigned char key[KeySize] = { Key };
	const unsigned char nonce[NonceSize] = { Nonce };
	const unsigned char ad[400] = { 0 };
	u64 tag[TagSize];
	int tagmatch;

	printf("\n");
	printf("Testing scheme. \n");

	//Reminder:
	//crypto_aead_encrypt(c, m, mlen, ad, adlen, nonce, key, tag)
	//crypto_aead_decrypt(c, clen, m, ad, adlen, nonce, key, tag)

	//No data
	printf("No message, no AD. \n");
	crypto_aead_encrypt(c, msg, 0, ad, 0, nonce, key, tag);
	tagmatch = crypto_aead_decrypt(c, 0, decrypted_msg, ad, 0, nonce, key, tag);
	if (tagmatch)
		printf("Tag failed!! \n");
	else
		printf("Tag passed. \n");
	memset(msg, 0, 400 * sizeof(u8));
	memset(decrypted_msg, 0, 400 * sizeof(u8));
	memset(c, 0, 400 * sizeof(u8));
	memset(tag, 0, TagSize * sizeof(u64));

	printf("\n");

	printf("Fractional message (less than 40 bytes), fractional AD (less than 40 bytes). \n");
	crypto_aead_encrypt(c, msg, 10, ad, 10, nonce, key, tag);
	tagmatch = crypto_aead_decrypt(c, 10, decrypted_msg, ad, 10, nonce, key, tag);
	if (tagmatch)
		printf("Tag failed!! \n");
	else
		printf("Tag passed. \n");
	memset(msg, 0, 400 * sizeof(u8));
	memset(decrypted_msg, 0, 400 * sizeof(u8));
	memset(c, 0, 400 * sizeof(u8));
	memset(tag, 0, TagSize * sizeof(u64));

	printf("\n");

	printf("Fractional message (more than 40 bytes), fractional AD (more than 40 bytes). \n");
	crypto_aead_encrypt(c, msg, 90, ad, 90, nonce, key, tag);
	tagmatch = crypto_aead_decrypt(c, 90, decrypted_msg, ad, 90, nonce, key, tag);
	if (tagmatch)
		printf("Tag failed!! \n");
	else
		printf("Tag passed. \n");
	memset(msg, 0, 400 * sizeof(u8));
	memset(decrypted_msg, 0, 400 * sizeof(u8));
	memset(c, 0, 400 * sizeof(u8));
	memset(tag, 0, TagSize * sizeof(u64));

	printf("\n");

	printf("integral message (40 bytes), integral AD (40 bytes). \n");
	crypto_aead_encrypt(c, msg, 40, ad, 40, nonce, key, tag);
	tagmatch = crypto_aead_decrypt(c, 40, decrypted_msg, ad, 40, nonce, key, tag);
	if (tagmatch)
		printf("Tag failed!! \n");
	else
		printf("Tag passed. \n");
	memset(msg, 0, 400 * sizeof(u8));
	memset(decrypted_msg, 0, 400 * sizeof(u8));
	memset(c, 0, 400 * sizeof(u8));
	memset(tag, 0, TagSize * sizeof(u64));

	printf("\n");

	printf("integral message (200 bytes), integral AD (200 bytes). \n");
	crypto_aead_encrypt(c, msg, 200, ad, 200, nonce, key, tag);
	tagmatch = crypto_aead_decrypt(c, 200, decrypted_msg, ad, 200, nonce, key, tag);
	if (tagmatch)
		printf("Tag failed!! \n");
	else
		printf("Tag passed. \n");
	memset(msg, 0, 400 * sizeof(u8));
	memset(decrypted_msg, 0, 400 * sizeof(u8));
	memset(c, 0, 400 * sizeof(u8));
	memset(tag, 0, TagSize * sizeof(u64));

	printf("\n");
}

//Benchmark the speed of the scheme.
void benchmark() {

	unsigned char *msg;
	unsigned char *decrypted_msg;
	unsigned char *c;
	u64 start, mid;
	u64 cpu_frequency;

	u64 adLength = 40;
	u8 ad[40] = { 0 };
	u8 nonce[NonceSize] = { Nonce };
	u8 key[KeySize] = { Key };
	u64 tag[TagSize];

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

	//Run only on one core
	SetThreadAffinityMask(GetCurrentThread(), 0x00000008); //Run on fourth core

	printf("Estimating cycle counter frequency... ");
	cpu_frequency = cpucycles();
	Sleep(5000);
	cpu_frequency = (cpucycles() - cpu_frequency) / 5;
	printf("%f GHz\n", cpu_frequency / 1e9);

	printf("Press to start...\n");
	getchar();

	//b loop encrypt
	for (int i = 0; i < iterations_b; i++) {
		msg = calloc(b_test_size, sizeof(u8));
		decrypted_msg = calloc(b_test_size + 40, sizeof(u8));
		c = calloc(b_test_size + 40, sizeof(u8));

		start = cpucycles();
		crypto_aead_encrypt(c, msg, b_test_size, ad, adLength, nonce, key, tag);
		mid = cpucycles();
		int tagmatch = crypto_aead_decrypt(c, b_test_size, decrypted_msg, ad, adLength, nonce, key, tag);
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
	double enc_cycles_per_byte_b = 1 / (double)(((double)b_test_size + adLength) / (double)enc_medianSpeed_b);
	double dec_cycles_per_byte_b = 1 / (double)(((double)b_test_size + adLength) / (double)dec_medianSpeed_b);

	//Output results:
	printf("*** byte-loop: *** \n");
	printf("Iterations: %i \n", iterations_b);
	printf("Test size data + AD (in bytes): %i + %llu \n", b_test_size, adLength);
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
		crypto_aead_encrypt(c, msg, kb_test_size, ad, adLength, nonce, key, tag);
		mid = cpucycles();
		int tagmatch = crypto_aead_decrypt(c, kb_test_size, decrypted_msg, ad, adLength, nonce, key, tag);
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
	double enc_cycles_per_byte_kb = 1 / (double)(((double)kb_test_size + adLength) / (double)enc_medianSpeed_kb);
	double dec_cycles_per_byte_kb = 1 / (double)(((double)kb_test_size + adLength) / (double)dec_medianSpeed_kb);

	//Output results:
	printf("*** kilobyte-loop: *** \n");
	printf("Iterations: %i \n", iterations_kb);
	printf("Test size data + AD (in bytes): %i + %llu \n", kb_test_size, adLength);
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
		crypto_aead_encrypt(c, msg, mb_test_size, ad, adLength, nonce, key, tag);
		mid = cpucycles();
		int tagmatch = crypto_aead_decrypt(c, mb_test_size, decrypted_msg, ad, adLength, nonce, key, tag);
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
	double enc_cycles_per_byte_mb = 1 / (double)(((double)mb_test_size + adLength) / (double)enc_medianSpeed_mb);
	double dec_cycles_per_byte_mb = 1 / (double)(((double)mb_test_size + adLength) / (double)dec_medianSpeed_mb);

	//Output results:
	printf("*** megabyte-loop: *** \n");
	printf("Iterations: %i \n", iterations_mb);
	printf("Test size data + AD (in bytes): %i + %llu \n", mb_test_size, adLength);
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

//Performs an ordinary encryption & decryption and prints input and output data.
void verboseEncryption() {
	unsigned char *msg = calloc(120, sizeof(u8));
	unsigned char *decrypted_msg = calloc(120, sizeof(u8));
	unsigned char *c = calloc(120, sizeof(u8)); 
	const unsigned char key[KeySize] = { Key };
	const unsigned char nonce[NonceSize] = { Nonce };
	const unsigned char ad[40] = { 0 };
	u64 tag[TagSize];

	int msgLength = 80;
	int AdLength = 40;

	crypto_aead_encrypt(c, msg, msgLength, ad, 40, nonce, key, tag);
	if (crypto_aead_decrypt(c, msgLength, decrypted_msg, ad, 40, nonce, key, tag))
		printf("Tag did not match! \n");

	printf("Key: \n");
	for (int i = 0; i < KeySize; i++) {
		if ((i + 1) % 8 == 1 && i != 0)
			printf("\t");
		printf("%02x ", key[i]);
	}
	printf("\n\n");

	printf("Nonce: \n");
	for (int i = 0; i < NonceSize; i++) {
		if ((i + 1) % 8 == 1 && i != 0)
			printf("\t");
		printf("%02x ", nonce[i]);
	}
	printf("\n\n");

	printf("Plaintext: \n");
	for (int i = 0; i < msgLength; i++) {
		if ((i + 1) % 8 == 1 && i != 0)
			printf("\t");
		printf("%02x ", msg[i]);
	}
	printf("\n\n");

	printf("Ciphertext: \n");
	for (int i = 0; i < msgLength; i++) {
		if ((i + 1) % 8 == 1 && i != 0)
			printf("\t");
		printf("%02x ", c[i]);
	}
	printf("\n\n");

	printf("Decrypted plaintext: \n");
	for (int i = 0; i < msgLength; i++) {
		if ((i + 1) % 8 == 1 && i != 0)
			printf("\t");
		printf("%02x ", decrypted_msg[i]);
	}
	printf("\n\n");
}

int main() {

	//Implementation version
	printf("APE120-BS V1.01 \n");

	//Needed before the PRIMATEs permutation is used.
	Initialize();

	//Test whether primates are working as intended
	if (Debug) {
		test_primates();
		testScheme();
	}

	if (Benchmark) {
		benchmark();
	}

	if (Verbose) {
		verboseEncryption();
	}

	printf("Push return to quit... \n");
	getchar();

	return 0;
}

static int bench_cmp(const void *x, const void *y)
{
	const u64 *ix = (const u64 *)x;
	const u64 *iy = (const u64 *)y;
	return (int) *ix - *iy;
}