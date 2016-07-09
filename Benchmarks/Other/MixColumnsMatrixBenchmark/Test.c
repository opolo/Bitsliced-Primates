#include <immintrin.h>
#include <stdlib.h>
#include <Windows.h>
#include <math.h>
#include <stdio.h>

typedef __m256i YMM;
typedef unsigned long long u64;

//Bit-wise operations on AVX registers
#define XOR(a, b) _mm256_xor_si256(a, b)
#define NEG(a) _mm256_xor_si256(m256iAllOne, a)
#define OR(a, b) _mm256_or_si256(a, b)
#define XOR3(a, b, c) _mm256_xor_si256(a, _mm256_xor_si256(b, c))
#define XOR4(a, b, c, d) _mm256_xor_si256(a, _mm256_xor_si256(b, _mm256_xor_si256(c, d)))
#define XOR5(a, b, c, d, e) _mm256_xor_si256(a, _mm256_xor_si256(b, _mm256_xor_si256(c, _mm256_xor_si256(d, e))))
#define XOR6(a, b, c, d, e, f) _mm256_xor_si256(a, _mm256_xor_si256(b, _mm256_xor_si256(c, _mm256_xor_si256(d, _mm256_xor_si256(e, f)))))
#define XOR7(a, b, c, d, e, f, g) _mm256_xor_si256(a, _mm256_xor_si256(b, _mm256_xor_si256(c, _mm256_xor_si256(d, _mm256_xor_si256(e, _mm256_xor_si256(f, g))))))
#define XOR8(a, b, c, d, e, f, g, h) _mm256_xor_si256(a, _mm256_xor_si256(b, _mm256_xor_si256(c, _mm256_xor_si256(d, _mm256_xor_si256(e, _mm256_xor_si256(f, _mm256_xor_si256(g, h)))))))
#define XOR9(a, b, c, d, e, f, g, h, i) _mm256_xor_si256(a, _mm256_xor_si256(b, _mm256_xor_si256(c, _mm256_xor_si256(d, _mm256_xor_si256(e, _mm256_xor_si256(f, _mm256_xor_si256(g, _mm256_xor_si256(h, i))))))))
#define XOR10(a, b, c, d, e, f, g, h, i, j) _mm256_xor_si256(a, _mm256_xor_si256(b, _mm256_xor_si256(c, _mm256_xor_si256(d, _mm256_xor_si256(e, _mm256_xor_si256(f, _mm256_xor_si256(g, _mm256_xor_si256(h, _mm256_xor_si256(i, j)))))))))

//For the MSVC compiler, since it do not support extract/insert intrinsics.
#if (_MSC_VER)
#define _mm256_extract_epi64(a, b) a.m256i_u64[b]
#define _mm256_extract_epi8(a, b) a.m256i_u8[b]
#define __mm256_insert_epi64(a, value, index) a.m256i_u64[index] = value
#define __mm256_insert_epi8(a, value, index) a.m256i_u8[index] = value
#else
#define __mm256_insert_epi64(a, value, index) a = _mm256_insert_epi64(a, value, index)
#define __mm256_insert_epi8(a, value, index) a = _mm256_insert_epi8(a, value, index)
#endif

void T2(__m256i (*state)[2], __m256i (*new_state)[2]);
int cmpfunc(const void * a, const void * b);

void mixcolumns_A7(__m256i (*state)[2]);
void mixcolumns_A6(__m256i (*state)[2]);
void mixcolumns_A5(__m256i (*state)[2]);
void mixcolumns_A4(__m256i (*state)[2]);
void mixcolumns_A3(__m256i (*state)[2]);
void mixcolumns_A2(__m256i (*state)[2]);
void mixcolumns_A1(__m256i (*state)[2]);

void optimized_mixcolumns_A2(__m256i (*state)[2]);
void optimized_mixcolumns_A3(__m256i (*state)[2]);
void optimized_mixcolumns_A4(__m256i (*state)[2]);
void optimized_mixcolumns_A5(__m256i (*state)[2]);
void optimized_mixcolumns_A6(__m256i (*state)[2]);
void optimized_mixcolumns_A7(__m256i (*state)[2]);

void mixcolumns_A1_80bit(__m256i (*state)[2]);
void mixcolumns_A2_80bit(__m256i (*state)[2]);
void mixcolumns_A3_80bit(__m256i (*state)[2]);
void mixcolumns_A4_80bit(__m256i (*state)[2]);
void mixcolumns_A5_80bit(__m256i (*state)[2]);

void optimized_mixcolumns_A2_80bit(__m256i (*state)[2]);
void optimized_mixcolumns_A3_80bit(__m256i (*state)[2]);
void optimized_mixcolumns_A4_80bit(__m256i (*state)[2]);
void optimized_mixcolumns_A5_80bit(__m256i (*state)[2]);

int main() {
 
	u64 start, finish, cpu_frequency;
	YMM test_state[5][2];

	//Run only on one core
	SetThreadAffinityMask(GetCurrentThread(), 0x00000008); //Run on fourth core

	start = __rdtsc();
	Sleep(5000);
	finish = __rdtsc();
	cpu_frequency = (finish - start) / 5;

	printf("CPU frequency: %llu \n", cpu_frequency);

	for (int i = 0; i < 5; i++) {
		test_state[i][0] = _mm256_set1_epi64x(1);
		test_state[i][1] = _mm256_set1_epi64x(1);
	}

	int iterations = 100000;
	int it_per_it = 200;

	u64 *results_A1_80bit = calloc(iterations, sizeof(u64));
	u64 *results_A2_80bit = calloc(iterations, sizeof(u64));
	u64 *results_A3_80bit = calloc(iterations, sizeof(u64));
	u64 *results_A4_80bit = calloc(iterations, sizeof(u64));
	u64 *results_A5_80bit = calloc(iterations, sizeof(u64));
	u64 *optimized_results_A2_80bit = calloc(iterations, sizeof(u64));
	u64 *optimized_results_A3_80bit = calloc(iterations, sizeof(u64));
	u64 *optimized_results_A4_80bit = calloc(iterations, sizeof(u64));
	u64 *optimized_results_A5_80bit = calloc(iterations, sizeof(u64));

	u64 *results_A1 = calloc(iterations, sizeof(u64));
	u64 *results_A2 = calloc(iterations, sizeof(u64));
	u64 *results_A3 = calloc(iterations, sizeof(u64));
	u64 *results_A4 = calloc(iterations, sizeof(u64));
	u64 *results_A5 = calloc(iterations, sizeof(u64));
	u64 *results_A6 = calloc(iterations, sizeof(u64));
	u64 *results_A7 = calloc(iterations, sizeof(u64));
	u64 *optimized_results_A2 = calloc(iterations, sizeof(u64));
	u64 *optimized_results_A3 = calloc(iterations, sizeof(u64));
	u64 *optimized_results_A4 = calloc(iterations, sizeof(u64));
	u64 *optimized_results_A5 = calloc(iterations, sizeof(u64));
	u64 *optimized_results_A6 = calloc(iterations, sizeof(u64));
	u64 *optimized_results_A7 = calloc(iterations, sizeof(u64));
	
	for (int i = 0; i < iterations; i++) {
		if (i % (iterations / 10) == 0) {
			//One tenth through...
			printf("Progress: %i/10 through. \n", (i / (iterations / 10) + 1));
		}

		//A1-80bit
		start = __rdtsc();
		for (int j = 0; j < it_per_it; j++)
			mixcolumns_A1_80bit(test_state);
		finish = __rdtsc();
		results_A1_80bit[i] = finish - start;

		//A2-80bit
		start = __rdtsc();
		for (int j = 0; j < it_per_it; j++)
			mixcolumns_A2_80bit(test_state);
		finish = __rdtsc();
		results_A2_80bit[i] = finish - start;

		//A3-80bit
		start = __rdtsc();
		for (int j = 0; j < it_per_it; j++)
			mixcolumns_A3_80bit(test_state);
		finish = __rdtsc();
		results_A3_80bit[i] = finish - start;

		//A4-80bit
		start = __rdtsc();
		for (int j = 0; j < it_per_it; j++)
			mixcolumns_A4_80bit(test_state);
		finish = __rdtsc();
		results_A4_80bit[i] = finish - start;

		//A5-80bit
		start = __rdtsc();
		for (int j = 0; j < it_per_it; j++)
			mixcolumns_A5_80bit(test_state);
		finish = __rdtsc();
		results_A5_80bit[i] = finish - start;


		//A1
		start = __rdtsc();
		for (int j = 0; j < it_per_it; j++)
			mixcolumns_A1(test_state);
		finish = __rdtsc();
		results_A1[i] = finish - start;

		//A2
		start = __rdtsc();
		for (int j = 0; j < it_per_it; j++)
			mixcolumns_A2(test_state);
		finish = __rdtsc();
		results_A2[i] = finish - start;

		//A3
		start = __rdtsc();
		for (int j = 0; j < it_per_it; j++)
			mixcolumns_A3(test_state);
		finish = __rdtsc();
		results_A3[i] = finish - start;

		//A4
		start = __rdtsc();
		for (int j = 0; j < it_per_it; j++)
			mixcolumns_A4(test_state);
		finish = __rdtsc();
		results_A4[i] = finish - start;

		//A5
		start = __rdtsc();
		for (int j = 0; j < it_per_it; j++)
			mixcolumns_A5(test_state);
		finish = __rdtsc();
		results_A5[i] = finish - start;

		//A6
		start = __rdtsc();
		for (int j = 0; j < it_per_it; j++)
			mixcolumns_A6(test_state);
		finish = __rdtsc();
		results_A6[i] = finish - start;

		//A7
		start = __rdtsc();
		for (int j = 0; j < it_per_it; j++)
			mixcolumns_A7(test_state);
		finish = __rdtsc();
		results_A7[i] = finish - start;


		//Opti_A2-80bit
		start = __rdtsc();
		for (int j = 0; j < it_per_it; j++)
			optimized_mixcolumns_A2_80bit(test_state);
		finish = __rdtsc();
		optimized_results_A2_80bit[i] = finish - start;

		//Opti_A3-80bit
		start = __rdtsc();
		for (int j = 0; j < it_per_it; j++)
			optimized_mixcolumns_A3_80bit(test_state);
		finish = __rdtsc();
		optimized_results_A3_80bit[i] = finish - start;

		//Opti_A4-80bit
		start = __rdtsc();
		for (int j = 0; j < it_per_it; j++)
			optimized_mixcolumns_A4_80bit(test_state);
		finish = __rdtsc();
		optimized_results_A4_80bit[i] = finish - start;

		//Opti_A5-80bit
		start = __rdtsc();
		for (int j = 0; j < it_per_it; j++)
			optimized_mixcolumns_A5_80bit(test_state);
		finish = __rdtsc();
		optimized_results_A5_80bit[i] = finish - start;


		//Opti_A2
		start = __rdtsc();
		for (int j = 0; j < it_per_it; j++)
			optimized_mixcolumns_A2(test_state);
		finish = __rdtsc();
		optimized_results_A2[i] = finish - start;

		//Opti_A3
		start = __rdtsc();
		for (int j = 0; j < it_per_it; j++)
			optimized_mixcolumns_A3(test_state);
		finish = __rdtsc();
		optimized_results_A3[i] = finish - start;

		//Opti_A4
		start = __rdtsc();
		for (int j = 0; j < it_per_it; j++)
			optimized_mixcolumns_A4(test_state);
		finish = __rdtsc();
		optimized_results_A4[i] = finish - start;

		//Opti_A5
		start = __rdtsc();
		for (int j = 0; j < it_per_it; j++)
			optimized_mixcolumns_A5(test_state);
		finish = __rdtsc();
		optimized_results_A5[i] = finish - start;

		//Opti_A6
		start = __rdtsc();
		for (int j = 0; j < it_per_it; j++)
			optimized_mixcolumns_A6(test_state);
		finish = __rdtsc();
		optimized_results_A6[i] = finish - start;

		//Opti_A7
		start = __rdtsc();
		for (int j = 0; j < it_per_it; j++)
			optimized_mixcolumns_A7(test_state);
		finish = __rdtsc();
		optimized_results_A7[i] = finish - start;
	}

	//Sort
	qsort(results_A1_80bit, iterations, sizeof(u64), cmpfunc);
	qsort(results_A2_80bit, iterations, sizeof(u64), cmpfunc);
	qsort(results_A3_80bit, iterations, sizeof(u64), cmpfunc);
	qsort(results_A4_80bit, iterations, sizeof(u64), cmpfunc);
	qsort(results_A5_80bit, iterations, sizeof(u64), cmpfunc);
	qsort(results_A1, iterations, sizeof(u64), cmpfunc);
	qsort(results_A2, iterations, sizeof(u64), cmpfunc);
	qsort(results_A3, iterations, sizeof(u64), cmpfunc);
	qsort(results_A4, iterations, sizeof(u64), cmpfunc);
	qsort(results_A5, iterations, sizeof(u64), cmpfunc);
	qsort(results_A6, iterations, sizeof(u64), cmpfunc);
	qsort(results_A7, iterations, sizeof(u64), cmpfunc);
	qsort(optimized_results_A2_80bit, iterations, sizeof(u64), cmpfunc);
	qsort(optimized_results_A3_80bit, iterations, sizeof(u64), cmpfunc);
	qsort(optimized_results_A4_80bit, iterations, sizeof(u64), cmpfunc);
	qsort(optimized_results_A5_80bit, iterations, sizeof(u64), cmpfunc);
	qsort(optimized_results_A2, iterations, sizeof(u64), cmpfunc);
	qsort(optimized_results_A3, iterations, sizeof(u64), cmpfunc);
	qsort(optimized_results_A4, iterations, sizeof(u64), cmpfunc);
	qsort(optimized_results_A5, iterations, sizeof(u64), cmpfunc);
	qsort(optimized_results_A6, iterations, sizeof(u64), cmpfunc);
	qsort(optimized_results_A7, iterations, sizeof(u64), cmpfunc);

	u64 A1_80bit_median = results_A1_80bit[iterations / 2];
	u64 A2_80bit_median = results_A2_80bit[iterations / 2];
	u64 A3_80bit_median = results_A3_80bit[iterations / 2];
	u64 A4_80bit_median = results_A4_80bit[iterations / 2];
	u64 A5_80bit_median = results_A5_80bit[iterations / 2];
	u64 opti_A2_80bit_median = optimized_results_A2_80bit[iterations / 2];
	u64 opti_A3_80bit_median = optimized_results_A3_80bit[iterations / 2];
	u64 opti_A4_80bit_median = optimized_results_A4_80bit[iterations / 2];
	u64 opti_A5_80bit_median = optimized_results_A5_80bit[iterations / 2];
	u64 A1_median = results_A1[iterations/2];
	u64 A2_median = results_A2[iterations / 2];
	u64 A3_median = results_A3[iterations / 2];
	u64 A4_median = results_A4[iterations / 2];
	u64 A5_median = results_A5[iterations / 2];
	u64 A6_median = results_A6[iterations / 2];
	u64 A7_median = results_A7[iterations / 2];
	u64 opti_A2_median = optimized_results_A2[iterations / 2];
	u64 opti_A3_median = optimized_results_A3[iterations / 2];
	u64 opti_A4_median = optimized_results_A4[iterations / 2];
	u64 opti_A5_median = optimized_results_A5[iterations / 2];
	u64 opti_A6_median = optimized_results_A6[iterations / 2];
	u64 opti_A7_median = optimized_results_A7[iterations / 2];

	printf("\n");
	printf("A1-80bit median: \n");
	printf("%llu \n", A1_80bit_median);
	printf("A2-80bit median: \n");
	printf("%llu \n", A2_80bit_median);
	printf("A3-80bit median: \n");
	printf("%llu \n", A3_80bit_median);
	printf("A4-80bit median: \n");
	printf("%llu \n", A4_80bit_median);
	printf("A5-80bit median: \n");
	printf("%llu \n", A5_80bit_median);
	printf("Opti_A2-80bit median: \n");
	printf("%llu \n", opti_A2_80bit_median);
	printf("Opti_A3-80bit median: \n");
	printf("%llu \n", opti_A3_80bit_median);
	printf("Opti_A4-80bit median: \n");
	printf("%llu \n", opti_A4_80bit_median);
	printf("Opti_A5-80bit median: \n");
	printf("%llu \n", opti_A5_80bit_median);

	printf("A1 median: \n");
	printf("%llu \n", A1_median);
	printf("A2 median: \n");
	printf("%llu \n", A2_median);
	printf("A3 median: \n");
	printf("%llu \n", A3_median);
	printf("A4 median: \n");
	printf("%llu \n", A4_median);
	printf("A5 median: \n");
	printf("%llu \n", A5_median);
	printf("A6 median: \n");
	printf("%llu \n", A6_median);
	printf("A7 median: \n");
	printf("%llu \n", A7_median);
	printf("Opti_A2 median: \n");
	printf("%llu \n", opti_A2_median);
	printf("Opti_A3 median: \n");
	printf("%llu \n", opti_A3_median);
	printf("Opti_A4 median: \n");
	printf("%llu \n", opti_A4_median);
	printf("Opti_A5 median: \n");
	printf("%llu \n", opti_A5_median);
	printf("Opti_A6 median: \n");
	printf("%llu \n", opti_A6_median);
	printf("Opti_A7 median: \n");
	printf("%llu \n", opti_A7_median);


	printf("\n");
	printf("A1-80bit index: \n");
	printf("%f \n", (A1_80bit_median / (double)A1_80bit_median) * 100);
	printf("A2-80bit index: \n");
	printf("%f \n", (A2_80bit_median / (double)A1_80bit_median) * 100);
	printf("A3-80bit index: \n");
	printf("%f \n", (A3_80bit_median / (double)A1_80bit_median) * 100);
	printf("A4-80bit index: \n");
	printf("%f \n", (A4_80bit_median / (double)A1_80bit_median) * 100);
	printf("A5-80bit index: \n");
	printf("%f \n", (A5_80bit_median / (double)A1_80bit_median) * 100);
	printf("Opti_A2-80bit index: \n");
	printf("%f \n", (opti_A2_80bit_median / (double)A1_80bit_median) * 100);
	printf("Opti_A3-80bit index: \n");
	printf("%f \n", (opti_A3_80bit_median / (double)A1_80bit_median) * 100);
	printf("Opti_A4-80bit index: \n");
	printf("%f \n", (opti_A4_80bit_median / (double)A1_80bit_median) * 100);
	printf("Opti_A5-80bit index: \n");
	printf("%f \n", (opti_A5_80bit_median / (double)A1_80bit_median) * 100);

	printf("A1 index: \n");
	printf("%f \n", (A1_median / (double)A1_80bit_median) * 100);
	printf("A2 index: \n");
	printf("%f \n", (A2_median / (double)A1_80bit_median) * 100);
	printf("A3 index: \n");
	printf("%f \n", (A3_median / (double)A1_80bit_median) * 100);
	printf("A4 index: \n");
	printf("%f \n", (A4_median / (double)A1_80bit_median) * 100);
	printf("A5 index: \n");
	printf("%f \n", (A5_median / (double)A1_80bit_median) * 100);
	printf("A6 index: \n");
	printf("%f \n", (A6_median / (double)A1_80bit_median) * 100);
	printf("A7 index: \n");
	printf("%f \n", (A7_median / (double)A1_80bit_median) * 100);
	printf("Opti_A2 index: \n");
	printf("%f \n", (opti_A2_median / (double)A1_80bit_median) * 100);
	printf("Opti_A3 index: \n");
	printf("%f \n", (opti_A3_median / (double)A1_80bit_median) * 100);
	printf("Opti_A4 index: \n");
	printf("%f \n", (opti_A4_median / (double)A1_80bit_median) * 100);
	printf("Opti_A5 index: \n");
	printf("%f \n", (opti_A5_median / (double)A1_80bit_median) * 100);
	printf("Opti_A6 index: \n");
	printf("%f \n", (opti_A6_median / (double)A1_80bit_median) * 100);
	printf("Opti_A7 index: \n");
	printf("%f \n", (opti_A7_median / (double)A1_80bit_median) * 100);

	getchar();

	free(results_A1);
	free(results_A2);
	free(results_A3);
	free(results_A4);
	free(results_A5);
	free(results_A6);
	free(results_A7);
	free(results_A1_80bit);
	free(results_A2_80bit);
	free(results_A3_80bit);
	free(results_A4_80bit);
	free(results_A5_80bit);
	free(optimized_results_A4);
	free(optimized_results_A5);
	free(optimized_results_A6);
	free(optimized_results_A7);

	return 0;
}

int cmpfunc(const void * a, const void * b)
{
	return (int) (*(u64*)a - *(u64*)b);
}




void T2(__m256i (*state)[2], __m256i (*new_state)[2]) {
	//Shift the bits in each primate element one to the left (i.e. shift bits between regs). and reduce with GF(2^5) mask x^5+x^2+1.
	new_state[0][0] = state[4][0];
	new_state[0][1] = state[4][1];

	new_state[1][0] = state[0][0];
	new_state[1][1] = state[0][1];

	new_state[2][0] = XOR(state[1][0], state[4][0]);
	new_state[2][1] = XOR(state[1][1], state[4][1]);

	new_state[3][0] = state[2][0];
	new_state[3][1] = state[2][1];

	new_state[4][0] = state[3][0];
	new_state[4][1] = state[3][1];
}


//************** 120 bit **************
void mixcolumns_A7(__m256i (*state)[2]) {
	/*
	A7:
	1       2       15      9       9       15      2
	2       5       28      29      27      23      11
	11      20      3       5       4       29      1
	1       9       27      10      12      11      31
	31      26      29      7       22      24      16
	16      26      17      25      3       29      29
	29      15      16      31      23      9       2
	*/

	__m256i T2_regs[5][2];
	__m256i T3_regs[5][2];
	__m256i T4_regs[5][2];
	__m256i T5_regs[5][2];
	__m256i T7_regs[5][2];
	__m256i T8_regs[5][2];
	__m256i T9_regs[5][2];
	__m256i T10_regs[5][2];
	__m256i T11_regs[5][2];
	__m256i T12_regs[5][2];
	__m256i T15_regs[5][2];
	__m256i T16_regs[5][2];
	__m256i T17_regs[5][2];
	__m256i T20_regs[5][2];
	__m256i T22_regs[5][2];
	__m256i T23_regs[5][2];
	__m256i T24_regs[5][2];
	__m256i T25_regs[5][2];
	__m256i T26_regs[5][2];
	__m256i T27_regs[5][2];
	__m256i T28_regs[5][2];
	__m256i T29_regs[5][2];
	__m256i T31_regs[5][2];


	T2(state, T2_regs); //T2
	T2(T2_regs, T4_regs); //T4
	T2(T4_regs, T8_regs); //T8
	T2(T8_regs, T16_regs); //T16

						   //T03
	for (int i = 0; i < 5; i++) {
		T3_regs[i][0] = XOR(state[i][0], T2_regs[i][0]);
		T3_regs[i][1] = XOR(state[i][1], T2_regs[i][1]);

		T5_regs[i][0] = XOR(state[i][0], T4_regs[i][0]);
		T5_regs[i][1] = XOR(state[i][1], T4_regs[i][1]);

		T7_regs[i][0] = XOR(T4_regs[i][0], T3_regs[i][0]);
		T7_regs[i][1] = XOR(T4_regs[i][1], T3_regs[i][1]);

		T9_regs[i][0] = XOR(state[i][0], T8_regs[i][0]);
		T9_regs[i][1] = XOR(state[i][1], T8_regs[i][1]);

		T10_regs[i][0] = XOR(T8_regs[i][0], T2_regs[i][0]);
		T10_regs[i][1] = XOR(T8_regs[i][1], T2_regs[i][1]);

		T11_regs[i][0] = XOR(T8_regs[i][0], T3_regs[i][0]);
		T11_regs[i][1] = XOR(T8_regs[i][1], T3_regs[i][1]);

		T12_regs[i][0] = XOR(T4_regs[i][0], T8_regs[i][0]);
		T12_regs[i][1] = XOR(T4_regs[i][1], T8_regs[i][1]);

		T15_regs[i][0] = XOR(T3_regs[i][0], T12_regs[i][0]);
		T15_regs[i][1] = XOR(T3_regs[i][1], T12_regs[i][1]);

		T17_regs[i][0] = XOR(state[i][0], T16_regs[i][0]);
		T17_regs[i][1] = XOR(state[i][1], T16_regs[i][1]);

		T20_regs[i][0] = XOR(T16_regs[i][0], T4_regs[i][0]);
		T20_regs[i][1] = XOR(T16_regs[i][1], T4_regs[i][1]);

		T22_regs[i][0] = XOR(T20_regs[i][0], T2_regs[i][0]);
		T22_regs[i][1] = XOR(T20_regs[i][1], T2_regs[i][1]);

		T23_regs[i][0] = XOR(T16_regs[i][0], T7_regs[i][0]);
		T23_regs[i][1] = XOR(T16_regs[i][1], T7_regs[i][1]);

		T24_regs[i][0] = XOR(T16_regs[i][0], T8_regs[i][0]);
		T24_regs[i][1] = XOR(T16_regs[i][1], T8_regs[i][1]);

		T25_regs[i][0] = XOR(T24_regs[i][0], state[i][0]);
		T25_regs[i][1] = XOR(T24_regs[i][1], state[i][1]);

		T26_regs[i][0] = XOR(T24_regs[i][0], T2_regs[i][0]);
		T26_regs[i][1] = XOR(T24_regs[i][1], T2_regs[i][1]);

		T27_regs[i][0] = XOR(T26_regs[i][0], state[i][0]);
		T27_regs[i][1] = XOR(T26_regs[i][1], state[i][1]);

		T28_regs[i][0] = XOR(T24_regs[i][0], T4_regs[i][0]);
		T28_regs[i][1] = XOR(T24_regs[i][1], T4_regs[i][1]);

		T29_regs[i][0] = XOR(T28_regs[i][0], state[i][0]);
		T29_regs[i][1] = XOR(T28_regs[i][1], state[i][1]);

		T31_regs[i][0] = XOR(T2_regs[i][0], T29_regs[i][0]);
		T31_regs[i][1] = XOR(T2_regs[i][1], T29_regs[i][1]);
	}

	for (int i = 0; i < 5; i++) {
		state[i][0] = XOR7(
			_mm256_setr_epi64x(_mm256_extract_epi64(   state[i][0], 0), _mm256_extract_epi64( T2_regs[i][0], 0), _mm256_extract_epi64(T11_regs[i][0], 0), _mm256_extract_epi64(   state[i][0], 0)),
			_mm256_setr_epi64x(_mm256_extract_epi64( T2_regs[i][0], 1), _mm256_extract_epi64( T5_regs[i][0], 1), _mm256_extract_epi64(T20_regs[i][0], 1), _mm256_extract_epi64( T9_regs[i][0], 1)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T15_regs[i][0], 2), _mm256_extract_epi64(T28_regs[i][0], 2), _mm256_extract_epi64( T3_regs[i][0], 2), _mm256_extract_epi64(T27_regs[i][0], 2)),
			_mm256_setr_epi64x(_mm256_extract_epi64( T9_regs[i][0], 3), _mm256_extract_epi64(T29_regs[i][0], 3), _mm256_extract_epi64( T5_regs[i][0], 3), _mm256_extract_epi64(T10_regs[i][0], 3)),
			_mm256_setr_epi64x(_mm256_extract_epi64( T9_regs[i][1], 0), _mm256_extract_epi64(T27_regs[i][1], 0), _mm256_extract_epi64( T4_regs[i][1], 0), _mm256_extract_epi64(T12_regs[i][1], 0)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T15_regs[i][1], 1), _mm256_extract_epi64(T23_regs[i][1], 1), _mm256_extract_epi64(T29_regs[i][1], 1), _mm256_extract_epi64(T11_regs[i][1], 1)),
			_mm256_setr_epi64x(_mm256_extract_epi64( T2_regs[i][1], 2), _mm256_extract_epi64(T11_regs[i][1], 2), _mm256_extract_epi64(   state[i][1], 2), _mm256_extract_epi64(T31_regs[i][1], 2)));
		
		state[i][1] = XOR7(
			_mm256_setr_epi64x(_mm256_extract_epi64(T31_regs[i][0], 0), _mm256_extract_epi64(T16_regs[i][0], 0), _mm256_extract_epi64(T29_regs[i][0], 0), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T26_regs[i][0], 1), _mm256_extract_epi64(T26_regs[i][0], 1), _mm256_extract_epi64(T15_regs[i][0], 1), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T29_regs[i][0], 2), _mm256_extract_epi64(T17_regs[i][0], 2), _mm256_extract_epi64(T16_regs[i][0], 2), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64( T7_regs[i][0], 3), _mm256_extract_epi64(T25_regs[i][0], 3), _mm256_extract_epi64(T31_regs[i][0], 3), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T22_regs[i][1], 0), _mm256_extract_epi64( T3_regs[i][1], 0), _mm256_extract_epi64(T23_regs[i][1], 0), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T24_regs[i][1], 1), _mm256_extract_epi64(T29_regs[i][1], 1), _mm256_extract_epi64( T9_regs[i][1], 1), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T16_regs[i][1], 2), _mm256_extract_epi64(T29_regs[i][1], 2), _mm256_extract_epi64( T2_regs[i][1], 2), 0));
	}
}

void mixcolumns_A6(__m256i (*state)[2]) {
	/*
	A6:
	0       0       0       0       0       0       1
	1       2       15      9       9       15      2
	2       5       28      29      27      23      11
	11      20      3       5       4       29      1
	1       9       27      10      12      11      31
	31      26      29      7       22      24      16
	16      26      17      25      3       29      29
	*/

	__m256i T2_regs[5][2];
	__m256i T3_regs[5][2];
	__m256i T4_regs[5][2];
	__m256i T5_regs[5][2];
	__m256i T7_regs[5][2];
	__m256i T8_regs[5][2];
	__m256i T9_regs[5][2];
	__m256i T10_regs[5][2];
	__m256i T11_regs[5][2];
	__m256i T12_regs[5][2];
	__m256i T15_regs[5][2];
	__m256i T16_regs[5][2];
	__m256i T17_regs[5][2];
	__m256i T20_regs[5][2];
	__m256i T22_regs[5][2];
	__m256i T23_regs[5][2];
	__m256i T24_regs[5][2];
	__m256i T25_regs[5][2];
	__m256i T26_regs[5][2];
	__m256i T27_regs[5][2];
	__m256i T28_regs[5][2];
	__m256i T29_regs[5][2];
	__m256i T31_regs[5][2];

	__m256i temp_state[5][2];

	T2(state, T2_regs); //T2
	T2(T2_regs, T4_regs); //T4
	T2(T4_regs, T8_regs); //T8
	T2(T8_regs, T16_regs); //T16

						   //T03
	for (int i = 0; i < 5; i++) {
		T3_regs[i][0] = XOR(state[i][0], T2_regs[i][0]);
		T3_regs[i][1] = XOR(state[i][1], T2_regs[i][1]);

		T5_regs[i][0] = XOR(state[i][0], T4_regs[i][0]);
		T5_regs[i][1] = XOR(state[i][1], T4_regs[i][1]);

		T7_regs[i][0] = XOR(T4_regs[i][0], T3_regs[i][0]);
		T7_regs[i][1] = XOR(T4_regs[i][1], T3_regs[i][1]);

		T9_regs[i][0] = XOR(state[i][0], T8_regs[i][0]);
		T9_regs[i][1] = XOR(state[i][1], T8_regs[i][1]);

		T10_regs[i][0] = XOR(T8_regs[i][0], T2_regs[i][0]);
		T10_regs[i][1] = XOR(T8_regs[i][1], T2_regs[i][1]);

		T11_regs[i][0] = XOR(T8_regs[i][0], T3_regs[i][0]);
		T11_regs[i][1] = XOR(T8_regs[i][1], T3_regs[i][1]);

		T12_regs[i][0] = XOR(T4_regs[i][0], T8_regs[i][0]);
		T12_regs[i][1] = XOR(T4_regs[i][1], T8_regs[i][1]);

		T15_regs[i][0] = XOR(T4_regs[i][0], T11_regs[i][0]);
		T15_regs[i][1] = XOR(T4_regs[i][1], T11_regs[i][1]);

		T17_regs[i][0] = XOR(state[i][0], T16_regs[i][0]);
		T17_regs[i][1] = XOR(state[i][1], T16_regs[i][1]);

		T20_regs[i][0] = XOR(T16_regs[i][0], T4_regs[i][0]);
		T20_regs[i][1] = XOR(T16_regs[i][1], T4_regs[i][1]);

		T22_regs[i][0] = XOR(T20_regs[i][0], T2_regs[i][0]);
		T22_regs[i][1] = XOR(T20_regs[i][1], T2_regs[i][1]);

		T23_regs[i][0] = XOR(T16_regs[i][0], T7_regs[i][0]);
		T23_regs[i][1] = XOR(T16_regs[i][1], T7_regs[i][1]);

		T24_regs[i][0] = XOR(T16_regs[i][0], T8_regs[i][0]);
		T24_regs[i][1] = XOR(T16_regs[i][1], T8_regs[i][1]);

		T25_regs[i][0] = XOR(T24_regs[i][0], state[i][0]);
		T25_regs[i][1] = XOR(T24_regs[i][1], state[i][1]);

		T26_regs[i][0] = XOR(T24_regs[i][0], T2_regs[i][0]);
		T26_regs[i][1] = XOR(T24_regs[i][1], T2_regs[i][1]);

		T27_regs[i][0] = XOR(T26_regs[i][0], state[i][0]);
		T27_regs[i][1] = XOR(T26_regs[i][1], state[i][1]);

		T28_regs[i][0] = XOR(T24_regs[i][0], T4_regs[i][0]);
		T28_regs[i][1] = XOR(T24_regs[i][1], T4_regs[i][1]);

		T29_regs[i][0] = XOR(T28_regs[i][0], state[i][0]);
		T29_regs[i][1] = XOR(T28_regs[i][1], state[i][1]);

		T31_regs[i][0] = XOR(T2_regs[i][0], T29_regs[i][0]);
		T31_regs[i][1] = XOR(T2_regs[i][1], T29_regs[i][1]);
	}

	//Shuffle bytes upwards due to lines 1,2,3,4,5,6 in matrix.
	for (int i = 0; i < 5; i++) {
		__mm256_insert_epi64(temp_state[i][0], _mm256_extract_epi64(state[i][1], 2), 0);
	}

	for (int i = 0; i < 5; i++) {
		temp_state[i][0] = XOR8(
			_mm256_setr_epi64x(0, _mm256_extract_epi64(   state[i][0], 0),	_mm256_extract_epi64( T2_regs[i][0], 0), _mm256_extract_epi64(T11_regs[i][0], 0)),
			_mm256_setr_epi64x(0, _mm256_extract_epi64( T2_regs[i][0], 1),	_mm256_extract_epi64( T5_regs[i][0], 1), _mm256_extract_epi64(T20_regs[i][0], 1)),
			_mm256_setr_epi64x(0, _mm256_extract_epi64(T15_regs[i][0], 2),	_mm256_extract_epi64(T28_regs[i][0], 2), _mm256_extract_epi64( T3_regs[i][0], 2)),
			_mm256_setr_epi64x(0, _mm256_extract_epi64( T9_regs[i][0], 3),	_mm256_extract_epi64(T29_regs[i][0], 3), _mm256_extract_epi64( T5_regs[i][0], 3)),
			_mm256_setr_epi64x(0, _mm256_extract_epi64( T9_regs[i][1], 0),	_mm256_extract_epi64(T27_regs[i][1], 0), _mm256_extract_epi64( T4_regs[i][1], 0)),
			_mm256_setr_epi64x(0, _mm256_extract_epi64(T15_regs[i][1], 1),	_mm256_extract_epi64(T23_regs[i][1], 1), _mm256_extract_epi64(T29_regs[i][1], 1)),
			_mm256_setr_epi64x(0, _mm256_extract_epi64( T2_regs[i][1], 2),	_mm256_extract_epi64(T11_regs[i][1], 2), _mm256_extract_epi64(   state[i][1], 2)),
			temp_state[i][0]);

		state[i][1] = XOR7(
			_mm256_setr_epi64x(_mm256_extract_epi64(   state[i][0], 0),	_mm256_extract_epi64(T31_regs[i][0], 0), _mm256_extract_epi64(T16_regs[i][0], 0), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64( T9_regs[i][0], 1), _mm256_extract_epi64(T26_regs[i][0], 1), _mm256_extract_epi64(T26_regs[i][0], 1), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T27_regs[i][0], 2), _mm256_extract_epi64(T29_regs[i][0], 2), _mm256_extract_epi64(T17_regs[i][0], 2), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T10_regs[i][0], 3), _mm256_extract_epi64( T7_regs[i][0], 3), _mm256_extract_epi64(T25_regs[i][0], 3), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T12_regs[i][1], 0), _mm256_extract_epi64(T22_regs[i][1], 0), _mm256_extract_epi64( T3_regs[i][1], 0), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T11_regs[i][1], 1), _mm256_extract_epi64(T24_regs[i][1], 1), _mm256_extract_epi64(T29_regs[i][1], 1), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T31_regs[i][1], 2), _mm256_extract_epi64(T16_regs[i][1], 2), _mm256_extract_epi64(T29_regs[i][1], 2), 0));

		state[i][0] = temp_state[i][0];
	}
}

void mixcolumns_A5(__m256i (*state)[2]) {
	/*
	A5:
	0       0       0       0       0       1       0
	0       0       0       0       0       0       1
	1       2       15      9       9       15      2
	2       5       28      29      27      23      11
	11      20      3       5       4       29      1
	1       9       27      10      12      11      31
	31      26      29      7       22      24      16

	*/

	__m256i T2_regs[5][2];
	__m256i T3_regs[5][2];
	__m256i T4_regs[5][2];
	__m256i T5_regs[5][2];
	__m256i T7_regs[5][2];
	__m256i T8_regs[5][2];
	__m256i T9_regs[5][2];
	__m256i T10_regs[5][2];
	__m256i T11_regs[5][2];
	__m256i T12_regs[5][2];
	__m256i T15_regs[5][2];
	__m256i T16_regs[5][2];
	__m256i T20_regs[5][2];
	__m256i T22_regs[5][2];
	__m256i T23_regs[5][2];
	__m256i T24_regs[5][2];
	__m256i T26_regs[5][2];
	__m256i T27_regs[5][2];
	__m256i T28_regs[5][2];
	__m256i T29_regs[5][2];
	__m256i T31_regs[5][2];

	__m256i temp_state[5][2];

	T2(state, T2_regs); //T2
	T2(T2_regs, T4_regs); //T4
	T2(T4_regs, T8_regs); //T8
	T2(T8_regs, T16_regs); //T16

						   //T3
	for (int i = 0; i < 5; i++) {
		T3_regs[i][0] = XOR(state[i][0], T2_regs[i][0]);
		T3_regs[i][1] = XOR(state[i][1], T2_regs[i][1]);

		T5_regs[i][0] = XOR(state[i][0], T4_regs[i][0]);
		T5_regs[i][1] = XOR(state[i][1], T4_regs[i][1]);

		T7_regs[i][0] = XOR(T4_regs[i][0], T3_regs[i][0]);
		T7_regs[i][1] = XOR(T4_regs[i][1], T3_regs[i][1]);

		T9_regs[i][0] = XOR(state[i][0], T8_regs[i][0]);
		T9_regs[i][1] = XOR(state[i][1], T8_regs[i][1]);

		T10_regs[i][0] = XOR(T8_regs[i][0], T2_regs[i][0]);
		T10_regs[i][1] = XOR(T8_regs[i][1], T2_regs[i][1]);

		T11_regs[i][0] = XOR(T8_regs[i][0], T3_regs[i][0]);
		T11_regs[i][1] = XOR(T8_regs[i][1], T3_regs[i][1]);

		T12_regs[i][0] = XOR(T4_regs[i][0], T8_regs[i][0]);
		T12_regs[i][1] = XOR(T4_regs[i][1], T8_regs[i][1]);

		T15_regs[i][0] = XOR(T4_regs[i][0], T11_regs[i][0]);
		T15_regs[i][1] = XOR(T4_regs[i][1], T11_regs[i][1]);

		T20_regs[i][0] = XOR(T16_regs[i][0], T4_regs[i][0]);
		T20_regs[i][1] = XOR(T16_regs[i][1], T4_regs[i][1]);

		T22_regs[i][0] = XOR(T20_regs[i][0], T2_regs[i][0]);
		T22_regs[i][1] = XOR(T20_regs[i][1], T2_regs[i][1]);

		T23_regs[i][0] = XOR(T16_regs[i][0], T7_regs[i][0]);
		T23_regs[i][1] = XOR(T16_regs[i][1], T7_regs[i][1]);

		T24_regs[i][0] = XOR(T16_regs[i][0], T8_regs[i][0]);
		T24_regs[i][1] = XOR(T16_regs[i][1], T8_regs[i][1]);

		T26_regs[i][0] = XOR(T24_regs[i][0], T2_regs[i][0]);
		T26_regs[i][1] = XOR(T24_regs[i][1], T2_regs[i][1]);

		T27_regs[i][0] = XOR(T26_regs[i][0], state[i][0]);
		T27_regs[i][1] = XOR(T26_regs[i][1], state[i][1]);

		T28_regs[i][0] = XOR(T24_regs[i][0], T4_regs[i][0]);
		T28_regs[i][1] = XOR(T24_regs[i][1], T4_regs[i][1]);

		T29_regs[i][0] = XOR(T28_regs[i][0], state[i][0]);
		T29_regs[i][1] = XOR(T28_regs[i][1], state[i][1]);

		T31_regs[i][0] = XOR(T2_regs[i][0], T29_regs[i][0]);
		T31_regs[i][1] = XOR(T2_regs[i][1], T29_regs[i][1]);
	}

	//Shuffle bytes upwards due to lines 1,2,3,4,5,6 in matrix.
	for (int i = 0; i < 5; i++) {
		__mm256_insert_epi64(temp_state[i][0], _mm256_extract_epi64(state[i][1], 1), 0);
		__mm256_insert_epi64(temp_state[i][0], _mm256_extract_epi64(state[i][1], 2), 1);
	}

	for (int i = 0; i < 5; i++) {
		temp_state[i][0] = XOR8(
			_mm256_setr_epi64x(0, 0, _mm256_extract_epi64(   state[i][0], 0), _mm256_extract_epi64( T2_regs[i][0], 0)),
			_mm256_setr_epi64x(0, 0, _mm256_extract_epi64( T2_regs[i][0], 1), _mm256_extract_epi64( T5_regs[i][0], 1)),
			_mm256_setr_epi64x(0, 0, _mm256_extract_epi64(T15_regs[i][0], 2), _mm256_extract_epi64(T28_regs[i][0], 2)),
			_mm256_setr_epi64x(0, 0, _mm256_extract_epi64( T9_regs[i][0], 3), _mm256_extract_epi64(T29_regs[i][0], 3)),
			_mm256_setr_epi64x(0, 0, _mm256_extract_epi64( T9_regs[i][1], 0), _mm256_extract_epi64(T27_regs[i][1], 0)),
			_mm256_setr_epi64x(0, 0, _mm256_extract_epi64(T15_regs[i][1], 1), _mm256_extract_epi64(T23_regs[i][1], 1)),
			_mm256_setr_epi64x(0, 0, _mm256_extract_epi64( T2_regs[i][1], 2), _mm256_extract_epi64(T11_regs[i][1], 2)),
			temp_state[i][0]);

		state[i][1] = XOR7(
			_mm256_setr_epi64x(_mm256_extract_epi64(T11_regs[i][0], 0), _mm256_extract_epi64(   state[i][0], 0), _mm256_extract_epi64(T31_regs[i][0], 0), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T20_regs[i][0], 1), _mm256_extract_epi64( T9_regs[i][0], 1), _mm256_extract_epi64(T26_regs[i][0], 1), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64( T3_regs[i][0], 2), _mm256_extract_epi64(T27_regs[i][0], 2), _mm256_extract_epi64(T29_regs[i][0], 2), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64( T5_regs[i][0], 3), _mm256_extract_epi64(T10_regs[i][0], 3), _mm256_extract_epi64( T7_regs[i][0], 3), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64( T4_regs[i][1], 0), _mm256_extract_epi64(T12_regs[i][1], 0), _mm256_extract_epi64(T22_regs[i][1], 0), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T29_regs[i][1], 1), _mm256_extract_epi64(T11_regs[i][1], 1), _mm256_extract_epi64(T24_regs[i][1], 1), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(   state[i][1], 2), _mm256_extract_epi64(T31_regs[i][1], 2), _mm256_extract_epi64(T16_regs[i][1], 2), 0));

		state[i][0] = temp_state[i][0];
	}
}

void mixcolumns_A4(__m256i (*state)[2]) {
	/*
	A4:
	0       0       0       0       1       0       0
	0       0       0       0       0       1       0
	0       0       0       0       0       0       1
	1       2       15      9       9       15      2
	2       5       28      29      27      23      11
	11      20      3       5       4       29      1
	1       9       27      10      12      11      31
	*/

	__m256i T2_regs[5][2];
	__m256i T3_regs[5][2];
	__m256i T4_regs[5][2];
	__m256i T5_regs[5][2];
	__m256i T8_regs[5][2];
	__m256i T9_regs[5][2];
	__m256i T10_regs[5][2];
	__m256i T11_regs[5][2];
	__m256i T12_regs[5][2];
	__m256i T15_regs[5][2];
	__m256i T16_regs[5][2];
	__m256i T20_regs[5][2];
	__m256i T23_regs[5][2];
	__m256i T27_regs[5][2];
	__m256i T28_regs[5][2];
	__m256i T29_regs[5][2];
	__m256i T31_regs[5][2];

	__m256i temp_state[5][2];

	T2(state, T2_regs); //T2
	T2(T2_regs, T4_regs); //T4
	T2(T4_regs, T8_regs); //T8
	T2(T8_regs, T16_regs); //T16

						   //T3
	for (int i = 0; i < 5; i++) {
		T3_regs[i][0] = XOR(state[i][0], T2_regs[i][0]);
		T3_regs[i][1] = XOR(state[i][1], T2_regs[i][1]);

		T5_regs[i][0] = XOR(state[i][0], T4_regs[i][0]);
		T5_regs[i][1] = XOR(state[i][1], T4_regs[i][1]);

		T9_regs[i][0] = XOR(state[i][0], T8_regs[i][0]);
		T9_regs[i][1] = XOR(state[i][1], T8_regs[i][1]);

		T10_regs[i][0] = XOR(T8_regs[i][0], T2_regs[i][0]);
		T10_regs[i][1] = XOR(T8_regs[i][1], T2_regs[i][1]);

		T11_regs[i][0] = XOR(T8_regs[i][0], T3_regs[i][0]);
		T11_regs[i][1] = XOR(T8_regs[i][1], T3_regs[i][1]);

		T12_regs[i][0] = XOR(T4_regs[i][0], T8_regs[i][0]);
		T12_regs[i][1] = XOR(T4_regs[i][1], T8_regs[i][1]);

		T15_regs[i][0] = XOR(T4_regs[i][0], T11_regs[i][0]);
		T15_regs[i][1] = XOR(T4_regs[i][1], T11_regs[i][1]);

		T20_regs[i][0] = XOR(T16_regs[i][0], T4_regs[i][0]);
		T20_regs[i][1] = XOR(T16_regs[i][1], T4_regs[i][1]);

		T23_regs[i][0] = XOR(T20_regs[i][0], T3_regs[i][0]);
		T23_regs[i][1] = XOR(T20_regs[i][1], T3_regs[i][1]);

		T27_regs[i][0] = XOR(T16_regs[i][0], T11_regs[i][0]);
		T27_regs[i][1] = XOR(T16_regs[i][1], T11_regs[i][1]);

		T28_regs[i][0] = XOR(T20_regs[i][0], T8_regs[i][0]);
		T28_regs[i][1] = XOR(T20_regs[i][1], T8_regs[i][1]);

		T29_regs[i][0] = XOR(T28_regs[i][0], state[i][0]);
		T29_regs[i][1] = XOR(T28_regs[i][1], state[i][1]);

		T31_regs[i][0] = XOR(T29_regs[i][0], T2_regs[i][0]);
		T31_regs[i][1] = XOR(T29_regs[i][1], T2_regs[i][1]);
	}

	//Shuffle bytes upwards due to lines 1,2,3,4,5,6 in matrix.
	for (int i = 0; i < 5; i++) {
		__mm256_insert_epi64(temp_state[i][0], _mm256_extract_epi64(state[i][1], 0), 0);
		__mm256_insert_epi64(temp_state[i][0], _mm256_extract_epi64(state[i][1], 1), 1);
		__mm256_insert_epi64(temp_state[i][0], _mm256_extract_epi64(state[i][1], 2), 2);
	}

	for (int i = 0; i < 5; i++) {

		temp_state[i][0] = XOR8(
			_mm256_setr_epi64x(0, 0, 0, _mm256_extract_epi64(   state[i][0], 0)),
			_mm256_setr_epi64x(0, 0, 0, _mm256_extract_epi64( T2_regs[i][0], 1)),
			_mm256_setr_epi64x(0, 0, 0, _mm256_extract_epi64(T15_regs[i][0], 2)),
			_mm256_setr_epi64x(0, 0, 0, _mm256_extract_epi64( T9_regs[i][0], 3)),
			_mm256_setr_epi64x(0, 0, 0, _mm256_extract_epi64( T9_regs[i][1], 0)),
			_mm256_setr_epi64x(0, 0, 0, _mm256_extract_epi64(T15_regs[i][1], 1)),
			_mm256_setr_epi64x(0, 0, 0, _mm256_extract_epi64( T2_regs[i][1], 2)),
			temp_state[i][0]);

		state[i][1] = XOR7(
			_mm256_setr_epi64x(_mm256_extract_epi64( T2_regs[i][0], 0),  _mm256_extract_epi64(T11_regs[i][0], 0),	_mm256_extract_epi64(state[i][0], 0), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64( T5_regs[i][0], 1),  _mm256_extract_epi64(T20_regs[i][0], 1),	_mm256_extract_epi64(T9_regs[i][0], 1), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T28_regs[i][0], 2),  _mm256_extract_epi64( T3_regs[i][0], 2),	_mm256_extract_epi64(T27_regs[i][0], 2), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T29_regs[i][0], 3),  _mm256_extract_epi64( T5_regs[i][0], 3),	_mm256_extract_epi64(T10_regs[i][0], 3), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T27_regs[i][1], 0),  _mm256_extract_epi64( T4_regs[i][1], 0),	_mm256_extract_epi64(T12_regs[i][1], 0), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T23_regs[i][1], 1),  _mm256_extract_epi64(T29_regs[i][1], 1),	_mm256_extract_epi64(T11_regs[i][1], 1), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T11_regs[i][1], 2),  _mm256_extract_epi64(   state[i][1], 2),	_mm256_extract_epi64(T31_regs[i][1], 2), 0));

		state[i][0] = temp_state[i][0];
	}
}

void mixcolumns_A3(__m256i (*state)[2]) {
	/*
	A3:
	0       0       0       1       0       0       0
	0       0       0       0       1       0       0
	0       0       0       0       0       1       0
	0       0       0       0       0       0       1
	1       2       15      9       9       15      2
	2       5       28      29      27      23      11
	11      20      3       5       4       29      1
	*/

	__m256i T2_regs[5][2];
	__m256i T3_regs[5][2];
	__m256i T4_regs[5][2];
	__m256i T5_regs[5][2];
	__m256i T8_regs[5][2];
	__m256i T9_regs[5][2];
	__m256i T11_regs[5][2];
	__m256i T15_regs[5][2];
	__m256i T16_regs[5][2];
	__m256i T20_regs[5][2];
	__m256i T23_regs[5][2];
	__m256i T27_regs[5][2];
	__m256i T28_regs[5][2];
	__m256i T29_regs[5][2];

	__m256i temp_state[5][2];

	T2(state, T2_regs); //T2
	T2(T2_regs, T4_regs); //T4
	T2(T4_regs, T8_regs); //T8
	T2(T8_regs, T16_regs); //T16

	for (int i = 0; i < 5; i++) {
		T3_regs[i][0] = XOR(state[i][0], T2_regs[i][0]);
		T3_regs[i][1] = XOR(state[i][1], T2_regs[i][1]);

		T5_regs[i][0] = XOR(state[i][0], T4_regs[i][0]);
		T5_regs[i][1] = XOR(state[i][1], T4_regs[i][1]);

		T9_regs[i][0] = XOR(state[i][0], T8_regs[i][0]);
		T9_regs[i][1] = XOR(state[i][1], T8_regs[i][1]);

		T11_regs[i][0] = XOR(T8_regs[i][0], T3_regs[i][0]);
		T11_regs[i][1] = XOR(T8_regs[i][1], T3_regs[i][1]);

		T15_regs[i][0] = XOR(T4_regs[i][0], T11_regs[i][0]);
		T15_regs[i][1] = XOR(T4_regs[i][1], T11_regs[i][1]);

		T20_regs[i][0] = XOR(T16_regs[i][0], T4_regs[i][0]);
		T20_regs[i][1] = XOR(T16_regs[i][1], T4_regs[i][1]);

		T23_regs[i][0] = XOR(T20_regs[i][0], T3_regs[i][0]);
		T23_regs[i][1] = XOR(T20_regs[i][1], T3_regs[i][1]);

		T27_regs[i][0] = XOR(T16_regs[i][0], T11_regs[i][0]);
		T27_regs[i][1] = XOR(T16_regs[i][1], T11_regs[i][1]);

		T28_regs[i][0] = XOR(T20_regs[i][0], T8_regs[i][0]);
		T28_regs[i][1] = XOR(T20_regs[i][1], T8_regs[i][1]);

		T29_regs[i][0] = XOR(T28_regs[i][0], state[i][0]);
		T29_regs[i][1] = XOR(T28_regs[i][1], state[i][1]);
	}

	//Shuffle bytes upwards due to lines 1,2,3,4,5,6 in matrix.
	for (int i = 0; i < 5; i++) {
		__mm256_insert_epi64(temp_state[i][0], _mm256_extract_epi64(state[i][0], 3), 0);
		__mm256_insert_epi64(temp_state[i][0], _mm256_extract_epi64(state[i][1], 0), 1);
		__mm256_insert_epi64(temp_state[i][0], _mm256_extract_epi64(state[i][1], 1), 2);
		__mm256_insert_epi64(temp_state[i][0], _mm256_extract_epi64(state[i][1], 2), 3);
	}

	for (int i = 0; i < 5; i++) {
		state[i][1] = XOR7(
			_mm256_setr_epi64x(_mm256_extract_epi64(state[i][0], 0),	_mm256_extract_epi64(T2_regs[i][0], 0),		_mm256_extract_epi64(T11_regs[i][0], 0), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T2_regs[i][0], 1),	_mm256_extract_epi64(T5_regs[i][0], 1),		_mm256_extract_epi64(T20_regs[i][0], 1), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T15_regs[i][0], 2), _mm256_extract_epi64(T28_regs[i][0], 2),	_mm256_extract_epi64(T3_regs[i][0], 2), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T9_regs[i][0], 3),	_mm256_extract_epi64(T29_regs[i][0], 3),	_mm256_extract_epi64(T5_regs[i][0], 3), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T9_regs[i][1], 0),	_mm256_extract_epi64(T27_regs[i][1], 0),	_mm256_extract_epi64(T4_regs[i][1], 0), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T15_regs[i][1], 1), _mm256_extract_epi64(T23_regs[i][1], 1),	_mm256_extract_epi64(T29_regs[i][1], 1), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T2_regs[i][1], 2),	_mm256_extract_epi64(T11_regs[i][1], 2),	_mm256_extract_epi64(state[i][1], 2), 0));

		state[i][0] = temp_state[i][0];
	}
}

void mixcolumns_A2(__m256i (*state)[2]) {
	/*0       0       1       0       0       0       0
	0       0       0       1       0       0       0
	0       0       0       0       1       0       0
	0       0       0       0       0       1       0
	0       0       0       0       0       0       1
	1       2       15      9       9       15      2
	2       5       28      29      27      23      11*/

	__m256i T2_regs[5][2];
	__m256i T4_regs[5][2];
	__m256i T5_regs[5][2];
	__m256i T8_regs[5][2];
	__m256i T9_regs[5][2];
	__m256i T11_regs[5][2];
	__m256i T15_regs[5][2];
	__m256i T16_regs[5][2];
	__m256i T23_regs[5][2];
	__m256i T27_regs[5][2];
	__m256i T28_regs[5][2];
	__m256i T29_regs[5][2];

	__m256i temp_state[5][2];

	T2(state, T2_regs); //T2
	T2(T2_regs, T4_regs); //T4
	T2(T4_regs, T8_regs); //T8
	T2(T8_regs, T16_regs); //T16

	for (int i = 0; i < 5; i++) {
		T5_regs[i][0] = XOR(state[i][0], T4_regs[i][0]);
		T5_regs[i][1] = XOR(state[i][1], T4_regs[i][1]);

		T9_regs[i][0] = XOR(state[i][0], T8_regs[i][0]);
		T9_regs[i][1] = XOR(state[i][1], T8_regs[i][1]);

		T11_regs[i][0] = XOR(T9_regs[i][0], T2_regs[i][0]);
		T11_regs[i][1] = XOR(T9_regs[i][1], T2_regs[i][1]);

		T15_regs[i][0] = XOR(T4_regs[i][0], T9_regs[i][0]);
		T15_regs[i][1] = XOR(T4_regs[i][1], T9_regs[i][1]);

		T23_regs[i][0] = XOR3(T16_regs[i][0], T5_regs[i][0], T2_regs[i][0]);
		T23_regs[i][1] = XOR3(T16_regs[i][1], T5_regs[i][1], T2_regs[i][1]);

		T27_regs[i][0] = XOR(T16_regs[i][0], T11_regs[i][0]);
		T27_regs[i][1] = XOR(T16_regs[i][1], T11_regs[i][1]);

		T28_regs[i][0] = XOR3(T16_regs[i][0], T8_regs[i][0], T4_regs[i][0]);
		T28_regs[i][1] = XOR3(T16_regs[i][1], T8_regs[i][1], T4_regs[i][1]);

		T29_regs[i][0] = XOR(T28_regs[i][0], state[i][0]);
		T29_regs[i][1] = XOR(T28_regs[i][1], state[i][1]);
	}

	//Shuffle bytes upwards due to lines 1,2,3,4,5,6 in matrix.
	for (int i = 0; i < 5; i++) {
		__mm256_insert_epi64(temp_state[i][0], _mm256_extract_epi64(state[i][0], 2), 0);
		__mm256_insert_epi64(temp_state[i][0], _mm256_extract_epi64(state[i][0], 3), 1);
		__mm256_insert_epi64(temp_state[i][0], _mm256_extract_epi64(state[i][1], 0), 2);
		__mm256_insert_epi64(temp_state[i][0], _mm256_extract_epi64(state[i][1], 1), 3);

		__mm256_insert_epi64(temp_state[i][1], _mm256_extract_epi64(state[i][1], 2), 0);
	}

	for (int i = 0; i < 5; i++) {
		state[i][1] = XOR8(
			_mm256_setr_epi64x(0, _mm256_extract_epi64(state[i][0], 0),		_mm256_extract_epi64(T2_regs[i][0], 0), 0),
			_mm256_setr_epi64x(0, _mm256_extract_epi64(T2_regs[i][0], 1),	_mm256_extract_epi64(T5_regs[i][0], 1), 0),
			_mm256_setr_epi64x(0, _mm256_extract_epi64(T15_regs[i][0], 2),	_mm256_extract_epi64(T28_regs[i][0], 2), 0),
			_mm256_setr_epi64x(0, _mm256_extract_epi64(T9_regs[i][0], 3),	_mm256_extract_epi64(T29_regs[i][0], 3), 0),
			_mm256_setr_epi64x(0, _mm256_extract_epi64(T9_regs[i][1], 0),	_mm256_extract_epi64(T27_regs[i][1], 0), 0),
			_mm256_setr_epi64x(0, _mm256_extract_epi64(T15_regs[i][1], 1),	_mm256_extract_epi64(T23_regs[i][1], 1), 0),
			_mm256_setr_epi64x(0, _mm256_extract_epi64(T2_regs[i][1], 2),	_mm256_extract_epi64(T11_regs[i][1], 2), 0),
			temp_state[i][1]);

		state[i][0] = temp_state[i][0];
	}
}

void mixcolumns_A1(__m256i (*state)[2]) {
	__m256i T2_regs[5][2];
	__m256i T4_regs[5][2];
	__m256i T8_regs[5][2];
	__m256i T9_regs[5][2];
	__m256i T15_regs[5][2];

	__m256i temp_state[5][2];

	//T2
	T2(state, T2_regs);
	T2(T2_regs, T4_regs); //T4
	T2(T4_regs, T8_regs); //T8

	for (int i = 0; i < 5; i++) {
		T9_regs[i][0] = XOR(state[i][0], T8_regs[i][0]);
		T9_regs[i][1] = XOR(state[i][1], T8_regs[i][1]);

		T15_regs[i][0] = XOR3(T2_regs[i][0], T4_regs[i][0], T9_regs[i][0]);
		T15_regs[i][1] = XOR3(T2_regs[i][1], T4_regs[i][1], T9_regs[i][1]);
	}

	//Shuffle bytes upwards due to lines 1,2,3,4 in matrix.
	for (int i = 0; i < 5; i++) {
		__mm256_insert_epi64(temp_state[i][0], _mm256_extract_epi64(state[i][0], 1), 0);
		__mm256_insert_epi64(temp_state[i][0], _mm256_extract_epi64(state[i][0], 2), 1);
		__mm256_insert_epi64(temp_state[i][0], _mm256_extract_epi64(state[i][0], 3), 2);
		__mm256_insert_epi64(temp_state[i][0], _mm256_extract_epi64(state[i][1], 0), 3);

		__mm256_insert_epi64(temp_state[i][1], _mm256_extract_epi64(state[i][1], 1), 0);
		__mm256_insert_epi64(temp_state[i][1], _mm256_extract_epi64(state[i][1], 2), 1);
	}

	//handle last line of matrix. Align all rows on the third u64
	for (int i = 0; i < 5; i++) {
		state[i][1] = XOR8(
			_mm256_setr_epi64x(0, 0, _mm256_extract_epi64(state[i][0], 0), 0),
			_mm256_setr_epi64x(0, 0, _mm256_extract_epi64(T2_regs[i][0], 1), 0),
			_mm256_setr_epi64x(0, 0, _mm256_extract_epi64(T15_regs[i][0], 2), 0),
			_mm256_setr_epi64x(0, 0, _mm256_extract_epi64(T9_regs[i][0], 3), 0),
			_mm256_setr_epi64x(0, 0, _mm256_extract_epi64(T9_regs[i][1], 0), 0),
			_mm256_setr_epi64x(0, 0, _mm256_extract_epi64(T15_regs[i][1], 1), 0),
			_mm256_setr_epi64x(0, 0, _mm256_extract_epi64(T2_regs[i][1], 2), 0),
			temp_state[i][1]);

		state[i][0] = temp_state[i][0];
	}
}

void optimized_mixcolumns_A7(__m256i (*state)[2]) {
	/*
	A7:
	1       2       15      9       9       15      2
	2       5       28      29      27      23      11
	11      20      3       5       4       29      1
	1       9       27      10      12      11      31
	31      26      29      7       22      24      16
	16      26      17      25      3       29      29
	29      15      16      31      23      9       2
	*/

	__m256i T2_regs[5][2];
	__m256i T3_regs[5][2];
	__m256i T4_regs[5][2];
	__m256i T5_regs[5];
	__m256i T7_regs[5];
	__m256i T8_regs[5][2];
	__m256i T9_regs[5][2];
	__m256i T10_regs[5];
	__m256i T11_regs[5][2];
	__m256i T12_regs[5];
	__m256i T15_regs[5][2];
	__m256i T16_regs[5][2];
	__m256i T17_regs[5];
	__m256i T20_regs[5][2];
	__m256i T22_regs[5];
	__m256i T23_regs[5];
	__m256i T24_regs[5][2];
	__m256i T25_regs[5];
	__m256i T26_regs[5][2];
	__m256i T27_regs[5][2];
	__m256i T28_regs[5];
	__m256i T29_regs[5][2];
	__m256i T31_regs[5][2];

	T2(state, T2_regs); //T2
	T2(T2_regs, T4_regs); //T4
	T2(T4_regs, T8_regs); //T8
	T2(T8_regs, T16_regs); //T16

						   //T29
	for (int i = 0; i < 5; i++) {

		T3_regs[i][0] = XOR(state[i][0], T2_regs[i][0]);
		T3_regs[i][1] = XOR(state[i][1], T2_regs[i][1]);

		T5_regs[i] = XOR(state[i][0], T4_regs[i][0]);

		T7_regs[i] = XOR(T5_regs[i], T2_regs[i][0]);

		T9_regs[i][0] = XOR(state[i][0], T8_regs[i][0]);
		T9_regs[i][1] = XOR(state[i][1], T8_regs[i][1]);

		T10_regs[i] = XOR(T8_regs[i][0], T2_regs[i][0]);

		T11_regs[i][0] = XOR(T3_regs[i][0], T8_regs[i][0]);
		T11_regs[i][1] = XOR(T3_regs[i][1], T8_regs[i][1]);

		T12_regs[i] = XOR(T4_regs[i][1], T8_regs[i][1]);

		T15_regs[i][0] = XOR(T11_regs[i][0], T4_regs[i][0]);
		T15_regs[i][1] = XOR(T11_regs[i][1], T4_regs[i][1]);

		T17_regs[i] = XOR(state[i][0], T16_regs[i][0]);

		T20_regs[i][0] = XOR(T16_regs[i][0], T4_regs[i][0]);
		T20_regs[i][1] = XOR(T16_regs[i][1], T4_regs[i][1]);

		T22_regs[i] = XOR(T20_regs[i][1], T2_regs[i][1]);

		T23_regs[i] = XOR(T20_regs[i][1], T3_regs[i][1]);

		T24_regs[i][0] = XOR(T16_regs[i][0], T8_regs[i][0]);
		T24_regs[i][1] = XOR(T16_regs[i][1], T8_regs[i][1]);

		T25_regs[i] = XOR(T24_regs[i][0], state[i][0]);

		T26_regs[i][0] = XOR(T24_regs[i][0], T2_regs[i][0]);
		T26_regs[i][1] = XOR(T24_regs[i][1], T2_regs[i][1]);

		T27_regs[i][0] = XOR(T26_regs[i][0], state[i][0]);
		T27_regs[i][1] = XOR(T26_regs[i][1], state[i][1]);

		T28_regs[i] = XOR3(T16_regs[i][0], T8_regs[i][0], T4_regs[i][0]);

		T29_regs[i][0] = XOR(T20_regs[i][0], T9_regs[i][0]);
		T29_regs[i][1] = XOR(T20_regs[i][1], T9_regs[i][1]);

		T31_regs[i][0] = XOR(T29_regs[i][0], T2_regs[i][0]);
		T31_regs[i][1] = XOR(T29_regs[i][1], T2_regs[i][1]);
	}

	for (int i = 0; i < 5; i++) {
		state[i][0] = XOR7(
			_mm256_setr_epi64x(_mm256_extract_epi64(state[i][0], 0), _mm256_extract_epi64(T2_regs[i][0], 0), _mm256_extract_epi64(T11_regs[i][0], 0), _mm256_extract_epi64(state[i][0], 0)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T2_regs[i][0], 1), _mm256_extract_epi64(T5_regs[i], 1), _mm256_extract_epi64(T20_regs[i][0], 1), _mm256_extract_epi64(T9_regs[i][0], 1)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T15_regs[i][0], 2), _mm256_extract_epi64(T28_regs[i], 2), _mm256_extract_epi64(T3_regs[i][0], 2), _mm256_extract_epi64(T27_regs[i][0], 2)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T9_regs[i][0], 3), _mm256_extract_epi64(T29_regs[i][0], 3), _mm256_extract_epi64(T5_regs[i], 3), _mm256_extract_epi64(T10_regs[i], 3)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T9_regs[i][1], 0), _mm256_extract_epi64(T27_regs[i][1], 0), _mm256_extract_epi64(T4_regs[i][1], 0), _mm256_extract_epi64(T12_regs[i], 0)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T15_regs[i][1], 1), _mm256_extract_epi64(T23_regs[i], 1), _mm256_extract_epi64(T29_regs[i][1], 1), _mm256_extract_epi64(T11_regs[i][1], 1)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T2_regs[i][1], 2), _mm256_extract_epi64(T11_regs[i][1], 2), _mm256_extract_epi64(state[i][1], 2), _mm256_extract_epi64(T31_regs[i][1], 2)));


		state[i][1] = XOR7(
			_mm256_setr_epi64x(_mm256_extract_epi64(T31_regs[i][0], 0), _mm256_extract_epi64(T16_regs[i][0], 0), _mm256_extract_epi64(T29_regs[i][0], 0), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T26_regs[i][0], 1), _mm256_extract_epi64(T26_regs[i][0], 1), _mm256_extract_epi64(T15_regs[i][0], 1), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T29_regs[i][0], 2), _mm256_extract_epi64(T17_regs[i], 2), _mm256_extract_epi64(T16_regs[i][0], 2), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T7_regs[i], 3), _mm256_extract_epi64(T25_regs[i], 3), _mm256_extract_epi64(T31_regs[i][0], 3), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T22_regs[i], 0), _mm256_extract_epi64(T3_regs[i][1], 0), _mm256_extract_epi64(T23_regs[i], 0), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T24_regs[i][1], 1), _mm256_extract_epi64(T29_regs[i][1], 1), _mm256_extract_epi64(T9_regs[i][1], 1), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T16_regs[i][1], 2), _mm256_extract_epi64(T29_regs[i][1], 2), _mm256_extract_epi64(T2_regs[i][1], 2), 0));
	}
}

void optimized_mixcolumns_A6(__m256i (*state)[2]) {
	/*
	A6:
	0       0       0       0       0       0       1
	1       2       15      9       9       15      2
	2       5       28      29      27      23      11
	11      20      3       5       4       29      1
	1       9       27      10      12      11      31
	31      26      29      7       22      24      16
	16      26      17      25      3       29      29
	*/

	__m256i T2_regs[5][2];
	__m256i T3_regs[5][2];
	__m256i T4_regs[5][2];
	__m256i T5_regs[5];
	__m256i T7_regs[5];
	__m256i T8_regs[5][2];
	__m256i T9_regs[5][2];
	__m256i T10_regs[5];
	__m256i T11_regs[5][2];
	__m256i T12_regs[5];
	__m256i T15_regs[5][2];
	__m256i T16_regs[5][2];
	__m256i T17_regs[5];
	__m256i T20_regs[5][2];
	__m256i T22_regs[5];
	__m256i T23_regs[5];
	__m256i T24_regs[5][2];
	__m256i T25_regs[5];
	__m256i T26_regs[5];
	__m256i T27_regs[5][2];
	__m256i T28_regs[5];
	__m256i T29_regs[5][2];
	__m256i T31_regs[5][2];

	__m256i temp_state[5][2];

	T2(state, T2_regs); //T2
	T2(T2_regs, T4_regs); //T4
	T2(T4_regs, T8_regs); //T8
	T2(T8_regs, T16_regs); //T16

	//T03
	for (int i = 0; i < 5; i++) {
		T3_regs[i][0] = XOR(state[i][0], T2_regs[i][0]);
		T3_regs[i][1] = XOR(state[i][1], T2_regs[i][1]);

		T5_regs[i] = XOR(state[i][0], T4_regs[i][0]);

		T7_regs[i] = XOR(T4_regs[i][0], T3_regs[i][0]);

		T9_regs[i][0] = XOR(state[i][0], T8_regs[i][0]);
		T9_regs[i][1] = XOR(state[i][1], T8_regs[i][1]);

		T10_regs[i] = XOR(T8_regs[i][0], T2_regs[i][0]);

		T11_regs[i][0] = XOR(T8_regs[i][0], T3_regs[i][0]);
		T11_regs[i][1] = XOR(T8_regs[i][1], T3_regs[i][1]);

		T12_regs[i] = XOR(T4_regs[i][1], T8_regs[i][1]);

		T15_regs[i][0] = XOR(T4_regs[i][0], T11_regs[i][0]);
		T15_regs[i][1] = XOR(T4_regs[i][1], T11_regs[i][1]);

		T17_regs[i] = XOR(state[i][0], T16_regs[i][0]);

		T20_regs[i][0] = XOR(T16_regs[i][0], T4_regs[i][0]);
		T20_regs[i][1] = XOR(T16_regs[i][1], T4_regs[i][1]);

		T22_regs[i] = XOR(T20_regs[i][1], T2_regs[i][1]);

		T23_regs[i] = XOR(T22_regs[i], state[i][1]);

		T24_regs[i][0] = XOR(T16_regs[i][0], T8_regs[i][0]);
		T24_regs[i][1] = XOR(T16_regs[i][1], T8_regs[i][1]);

		T25_regs[i] = XOR(T24_regs[i][0], state[i][0]);

		T26_regs[i] = XOR(T24_regs[i][0], T2_regs[i][0]);

		T27_regs[i][0] = XOR(T16_regs[i][0], T11_regs[i][0]);
		T27_regs[i][1] = XOR(T16_regs[i][1], T11_regs[i][1]);

		T28_regs[i] = XOR(T24_regs[i][0], T4_regs[i][0]);

		T29_regs[i][0] = XOR(T20_regs[i][0], T9_regs[i][0]);
		T29_regs[i][1] = XOR(T20_regs[i][1], T9_regs[i][1]);

		T31_regs[i][0] = XOR(T29_regs[i][0], T2_regs[i][0]);
		T31_regs[i][1] = XOR(T29_regs[i][1], T2_regs[i][1]);
	}

	//Shuffle bytes upwards due to lines 1,2,3,4,5,6 in matrix.
	for (int i = 0; i < 5; i++) {
		__mm256_insert_epi64(temp_state[i][0], _mm256_extract_epi64(state[i][1], 2), 0);
	}

	for (int i = 0; i < 5; i++) {
		temp_state[i][0] = XOR8(
			_mm256_setr_epi64x(0, _mm256_extract_epi64(state[i][0], 0),		_mm256_extract_epi64(T2_regs[i][0], 0),		_mm256_extract_epi64(T11_regs[i][0], 0)),
			_mm256_setr_epi64x(0, _mm256_extract_epi64(T2_regs[i][0], 1),	_mm256_extract_epi64(T5_regs[i], 1),		_mm256_extract_epi64(T20_regs[i][0], 1)),
			_mm256_setr_epi64x(0, _mm256_extract_epi64(T15_regs[i][0], 2),	_mm256_extract_epi64(T28_regs[i], 2),		_mm256_extract_epi64(T3_regs[i][0], 2)),
			_mm256_setr_epi64x(0, _mm256_extract_epi64(T9_regs[i][0], 3),	_mm256_extract_epi64(T29_regs[i][0], 3),	_mm256_extract_epi64(T5_regs[i], 3)),
			_mm256_setr_epi64x(0, _mm256_extract_epi64(T9_regs[i][1], 0),	_mm256_extract_epi64(T27_regs[i][1], 0),	_mm256_extract_epi64(T4_regs[i][1], 0)),
			_mm256_setr_epi64x(0, _mm256_extract_epi64(T15_regs[i][1], 1),	_mm256_extract_epi64(T23_regs[i], 1),		_mm256_extract_epi64(T29_regs[i][1], 1)),
			_mm256_setr_epi64x(0, _mm256_extract_epi64(T2_regs[i][1], 2),	_mm256_extract_epi64(T11_regs[i][1], 2),	_mm256_extract_epi64(state[i][1], 2)),
			temp_state[i][0]);

		state[i][1] = XOR7(
			_mm256_setr_epi64x(_mm256_extract_epi64(state[i][0], 0),	 _mm256_extract_epi64(T31_regs[i][0], 0),	_mm256_extract_epi64(T16_regs[i][0], 0), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T9_regs[i][0], 1),	 _mm256_extract_epi64(T26_regs[i], 1),		_mm256_extract_epi64(T26_regs[i], 1), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T27_regs[i][0], 2),  _mm256_extract_epi64(T29_regs[i][0], 2),	_mm256_extract_epi64(T17_regs[i], 2), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T10_regs[i], 3),     _mm256_extract_epi64(T7_regs[i], 3),		_mm256_extract_epi64(T25_regs[i], 3), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T12_regs[i], 0),	 _mm256_extract_epi64(T22_regs[i], 0),		_mm256_extract_epi64(T3_regs[i][1], 0), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T11_regs[i][1], 1),  _mm256_extract_epi64(T24_regs[i][1], 1),	_mm256_extract_epi64(T29_regs[i][1], 1), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T31_regs[i][1], 2),  _mm256_extract_epi64(T16_regs[i][1], 2),	_mm256_extract_epi64(T29_regs[i][1], 2), 0));

		state[i][0] = temp_state[i][0];
	}
}

void optimized_mixcolumns_A5(__m256i (*state)[2]) {
	/*
	A5:
	0       0       0       0       0       1       0
	0       0       0       0       0       0       1
	1       2       15      9       9       15      2
	2       5       28      29      27      23      11
	11      20      3       5       4       29      1
	1       9       27      10      12      11      31
	31      26      29      7       22      24      16

	*/

	__m256i T2_regs[5][2];
	__m256i T3_regs[5][2];
	__m256i T4_regs[5][2];
	__m256i T5_regs[5][2];
	__m256i T7_regs[5];
	__m256i T8_regs[5][2];
	__m256i T9_regs[5][2];
	__m256i T10_regs[5];
	__m256i T11_regs[5][2];
	__m256i T12_regs[5];
	__m256i T15_regs[5][2];
	__m256i T16_regs[5][2];
	__m256i T20_regs[5][2];
	__m256i T22_regs[5];
	__m256i T23_regs[5];
	__m256i T24_regs[5];
	__m256i T26_regs[5];
	__m256i T27_regs[5][2];
	__m256i T28_regs[5];
	__m256i T29_regs[5][2];
	__m256i T31_regs[5][2];

	__m256i temp_state[5][2];

	T2(state, T2_regs); //T2
	T2(T2_regs, T4_regs); //T4
	T2(T4_regs, T8_regs); //T8
	T2(T8_regs, T16_regs); //T16

	// T3
	for (int i = 0; i < 5; i++) {
		T3_regs[i][0] = XOR(state[i][0], T2_regs[i][0]);
		T3_regs[i][1] = XOR(state[i][1], T2_regs[i][1]);

		T5_regs[i][0] = XOR(state[i][0], T4_regs[i][0]);
		T5_regs[i][1] = XOR(state[i][1], T4_regs[i][1]);

		T7_regs[i] = XOR(T5_regs[i][0], T2_regs[i][0]);

		T9_regs[i][0] = XOR(state[i][0], T8_regs[i][0]);
		T9_regs[i][1] = XOR(state[i][1], T8_regs[i][1]);

		T10_regs[i] = XOR(T8_regs[i][0], T2_regs[i][0]);

		T11_regs[i][0] = XOR(T8_regs[i][0], T3_regs[i][0]);
		T11_regs[i][1] = XOR(T8_regs[i][1], T3_regs[i][1]);

		T12_regs[i] = XOR(T4_regs[i][1], T8_regs[i][1]);

		T15_regs[i][0] = XOR(T4_regs[i][0], T11_regs[i][0]);
		T15_regs[i][1] = XOR(T4_regs[i][1], T11_regs[i][1]);

		T20_regs[i][0] = XOR(T16_regs[i][0], T4_regs[i][0]);
		T20_regs[i][1] = XOR(T16_regs[i][1], T4_regs[i][1]);

		T22_regs[i] = XOR(T20_regs[i][1], T2_regs[i][1]);

		T23_regs[i] = XOR(T22_regs[i], state[i][1]);

		T24_regs[i] = XOR(T16_regs[i][1], T8_regs[i][1]);

		T26_regs[i] = XOR(T16_regs[i][0], T10_regs[i]);

		T27_regs[i][0] = XOR(T16_regs[i][0], T11_regs[i][0]);
		T27_regs[i][1] = XOR(T16_regs[i][1], T11_regs[i][1]);

		T28_regs[i] = XOR(T20_regs[i][0], T8_regs[i][0]);

		T29_regs[i][0] = XOR(T20_regs[i][0], T9_regs[i][0]);
		T29_regs[i][1] = XOR(T20_regs[i][1], T9_regs[i][1]);

		T31_regs[i][0] = XOR(T29_regs[i][0], T2_regs[i][0]);
		T31_regs[i][1] = XOR(T29_regs[i][1], T2_regs[i][1]);
	}

	//Shuffle bytes upwards due to lines 1,2,3,4,5,6 in matrix.
	for (int i = 0; i < 5; i++) {
		__mm256_insert_epi64(temp_state[i][0], _mm256_extract_epi64(state[i][1], 1), 0);
		__mm256_insert_epi64(temp_state[i][0], _mm256_extract_epi64(state[i][1], 2), 1);
	}

	for (int i = 0; i < 5; i++) {
		temp_state[i][0] = XOR8(
			_mm256_setr_epi64x(0, 0, _mm256_extract_epi64(state[i][0], 0),		_mm256_extract_epi64(T2_regs[i][0], 0)),
			_mm256_setr_epi64x(0, 0, _mm256_extract_epi64(T2_regs[i][0], 1),	_mm256_extract_epi64(T5_regs[i][0], 1)),
			_mm256_setr_epi64x(0, 0, _mm256_extract_epi64(T15_regs[i][0], 2),	_mm256_extract_epi64(T28_regs[i], 2)),
			_mm256_setr_epi64x(0, 0, _mm256_extract_epi64(T9_regs[i][0], 3),	_mm256_extract_epi64(T29_regs[i][0], 3)),
			_mm256_setr_epi64x(0, 0, _mm256_extract_epi64(T9_regs[i][1], 0),	_mm256_extract_epi64(T27_regs[i][1], 0)),
			_mm256_setr_epi64x(0, 0, _mm256_extract_epi64(T15_regs[i][1], 1),	_mm256_extract_epi64(T23_regs[i], 1)),
			_mm256_setr_epi64x(0, 0, _mm256_extract_epi64(T2_regs[i][1], 2),	_mm256_extract_epi64(T11_regs[i][1], 2)),
			temp_state[i][0]);

		state[i][1] = XOR7(
			_mm256_setr_epi64x(_mm256_extract_epi64(T11_regs[i][0], 0), _mm256_extract_epi64(state[i][0], 0),	 _mm256_extract_epi64(T31_regs[i][0], 0),	0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T20_regs[i][0], 1), _mm256_extract_epi64(T9_regs[i][0], 1),  _mm256_extract_epi64(T26_regs[i], 1),		0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T3_regs[i][0], 2),	_mm256_extract_epi64(T27_regs[i][0], 2), _mm256_extract_epi64(T29_regs[i][0], 2),	0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T5_regs[i][0], 3),	_mm256_extract_epi64(T10_regs[i], 3),	 _mm256_extract_epi64(T7_regs[i], 3),		0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T4_regs[i][1], 0),	_mm256_extract_epi64(T12_regs[i], 0),	 _mm256_extract_epi64(T22_regs[i], 0),		0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T29_regs[i][1], 1), _mm256_extract_epi64(T11_regs[i][1], 1), _mm256_extract_epi64(T24_regs[i], 1),		0),
			_mm256_setr_epi64x(_mm256_extract_epi64(state[i][1], 2),	_mm256_extract_epi64(T31_regs[i][1], 2), _mm256_extract_epi64(T16_regs[i][1], 2),	0));

		state[i][0] = temp_state[i][0];
	}
}

void optimized_mixcolumns_A4(__m256i (*state)[2]) {
	/*
	A4:
	0       0       0       0       1       0       0
	0       0       0       0       0       1       0
	0       0       0       0       0       0       1
	1       2       15      9       9       15      2
	2       5       28      29      27      23      11
	11      20      3       5       4       29      1
	1       9       27      10      12      11      31
	*/

	__m256i T2_regs[5][2];
	__m256i T3_regs[5][2];
	__m256i T4_regs[5][2];
	__m256i T5_regs[5];
	__m256i T8_regs[5][2];
	__m256i T9_regs[5][2];
	__m256i T10_regs[5];
	__m256i T11_regs[5][2];
	__m256i T12_regs[5];
	__m256i T15_regs[5][2];
	__m256i T16_regs[5][2];
	__m256i T20_regs[5][2];
	__m256i T23_regs[5];
	__m256i T27_regs[5][2];
	__m256i T28_regs[5];
	__m256i T29_regs[5][2];
	__m256i T31_regs[5];

	__m256i temp_state[5][2];

	T2(state, T2_regs); //T2
	T2(T2_regs, T4_regs); //T4
	T2(T4_regs, T8_regs); //T8
	T2(T8_regs, T16_regs); //T16

						   //T3
	for (int i = 0; i < 5; i++) {
		T3_regs[i][0] = XOR(state[i][0], T2_regs[i][0]);
		T3_regs[i][1] = XOR(state[i][1], T2_regs[i][1]);

		T5_regs[i] = XOR(state[i][0], T4_regs[i][0]);

		T9_regs[i][0] = XOR(state[i][0], T8_regs[i][0]);
		T9_regs[i][1] = XOR(state[i][1], T8_regs[i][1]);

		T10_regs[i] = XOR(T8_regs[i][0], T2_regs[i][0]);

		T11_regs[i][0] = XOR(T8_regs[i][0], T3_regs[i][0]);
		T11_regs[i][1] = XOR(T8_regs[i][1], T3_regs[i][1]);

		T12_regs[i] = XOR(T4_regs[i][1], T8_regs[i][1]);

		T15_regs[i][0] = XOR(T4_regs[i][0], T11_regs[i][0]);
		T15_regs[i][1] = XOR(T4_regs[i][1], T11_regs[i][1]);

		T20_regs[i][0] = XOR(T16_regs[i][0], T4_regs[i][0]);
		T20_regs[i][1] = XOR(T16_regs[i][1], T4_regs[i][1]);

		T23_regs[i] = XOR(T20_regs[i][1], T3_regs[i][1]);

		T27_regs[i][0] = XOR(T16_regs[i][0], T11_regs[i][0]);
		T27_regs[i][1] = XOR(T16_regs[i][1], T11_regs[i][1]);

		T28_regs[i] = XOR(T20_regs[i][0], T8_regs[i][0]);

		T29_regs[i][0] = XOR(T20_regs[i][0], T9_regs[i][0]);
		T29_regs[i][1] = XOR(T20_regs[i][1], T9_regs[i][1]);

		T31_regs[i] = XOR(T29_regs[i][1], T2_regs[i][1]);
	}

	//Shuffle bytes upwards due to lines 1,2,3,4,5,6 in matrix.
	for (int i = 0; i < 5; i++) {
		__mm256_insert_epi64(temp_state[i][0], _mm256_extract_epi64(state[i][1], 0), 0);
		__mm256_insert_epi64(temp_state[i][0], _mm256_extract_epi64(state[i][1], 1), 1);
		__mm256_insert_epi64(temp_state[i][0], _mm256_extract_epi64(state[i][1], 2), 2);
	}

	for (int i = 0; i < 5; i++) {
		temp_state[i][0] = XOR8(
			_mm256_setr_epi64x(0, 0, 0, _mm256_extract_epi64(state[i][0], 0)),
			_mm256_setr_epi64x(0, 0, 0, _mm256_extract_epi64(T2_regs[i][0], 1)),
			_mm256_setr_epi64x(0, 0, 0, _mm256_extract_epi64(T15_regs[i][0], 2)),
			_mm256_setr_epi64x(0, 0, 0, _mm256_extract_epi64(T9_regs[i][0], 3)),
			_mm256_setr_epi64x(0, 0, 0, _mm256_extract_epi64(T9_regs[i][1], 0)),
			_mm256_setr_epi64x(0, 0, 0, _mm256_extract_epi64(T15_regs[i][1], 1)),
			_mm256_setr_epi64x(0, 0, 0, _mm256_extract_epi64(T2_regs[i][1], 2)),
			temp_state[i][0]);

		state[i][1] = XOR7(
			_mm256_setr_epi64x(_mm256_extract_epi64(T2_regs[i][0], 0),  _mm256_extract_epi64(T11_regs[i][0], 0), _mm256_extract_epi64(state[i][0], 0), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T5_regs[i], 1),		_mm256_extract_epi64(T20_regs[i][0], 1), _mm256_extract_epi64(T9_regs[i][0], 1), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T28_regs[i], 2),	_mm256_extract_epi64(T3_regs[i][0], 2),  _mm256_extract_epi64(T27_regs[i][0], 2), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T29_regs[i][0], 3), _mm256_extract_epi64(T5_regs[i], 3),	 _mm256_extract_epi64(T10_regs[i], 3), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T27_regs[i][1], 0), _mm256_extract_epi64(T4_regs[i][1], 0),  _mm256_extract_epi64(T12_regs[i], 0), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T23_regs[i], 1),	_mm256_extract_epi64(T29_regs[i][1], 1), _mm256_extract_epi64(T11_regs[i][1], 1), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T11_regs[i][1], 2), _mm256_extract_epi64(state[i][1], 2),	 _mm256_extract_epi64(T31_regs[i], 2), 0));

		state[i][0] = temp_state[i][0];
	}
}

void optimized_mixcolumns_A3(__m256i (*state)[2]) {
	/*
	A3:
	0       0       0       1       0       0       0
	0       0       0       0       1       0       0
	0       0       0       0       0       1       0
	0       0       0       0       0       0       1
	1       2       15      9       9       15      2
	2       5       28      29      27      23      11
	11      20      3       5       4       29      1
	*/

	__m256i T2_regs[5][2];
	__m256i T3_regs[5][2];
	__m256i T4_regs[5][2];
	__m256i T5_regs[5];
	__m256i T8_regs[5][2];
	__m256i T9_regs[5][2];
	__m256i T11_regs[5][2];
	__m256i T15_regs[5][2];
	__m256i T16_regs[5][2];
	__m256i T20_regs[5][2];
	__m256i T23_regs[5];
	__m256i T27_regs[5];
	__m256i T28_regs[5];
	__m256i T29_regs[5][2];

	__m256i temp_state[5][2];

	T2(state, T2_regs); //T2
	T2(T2_regs, T4_regs); //T4
	T2(T4_regs, T8_regs); //T8
	T2(T8_regs, T16_regs); //T16

	for (int i = 0; i < 5; i++) {
		T3_regs[i][0] = XOR(state[i][0], T2_regs[i][0]);
		T3_regs[i][1] = XOR(state[i][1], T2_regs[i][1]);

		T5_regs[i] = XOR(state[i][0], T4_regs[i][0]);

		T9_regs[i][0] = XOR(state[i][0], T8_regs[i][0]);
		T9_regs[i][1] = XOR(state[i][1], T8_regs[i][1]);

		T11_regs[i][0] = XOR(T9_regs[i][0], T2_regs[i][0]);
		T11_regs[i][1] = XOR(T9_regs[i][1], T2_regs[i][1]);

		T15_regs[i][0] = XOR(T4_regs[i][0], T11_regs[i][0]);
		T15_regs[i][1] = XOR(T4_regs[i][1], T11_regs[i][1]);

		T20_regs[i][0] = XOR(T16_regs[i][0], T4_regs[i][0]);
		T20_regs[i][1] = XOR(T16_regs[i][1], T4_regs[i][1]);

		T23_regs[i] = XOR(T20_regs[i][1], T3_regs[i][1]);

		T27_regs[i] = XOR(T16_regs[i][1], T11_regs[i][1]);

		T28_regs[i] = XOR(T20_regs[i][0], T8_regs[i][0]);

		T29_regs[i][0] = XOR(T20_regs[i][0], T9_regs[i][0]);
		T29_regs[i][1] = XOR(T20_regs[i][1], T9_regs[i][1]);
	}

	//Shuffle bytes upwards due to lines 1,2,3,4,5,6 in matrix.
	for (int i = 0; i < 5; i++) {
		__mm256_insert_epi64(temp_state[i][0], _mm256_extract_epi64(state[i][0], 3), 0);
		__mm256_insert_epi64(temp_state[i][0], _mm256_extract_epi64(state[i][1], 0), 1);
		__mm256_insert_epi64(temp_state[i][0], _mm256_extract_epi64(state[i][1], 1), 2);
		__mm256_insert_epi64(temp_state[i][0], _mm256_extract_epi64(state[i][1], 2), 3);
	}

	for (int i = 0; i < 5; i++) {
		state[i][1] = XOR7(
			_mm256_setr_epi64x(_mm256_extract_epi64(state[i][0], 0),	_mm256_extract_epi64(T2_regs[i][0], 0),  _mm256_extract_epi64(T11_regs[i][0], 0), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T2_regs[i][0], 1),	_mm256_extract_epi64(T5_regs[i], 1),	 _mm256_extract_epi64(T20_regs[i][0], 1), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T15_regs[i][0], 2), _mm256_extract_epi64(T28_regs[i], 2),	 _mm256_extract_epi64(T3_regs[i][0], 2), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T9_regs[i][0], 3),	_mm256_extract_epi64(T29_regs[i][0], 3), _mm256_extract_epi64(T5_regs[i], 3), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T9_regs[i][1], 0),	_mm256_extract_epi64(T27_regs[i], 0),	 _mm256_extract_epi64(T4_regs[i][1], 0), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T15_regs[i][1], 1), _mm256_extract_epi64(T23_regs[i], 1),	 _mm256_extract_epi64(T29_regs[i][1], 1), 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T2_regs[i][1], 2),	_mm256_extract_epi64(T11_regs[i][1], 2), _mm256_extract_epi64(state[i][1], 2), 0));

		state[i][0] = temp_state[i][0];
	}
}

void optimized_mixcolumns_A2(__m256i (*state)[2]) {
	/*0       0       1       0       0       0       0
	0       0       0       1       0       0       0
	0       0       0       0       1       0       0
	0       0       0       0       0       1       0
	0       0       0       0       0       0       1
	1       2       15      9       9       15      2
	2       5       28      29      27      23      11*/

	__m256i T2_regs[5][2];
	__m256i T4_regs[5][2];
	__m256i T5_regs[5][2];
	__m256i T8_regs[5][2];
	__m256i T9_regs[5][2];
	__m256i T11_regs[5];
	__m256i T15_regs[5][2];
	__m256i T16_regs[5][2];
	__m256i T23_regs[5];
	__m256i T27_regs[5];
	__m256i T28_regs[5];
	__m256i T29_regs[5];

	__m256i temp_state[5][2];

	T2(state, T2_regs); //T2
	T2(T2_regs, T4_regs); //T4
	T2(T4_regs, T8_regs); //T8
	T2(T8_regs, T16_regs); //T16

	//T5
	for (int i = 0; i < 5; i++) {
		T5_regs[i][0] = XOR(state[i][0], T4_regs[i][0]);
		T5_regs[i][1] = XOR(state[i][1], T4_regs[i][1]);

		T9_regs[i][0] = XOR(state[i][0], T8_regs[i][0]);
		T9_regs[i][1] = XOR(state[i][1], T8_regs[i][1]);

		T11_regs[i] = XOR(T9_regs[i][1], T2_regs[i][1]);

		T15_regs[i][0] = XOR3(T2_regs[i][0], T4_regs[i][0], T9_regs[i][0]);
		T15_regs[i][1] = XOR(T4_regs[i][1], T11_regs[i]);

		T23_regs[i] = XOR3(T16_regs[i][1], T5_regs[i][1], T2_regs[i][1]);

		T27_regs[i] = XOR(T16_regs[i][1], T11_regs[i]);

		T28_regs[i] = XOR3(T16_regs[i][0], T8_regs[i][0], T4_regs[i][0]);

		T29_regs[i] = XOR(T28_regs[i], state[i][0]);
	}

	//Shuffle bytes upwards due to lines 1,2,3,4,5,6 in matrix.
	for (int i = 0; i < 5; i++) {
		__mm256_insert_epi64(temp_state[i][0], _mm256_extract_epi64(state[i][0], 2), 0);
		__mm256_insert_epi64(temp_state[i][0], _mm256_extract_epi64(state[i][0], 3), 1);
		__mm256_insert_epi64(temp_state[i][0], _mm256_extract_epi64(state[i][1], 0), 2);
		__mm256_insert_epi64(temp_state[i][0], _mm256_extract_epi64(state[i][1], 1), 3);

		__mm256_insert_epi64(temp_state[i][1], _mm256_extract_epi64(state[i][1], 2), 0);
	}

	for (int i = 0; i < 5; i++) {
		state[i][1] = XOR8(
			_mm256_setr_epi64x(0, _mm256_extract_epi64(state[i][0], 0),		_mm256_extract_epi64(T2_regs[i][0], 0), 0),
			_mm256_setr_epi64x(0, _mm256_extract_epi64(T2_regs[i][0], 1),	_mm256_extract_epi64(T5_regs[i][0], 1), 0),
			_mm256_setr_epi64x(0, _mm256_extract_epi64(T15_regs[i][0], 2),	_mm256_extract_epi64(T28_regs[i], 2), 0),
			_mm256_setr_epi64x(0, _mm256_extract_epi64(T9_regs[i][0], 3),	_mm256_extract_epi64(T29_regs[i], 3), 0),
			_mm256_setr_epi64x(0, _mm256_extract_epi64(T9_regs[i][1], 0),	_mm256_extract_epi64(T27_regs[i], 0), 0),
			_mm256_setr_epi64x(0, _mm256_extract_epi64(T15_regs[i][1], 1),	_mm256_extract_epi64(T23_regs[i], 1), 0),
			_mm256_setr_epi64x(0, _mm256_extract_epi64(T2_regs[i][1], 2),	_mm256_extract_epi64(T11_regs[i], 2), 0),
			temp_state[i][1]);

		state[i][0] = temp_state[i][0];
	}
}


//*************** 80 bit ***************
void mixcolumns_A5_80bit(__m256i (*state)[2]) {
	/*
	1       18      2       2       18
	18      8       19      3       11
	11      5       30      5       20
	20      1       8       19      15
	15      1       31      22      6
	*/

	__m256i T2_regs[5][2];
	__m256i T3_regs[5][2];
	__m256i T4_regs[5][2];
	__m256i T5_regs[5][2];
	__m256i T6_regs[5][2];
	__m256i T8_regs[5][2];
	__m256i T11_regs[5][2];
	__m256i T15_regs[5][2];
	__m256i T16_regs[5][2];
	__m256i T18_regs[5][2];
	__m256i T19_regs[5][2];
	__m256i T20_regs[5][2];
	__m256i T22_regs[5][2];
	__m256i T30_regs[5][2];
	__m256i T31_regs[5][2];

	T2(state, T2_regs); //T2
	T2(T2_regs, T4_regs); //T4
	T2(T4_regs, T8_regs); //T8
	T2(T8_regs, T16_regs); //T16
				   
	for (int i = 0; i < 5; i++) {
		T3_regs[i][0] = XOR(state[i][0], T2_regs[i][0]);
		T3_regs[i][1] = XOR(state[i][1], T2_regs[i][1]);

		T5_regs[i][0] = XOR(state[i][0], T4_regs[i][0]);
		T5_regs[i][1] = XOR(state[i][1], T4_regs[i][1]);

		T6_regs[i][0] = XOR(T2_regs[i][0], T4_regs[i][0]);
		T6_regs[i][1] = XOR(T2_regs[i][1], T4_regs[i][1]);

		T11_regs[i][0] = XOR(T3_regs[i][0], T8_regs[i][0]);
		T11_regs[i][1] = XOR(T3_regs[i][1], T8_regs[i][1]);

		T15_regs[i][0] = XOR(T11_regs[i][0], T4_regs[i][0]);
		T15_regs[i][1] = XOR(T11_regs[i][1], T4_regs[i][1]);

		T18_regs[i][0] = XOR(T16_regs[i][0], T2_regs[i][0]);
		T18_regs[i][1] = XOR(T16_regs[i][1], T2_regs[i][1]);

		T19_regs[i][0] = XOR(T16_regs[i][0], T3_regs[i][0]);
		T19_regs[i][1] = XOR(T16_regs[i][1], T3_regs[i][1]);

		T20_regs[i][0] = XOR(T16_regs[i][0], T4_regs[i][0]);
		T20_regs[i][1] = XOR(T16_regs[i][1], T4_regs[i][1]);

		T22_regs[i][0] = XOR(T20_regs[i][0], T2_regs[i][0]);
		T22_regs[i][1] = XOR(T20_regs[i][1], T2_regs[i][1]);

		T30_regs[i][0] = XOR(T22_regs[i][0], T8_regs[i][0]);
		T30_regs[i][1] = XOR(T22_regs[i][1], T8_regs[i][1]);

		T31_regs[i][0] = XOR(T30_regs[i][0], state[i][0]);
		T31_regs[i][1] = XOR(T30_regs[i][1], state[i][1]);
	}

	//15      1       31      22      6
	for (int i = 0; i < 5; i++) {
		state[i][1] = XOR5(
			_mm256_setr_epi64x(_mm256_extract_epi64(T15_regs[i][0], 0), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(state[i][0], 1), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T31_regs[i][0], 2), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T22_regs[i][0], 3), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T6_regs[i][1], 0), 0, 0, 0));

		state[i][0] = XOR5(
			_mm256_setr_epi64x(_mm256_extract_epi64(state[i][0], 0), _mm256_extract_epi64(T18_regs[i][0], 0), _mm256_extract_epi64(T11_regs[i][0], 0), _mm256_extract_epi64(T20_regs[i][0], 0)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T18_regs[i][0], 1), _mm256_extract_epi64(T8_regs[i][0], 1), _mm256_extract_epi64(T5_regs[i][0], 1), _mm256_extract_epi64(state[i][0], 1)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T2_regs[i][0], 2), _mm256_extract_epi64(T19_regs[i][0], 2), _mm256_extract_epi64(T30_regs[i][0], 2), _mm256_extract_epi64(T8_regs[i][0], 2)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T2_regs[i][0], 3), _mm256_extract_epi64(T3_regs[i][0], 3), _mm256_extract_epi64(T5_regs[i][0], 3), _mm256_extract_epi64(T19_regs[i][0], 3)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T18_regs[i][1], 0), _mm256_extract_epi64(T11_regs[i][1], 0), _mm256_extract_epi64(T20_regs[i][1], 0), _mm256_extract_epi64(T15_regs[i][1], 0)));
	}
}

void mixcolumns_A4_80bit(__m256i (*state)[2]) {
	/*
	0       0       0       0       1
	1       18      2       2       18
	18      8       19      3       11
	11      5       30      5       20
	20      1       8       19      15
	*/

	__m256i T2_regs[5][2];
	__m256i T3_regs[5][2];
	__m256i T4_regs[5][2];
	__m256i T5_regs[5][2];
	__m256i T8_regs[5][2];
	__m256i T11_regs[5][2];
	__m256i T15_regs[5][2];
	__m256i T16_regs[5][2];
	__m256i T18_regs[5][2];
	__m256i T19_regs[5][2];
	__m256i T20_regs[5][2];
	__m256i T30_regs[5][2];
	__m256i temp_state[5];

	T2(state, T2_regs); //T2
	T2(T2_regs, T4_regs); //T4
	T2(T4_regs, T8_regs); //T8
	T2(T8_regs, T16_regs); //T16

	for (int i = 0; i < 5; i++) {
		T3_regs[i][0] = XOR(state[i][0], T2_regs[i][0]);
		T3_regs[i][1] = XOR(state[i][1], T2_regs[i][1]);

		T5_regs[i][0] = XOR(state[i][0], T4_regs[i][0]);
		T5_regs[i][1] = XOR(state[i][1], T4_regs[i][1]);

		T11_regs[i][0] = XOR(T3_regs[i][0], T8_regs[i][0]);
		T11_regs[i][1] = XOR(T3_regs[i][1], T8_regs[i][1]);

		T15_regs[i][0] = XOR(T4_regs[i][0], T11_regs[i][0]);
		T15_regs[i][1] = XOR(T4_regs[i][1], T11_regs[i][1]);

		T18_regs[i][0] = XOR(T16_regs[i][0], T2_regs[i][0]);
		T18_regs[i][1] = XOR(T16_regs[i][1], T2_regs[i][1]);

		T19_regs[i][0] = XOR(T16_regs[i][0], T3_regs[i][0]);
		T19_regs[i][1] = XOR(T16_regs[i][1], T3_regs[i][1]);

		T20_regs[i][0] = XOR(T16_regs[i][0], T4_regs[i][0]);
		T20_regs[i][1] = XOR(T16_regs[i][1], T4_regs[i][1]);

		T30_regs[i][0] = XOR3(T20_regs[i][0], T8_regs[i][0], T2_regs[i][0]);
		T30_regs[i][1] = XOR3(T20_regs[i][1], T8_regs[i][1], T2_regs[i][1]);
	}

	//Shuffle bytes upwards due to lines 1,2,3,4,5,6 in matrix.
	for (int i = 0; i < 5; i++) {
		__mm256_insert_epi64(temp_state[i], _mm256_extract_epi64(state[i][1], 0), 0);
		__mm256_insert_epi64(temp_state[i], 0, 1);
		__mm256_insert_epi64(temp_state[i], 0, 2);
		__mm256_insert_epi64(temp_state[i], 0, 3);
	}

	for (int i = 0; i < 5; i++) {
		state[i][1] = XOR5(
			_mm256_setr_epi64x(_mm256_extract_epi64(T20_regs[i][0], 0), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(state[i][0], 1), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T8_regs[i][0], 2), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T19_regs[i][0], 3), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T15_regs[i][1], 0), 0, 0, 0));

		state[i][0] = XOR6(
			_mm256_setr_epi64x(0, _mm256_extract_epi64(state[i][0], 0), _mm256_extract_epi64(T18_regs[i][0], 0), _mm256_extract_epi64(T11_regs[i][0], 0)),
			_mm256_setr_epi64x(0, _mm256_extract_epi64(T18_regs[i][0], 1), _mm256_extract_epi64(T8_regs[i][0], 1), _mm256_extract_epi64(T5_regs[i][0], 1)),
			_mm256_setr_epi64x(0, _mm256_extract_epi64(T2_regs[i][0], 2), _mm256_extract_epi64(T19_regs[i][0], 2), _mm256_extract_epi64(T30_regs[i][0], 2)),
			_mm256_setr_epi64x(0, _mm256_extract_epi64(T2_regs[i][0], 3), _mm256_extract_epi64(T3_regs[i][0], 3), _mm256_extract_epi64(T5_regs[i][0], 3)),
			_mm256_setr_epi64x(0, _mm256_extract_epi64(T18_regs[i][1], 0), _mm256_extract_epi64(T11_regs[i][1], 0), _mm256_extract_epi64(T20_regs[i][1], 0)),
			temp_state[i]);
	}
}

void mixcolumns_A3_80bit(__m256i (*state)[2]) {
	/*
	0       0       0       1       0
	0       0       0       0       1
	1       18      2       2       18
	18      8       19      3       11
	11      5       30      5       20
	*/

	__m256i T2_regs[5][2];
	__m256i T3_regs[5][2];
	__m256i T4_regs[5][2];
	__m256i T5_regs[5][2];
	__m256i T8_regs[5][2];
	__m256i T11_regs[5][2];
	__m256i T16_regs[5][2];
	__m256i T18_regs[5][2];
	__m256i T19_regs[5][2];
	__m256i T20_regs[5][2];
	__m256i T30_regs[5][2];
	__m256i temp_state[5];

	T2(state, T2_regs); //T2
	T2(T2_regs, T4_regs); //T4
	T2(T4_regs, T8_regs); //T8
	T2(T8_regs, T16_regs); //T16

	for (int i = 0; i < 5; i++) {
		T3_regs[i][0] = XOR(state[i][0], T2_regs[i][0]);
		T3_regs[i][1] = XOR(state[i][1], T2_regs[i][1]);

		T5_regs[i][0] = XOR(state[i][0], T4_regs[i][0]);
		T5_regs[i][1] = XOR(state[i][1], T4_regs[i][1]);

		T11_regs[i][0] = XOR(T3_regs[i][0], T8_regs[i][0]);
		T11_regs[i][1] = XOR(T3_regs[i][1], T8_regs[i][1]);

		T18_regs[i][0] = XOR(T16_regs[i][0], T2_regs[i][0]);
		T18_regs[i][1] = XOR(T16_regs[i][1], T2_regs[i][1]);

		T19_regs[i][0] = XOR(T16_regs[i][0], T3_regs[i][0]);
		T19_regs[i][1] = XOR(T16_regs[i][1], T3_regs[i][1]);

		T20_regs[i][0] = XOR(T16_regs[i][0], T4_regs[i][0]);
		T20_regs[i][1] = XOR(T16_regs[i][1], T4_regs[i][1]);

		T30_regs[i][0] = XOR3(T20_regs[i][0], T8_regs[i][0], T2_regs[i][0]);
		T30_regs[i][1] = XOR3(T20_regs[i][1], T8_regs[i][1], T2_regs[i][1]);
	}

	//Shuffle bytes upwards due to lines 1,2,3,4,5,6 in matrix.
	for (int i = 0; i < 5; i++) {
		__mm256_insert_epi64(temp_state[i], _mm256_extract_epi64(state[i][0], 3), 0);
		__mm256_insert_epi64(temp_state[i], _mm256_extract_epi64(state[i][1], 0), 1);
		__mm256_insert_epi64(temp_state[i], 0, 2);
		__mm256_insert_epi64(temp_state[i], 0, 3);
	}

	for (int i = 0; i < 5; i++) {
		state[i][0] = XOR6(
			_mm256_setr_epi64x(0, 0, _mm256_extract_epi64(state[i][0], 0), _mm256_extract_epi64(T18_regs[i][0], 0)),
			_mm256_setr_epi64x(0, 0, _mm256_extract_epi64(T18_regs[i][0], 1), _mm256_extract_epi64(T8_regs[i][0], 1)),
			_mm256_setr_epi64x(0, 0, _mm256_extract_epi64(T2_regs[i][0], 2), _mm256_extract_epi64(T19_regs[i][0], 2)),
			_mm256_setr_epi64x(0, 0, _mm256_extract_epi64(T2_regs[i][0], 3), _mm256_extract_epi64(T3_regs[i][0], 3)),
			_mm256_setr_epi64x(0, 0, _mm256_extract_epi64(T18_regs[i][1], 0), _mm256_extract_epi64(T11_regs[i][1], 0)),
			temp_state[i]);
			
		state[i][1] = XOR5(
			_mm256_setr_epi64x(_mm256_extract_epi64(T11_regs[i][0], 0), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T5_regs[i][0], 1),	0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T30_regs[i][0], 2),	0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T5_regs[i][0], 3),	0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T20_regs[i][1], 0), 0, 0, 0));
	}
}

void mixcolumns_A2_80bit(__m256i (*state)[2]) {
	/*
	0       0       1       0       0
	0       0       0       1       0
	0       0       0       0       1
	1       18      2       2       18
	18      8       19      3       11
	*/

	__m256i T2_regs[5][2];
	__m256i T3_regs[5][2];
	__m256i T4_regs[5][2];
	__m256i T8_regs[5][2];
	__m256i T11_regs[5][2];
	__m256i T16_regs[5][2];
	__m256i T18_regs[5][2];
	__m256i T19_regs[5][2];
	__m256i temp_state[5];

	T2(state, T2_regs); //T2
	T2(T2_regs, T4_regs); //T4
	T2(T4_regs, T8_regs); //T8
	T2(T8_regs, T16_regs); //T16

	for (int i = 0; i < 5; i++) {
		T3_regs[i][0] = XOR(state[i][0], T2_regs[i][0]);
		T3_regs[i][1] = XOR(state[i][1], T2_regs[i][1]);

		T11_regs[i][0] = XOR(T3_regs[i][0], T8_regs[i][0]);
		T11_regs[i][1] = XOR(T3_regs[i][1], T8_regs[i][1]);

		T18_regs[i][0] = XOR(T16_regs[i][0], T2_regs[i][0]);
		T18_regs[i][1] = XOR(T16_regs[i][1], T2_regs[i][1]);

		T19_regs[i][0] = XOR(T16_regs[i][0], T3_regs[i][0]);
		T19_regs[i][1] = XOR(T16_regs[i][1], T3_regs[i][1]);
	}

	//Shuffle bytes upwards due to lines 1,2,3,4,5,6 in matrix.
	for (int i = 0; i < 5; i++) {
		__mm256_insert_epi64(temp_state[i], _mm256_extract_epi64(state[i][0], 2), 0);
		__mm256_insert_epi64(temp_state[i], _mm256_extract_epi64(state[i][0], 3), 1);
		__mm256_insert_epi64(temp_state[i], _mm256_extract_epi64(state[i][1], 0), 2);
		__mm256_insert_epi64(temp_state[i], 0, 3);
	}

	//handle 2 last rows
	for (int i = 0; i < 5; i++) {
		state[i][0] = XOR6(
			_mm256_setr_epi64x(0, 0, 0, _mm256_extract_epi64(state[i][0], 0)),
			_mm256_setr_epi64x(0, 0, 0, _mm256_extract_epi64(T18_regs[i][0], 1)),
			_mm256_setr_epi64x(0, 0, 0, _mm256_extract_epi64(T2_regs[i][0], 2)),
			_mm256_setr_epi64x(0, 0, 0, _mm256_extract_epi64(T2_regs[i][0], 3)),
			_mm256_setr_epi64x(0, 0, 0, _mm256_extract_epi64(T18_regs[i][1], 0)),
			temp_state[i]);
			
		state[i][1] = XOR5(
			_mm256_setr_epi64x(_mm256_extract_epi64(T18_regs[i][0], 0), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T8_regs[i][0], 1), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T19_regs[i][0], 2), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T3_regs[i][0], 3), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T11_regs[i][1], 0), 0, 0, 0));
			
	}
}

void mixcolumns_A1_80bit(__m256i (*state)[2]) {
	__m256i T2_regs[5][2];
	__m256i T4_regs[5][2];
	__m256i T8_regs[5][2];
	__m256i T16_regs[5][2];
	__m256i T18_regs[5][2];
	__m256i temp_state[5];

	T2(state, T2_regs); //T2
	T2(T2_regs, T4_regs); //T4
	T2(T4_regs, T8_regs); //T8
	T2(T8_regs, T16_regs); //T16

	//T18
	for (int i = 0; i < 5; i++) {
		T18_regs[i][0] = XOR(T16_regs[i][0], T2_regs[i][0]);
		T18_regs[i][1] = XOR(T16_regs[i][1], T2_regs[i][1]);
	}

	//Shuffle bytes upwards due to lines 1,2,3,4 in matrix.
	for (int i = 0; i < 5; i++) {
		__mm256_insert_epi64(temp_state[i], _mm256_extract_epi64(state[i][0], 1), 0);
		__mm256_insert_epi64(temp_state[i], _mm256_extract_epi64(state[i][0], 2), 1);
		__mm256_insert_epi64(temp_state[i], _mm256_extract_epi64(state[i][0], 3), 2);
		__mm256_insert_epi64(temp_state[i], _mm256_extract_epi64(state[i][1], 0), 3);
	}

	//[1, 18, 2, 2, 18]
	for (int i = 0; i < 5; i++) {
		state[i][1] = XOR5(
			_mm256_setr_epi64x(_mm256_extract_epi64(state[i][0], 0), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T18_regs[i][0], 1), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T2_regs[i][0], 2), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T2_regs[i][0], 3), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T18_regs[i][1], 0), 0, 0, 0));

		state[i][0] = temp_state[i];
	}
}


void optimized_mixcolumns_A5_80bit(__m256i (*state)[2]) {
	/*
	1       18      2       2       18
	18      8       19      3       11
	11      5       30      5       20
	20      1       8       19      15
	15      1       31      22      6
	*/

	__m256i T2_regs[5][2];
	__m256i T3_regs[5][2];
	__m256i T4_regs[5][2];
	__m256i T5_regs[5];
	__m256i T6_regs[5];
	__m256i T8_regs[5][2];
	__m256i T11_regs[5][2];
	__m256i T15_regs[5][2];
	__m256i T16_regs[5][2];
	__m256i T18_regs[5][2];
	__m256i T19_regs[5];
	__m256i T20_regs[5][2];
	__m256i T22_regs[5];
	__m256i T30_regs[5];
	__m256i T31_regs[5];

	T2(state, T2_regs); //T2
	T2(T2_regs, T4_regs); //T4
	T2(T4_regs, T8_regs); //T8
	T2(T8_regs, T16_regs); //T16

	for (int i = 0; i < 5; i++) {
		T3_regs[i][0] = XOR(state[i][0], T2_regs[i][0]);
		T3_regs[i][1] = XOR(state[i][1], T2_regs[i][1]);

		T5_regs[i] = XOR(state[i][0], T4_regs[i][0]);

		T6_regs[i] = XOR(T2_regs[i][1], T4_regs[i][1]);

		T11_regs[i][0] = XOR(T3_regs[i][0], T8_regs[i][0]);
		T11_regs[i][1] = XOR(T3_regs[i][1], T8_regs[i][1]);

		T15_regs[i][0] = XOR(T4_regs[i][0], T11_regs[i][0]);
		T15_regs[i][1] = XOR(T4_regs[i][1], T11_regs[i][1]);

		T18_regs[i][0] = XOR(T16_regs[i][0], T2_regs[i][0]);
		T18_regs[i][1] = XOR(T16_regs[i][1], T2_regs[i][1]);

		T19_regs[i] = XOR(T18_regs[i][0], state[i][0]);

		T20_regs[i][0] = XOR(T16_regs[i][0], T4_regs[i][0]);
		T20_regs[i][1] = XOR(T16_regs[i][1], T4_regs[i][1]);

		T22_regs[i] = XOR(T20_regs[i][0], T2_regs[i][0]);

		T30_regs[i] = XOR(T22_regs[i], T8_regs[i][0]);

		T31_regs[i] = XOR(T30_regs[i], state[i][0]);
	}

	//15      1       31      22      6
	for (int i = 0; i < 5; i++) {

		state[i][1] = XOR5(
			_mm256_setr_epi64x(_mm256_extract_epi64(T15_regs[i][0], 0), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(state[i][0], 1), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T31_regs[i], 2), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T22_regs[i], 3), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T6_regs[i], 0), 0, 0, 0));

		state[i][0] = XOR5(
			_mm256_setr_epi64x(_mm256_extract_epi64(state[i][0], 0), _mm256_extract_epi64(T18_regs[i][0], 0), _mm256_extract_epi64(T11_regs[i][0], 0), _mm256_extract_epi64(T20_regs[i][0], 0)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T18_regs[i][0], 1), _mm256_extract_epi64(T8_regs[i][0], 1), _mm256_extract_epi64(T5_regs[i], 1), _mm256_extract_epi64(state[i][0], 1)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T2_regs[i][0], 2), _mm256_extract_epi64(T19_regs[i], 2), _mm256_extract_epi64(T30_regs[i], 2), _mm256_extract_epi64(T8_regs[i][0], 2)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T2_regs[i][0], 3), _mm256_extract_epi64(T3_regs[i][0], 3), _mm256_extract_epi64(T5_regs[i], 3), _mm256_extract_epi64(T19_regs[i], 3)),
			_mm256_setr_epi64x(_mm256_extract_epi64(T18_regs[i][1], 0), _mm256_extract_epi64(T11_regs[i][1], 0), _mm256_extract_epi64(T20_regs[i][1], 0), _mm256_extract_epi64(T15_regs[i][1], 0)));
	}
}

void optimized_mixcolumns_A4_80bit(__m256i (*state)[2]) {
	/*
	0       0       0       0       1
	1       18      2       2       18
	18      8       19      3       11
	11      5       30      5       20
	20      1       8       19      15
	*/

	__m256i T2_regs[5][2];
	__m256i T3_regs[5][2];
	__m256i T4_regs[5][2];
	__m256i T5_regs[5];
	__m256i T8_regs[5][2];
	__m256i T11_regs[5][2];
	__m256i T15_regs[5];
	__m256i T16_regs[5][2];
	__m256i T18_regs[5][2];
	__m256i T19_regs[5];
	__m256i T20_regs[5][2];
	__m256i T30_regs[5];
	__m256i temp_state[5];

	T2(state, T2_regs); //T2
	T2(T2_regs, T4_regs); //T4
	T2(T4_regs, T8_regs); //T8
	T2(T8_regs, T16_regs); //T16

	for (int i = 0; i < 5; i++) {
		T3_regs[i][0] = XOR(state[i][0], T2_regs[i][0]);
		T3_regs[i][1] = XOR(state[i][1], T2_regs[i][1]);

		T5_regs[i] = XOR(state[i][0], T4_regs[i][0]);

		T11_regs[i][0] = XOR(T3_regs[i][0], T8_regs[i][0]);
		T11_regs[i][1] = XOR(T3_regs[i][1], T8_regs[i][1]);

		T15_regs[i] = XOR(T4_regs[i][1], T11_regs[i][1]);

		T18_regs[i][0] = XOR(T16_regs[i][0], T2_regs[i][0]);
		T18_regs[i][1] = XOR(T16_regs[i][1], T2_regs[i][1]);

		T19_regs[i] = XOR(T16_regs[i][0], T3_regs[i][0]);

		T20_regs[i][0] = XOR(T16_regs[i][0], T4_regs[i][0]);
		T20_regs[i][1] = XOR(T16_regs[i][1], T4_regs[i][1]);

		T30_regs[i] = XOR3(T20_regs[i][0], T4_regs[i][0], T2_regs[i][0]);
	}

	//Shuffle bytes upwards due to lines 1,2,3,4,5,6 in matrix.
	for (int i = 0; i < 5; i++) {
		__mm256_insert_epi64(temp_state[i], _mm256_extract_epi64(state[i][1], 0), 0);
		__mm256_insert_epi64(temp_state[i], 0, 1);
		__mm256_insert_epi64(temp_state[i], 0, 2);
		__mm256_insert_epi64(temp_state[i], 0, 3);
	}

	for (int i = 0; i < 5; i++) {
		state[i][1] = XOR5(
			_mm256_setr_epi64x(_mm256_extract_epi64(T20_regs[i][0], 0), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(state[i][0], 1), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T8_regs[i][0], 2), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T19_regs[i], 3), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T15_regs[i], 0), 0, 0, 0));

		state[i][0] = XOR6(
			_mm256_setr_epi64x(0, _mm256_extract_epi64(state[i][0], 0), _mm256_extract_epi64(T18_regs[i][0], 0), _mm256_extract_epi64(T11_regs[i][0], 0)),
			_mm256_setr_epi64x(0, _mm256_extract_epi64(T18_regs[i][0], 1), _mm256_extract_epi64(T8_regs[i][0], 1), _mm256_extract_epi64(T5_regs[i], 1)),
			_mm256_setr_epi64x(0, _mm256_extract_epi64(T2_regs[i][0], 2), _mm256_extract_epi64(T19_regs[i], 2), _mm256_extract_epi64(T30_regs[i], 2)),
			_mm256_setr_epi64x(0, _mm256_extract_epi64(T2_regs[i][0], 3), _mm256_extract_epi64(T3_regs[i][0], 3), _mm256_extract_epi64(T5_regs[i], 3)),
			_mm256_setr_epi64x(0, _mm256_extract_epi64(T18_regs[i][1], 0), _mm256_extract_epi64(T11_regs[i][1], 0), _mm256_extract_epi64(T20_regs[i][1], 0)),
			temp_state[i]);
	}
}

void optimized_mixcolumns_A3_80bit(__m256i (*state)[2]) {
	/*
	0       0       0       1       0
	0       0       0       0       1
	1       18      2       2       18
	18      8       19      3       11
	11      5       30      5       20
	*/

	__m256i T2_regs[5][2];
	__m256i T3_regs[5];
	__m256i T4_regs[5][2];
	__m256i T5_regs[5];
	__m256i T8_regs[5][2];
	__m256i T11_regs[5][2];
	__m256i T16_regs[5][2];
	__m256i T18_regs[5][2];
	__m256i T19_regs[5];
	__m256i T20_regs[5];
	__m256i T30_regs[5];
	__m256i temp_state[5];

	T2(state, T2_regs); //T2
	T2(T2_regs, T4_regs); //T4
	T2(T4_regs, T8_regs); //T8
	T2(T8_regs, T16_regs); //T16

	for (int i = 0; i < 5; i++) {
		T3_regs[i] = XOR(state[i][0], T2_regs[i][0]);

		T5_regs[i] = XOR(state[i][0], T4_regs[i][0]);

		T11_regs[i][0] = XOR(T3_regs[i], T8_regs[i][0]);
		T11_regs[i][1] = XOR3(state[i][1], T2_regs[i][1], T8_regs[i][1]);

		T18_regs[i][0] = XOR(T16_regs[i][0], T2_regs[i][0]);
		T18_regs[i][1] = XOR(T16_regs[i][1], T2_regs[i][1]);

		T19_regs[i] = XOR(T16_regs[i][0], T3_regs[i]);

		T20_regs[i] = XOR(T16_regs[i][1], T4_regs[i][1]);

		T30_regs[i] = XOR3(T8_regs[i][0], T4_regs[i][0], T18_regs[i][0]);
	}

	//Shuffle bytes upwards due to lines 1,2,3,4,5,6 in matrix.
	for (int i = 0; i < 5; i++) {
		__mm256_insert_epi64(temp_state[i], _mm256_extract_epi64(state[i][0], 3), 0);
		__mm256_insert_epi64(temp_state[i], _mm256_extract_epi64(state[i][1], 0), 1);
		__mm256_insert_epi64(temp_state[i], 0, 2);
		__mm256_insert_epi64(temp_state[i], 0, 3);
	}

	for (int i = 0; i < 5; i++) {
		state[i][1] = XOR5(
			_mm256_setr_epi64x(_mm256_extract_epi64(T11_regs[i][0], 0), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T5_regs[i], 1), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T30_regs[i], 2), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T5_regs[i], 3), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T20_regs[i], 0), 0, 0, 0));

		state[i][0] = XOR6(
			_mm256_setr_epi64x(0, 0, _mm256_extract_epi64(state[i][0], 0), _mm256_extract_epi64(T18_regs[i][0], 0)),
			_mm256_setr_epi64x(0, 0, _mm256_extract_epi64(T18_regs[i][0], 1), _mm256_extract_epi64(T8_regs[i][0], 1)),
			_mm256_setr_epi64x(0, 0, _mm256_extract_epi64(T2_regs[i][0], 2), _mm256_extract_epi64(T19_regs[i], 2)),
			_mm256_setr_epi64x(0, 0, _mm256_extract_epi64(T2_regs[i][0], 3), _mm256_extract_epi64(T3_regs[i], 3)),
			_mm256_setr_epi64x(0, 0, _mm256_extract_epi64(T18_regs[i][1], 0), _mm256_extract_epi64(T11_regs[i][1], 0)),
			temp_state[i]);
	}
}

void optimized_mixcolumns_A2_80bit(__m256i (*state)[2]) {
	/*
	0       0       1       0       0
	0       0       0       1       0
	0       0       0       0       1
	1       18      2       2       18
	18      8       19      3       11
	*/

	__m256i T2_regs[5][2];
	__m256i T3_regs[5];
	__m256i T4_regs[5][2];
	__m256i T8_regs[5][2];
	__m256i T11_regs[5];
	__m256i T16_regs[5][2];
	__m256i T18_regs[5][2];
	__m256i T19_regs[5];
	__m256i temp_state[5];

	T2(state, T2_regs); //T2
	T2(T2_regs, T4_regs); //T4
	T2(T4_regs, T8_regs); //T8
	T2(T8_regs, T16_regs); //T16

	for (int i = 0; i < 5; i++) {
		T3_regs[i] = XOR(state[i][0], T2_regs[i][0]);

		T11_regs[i] = XOR3(state[i][1], T2_regs[i][1], T8_regs[i][1]);

		T18_regs[i][0] = XOR(T16_regs[i][0], T2_regs[i][0]);
		T18_regs[i][1] = XOR(T16_regs[i][1], T2_regs[i][1]);

		T19_regs[i] = XOR(T16_regs[i][0], T3_regs[i]);
	}

	//Shuffle bytes upwards due to lines 1,2,3,4,5,6 in matrix.
	for (int i = 0; i < 5; i++) {
		__mm256_insert_epi64(temp_state[i], _mm256_extract_epi64(state[i][0], 2), 0);
		__mm256_insert_epi64(temp_state[i], _mm256_extract_epi64(state[i][0], 3), 1);
		__mm256_insert_epi64(temp_state[i], _mm256_extract_epi64(state[i][1], 0), 2);
		__mm256_insert_epi64(temp_state[i], 0, 3);
	}

	for (int i = 0; i < 5; i++) {
		state[i][1] = XOR5(
			_mm256_setr_epi64x(_mm256_extract_epi64(T18_regs[i][0], 0), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T8_regs[i][0], 1), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T19_regs[i], 2), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T3_regs[i], 3), 0, 0, 0),
			_mm256_setr_epi64x(_mm256_extract_epi64(T11_regs[i], 0), 0, 0, 0));

		state[i][0] = XOR6(
			_mm256_setr_epi64x(0, 0, 0, _mm256_extract_epi64(state[i][0], 0)),
			_mm256_setr_epi64x(0, 0, 0, _mm256_extract_epi64(T18_regs[i][0], 1)),
			_mm256_setr_epi64x(0, 0, 0, _mm256_extract_epi64(T2_regs[i][0], 2)),
			_mm256_setr_epi64x(0, 0, 0, _mm256_extract_epi64(T2_regs[i][0], 3)),
			_mm256_setr_epi64x(0, 0, 0, _mm256_extract_epi64(T18_regs[i][1], 0)),
			temp_state[i]);
	}
}
