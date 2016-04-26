#include "Primates.h"
#include <stdio.h>
#include <string.h>
#include <immintrin.h>

//1 key section = 2 rows in the primate state.
#define P120KeySec1	  {0x00},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},		{0x0},	{0x0},	{0x0},	{0x0},	{0x0},	{0x0},	{0x0},	{0x0}, 
#define P120KeySec2	  {0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},		{0x0},	{0x0},	{0x0},	{0x0},	{0x0},	{0x0},	{0x0},	{0x0},
#define P120KeySec3	  {0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},		{0x0},	{0x0},	{0x0},	{0x0},	{0x0},	{0x00}, {0x0},	{0xFF}

//1 nonce section = 1 row (the rate-row) in the primate state 
#define P120NonceSec1 {0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},
#define P120NonceSec2 {0x0},	{0x0},	{0x0},	{0x0},	{0x0},	{0x0},	{0x0},	{0x0},
#define P120NonceSec3 {0x0},	{0x0},	{0x0},	{0x0},	{0x0},	{0x0},	{0x0},	{0x0}

//1 rate section = 1 row (the rate-row) in the primate state 
#define ADSec1		  {0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},
#define ADSec2		  {0x0},	{0x0},	{0x0},	{0x0},	{0x0},	{0x0},	{0x0},	{0x0},
#define ADSec3		  {0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF},	{0xFF}
#define ADLen		  24 //Should always be based on the size of above

void main() {

	//constant length
	const unsigned char keys[4][keyLength] = { { P120KeySec1 P120KeySec2 P120KeySec3 }, { P120KeySec1 P120KeySec2 P120KeySec3 }, { P120KeySec1 P120KeySec2 P120KeySec3 }, { P120KeySec1 P120KeySec2 P120KeySec3 } };
	const unsigned char nonces[4][NonceLength] = { { P120NonceSec1 P120NonceSec2 P120NonceSec3 }, 
												   { P120NonceSec1 P120NonceSec2 P120NonceSec3 }, 
												   { P120NonceSec1 P120NonceSec2 P120NonceSec3 }, 
												   { P120NonceSec1 P120NonceSec2 P120NonceSec3 } };
	
	//variable length
	const unsigned char *msg[4];
	const unsigned char msg0[ADLen] = { ADSec1 ADSec2 ADSec3 };
	const unsigned char msg1[ADLen] = { ADSec1 ADSec2 ADSec3 };
	const unsigned char msg2[ADLen] = { ADSec1 ADSec2 ADSec3 };
	const unsigned char msg3[ADLen] = { ADSec1 ADSec2 ADSec3 };
	msg[0] = &msg0;
	msg[1] = &msg1;
	msg[2] = &msg2;
	msg[3] = &msg3;

	u64 msgLengths[4] = { { ADLen },{ ADLen },{ ADLen },{ ADLen } };

	const unsigned char *ad[4];
	const unsigned char ad0[ADLen] = { ADSec1 ADSec2 ADSec3 };
	const unsigned char ad1[ADLen] = { ADSec1 ADSec2 ADSec3 };
	const unsigned char ad2[ADLen] = { ADSec1 ADSec2 ADSec3 };
	const unsigned char ad3[ADLen] = { ADSec1 ADSec2 ADSec3 };
	ad[0] = &ad0;
	ad[1] = &ad1;
	ad[2] = &ad2;
	ad[3] = &ad3;

	u64 adLengths[4] = { { ADLen },{ ADLen },{ ADLen },{ ADLen } };
	
	//Return data from function. TODO: Make ciphertext support different lengths between states
	unsigned char *ciphertexts[4];
	ciphertexts[0] = malloc(sizeof(unsigned char)*ADLen);
	ciphertexts[1] = malloc(sizeof(unsigned char)*ADLen);
	ciphertexts[2] = malloc(sizeof(unsigned char)*ADLen);
	ciphertexts[3] = malloc(sizeof(unsigned char)*ADLen);
	unsigned char tags[4][keyLength];

	unsigned char *plaintexts[4];
	plaintexts[0] = malloc(sizeof(unsigned char)*ADLen);
	plaintexts[1] = malloc(sizeof(unsigned char)*ADLen);
	plaintexts[2] = malloc(sizeof(unsigned char)*ADLen);
	plaintexts[3] = malloc(sizeof(unsigned char)*ADLen);
	
	primates120_encrypt(keys, msg, msgLengths, ad, adLengths, nonces, ciphertexts, tags);
	
	//Print plaintext state 0 before enc
	printf("Before encrypt: \n");
	for (int i = 0; i < ADLen; i++) {
		//before
		printf("%02x ", byte_to_primate_element(msg[0][i]));
	}

	printf("\n");
	//Print output after enc
	printf("After encrypt: \n");
	for (int i = 0; i < ADLen; i++) {
		//before
		printf("%02x ", byte_to_primate_element(ciphertexts[0][i]));
	}
	printf("\n");

	primates120_decrypt(keys, ciphertexts, msgLengths, ad, adLengths, nonces, plaintexts, tags);
	//print output after decryption
	printf("After decrypt: \n");
	for (int i = 0; i < ADLen; i++) {
		printf("%02x ", plaintexts[0][i]);
	}

	getchar();
}