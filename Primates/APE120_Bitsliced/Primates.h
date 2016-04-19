#pragma once

#define PrimateRounds 12
#define keyLength 30 //Bytes = 240 bits
#define NonceLength 15 //Bytes
#define RateSize 5 //Bytes

#define PrimateRounds 12 //p1

//Round constants for p1: 01, 02, 05, 0a, 15, 0b, 17, 0e, 1d, 1b, 16, 0c
//The second element of the second row is located at YMM bits: 47 + 64*n (where n<4) 
//A binary number with 1 at index 47 has the 
//decimal value: 70368744177664
//Hex value: 400000000000
//Aligned in YMMs, so they are XORed to the second element of the second row:

//R0: 01 = Binary 1
#define R0YMM0 70368744177664
#define R0YMM1 0
#define R0YMM2 0
#define R0YMM3 0
#define R0YMM4 0

//R1: 02 = binary 10
#define R1YMM0 0
#define R1YMM1 70368744177664
#define R1YMM2 0
#define R1YMM3 0
#define R1YMM4 0

//R2 05 = binary 101
#define R2YMM0 70368744177664
#define R2YMM1 0
#define R2YMM2 70368744177664
#define R2YMM3 0
#define R2YMM4 0

//R3 0a = binary 1010
#define R3YMM0 0
#define R3YMM1 70368744177664
#define R3YMM2 0
#define R3YMM3 70368744177664
#define R3YMM4 0

//R4 15 = binary 10101
#define R4YMM0 70368744177664
#define R4YMM1 0
#define R4YMM2 70368744177664
#define R4YMM3 0
#define R4YMM4 70368744177664

//R5 0b = binary 1011
#define R4YMM0 70368744177664
#define R4YMM1 70368744177664
#define R4YMM2 0
#define R4YMM3 70368744177664
#define R4YMM4 0

//R6 17 = binary 10111
#define R4YMM0 70368744177664
#define R4YMM1 70368744177664
#define R4YMM2 70368744177664
#define R4YMM3 0
#define R4YMM4 70368744177664

//R7 0e = binary 1110
#define R4YMM0 0
#define R4YMM1 70368744177664
#define R4YMM2 70368744177664
#define R4YMM3 70368744177664
#define R4YMM4 0

//R8 1d = binary 11101
#define R4YMM0 70368744177664
#define R4YMM1 0
#define R4YMM2 70368744177664
#define R4YMM3 70368744177664
#define R4YMM4 70368744177664

//R9 1b = binary 11011
#define R4YMM0 70368744177664
#define R4YMM1 70368744177664
#define R4YMM2 0
#define R4YMM3 70368744177664
#define R4YMM4 70368744177664

//R10 16 = binary 10110
#define R4YMM0 0
#define R4YMM1 70368744177664
#define R4YMM2 70368744177664
#define R4YMM3 0
#define R4YMM4 70368744177664

//R11 0c = binary 1100
#define R4YMM0 0
#define R4YMM1 0
#define R4YMM2 70368744177664
#define R4YMM3 70368744177664
#define R4YMM4 0

typedef unsigned long long u64;

void primates120_encrypt(const unsigned char *k[4],
	const unsigned char *m[4], u64 mlen[4],
	const unsigned char *ad[4], u64 adlen[4],
	const unsigned char *npub[4]);