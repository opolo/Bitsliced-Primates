#include "Primates.h"

//One row = 40 bits. 
//First row is rate. Key is only added afterwards (i.e. as the capacity), which begins at row 1, that should be all 1, then all 0, then all 1, etc.
#define rowNumberedKey {0xFF}, {0xFF}, {0xFF}, {0xFF}, {0xFF}, {0x0}, {0x0}, {0x0}, {0x0}, {0x0}, {0xFF}, {0xFF}, {0xFF}, {0xFF}, {0xFF}, {0x0}, {0x0}, {0x0}, {0x0}, {0x0}, {0xFF}, {0xFF}, {0xFF}, {0xFF}, {0xFF}, {0xFF}, {0x0}, {0x0}, {0x0}, {0x00}

void main() {

	//constant length
	const unsigned char keys[4][keyLength] = { { rowNumberedKey }, {rowNumberedKey}, {rowNumberedKey}, {rowNumberedKey} };
	const unsigned char nonces[4][NonceLength] = { {0x0}, { 0x0 }, { 0x0 }, { 0x0 , 0x1 } };
	
	//variable length
	unsigned char *msg[4]; 
	msg[0] = "My"; msg[1] = "name";
	msg[2] = "is"; msg[3] = "Jonas";
	u64 mLengths[4] = { { 2 },{ 4 },{ 2 },{ 5 } };

	const unsigned char *ad[4];
	ad[0] = "0";
	ad[1] = "";
	ad[2] = "12";
	ad[3] = "";
	u64 adLengths[4] = { { 1 },{ 0 },{ 2 },{ 0 } };
	

	primates120_encrypt(keys, msg, mLengths, ad, adLengths, nonces);
}