#include "Debug.h"

#define YMMCount 5
#define YMMLength 256


void print_keys_hex(const unsigned char k[4][keyLength]) {

	for (int strNo = 0; strNo < 4; strNo++) {
		const char *s = k[strNo];
		printf("Key %i : ", strNo);

		for (int byte = 0; byte < keyLength; byte++) {
			printf("%02x ", s[byte] & 31); //Only print 5 LSB, as we will be using those only
		}
		printf("\n");
	}
}

void print_nonces_hex(const unsigned char npub[4][NonceLength]) {

	for (int strNo = 0; strNo < 4; strNo++) {
		const char *s = npub[strNo];
		printf("Nonce %i : ", strNo);

		for (int byte = 0; byte < NonceLength; byte++) {
			printf("%02x ", s[byte] & 31 ); //Only print 5 LSB, as we will be using those only
		}
		printf("\n");
	}
}

void print_ad_hex(const unsigned char *ad[4], u64 adlen) {
	if (adlen == 0) {
		printf("No associated data with any current encryption \n");
		return;
	}

	for (int state = 0; state < 4; state++) {
		
		printf("Ass. Data %i : ", state);
		if (adlen == 0) {
			printf("No data.");
		}

		for (int primate_element = 0; primate_element < adlen; primate_element++) {
			//Draw a divisor for each 8 element to make it easier to read... and because the ratepart is 8 elements long, so its easy to see the complete data loaded each time.
			if (primate_element % 8 == 0 && primate_element != 0) {
				printf(" -- ");
			}
			printf("%02x ", ad[state][primate_element] & 31); //Only print 5 LSB, as we will be using those only
		}
		printf("\n");
	}
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

void print_state_as_binary(__m256i *states, int state_no) {
	int stateoffset = 8 * state_no; //offset between states in bytes

	printf("Printing state %d: \n", state_no);

	for (int ymm_no = 0; ymm_no < 5; ymm_no++) { //A state is spread out over all 5 YMM

		unsigned char *YMM = _aligned_malloc(sizeof(char) * 32, 32); //Allocate 32byte, 32byte aligned
		_mm256_store_si256(YMM, states[ymm_no]);

		for (int byte_no = 6; byte_no >= 0; byte_no--) { //7 bytes = 56 bit = one state
			unsigned char binary[8];
			byte_to_binary(binary, YMM[byte_no + stateoffset]);

			for (int bit = 7; bit >= 0; bit--) {
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