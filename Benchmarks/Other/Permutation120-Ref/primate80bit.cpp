#include <stdio.h>
#include "parameters.h"
#include "primate80.h"

unsigned char  rc_p1_80bit[p1rounds];
unsigned char  rc_p2_80bit[p2rounds];
unsigned char  rc_p3_80bit[p3rounds];
unsigned char  rc_p4_80bit[p4rounds];

unsigned char  sbox5_80bit[32] = { 1, 0, 25, 26, 17, 29, 21, 27, 20, 5, 4, 23, 14, 18, 2, 28, 15, 8, 6, 3, 13, 7, 24, 16, 30, 9, 31, 10, 22, 12, 11, 19 };
unsigned char  sbox5inv_80bit[32] = { 1, 0, 14, 19, 10, 9, 18, 21, 17, 25, 27, 30, 29, 20, 12, 16, 23, 4, 13, 31, 8, 6, 28, 11, 22, 2, 3, 7, 15, 5, 24, 26 };
int shiftconst_80bit[5] = { 0,1,2,4,7 };

/*
unsigned char Get5bits(const unsigned char *in, int i) // selects bits 5*i to 5*i+4 out of an array of bytes, bit 0 = MSB
{
	unsigned int x = in[(5 * i) / 8] << 8; // select byte containing bit 5*i, move to upper 8 bits of 16-bit word
	if ((5 * i) % 8 > (8 - 5)) x |= in[((5 * i) / 8) + 1]; // bit 5*i+4 is in another byte, add to lower 8 bits of 16-bit word

	x <<= (5 * i) % 8; // move bit 5*i to MSB of 16-bit word
	return (x & 0xFFFF) >> (16 - 5); // select 5 highest bits of 16-bit word
}

unsigned char Get8bits(const unsigned char *in, int i) // selects bits 8*i to 8*i+7 out of an array of 5-bit elements, bit 0 = MSB
{
	unsigned int x = in[(8 * i) / 5] << 10; // select element containing bit 8*i, move to upper 5 bits of 15-bit word
	x |= (((in[((8 * i) / 5) + 1]) & 0x3FF) << 5); // bit 8*i+7 is definitely in another element, add to middle 5 bits of 15-bit word
	if ((8 * i) % 5 > (10 - 8)) x |= ((in[((8 * i) / 5) + 2]) & 0x3FF); // bit 8*i+7 is in yet another element, add to lower 5 bits of 10-bit word
	x <<= (8 * i) % 5; // move bit 8*i to MSB of 15-bit word
	return (x & 0x7FFF) >> (15 - 8); // select 8 highest bits of 15-bit word
}


void Bytes2Element(unsigned char *inE, const unsigned char *in, unsigned long long start, int numElem) //Transforming a byte array to an element array of length numElem
{
	for (int i = 0; i<numElem; i++)
		inE[i] = Get5bits(in + start, i);
}

void Element2Bytes(unsigned char *in, unsigned char *inE, unsigned long long start, int numBytes) //Transforming an element array to a byte array of length numBytes
{
	for (int i = 0; i<numBytes; i++)
		in[i + start] = Get8bits(inE, i);
}
*/

void SubElements_80bit(unsigned char *state) //SubElements operation on the state
{
	for (int i = 0; i<StateSize; i++)
		state[i] = sbox5_80bit[state[i]];
}

void SubElementsInv_80bit(unsigned char *state) //Inverse SubElements operation on the state
{
	for (int i = 0; i<StateSize; i++)
		state[i] = sbox5inv_80bit[state[i]];
}

void ShiftRows_80bit(unsigned char *state) //ShiftRows operation on the state
{
	unsigned char tmp[StateSize];
	for (int j = 0; j<NumberRows; j++) {
		for (int i = 0; i<NumberColumns; i++) {
			tmp[NumberColumns*j + i] = state[NumberColumns*j + ((i + shiftconst_80bit[j]) % NumberColumns)];
		}
	}
	for (int j = 0; j<StateSize; j++)
		state[j] = tmp[j];
}


void ShiftRowsInv_80bit(unsigned char *state) //Inverse ShiftRows operation on the state
{
	unsigned char tmp[StateSize];
	for (int j = 0; j<NumberRows; j++) {
		for (int i = 0; i<NumberColumns; i++) {
			tmp[NumberColumns*j + i] = state[NumberColumns*j + ((i - shiftconst_80bit[j] + NumberColumns) % NumberColumns)];
		}
	}
	for (int j = 0; j<StateSize; j++)
		state[j] = tmp[j];
}

unsigned char T2_80bit(unsigned char element) //Field Multiplication by 2 of an element
{
	unsigned char tmp;
	tmp = ((element << 1) & 31) ^ (((element >> 4) & 1) ? 5 : 0);
	return tmp;
}

unsigned char T18_80bit(unsigned char element) //Field Multiplication by 18 of an element
{
	unsigned char tmp, t2, t4, t8, t16;
	t2 = T2_80bit(element);
	t4 = T2_80bit(t2);
	t8 = T2_80bit(t4);
	t16 = T2_80bit(t8);
	tmp = t16^t2;
	return tmp;
}

void MixColumns_80bit(unsigned char *state) //MixColumns operation on the state
{
	unsigned char tmp[StateSize];
	for (int p = 0; p<NumberRows; p++) {
		for (int i = 0; i<NumberColumns; i++) {
			for (int j = 0; j<(NumberRows - 1); j++)
				tmp[NumberColumns*j + i] = state[NumberColumns*(j + 1) + i];
			tmp[NumberColumns * 4 + i] = state[i] ^ T18_80bit(state[NumberColumns * 1 + i]) ^ T2_80bit(state[NumberColumns * 2 + i]) ^ T2_80bit(state[NumberColumns * 3 + i]) ^ T18_80bit(state[NumberColumns * 4 + i]);
		}
		for (int j = 0; j<StateSize; j++)
			state[j] = tmp[j];
	}
}

void MixColumnsInv_80bit(unsigned char *state) //Inverse MixColumns operation on the state
{
	unsigned char tmp[StateSize];
	for (int p = 0; p<NumberRows; p++) {
		for (int i = 0; i<NumberColumns; i++) {
			tmp[i] = T18_80bit(state[i]) ^ T2_80bit(state[NumberColumns * 1 + i]) ^ T2_80bit(state[NumberColumns * 2 + i]) ^ T18_80bit(state[NumberColumns * 3 + i]) ^ state[NumberColumns * 4 + i];
			for (int j = 1; j<NumberRows; j++)
				tmp[NumberColumns*j + i] = state[NumberColumns*(j - 1) + i];
		}
		for (int j = 0; j<StateSize; j++)
			state[j] = tmp[j];
	}
}

void primate_80bit(unsigned char *state) //One round Primate permutation without constant addition 
{

	//SubElements
	SubElements_80bit(state);
	//printf("\nState after SubElements:\n"); PrintElementsStateDec(state);

	//ShiftRows
	ShiftRows_80bit(state);
	//printf("\nState after ShiftRows:\n"); PrintElementsStateDec(state);

	//MixColumns
	MixColumns_80bit(state);
	//printf("\nState after Mixcolumn:\n"); PrintElementsStateDec(state);
}

void primateInv_80bit(unsigned char *state) //One round Primate permutation without constant addition 
{
	//MixColumnsInv
	MixColumnsInv_80bit(state);
	//printf("\nState after MixcolumnInv:\n"); PrintElementsStateDec(state);

	//SubElementsInv
	SubElementsInv_80bit(state);
	//printf("\nState after SubElementsInv:\n"); PrintElementsStateDec(state);

	//ShiftRowsInv
	ShiftRowsInv_80bit(state);
	//printf("\nState after ShiftRowsInv:\n"); PrintElementsStateDec(state);
}

void GenerateRoundConstants_80bit() //Generation of 5-bit round constant by using the primitive polynomial x^5+x^2+1 with initial value 1.
{
	//Round constants for p_1
	rc_p1_80bit[0] = 1;
	//printf("\nRound constants for p_1: \n %.2x", rc_p1[0]);
	for (int i = 1; i<p1rounds; i++) {
		rc_p1_80bit[i] = ((rc_p1_80bit[i - 1] << 1) ^ ((rc_p1_80bit[i - 1] >> 1) & 1) ^ ((rc_p1_80bit[i - 1] >> 4) & 1)) & 31;
		//printf("\n %.2x", rc_p1[i]);
	}

	//Round constant for p_2
	rc_p2_80bit[0] = ((rc_p1_80bit[p1rounds - 1] << 1) ^ ((rc_p1_80bit[p1rounds - 1] >> 1) & 1) ^ ((rc_p1_80bit[p1rounds - 1] >> 4) & 1)) & 31;
	//printf("\nRound constants for p_2: \n %.2x", rc_p2[0]);
	for (int i = 1; i<p2rounds; i++) {
		rc_p2_80bit[i] = ((rc_p2_80bit[i - 1] << 1) ^ ((rc_p2_80bit[i - 1] >> 1) & 1) ^ ((rc_p2_80bit[i - 1] >> 4) & 1)) & 31;
		//printf("\n %.2x", rc_p2[i]);
	}

	//Round constant for p_3
	rc_p3_80bit[0] = ((rc_p2_80bit[p2rounds - 1] << 1) ^ ((rc_p2_80bit[p2rounds - 1] >> 1) & 1) ^ ((rc_p2_80bit[p2rounds - 1] >> 4) & 1)) & 31;
	//printf("\nRound constants for p_3: \n %.2x", rc_p3[0]);
	for (int i = 1; i<p3rounds; i++) {
		rc_p3_80bit[i] = ((rc_p3_80bit[i - 1] << 1) ^ ((rc_p3_80bit[i - 1] >> 1) & 1) ^ ((rc_p3_80bit[i - 1] >> 4) & 1)) & 31;
		//printf("\n %.2x", rc_p3[i]);
	}

	//Round constant for p_4
	rc_p4_80bit[0] = ((rc_p1_80bit[p1rounds - 1] << 1) ^ ((rc_p1_80bit[p1rounds - 1] >> 1) & 1) ^ ((rc_p1_80bit[p1rounds - 1] >> 4) & 1)) & 31;
	//printf("\nRound constants for p_4: \n %.2x", rc_p4[0]);
	for (int i = 1; i<p4rounds; i++) {
		rc_p4_80bit[i] = ((rc_p4_80bit[i - 1] << 1) ^ ((rc_p4_80bit[i - 1] >> 1) & 1) ^ ((rc_p4_80bit[i - 1] >> 4) & 1)) & 31;
		//printf("\n %.2x", rc_p4[i]);
	}
}

void InitializeState_80bit(unsigned char *state, unsigned char *key)
{
	GenerateRoundConstants();

	for (int i = 0; i<RateSizeElements; i++)
		state[i] = 0;
	for (int i = 0; i<KeySizeElements; i++)
		state[RateSizeElements + i] = key[i];
}

void p_1_80bit(unsigned char *state)
{
	for (int rnd = 0; rnd<p1rounds; rnd++)
	{
		primate_80bit(state);
		state[RateSizeElements + 1] ^= rc_p1_80bit[rnd];
		//printf("\nState after AddConstant:\n\n\n"); PrintElementsStateDec(state);
	}
}

void p_2_80bit(unsigned char *state)
{
	for (int rnd = 0; rnd<p2rounds; rnd++)
	{
		primate_80bit(state);
		state[RateSizeElements + 1] ^= rc_p2_80bit[rnd];
	}
}

void p_3_80bit(unsigned char *state)
{
	for (int rnd = 0; rnd<p3rounds; rnd++)
	{
		primate_80bit(state);
		state[RateSizeElements + 1] ^= rc_p3_80bit[rnd];
	}
}

void p_4_80bit(unsigned char *state)
{
	for (int rnd = 0; rnd<p4rounds; rnd++)
	{
		primate_80bit(state);
		state[RateSizeElements + 1] ^= rc_p4_80bit[rnd];
	}
}


void p_1_inv_80bit(unsigned char *state)
{
	for (int rnd = 0; rnd<p1rounds; rnd++)
	{
		state[RateSizeElements + 1] ^= rc_p1_80bit[p1rounds - rnd - 1];
		//printf("\nState after AddConstant:\n\n\n"); PrintElementsStateDec(state);
		primateInv_80bit(state);
	}
}

void p_2_inv_80bit(unsigned char *state)
{
	for (int rnd = 0; rnd<p2rounds; rnd++)
	{
		state[RateSizeElements + 1] ^= rc_p2_80bit[p2rounds - rnd - 1];
		//printf("\nState after AddConstant:\n\n\n"); PrintElementsStateDec(state);
		primateInv_80bit(state);
	}
}

void p_3_inv_80bit(unsigned char *state)
{
	for (int rnd = 0; rnd<p3rounds; rnd++)
	{
		state[RateSizeElements + 1] ^= rc_p3_80bit[p3rounds - rnd - 1];
		//printf("\nState after AddConstant:\n\n\n"); PrintElementsStateDec(state);
		primateInv_80bit(state);
	}
}

void p_4_inv_80bit(unsigned char *state)
{
	for (int rnd = 0; rnd<p4rounds; rnd++)
	{
		state[RateSizeElements + 1] ^= rc_p4_80bit[p4rounds - rnd - 1];
		//printf("\nState after AddConstant:\n\n\n"); PrintElementsStateDec(state);
		primateInv_80bit(state);
	}
}
