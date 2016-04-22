#include "Debug.h"

#define YMMCount 5
#define YMMLength 256

//void print_YMMs(__m256i YMM[5]) {}

void print_keys_hex(const unsigned char k[4][keyLength]) {

	for (int strNo = 0; strNo < 4; strNo++) {
		const char *s = k[strNo];
		printf("Key %i : ", strNo);

		for (int byte = 0; byte < keyLength; byte++) {
			printf("%02x ", s[byte]);
		}
		printf("\n");
	}
}
void print_keys(const unsigned char k[4][keyLength]) {
	printf("Key 0: %s \n", k[0]);
	printf("Key 1: %s \n", k[1]);
	printf("Key 2: %s \n", k[2]);
	printf("Key 3: %s \n", k[3]);
}

void print_nonces_hex(const unsigned char npub[4][NonceLength]) {

	for (int strNo = 0; strNo < 4; strNo++) {
		const char *s = npub[strNo];
		printf("Nonce %i : ", strNo);

		for (int byte = 0; byte < NonceLength; byte++) {
			printf("%02x ", s[byte]);
		}
		printf("\n");
	}
}
void print_nonces(const unsigned char npub[4][NonceLength]) {
	printf("Nonce 0: %s \n", npub[0]);
	printf("Nonce 1: %s \n", npub[1]);
	printf("Nonce 2: %s \n", npub[2]);
	printf("Nonce 3: %s \n", npub[3]);
}

void print_ad_hex(const unsigned char *ad[4], u64 adlen[4]) {
	if (adlen == 0) {
		printf("No associated data with any current encryption \n");
		return;
	}

	for (int strNo = 0; strNo < 4; strNo++) {
		const char *s = ad[strNo];
		printf("Ass. Data %i : ", strNo);

		if (adlen[strNo] != 0) {
			for (int byte = 0; byte < adlen[strNo]; byte++) {
				printf("%02x ", s[byte]);
			}
		}
		else {
			printf("No data.");
		}
		printf("\n");
	}
}
void print_ad(const unsigned char *ad[4], u64 adlen[4]) {
	if (adlen == 0) {
		printf("No associated data \n");
		return;
	}

	if (adlen[0] != 0)
		printf("Ass. Data 0: %s \n", ad[0]);
	if (adlen[1] != 0)
		printf("Ass. Data 1: %s \n", ad[1]);
	if (adlen[2] != 0)
		printf("Ass. Data 2: %s \n", ad[2]);
	if (adlen[3] != 0)
		printf("Ass. Data 3: %s \n", ad[3]);
}

void print_YMMs(__m256i *YMMs) {
	unsigned char *YMM0 = _aligned_malloc(sizeof(char) * 32, 32); //Allocate 32byte, 32byte aligned
	unsigned char *YMM1 = _aligned_malloc(sizeof(char) * 32, 32); //Allocate 32byte, 32byte aligned
	unsigned char *YMM2 = _aligned_malloc(sizeof(char) * 32, 32); //Allocate 32byte, 32byte aligned
	unsigned char *YMM3 = _aligned_malloc(sizeof(char) * 32, 32); //Allocate 32byte, 32byte aligned
	unsigned char *YMM4 = _aligned_malloc(sizeof(char) * 32, 32); //Allocate 32byte, 32byte aligned


	_mm256_store_si256(YMM0, YMMs[0]);
	_mm256_store_si256(YMM1, YMMs[1]);
	_mm256_store_si256(YMM2, YMMs[2]);
	_mm256_store_si256(YMM3, YMMs[3]);
	_mm256_store_si256(YMM4, YMMs[4]);

	printf("YMM 0: %02X \n", YMM0);
}


void print_u64s_as_binary(u64 transposedData[5][4]) {
	char reg[YMMCount][YMMLength];

	for (int YMM_no = 0; YMM_no < YMMCount; YMM_no++) {
		char registerString[256];
		char tempString[64];

		u64_to_string(tempString, transposedData[YMM_no][0]);
		strcat(registerString, tempString);
		u64_to_string(tempString, transposedData[YMM_no][1]);
		strcat(registerString, tempString);
		u64_to_string(tempString, transposedData[YMM_no][2]);
		strcat(registerString, tempString);
		u64_to_string(tempString, transposedData[YMM_no][3]);
		strcat(registerString, tempString);

		printf("Reg %i: %s \n", YMM_no, registerString);
	}
}

void print_state_as_binary(__m256i *states, int state_no) {
	int stateoffset = 64 * state_no;

	printf("Printing state %d: \n", state_no);

	for (int ymm_no = 0; ymm_no < 5; ymm_no++) {

		unsigned char *YMM = _aligned_malloc(sizeof(char) * 32, 32); //Allocate 32byte, 32byte aligned
		_mm256_store_si256(YMM, states[state_no]);

		for (int byte_no = 0; byte_no < 7; byte_no++) { //7 bytes = 56 bit = one state
			unsigned char binary[8];
			byte_to_binary(binary, YMM[byte_no]);

			for (int bit = 0; bit < 8; bit++) {
				printf("%d", binary[bit]);
			}
			printf(" ");
		}

		printf("\n");

		_aligned_free(YMM);
	}
}

void byte_to_binary(unsigned char *binarystr, unsigned char byte) {

	binarystr[0] = byte & 1 ? 1 : 0;
	binarystr[1] = byte & 2 ? 1 : 0;
	binarystr[2] = byte & 4 ? 1 : 0;
	binarystr[3] = byte & 8 ? 1 : 0;

	binarystr[4] = byte & 16 ? 1 : 0;
	binarystr[5] = byte & 32 ? 1 : 0;
	binarystr[6] = byte & 64 ? 1 : 0;
	binarystr[7] = byte & 128 ? 1 : 0;
}