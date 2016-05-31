#include "Encrypt.h"
#include <stdio.h>
#include "Primate.h"
#include <time.h>

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

#define DataZeroes	{0x00},	{0x00},	{0x00},	{0x00},	{0x00},	{0x00},	{0x00},	{0x00}
#define DataZeroes40 DataZeroes, DataZeroes, DataZeroes, DataZeroes, DataZeroes

//Constants for datalength. Change as needed
//#define MsgLength 80
#define MsgLength 100'000'000
#define AdLength 40

void main() {
	
	//data
	unsigned char *msg = calloc(MsgLength, sizeof(u8)); //8mb (8421376 bytes)
	unsigned char *decrypted_msg = calloc(MsgLength + 40, sizeof(u8)); //8mb (8421376 bytes)
	unsigned char *c = calloc(MsgLength + 40, sizeof(u8)); //8mb (8421376 bytes)

	//data
	//const unsigned char msg[MsgLength] = { DataOnes40,  DataZeroes40 };
	//const unsigned char decrypted_msg[MsgLength];
	//unsigned char *c = calloc(100, sizeof(u8)); //Make room for message + 40 extra byte due to how implementation works.. could be done more tidy

	//constant length data
	const unsigned char key[GibbonKeyLength] = { GibbonKey };
	const unsigned char nonce[GibbonNonceLength] = { GibbonNonce };
	const unsigned char ad[AdLength] = { DataOnes40 };
	unsigned char tag[GibbonTagLength];

	u64 cLength = 0;
	
	Initialize();

	//Not needed but a nice check to do during development
	if (DEBUG) {
		test_primates();
	}
		
	clock_t start, finish;
	if (DEBUG) {
		start = clock();
	}

	crypto_aead_encrypt(c, msg, MsgLength, ad, AdLength, nonce, key, tag);
	crypto_aead_decrypt(c, MsgLength, decrypted_msg, ad, AdLength, nonce, key, tag);

	if (DEBUG) {
		finish = clock();
		printf("Time taken: %f", (double)((finish - start) / CLOCKS_PER_SEC));
		printf("\nClocks: %f", (double)((finish - start)));
	}

	/*
	printf("Key: \n");
	for (int i = 0; i < GibbonKeyLength; i++) {
		printf("%02x ", key[i]);
	}
	printf("\n\n");

	printf("Nonce: \n");
	for (int i = 0; i < GibbonNonceLength; i++) {
		printf("%02x ", nonce[i]);
	}
	printf("\n\n");

	printf("Plaintext: \n");
	for (int i = 0; i < MsgLength; i++) {
		printf("%02x ", msg[i]);
	}
	printf("\n\n");

	printf("Ciphertext: \n" );
	for (int i = 0; i < MsgLength; i++) {
		printf("%02x ", c[i]);
	}
	printf("\n\n");

	printf("Decrypted plaintext: \n");
	for (int i = 0; i < MsgLength; i++) {
		printf("%02x ", decrypted_msg[i]);
	}
	printf("\n\n");
	*/

	getchar();
}