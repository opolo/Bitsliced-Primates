#include "Primates.h"


void main() {

	//constant length
	const unsigned char keys[4][keyLength] = { { 0x1 }, { 0x0 }, { 0x0, 0x0 }, {0x0} }; //max value 255 = 1 byte
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