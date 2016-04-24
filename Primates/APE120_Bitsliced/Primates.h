#include "Parameters.h"

#pragma once

//A decimal value with index 48-55 full of ones is: 71776119061217280

//Round constants for p1: 01, 02, 05, 0a, 15, 0b, 17, 0e, 1d, 1b, 16, 0c
//The second element of the second row is located at YMM bits: 47 + 64*n (where n<4) 
//A binary number with 1 at index 47 has the 
//decimal value: 70368744177664
//Hex value: 400000000000
//Aligned in YMMs, so they are XORed to the second element of the second row:

//R0: 01 = Binary 1
//R1: 02 = binary 10
//R2 05 = binary 101
//R3 0a = binary 1010
//R4 15 = binary 10101
//R5 0b = binary 1011
//R6 17 = binary 10111
//R7 0e = binary 1110
//R8 1d = binary 11101
//R9 1b = binary 11011
//R10 16 = binary 10110
//R11 0c = binary 1100


typedef unsigned long long u64;
