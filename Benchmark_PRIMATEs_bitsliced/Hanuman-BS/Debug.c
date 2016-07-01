#include "Debug.h"

void print_state_as_hex_with_label(u8 *label, YMM(*state)[2]) {
	printf("\n");
	printf("%s", label);
	print_state_as_hex(state);
}

void print_state_as_hex(YMM(*state)[2]) {

	//Print u64 index for help
	printf("\n");
	for (int i = 0; i < 8; i++) printf("i0 ");
	printf("\t");

	for (int i = 0; i < 8; i++) printf("i1 ");
	printf("\t");

	for (int i = 0; i < 8; i++) printf("i2 ");
	printf("\t");

	for (int i = 0; i < 8; i++) printf("i3 ");
	printf("\t");

	for (int i = 0; i < 8; i++) printf("i0 ");
	printf("\t");

	for (int i = 0; i < 8; i++) printf("i1 ");
	printf("\t");

	for (int i = 0; i < 8; i++) printf("i2 ");
	printf("\t");

	for (int i = 0; i < 8; i++) printf("i3 ");
	printf("\t");
	printf("\n");

	for (int reg_no = 0; reg_no < 5; reg_no++) {
		//Print first reg section 
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][0], 0));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][0], 1));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][0], 2));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][0], 3));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][0], 4));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][0], 5));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][0], 6));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][0], 7));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][0], 8));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][0], 9));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][0], 10));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][0], 11));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][0], 12));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][0], 13));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][0], 14));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][0], 15));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][0], 16));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][0], 17));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][0], 18));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][0], 19));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][0], 20));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][0], 21));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][0], 22));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][0], 23));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][0], 24));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][0], 25));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][0], 26));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][0], 27));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][0], 28));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][0], 29));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][0], 30));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][0], 31));

		printf("%02x \t", _mm256_extract_epi8(state[reg_no][1], 0));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][1], 1));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][1], 2));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][1], 3));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][1], 4));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][1], 5));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][1], 6));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][1], 7));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][1], 8));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][1], 9));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][1], 10));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][1], 11));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][1], 12));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][1], 13));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][1], 14));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][1], 15));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][1], 16));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][1], 17));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][1], 18));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][1], 19));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][1], 20));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][1], 21));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][1], 22));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][1], 23));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][1], 24));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][1], 25));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][1], 26));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][1], 27));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][1], 28));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][1], 29));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][1], 30));
		printf("%02x \t", _mm256_extract_epi8(state[reg_no][1], 31));
		printf("\n");
	}
}