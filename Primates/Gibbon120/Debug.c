#include "Debug.h"

void print_state_as_hex(YMM(*state)[2]) {

	//Print u64 index for help
	printf("\n");
	for (int i = 0; i < 8; i++) printf("i0 ");
	for (int i = 0; i < 8; i++) printf("i1 ");
	for (int i = 0; i < 8; i++) printf("i2 ");
	for (int i = 0; i < 8; i++) printf("i3 ");
	printf("\t");
	for (int i = 0; i < 8; i++) printf("i0 ");
	for (int i = 0; i < 8; i++) printf("i1 ");
	for (int i = 0; i < 8; i++) printf("i2 ");
	for (int i = 0; i < 8; i++) printf("i3 ");
	printf("\n");

	for (int reg_no = 0; reg_no < 5; reg_no++) {
		
		//Print first reg section
		for (int i = 0; i < 32; i++) {
			printf("%02x ", state[reg_no][0].m256i_u8[i]);
		}
		printf("\t");

		//Print second reg section
		for (int i = 0; i < 32; i++) {
			printf("%02x ", state[reg_no][1].m256i_u8[i]);
		}
		printf("\n");
	}
}