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
		for (int i = 0; i < 32; i++) {
			printf("%02x ", state[reg_no][0].m256i_u8[i]);
			if ((i + 1) % 8 == 0) {
				printf("\t");
			}
		}

		//Print second reg section
		for (int i = 0; i < 32; i++) {
			printf("%02x ", state[reg_no][1].m256i_u8[i]);
			if ((i + 1) % 8 == 0) {
				printf("\t");
			}
		}
		printf("\n");
	}
}