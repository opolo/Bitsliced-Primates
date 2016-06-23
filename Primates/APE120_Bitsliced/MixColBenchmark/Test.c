#include <immintrin.h>
#include <stdlib.h>
#include <Windows.h>
#include <math.h>
#include <stdio.h>
typedef __m256i YMM;

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

void T2(__m256i (*state)[2], __m256i (*new_state)[2]);
int cmpfunc(const void * a, const void * b);

void mixcolumns_A7(__m256i (*state)[2]);
void mixcolumns_A6(__m256i (*state)[2]);
void mixcolumns_A5(__m256i (*state)[2]);
void mixcolumns_A4(__m256i (*state)[2]);
void mixcolumns_A3(__m256i (*state)[2]);
void mixcolumns_A2(__m256i (*state)[2]);
void mixcolumns_A1(__m256i (*state)[2]);

void main() {
 
	LARGE_INTEGER start, finish;
	YMM test_state[5][2];
	for (int i = 0; i < 5; i++) {
		test_state[i][0] = _mm256_set1_epi64x(1);
		test_state[i][1] = _mm256_set1_epi64x(1);
	}

	int iterations = 1'000'00;
	int it_per_it = 200;
	double *results_A1 = calloc(iterations, sizeof(double));
	double *results_A2 = calloc(iterations, sizeof(double));
	double *results_A3 = calloc(iterations, sizeof(double));
	double *results_A4 = calloc(iterations, sizeof(double));
	double *results_A5 = calloc(iterations, sizeof(double));
	double *results_A6 = calloc(iterations, sizeof(double));
	double *results_A7 = calloc(iterations, sizeof(double));
	
	for (int i = 0; i < iterations; i++) {
		if (i % (iterations / 10) == 0) {
			//One tenth through...
			printf("Progress: %i/10 through. \n", (i / (iterations / 10) + 1));
		}

		//A1
		QueryPerformanceCounter(&start);
		for (int j = 0; j < it_per_it; j++)
			mixcolumns_A1(test_state);
		QueryPerformanceCounter(&finish);
		results_A1[i] = (double)(finish.QuadPart - start.QuadPart);

		//A2
		QueryPerformanceCounter(&start);
		for (int j = 0; j < it_per_it; j++)
			mixcolumns_A2(test_state);
		QueryPerformanceCounter(&finish);
		results_A2[i] = (double)(finish.QuadPart - start.QuadPart);

		//A3
		QueryPerformanceCounter(&start);
		for (int j = 0; j < it_per_it; j++)
			mixcolumns_A3(test_state);
		QueryPerformanceCounter(&finish);
		results_A3[i] = (double)(finish.QuadPart - start.QuadPart);

		//A4
		QueryPerformanceCounter(&start);
		for (int j = 0; j < it_per_it; j++)
			mixcolumns_A4(test_state);
		QueryPerformanceCounter(&finish);
		results_A4[i] = (double)(finish.QuadPart - start.QuadPart);

		//A5
		QueryPerformanceCounter(&start);
		for (int j = 0; j < it_per_it; j++)
			mixcolumns_A5(test_state);
		QueryPerformanceCounter(&finish);
		results_A5[i] = (double)(finish.QuadPart - start.QuadPart);

		//A6
		QueryPerformanceCounter(&start);
		for (int j = 0; j < it_per_it; j++)
			mixcolumns_A6(test_state);
		QueryPerformanceCounter(&finish);
		results_A6[i] = (double)(finish.QuadPart - start.QuadPart);

		//A7
		QueryPerformanceCounter(&start);
		for (int j = 0; j < it_per_it; j++)
			mixcolumns_A7(test_state);
		QueryPerformanceCounter(&finish);
		results_A7[i] = (double)(finish.QuadPart - start.QuadPart);
	}

	//Sort
	qsort(results_A1, iterations, sizeof(double), cmpfunc);
	qsort(results_A2, iterations, sizeof(double), cmpfunc);
	qsort(results_A3, iterations, sizeof(double), cmpfunc);
	qsort(results_A4, iterations, sizeof(double), cmpfunc);
	qsort(results_A5, iterations, sizeof(double), cmpfunc);
	qsort(results_A6, iterations, sizeof(double), cmpfunc);
	qsort(results_A7, iterations, sizeof(double), cmpfunc);

	double A1_median = results_A1[iterations/2];
	double A2_median = results_A2[iterations / 2];
	double A3_median = results_A3[iterations / 2];
	double A4_median = results_A4[iterations / 2];
	double A5_median = results_A5[iterations / 2];
	double A6_median = results_A6[iterations / 2];
	double A7_median = results_A7[iterations / 2];

	printf("\n");
	printf("A1 median: \n");
	printf("%f \n", A1_median);
	printf("A2 median: \n");
	printf("%f \n", A2_median);
	printf("A3 median: \n");
	printf("%f \n", A3_median);
	printf("A4 median: \n");
	printf("%f \n", A4_median);
	printf("A5 median: \n");
	printf("%f \n", A5_median);
	printf("A6 median: \n");
	printf("%f \n", A6_median);
	printf("A7 median: \n");
	printf("%f \n", A7_median);

	printf("\n");
	printf("A1 index: \n");
	printf("%f \n", (A1_median / A1_median) * 100);
	printf("A2 index: \n");
	printf("%f \n", (A2_median / A1_median) * 100);
	printf("A3 index: \n");
	printf("%f \n", (A3_median / A1_median) * 100);
	printf("A4 index: \n");
	printf("%f \n", (A4_median / A1_median) * 100);
	printf("A5 index: \n");
	printf("%f \n", (A5_median / A1_median) * 100);
	printf("A6 index: \n");
	printf("%f \n", (A6_median / A1_median) * 100);
	printf("A7 index: \n");
	printf("%f \n", (A7_median / A1_median) * 100);

	getchar();

	free(results_A1);
	free(results_A2);
	free(results_A3);
	free(results_A4);
	free(results_A5);
	free(results_A6);
	free(results_A7);

	printf("\n\n print this to avoid it from getting optimized away: %u", test_state[0][1].m256i_u64[0]);
}

int cmpfunc(const void * a, const void * b)
{
	return (int) (*(double*)a - *(double*)b);
}

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
	}

	//T5
	for (int i = 0; i < 5; i++) {
		T5_regs[i][0] = XOR(state[i][0], T4_regs[i][0]);
		T5_regs[i][1] = XOR(state[i][1], T4_regs[i][1]);
	}

	//T7
	for (int i = 0; i < 5; i++) {
		T7_regs[i][0] = XOR3(state[i][0], T4_regs[i][0], T2_regs[i][0]);
		T7_regs[i][1] = XOR3(state[i][1], T4_regs[i][1], T2_regs[i][1]);
	}

	//T9
	for (int i = 0; i < 5; i++) {
		T9_regs[i][0] = XOR(state[i][0], T8_regs[i][0]);
		T9_regs[i][1] = XOR(state[i][1], T8_regs[i][1]);
	}

	//T10
	for (int i = 0; i < 5; i++) {
		T10_regs[i][0] = XOR(T8_regs[i][0], T2_regs[i][0]);
		T10_regs[i][1] = XOR(T8_regs[i][1], T2_regs[i][1]);
	}

	//T11
	for (int i = 0; i < 5; i++) {
		T11_regs[i][0] = XOR3(state[i][0], T8_regs[i][0], T2_regs[i][0]);
		T11_regs[i][1] = XOR3(state[i][1], T8_regs[i][1], T2_regs[i][1]);
	}

	//T12
	for (int i = 0; i < 5; i++) {
		T12_regs[i][0] = XOR(T4_regs[i][0], T8_regs[i][0]);
		T12_regs[i][1] = XOR(T4_regs[i][1], T8_regs[i][1]);
	}

	//T15
	for (int i = 0; i < 5; i++) {
		T15_regs[i][0] = XOR4(state[i][0], T2_regs[i][0], T4_regs[i][0], T8_regs[i][0]);
		T15_regs[i][1] = XOR4(state[i][1], T2_regs[i][1], T4_regs[i][1], T8_regs[i][1]);
	}

	//T17
	for (int i = 0; i < 5; i++) {
		T17_regs[i][0] = XOR(state[i][0], T16_regs[i][0]);
		T17_regs[i][1] = XOR(state[i][1], T16_regs[i][1]);
	}

	//T20
	for (int i = 0; i < 5; i++) {
		T20_regs[i][0] = XOR(T16_regs[i][0], T4_regs[i][0]);
		T20_regs[i][1] = XOR(T16_regs[i][1], T4_regs[i][1]);
	}

	//T22
	for (int i = 0; i < 5; i++) {
		T22_regs[i][0] = XOR3(T16_regs[i][0], T4_regs[i][0], T2_regs[i][0]);
		T22_regs[i][1] = XOR3(T16_regs[i][1], T4_regs[i][1], T2_regs[i][1]);
	}

	//T23
	for (int i = 0; i < 5; i++) {
		T23_regs[i][0] = XOR3(T16_regs[i][0], T5_regs[i][0], T2_regs[i][0]);
		T23_regs[i][1] = XOR3(T16_regs[i][1], T5_regs[i][1], T2_regs[i][1]);
	}

	//T24
	for (int i = 0; i < 5; i++) {
		T24_regs[i][0] = XOR(T16_regs[i][0], T8_regs[i][0]);
		T24_regs[i][1] = XOR(T16_regs[i][1], T8_regs[i][1]);
	}

	//T25
	for (int i = 0; i < 5; i++) {
		T25_regs[i][0] = XOR3(T16_regs[i][0], T8_regs[i][0], state[i][0]);
		T25_regs[i][1] = XOR3(T16_regs[i][1], T8_regs[i][1], state[i][1]);
	}

	//T26
	for (int i = 0; i < 5; i++) {
		T26_regs[i][0] = XOR3(T16_regs[i][0], T8_regs[i][0], T2_regs[i][0]);
		T26_regs[i][1] = XOR3(T16_regs[i][1], T8_regs[i][1], T2_regs[i][1]);
	}

	//T27
	for (int i = 0; i < 5; i++) {
		T27_regs[i][0] = XOR4(T16_regs[i][0], T8_regs[i][0], T2_regs[i][0], state[i][0]);
		T27_regs[i][1] = XOR4(T16_regs[i][1], T8_regs[i][1], T2_regs[i][1], state[i][1]);
	}

	//T28
	for (int i = 0; i < 5; i++) {
		T28_regs[i][0] = XOR3(T16_regs[i][0], T8_regs[i][0], T4_regs[i][0]);
		T28_regs[i][1] = XOR3(T16_regs[i][1], T8_regs[i][1], T4_regs[i][1]);
	}

	//T29
	for (int i = 0; i < 5; i++) {
		T29_regs[i][0] = XOR4(T16_regs[i][0], T8_regs[i][0], T4_regs[i][0], state[i][0]);
		T29_regs[i][1] = XOR4(T16_regs[i][1], T8_regs[i][1], T4_regs[i][1], state[i][1]);
	}

	//T31
	for (int i = 0; i < 5; i++) {
		T31_regs[i][0] = XOR5(T16_regs[i][0], T8_regs[i][0], T4_regs[i][0], T2_regs[i][0], state[i][0]);
		T31_regs[i][1] = XOR5(T16_regs[i][1], T8_regs[i][1], T4_regs[i][1], T2_regs[i][1], state[i][1]);
	}

	//handle last line of matrix. Align all rows on the third u64
	YMM aligned_last_row0[5];
	YMM aligned_last_row1[5];
	YMM aligned_last_row2[5];
	YMM aligned_last_row3[5];
	YMM aligned_last_row4[5];
	YMM aligned_last_row5[5];
	YMM aligned_last_row6[5];

	//29      15      16      31      23      9       2
	YMM aligned_firststate_row0[5];
	YMM aligned_firststate_row1[5];
	YMM aligned_firststate_row2[5];
	YMM aligned_firststate_row3[5];
	YMM aligned_firststate_row4[5];
	YMM aligned_firststate_row5[5];
	YMM aligned_firststate_row6[5];
	for (int i = 0; i < 5; i++) {

		aligned_firststate_row0[i] = _mm256_setr_epi64x(state[i][0].m256i_u64[0], T2_regs[i][0].m256i_u64[0], T11_regs[i][0].m256i_u64[0], state[i][0].m256i_u64[0]);
		aligned_firststate_row1[i] = _mm256_setr_epi64x(T2_regs[i][0].m256i_u64[1], T5_regs[i][0].m256i_u64[1], T20_regs[i][0].m256i_u64[1], T9_regs[i][0].m256i_u64[1]);
		aligned_firststate_row2[i] = _mm256_setr_epi64x(T15_regs[i][0].m256i_u64[2], T28_regs[i][0].m256i_u64[2], T3_regs[i][0].m256i_u64[2], T27_regs[i][0].m256i_u64[2]);
		aligned_firststate_row3[i] = _mm256_setr_epi64x(T9_regs[i][0].m256i_u64[3], T29_regs[i][0].m256i_u64[3], T5_regs[i][0].m256i_u64[3], T10_regs[i][0].m256i_u64[3]);
		aligned_firststate_row4[i] = _mm256_setr_epi64x(T9_regs[i][1].m256i_u64[0], T27_regs[i][1].m256i_u64[0], T4_regs[i][1].m256i_u64[0], T12_regs[i][1].m256i_u64[0]);
		aligned_firststate_row5[i] = _mm256_setr_epi64x(T15_regs[i][1].m256i_u64[1], T23_regs[i][1].m256i_u64[1], T29_regs[i][1].m256i_u64[1], T11_regs[i][1].m256i_u64[1]);
		aligned_firststate_row6[i] = _mm256_setr_epi64x(T2_regs[i][1].m256i_u64[2], T11_regs[i][1].m256i_u64[2], state[i][1].m256i_u64[2], T31_regs[i][1].m256i_u64[2]);

		aligned_last_row0[i] = _mm256_setr_epi64x(T31_regs[i][0].m256i_u64[0], T16_regs[i][0].m256i_u64[0], T29_regs[i][0].m256i_u64[0], 0);
		aligned_last_row1[i] = _mm256_setr_epi64x(T26_regs[i][0].m256i_u64[1], T26_regs[i][0].m256i_u64[1], T15_regs[i][0].m256i_u64[1], 0);
		aligned_last_row2[i] = _mm256_setr_epi64x(T29_regs[i][0].m256i_u64[2], T17_regs[i][0].m256i_u64[2], T16_regs[i][0].m256i_u64[2], 0);
		aligned_last_row3[i] = _mm256_setr_epi64x(T7_regs[i][0].m256i_u64[3], T25_regs[i][0].m256i_u64[3], T31_regs[i][0].m256i_u64[3], 0);
		aligned_last_row4[i] = _mm256_setr_epi64x(T22_regs[i][1].m256i_u64[0], T3_regs[i][1].m256i_u64[0], T23_regs[i][1].m256i_u64[0], 0);
		aligned_last_row5[i] = _mm256_setr_epi64x(T24_regs[i][1].m256i_u64[1], T29_regs[i][1].m256i_u64[1], T9_regs[i][1].m256i_u64[1], 0);
		aligned_last_row6[i] = _mm256_setr_epi64x(T16_regs[i][1].m256i_u64[2], T29_regs[i][1].m256i_u64[2], T2_regs[i][1].m256i_u64[2], 0);
	}

	YMM last_row_calculated[5];
	YMM last_rowfirststate_calculated[5];
	for (int i = 0; i < 5; i++) {
		last_rowfirststate_calculated[i] = XOR7(aligned_firststate_row0[i], aligned_firststate_row1[i], aligned_firststate_row2[i], aligned_firststate_row3[i], aligned_firststate_row4[i], aligned_firststate_row5[i], aligned_firststate_row6[i]);
		last_row_calculated[i] = XOR7(aligned_last_row0[i], aligned_last_row1[i], aligned_last_row2[i], aligned_last_row3[i], aligned_last_row4[i], aligned_last_row5[i], aligned_last_row6[i]);
	}

	//Assign data to state
	for (int i = 0; i < 5; i++) {
		state[i][0] = last_rowfirststate_calculated[i];
		state[i][1] = last_row_calculated[i];
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
	}

	//T5
	for (int i = 0; i < 5; i++) {
		T5_regs[i][0] = XOR(state[i][0], T4_regs[i][0]);
		T5_regs[i][1] = XOR(state[i][1], T4_regs[i][1]);
	}

	//T7
	for (int i = 0; i < 5; i++) {
		T7_regs[i][0] = XOR3(state[i][0], T4_regs[i][0], T2_regs[i][0]);
		T7_regs[i][1] = XOR3(state[i][1], T4_regs[i][1], T2_regs[i][1]);
	}

	//T9
	for (int i = 0; i < 5; i++) {
		T9_regs[i][0] = XOR(state[i][0], T8_regs[i][0]);
		T9_regs[i][1] = XOR(state[i][1], T8_regs[i][1]);
	}

	//T10
	for (int i = 0; i < 5; i++) {
		T10_regs[i][0] = XOR(T8_regs[i][0], T2_regs[i][0]);
		T10_regs[i][1] = XOR(T8_regs[i][1], T2_regs[i][1]);
	}

	//T11
	for (int i = 0; i < 5; i++) {
		T11_regs[i][0] = XOR3(state[i][0], T8_regs[i][0], T2_regs[i][0]);
		T11_regs[i][1] = XOR3(state[i][1], T8_regs[i][1], T2_regs[i][1]);
	}

	//T12
	for (int i = 0; i < 5; i++) {
		T12_regs[i][0] = XOR(T4_regs[i][0], T8_regs[i][0]);
		T12_regs[i][1] = XOR(T4_regs[i][1], T8_regs[i][1]);
	}

	//T15
	for (int i = 0; i < 5; i++) {
		T15_regs[i][0] = XOR4(state[i][0], T2_regs[i][0], T4_regs[i][0], T8_regs[i][0]);
		T15_regs[i][1] = XOR4(state[i][1], T2_regs[i][1], T4_regs[i][1], T8_regs[i][1]);
	}

	//T17
	for (int i = 0; i < 5; i++) {
		T17_regs[i][0] = XOR(state[i][0], T16_regs[i][0]);
		T17_regs[i][1] = XOR(state[i][1], T16_regs[i][1]);
	}

	//T20
	for (int i = 0; i < 5; i++) {
		T20_regs[i][0] = XOR(T16_regs[i][0], T4_regs[i][0]);
		T20_regs[i][1] = XOR(T16_regs[i][1], T4_regs[i][1]);
	}

	//T22
	for (int i = 0; i < 5; i++) {
		T22_regs[i][0] = XOR3(T16_regs[i][0], T4_regs[i][0], T2_regs[i][0]);
		T22_regs[i][1] = XOR3(T16_regs[i][1], T4_regs[i][1], T2_regs[i][1]);
	}

	//T23
	for (int i = 0; i < 5; i++) {
		T23_regs[i][0] = XOR3(T16_regs[i][0], T5_regs[i][0], T2_regs[i][0]);
		T23_regs[i][1] = XOR3(T16_regs[i][1], T5_regs[i][1], T2_regs[i][1]);
	}

	//T24
	for (int i = 0; i < 5; i++) {
		T24_regs[i][0] = XOR(T16_regs[i][0], T8_regs[i][0]);
		T24_regs[i][1] = XOR(T16_regs[i][1], T8_regs[i][1]);
	}

	//T25
	for (int i = 0; i < 5; i++) {
		T25_regs[i][0] = XOR3(T16_regs[i][0], T8_regs[i][0], state[i][0]);
		T25_regs[i][1] = XOR3(T16_regs[i][1], T8_regs[i][1], state[i][1]);
	}

	//T26
	for (int i = 0; i < 5; i++) {
		T26_regs[i][0] = XOR3(T16_regs[i][0], T8_regs[i][0], T2_regs[i][0]);
		T26_regs[i][1] = XOR3(T16_regs[i][1], T8_regs[i][1], T2_regs[i][1]);
	}

	//T27
	for (int i = 0; i < 5; i++) {
		T27_regs[i][0] = XOR4(T16_regs[i][0], T8_regs[i][0], T2_regs[i][0], state[i][0]);
		T27_regs[i][1] = XOR4(T16_regs[i][1], T8_regs[i][1], T2_regs[i][1], state[i][1]);
	}

	//T28
	for (int i = 0; i < 5; i++) {
		T28_regs[i][0] = XOR3(T16_regs[i][0], T8_regs[i][0], T4_regs[i][0]);
		T28_regs[i][1] = XOR3(T16_regs[i][1], T8_regs[i][1], T4_regs[i][1]);
	}

	//T29
	for (int i = 0; i < 5; i++) {
		T29_regs[i][0] = XOR4(T16_regs[i][0], T8_regs[i][0], T4_regs[i][0], state[i][0]);
		T29_regs[i][1] = XOR4(T16_regs[i][1], T8_regs[i][1], T4_regs[i][1], state[i][1]);
	}

	//T31
	for (int i = 0; i < 5; i++) {
		T31_regs[i][0] = XOR5(T16_regs[i][0], T8_regs[i][0], T4_regs[i][0], T2_regs[i][0], state[i][0]);
		T31_regs[i][1] = XOR5(T16_regs[i][1], T8_regs[i][1], T4_regs[i][1], T2_regs[i][1], state[i][1]);
	}

	//Shuffle bytes upwards due to lines 1,2,3,4,5,6 in matrix.
	for (int i = 0; i < 5; i++) {
		temp_state[i][0].m256i_u64[0] = state[i][1].m256i_u64[2];
	}

	//handle last line of matrix. Align all rows on the third u64
	YMM aligned_last_row0[5];
	YMM aligned_last_row1[5];
	YMM aligned_last_row2[5];
	YMM aligned_last_row3[5];
	YMM aligned_last_row4[5];
	YMM aligned_last_row5[5];
	YMM aligned_last_row6[5];

	//16      26      17      25      3       29      29
	YMM aligned_firststate_row0[5];
	YMM aligned_firststate_row1[5];
	YMM aligned_firststate_row2[5];
	YMM aligned_firststate_row3[5];
	YMM aligned_firststate_row4[5];
	YMM aligned_firststate_row5[5];
	YMM aligned_firststate_row6[5];
	for (int i = 0; i < 5; i++) {

		aligned_firststate_row0[i] = _mm256_setr_epi64x(0, state[i][0].m256i_u64[0], T2_regs[i][0].m256i_u64[0], T11_regs[i][0].m256i_u64[0]);
		aligned_firststate_row1[i] = _mm256_setr_epi64x(0, T2_regs[i][0].m256i_u64[1], T5_regs[i][0].m256i_u64[1], T20_regs[i][0].m256i_u64[1]);
		aligned_firststate_row2[i] = _mm256_setr_epi64x(0, T15_regs[i][0].m256i_u64[2], T28_regs[i][0].m256i_u64[2], T3_regs[i][0].m256i_u64[2]);
		aligned_firststate_row3[i] = _mm256_setr_epi64x(0, T9_regs[i][0].m256i_u64[3], T29_regs[i][0].m256i_u64[3], T5_regs[i][0].m256i_u64[3]);
		aligned_firststate_row4[i] = _mm256_setr_epi64x(0, T9_regs[i][1].m256i_u64[0], T27_regs[i][1].m256i_u64[0], T4_regs[i][1].m256i_u64[0]);
		aligned_firststate_row5[i] = _mm256_setr_epi64x(0, T15_regs[i][1].m256i_u64[1], T23_regs[i][1].m256i_u64[1], T29_regs[i][1].m256i_u64[1]);
		aligned_firststate_row6[i] = _mm256_setr_epi64x(0, T2_regs[i][1].m256i_u64[2], T11_regs[i][1].m256i_u64[2], state[i][1].m256i_u64[2]);

		aligned_last_row0[i] = _mm256_setr_epi64x(state[i][0].m256i_u64[0],	T31_regs[i][0].m256i_u64[0], T16_regs[i][0].m256i_u64[0], 0);
		aligned_last_row1[i] = _mm256_setr_epi64x(T9_regs[i][0].m256i_u64[1], T26_regs[i][0].m256i_u64[1], T26_regs[i][0].m256i_u64[1], 0);
		aligned_last_row2[i] = _mm256_setr_epi64x(T27_regs[i][0].m256i_u64[2], T29_regs[i][0].m256i_u64[2], T17_regs[i][0].m256i_u64[2], 0);
		aligned_last_row3[i] = _mm256_setr_epi64x(T10_regs[i][0].m256i_u64[3], T7_regs[i][0].m256i_u64[3], T25_regs[i][0].m256i_u64[3],  0);
		aligned_last_row4[i] = _mm256_setr_epi64x(T12_regs[i][1].m256i_u64[0], T22_regs[i][1].m256i_u64[0], T3_regs[i][1].m256i_u64[0], 0);
		aligned_last_row5[i] = _mm256_setr_epi64x(T11_regs[i][1].m256i_u64[1], T24_regs[i][1].m256i_u64[1], T29_regs[i][1].m256i_u64[1], 0);
		aligned_last_row6[i] = _mm256_setr_epi64x(T31_regs[i][1].m256i_u64[2], T16_regs[i][1].m256i_u64[2], T29_regs[i][1].m256i_u64[2], 0);
	}

	YMM last_row_calculated[5];
	YMM last_rowfirststate_calculated[5];
	for (int i = 0; i < 5; i++) {
		last_rowfirststate_calculated[i] = XOR7(aligned_firststate_row0[i], aligned_firststate_row1[i], aligned_firststate_row2[i], aligned_firststate_row3[i], aligned_firststate_row4[i], aligned_firststate_row5[i], aligned_firststate_row6[i]);
		last_row_calculated[i] = XOR7(aligned_last_row0[i], aligned_last_row1[i], aligned_last_row2[i], aligned_last_row3[i], aligned_last_row4[i], aligned_last_row5[i], aligned_last_row6[i]);
	}

	//Assign data to state
	for (int i = 0; i < 5; i++) {
		state[i][0] = XOR(temp_state[i][0], last_rowfirststate_calculated[i]);
		state[i][1] = last_row_calculated[i];
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

	//T5
	for (int i = 0; i < 5; i++) {
		T5_regs[i][0] = XOR(state[i][0], T4_regs[i][0]);
		T5_regs[i][1] = XOR(state[i][1], T4_regs[i][1]);
	}

	//T7
	for (int i = 0; i < 5; i++) {
		T7_regs[i][0] = XOR3(state[i][0], T4_regs[i][0], T2_regs[i][0]);
		T7_regs[i][1] = XOR3(state[i][1], T4_regs[i][1], T2_regs[i][1]);
	}

	//T9
	for (int i = 0; i < 5; i++) {
		T9_regs[i][0] = XOR(state[i][0], T8_regs[i][0]);
		T9_regs[i][1] = XOR(state[i][1], T8_regs[i][1]);
	}

	//T10
	for (int i = 0; i < 5; i++) {
		T10_regs[i][0] = XOR(T8_regs[i][0], T2_regs[i][0]);
		T10_regs[i][1] = XOR(T8_regs[i][1], T2_regs[i][1]);
	}

	//T11
	for (int i = 0; i < 5; i++) {
		T11_regs[i][0] = XOR3(state[i][0], T8_regs[i][0], T2_regs[i][0]);
		T11_regs[i][1] = XOR3(state[i][1], T8_regs[i][1], T2_regs[i][1]);
	}

	//T12
	for (int i = 0; i < 5; i++) {
		T12_regs[i][0] = XOR(T4_regs[i][0], T8_regs[i][0]);
		T12_regs[i][1] = XOR(T4_regs[i][1], T8_regs[i][1]);
	}

	//T15
	for (int i = 0; i < 5; i++) {
		T15_regs[i][0] = XOR4(state[i][0], T2_regs[i][0], T4_regs[i][0], T8_regs[i][0]);
		T15_regs[i][1] = XOR4(state[i][1], T2_regs[i][1], T4_regs[i][1], T8_regs[i][1]);
	}

	//T20
	for (int i = 0; i < 5; i++) {
		T20_regs[i][0] = XOR(T16_regs[i][0], T4_regs[i][0]);
		T20_regs[i][1] = XOR(T16_regs[i][1], T4_regs[i][1]);
	}

	//T22
	for (int i = 0; i < 5; i++) {
		T22_regs[i][0] = XOR3(T16_regs[i][0], T4_regs[i][0], T2_regs[i][0]);
		T22_regs[i][1] = XOR3(T16_regs[i][1], T4_regs[i][1], T2_regs[i][1]);
	}

	//T23
	for (int i = 0; i < 5; i++) {
		T23_regs[i][0] = XOR3(T16_regs[i][0], T5_regs[i][0], T2_regs[i][0]);
		T23_regs[i][1] = XOR3(T16_regs[i][1], T5_regs[i][1], T2_regs[i][1]);
	}

	//T24
	for (int i = 0; i < 5; i++) {
		T24_regs[i][0] = XOR(T16_regs[i][0], T8_regs[i][0]);
		T24_regs[i][1] = XOR(T16_regs[i][1], T8_regs[i][1]);
	}

	//T26
	for (int i = 0; i < 5; i++) {
		T26_regs[i][0] = XOR3(T16_regs[i][0], T8_regs[i][0], T2_regs[i][0]);
		T26_regs[i][1] = XOR3(T16_regs[i][1], T8_regs[i][1], T2_regs[i][1]);
	}

	//T27
	for (int i = 0; i < 5; i++) {
		T27_regs[i][0] = XOR4(T16_regs[i][0], T8_regs[i][0], T2_regs[i][0], state[i][0]);
		T27_regs[i][1] = XOR4(T16_regs[i][1], T8_regs[i][1], T2_regs[i][1], state[i][1]);
	}

	//T28
	for (int i = 0; i < 5; i++) {
		T28_regs[i][0] = XOR3(T16_regs[i][0], T8_regs[i][0], T4_regs[i][0]);
		T28_regs[i][1] = XOR3(T16_regs[i][1], T8_regs[i][1], T4_regs[i][1]);
	}

	//T29
	for (int i = 0; i < 5; i++) {
		T29_regs[i][0] = XOR4(T16_regs[i][0], T8_regs[i][0], T4_regs[i][0], state[i][0]);
		T29_regs[i][1] = XOR4(T16_regs[i][1], T8_regs[i][1], T4_regs[i][1], state[i][1]);
	}

	//T31
	for (int i = 0; i < 5; i++) {
		T31_regs[i][0] = XOR5(T16_regs[i][0], T8_regs[i][0], T4_regs[i][0], T2_regs[i][0], state[i][0]);
		T31_regs[i][1] = XOR5(T16_regs[i][1], T8_regs[i][1], T4_regs[i][1], T2_regs[i][1], state[i][1]);
	}

	//Shuffle bytes upwards due to lines 1,2,3,4,5,6 in matrix.
	for (int i = 0; i < 5; i++) {
		temp_state[i][0].m256i_u64[0] = state[i][1].m256i_u64[1];
		temp_state[i][0].m256i_u64[1] = state[i][1].m256i_u64[2];
	}

	//handle last line of matrix. Align all rows on the third u64
	YMM aligned_last_row0[5];
	YMM aligned_last_row1[5];
	YMM aligned_last_row2[5];
	YMM aligned_last_row3[5];
	YMM aligned_last_row4[5];
	YMM aligned_last_row5[5];
	YMM aligned_last_row6[5];

	//31      26      29      7       22      24      16
	YMM aligned_firststate_row0[5];
	YMM aligned_firststate_row1[5];
	YMM aligned_firststate_row2[5];
	YMM aligned_firststate_row3[5];
	YMM aligned_firststate_row4[5];
	YMM aligned_firststate_row5[5];
	YMM aligned_firststate_row6[5];
	for (int i = 0; i < 5; i++) {

		aligned_firststate_row0[i] = _mm256_setr_epi64x(0, 0, state[i][0].m256i_u64[0], T2_regs[i][0].m256i_u64[0]);
		aligned_firststate_row1[i] = _mm256_setr_epi64x(0, 0, T2_regs[i][0].m256i_u64[1], T5_regs[i][0].m256i_u64[1]);
		aligned_firststate_row2[i] = _mm256_setr_epi64x(0, 0, T15_regs[i][0].m256i_u64[2], T28_regs[i][0].m256i_u64[2]);
		aligned_firststate_row3[i] = _mm256_setr_epi64x(0, 0, T9_regs[i][0].m256i_u64[3], T29_regs[i][0].m256i_u64[3]);
		aligned_firststate_row4[i] = _mm256_setr_epi64x(0, 0, T9_regs[i][1].m256i_u64[0], T27_regs[i][1].m256i_u64[0]);
		aligned_firststate_row5[i] = _mm256_setr_epi64x(0, 0, T15_regs[i][1].m256i_u64[1], T23_regs[i][1].m256i_u64[1]);
		aligned_firststate_row6[i] = _mm256_setr_epi64x(0, 0, T2_regs[i][1].m256i_u64[2], T11_regs[i][1].m256i_u64[2]);

		aligned_last_row0[i] = _mm256_setr_epi64x(T11_regs[i][0].m256i_u64[0],	state[i][0].m256i_u64[0], T31_regs[i][0].m256i_u64[0], 0);
		aligned_last_row1[i] = _mm256_setr_epi64x(T20_regs[i][0].m256i_u64[1], T9_regs[i][0].m256i_u64[1], T26_regs[i][0].m256i_u64[1], 0);
		aligned_last_row2[i] = _mm256_setr_epi64x(T3_regs[i][0].m256i_u64[2], T27_regs[i][0].m256i_u64[2], T29_regs[i][0].m256i_u64[2],	0);
		aligned_last_row3[i] = _mm256_setr_epi64x(T5_regs[i][0].m256i_u64[3], T10_regs[i][0].m256i_u64[3], T7_regs[i][0].m256i_u64[3],	0);
		aligned_last_row4[i] = _mm256_setr_epi64x(T4_regs[i][1].m256i_u64[0], T12_regs[i][1].m256i_u64[0], T22_regs[i][1].m256i_u64[0],	0);
		aligned_last_row5[i] = _mm256_setr_epi64x(T29_regs[i][1].m256i_u64[1], T11_regs[i][1].m256i_u64[1], T24_regs[i][1].m256i_u64[1], 0);
		aligned_last_row6[i] = _mm256_setr_epi64x(state[i][1].m256i_u64[2], T31_regs[i][1].m256i_u64[2], T16_regs[i][1].m256i_u64[2],	0);
	}

	YMM last_row_calculated[5];
	YMM last_rowfirststate_calculated[5];
	for (int i = 0; i < 5; i++) {
		last_rowfirststate_calculated[i] = XOR7(aligned_firststate_row0[i], aligned_firststate_row1[i], aligned_firststate_row2[i], aligned_firststate_row3[i], aligned_firststate_row4[i], aligned_firststate_row5[i], aligned_firststate_row6[i]);
		last_row_calculated[i] = XOR7(aligned_last_row0[i], aligned_last_row1[i], aligned_last_row2[i], aligned_last_row3[i], aligned_last_row4[i], aligned_last_row5[i], aligned_last_row6[i]);
	}

	//Assign data to state
	for (int i = 0; i < 5; i++) {
		state[i][0] = XOR(temp_state[i][0], last_rowfirststate_calculated[i]);
		state[i][1] = last_row_calculated[i];
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

						   //T5
	for (int i = 0; i < 5; i++) {
		T5_regs[i][0] = XOR(state[i][0], T4_regs[i][0]);
		T5_regs[i][1] = XOR(state[i][1], T4_regs[i][1]);
	}

	//T9
	for (int i = 0; i < 5; i++) {
		T9_regs[i][0] = XOR(state[i][0], T8_regs[i][0]);
		T9_regs[i][1] = XOR(state[i][1], T8_regs[i][1]);
	}

	//T10
	for (int i = 0; i < 5; i++) {
		T10_regs[i][0] = XOR(T8_regs[i][0], T2_regs[i][0]);
		T10_regs[i][1] = XOR(T8_regs[i][1], T2_regs[i][1]);
	}

	//T11
	for (int i = 0; i < 5; i++) {
		T11_regs[i][0] = XOR3(state[i][0], T8_regs[i][0], T2_regs[i][0]);
		T11_regs[i][1] = XOR3(state[i][1], T8_regs[i][1], T2_regs[i][1]);
	}

	//T12
	for (int i = 0; i < 5; i++) {
		T12_regs[i][0] = XOR(T4_regs[i][0], T8_regs[i][0]);
		T12_regs[i][1] = XOR(T4_regs[i][1], T8_regs[i][1]);
	}

	//T15
	for (int i = 0; i < 5; i++) {
		T15_regs[i][0] = XOR4(state[i][0], T2_regs[i][0], T4_regs[i][0], T8_regs[i][0]);
		T15_regs[i][1] = XOR4(state[i][1], T2_regs[i][1], T4_regs[i][1], T8_regs[i][1]);
	}

	//T20
	for (int i = 0; i < 5; i++) {
		T20_regs[i][0] = XOR(T16_regs[i][0], T4_regs[i][0]);
		T20_regs[i][1] = XOR(T16_regs[i][1], T4_regs[i][1]);
	}

	//T23
	for (int i = 0; i < 5; i++) {
		T23_regs[i][0] = XOR3(T16_regs[i][0], T5_regs[i][0], T2_regs[i][0]);
		T23_regs[i][1] = XOR3(T16_regs[i][1], T5_regs[i][1], T2_regs[i][1]);
	}

	//T27
	for (int i = 0; i < 5; i++) {
		T27_regs[i][0] = XOR4(T16_regs[i][0], T8_regs[i][0], T2_regs[i][0], state[i][0]);
		T27_regs[i][1] = XOR4(T16_regs[i][1], T8_regs[i][1], T2_regs[i][1], state[i][1]);
	}

	//T28
	for (int i = 0; i < 5; i++) {
		T28_regs[i][0] = XOR3(T16_regs[i][0], T8_regs[i][0], T4_regs[i][0]);
		T28_regs[i][1] = XOR3(T16_regs[i][1], T8_regs[i][1], T4_regs[i][1]);
	}

	//T29
	for (int i = 0; i < 5; i++) {
		T29_regs[i][0] = XOR4(T16_regs[i][0], T8_regs[i][0], T4_regs[i][0], state[i][0]);
		T29_regs[i][1] = XOR4(T16_regs[i][1], T8_regs[i][1], T4_regs[i][1], state[i][1]);
	}

	//T31
	for (int i = 0; i < 5; i++) {
		T31_regs[i][0] = XOR5(T16_regs[i][0], T8_regs[i][0], T4_regs[i][0], T2_regs[i][0], state[i][0]);
		T31_regs[i][1] = XOR5(T16_regs[i][1], T8_regs[i][1], T4_regs[i][1], T2_regs[i][1], state[i][1]);
	}

	//Shuffle bytes upwards due to lines 1,2,3,4,5,6 in matrix.
	for (int i = 0; i < 5; i++) {
		temp_state[i][0].m256i_u64[0] = state[i][1].m256i_u64[0];
		temp_state[i][0].m256i_u64[1] = state[i][1].m256i_u64[1];
		temp_state[i][0].m256i_u64[2] = state[i][1].m256i_u64[2];
	}

	//handle last line of matrix. Align all rows on the third u64
	YMM aligned_last_row0[5];
	YMM aligned_last_row1[5];
	YMM aligned_last_row2[5];
	YMM aligned_last_row3[5];
	YMM aligned_last_row4[5];
	YMM aligned_last_row5[5];
	YMM aligned_last_row6[5];

	//1       9       27      10      12      11      31
	YMM aligned_firststate_row0[5];
	YMM aligned_firststate_row1[5];
	YMM aligned_firststate_row2[5];
	YMM aligned_firststate_row3[5];
	YMM aligned_firststate_row4[5];
	YMM aligned_firststate_row5[5];
	YMM aligned_firststate_row6[5];
	for (int i = 0; i < 5; i++) {

		aligned_firststate_row0[i] = _mm256_setr_epi64x(0, 0, 0, state[i][0].m256i_u64[0]);
		aligned_firststate_row1[i] = _mm256_setr_epi64x(0, 0, 0, T2_regs[i][0].m256i_u64[1]);
		aligned_firststate_row2[i] = _mm256_setr_epi64x(0, 0, 0, T15_regs[i][0].m256i_u64[2]);
		aligned_firststate_row3[i] = _mm256_setr_epi64x(0, 0, 0, T9_regs[i][0].m256i_u64[3]);
		aligned_firststate_row4[i] = _mm256_setr_epi64x(0, 0, 0, T9_regs[i][1].m256i_u64[0]);
		aligned_firststate_row5[i] = _mm256_setr_epi64x(0, 0, 0, T15_regs[i][1].m256i_u64[1]);
		aligned_firststate_row6[i] = _mm256_setr_epi64x(0, 0, 0, T2_regs[i][1].m256i_u64[2]);

		aligned_last_row0[i] = _mm256_setr_epi64x(T2_regs[i][0].m256i_u64[0], T11_regs[i][0].m256i_u64[0],	state[i][0].m256i_u64[0], 0);
		aligned_last_row1[i] = _mm256_setr_epi64x(T5_regs[i][0].m256i_u64[1], T20_regs[i][0].m256i_u64[1],	T9_regs[i][0].m256i_u64[1], 0);
		aligned_last_row2[i] = _mm256_setr_epi64x(T28_regs[i][0].m256i_u64[2], T3_regs[i][0].m256i_u64[2],	T27_regs[i][0].m256i_u64[2], 0);
		aligned_last_row3[i] = _mm256_setr_epi64x(T29_regs[i][0].m256i_u64[3], T5_regs[i][0].m256i_u64[3],	T10_regs[i][0].m256i_u64[3], 0);
		aligned_last_row4[i] = _mm256_setr_epi64x(T27_regs[i][1].m256i_u64[0], T4_regs[i][1].m256i_u64[0],	T12_regs[i][1].m256i_u64[0], 0);
		aligned_last_row5[i] = _mm256_setr_epi64x(T23_regs[i][1].m256i_u64[1], T29_regs[i][1].m256i_u64[1], T11_regs[i][1].m256i_u64[1], 0);
		aligned_last_row6[i] = _mm256_setr_epi64x(T11_regs[i][1].m256i_u64[2], state[i][1].m256i_u64[2],	T31_regs[i][1].m256i_u64[2], 0);
	}

	YMM last_row_calculated[5];
	YMM last_rowfirststate_calculated[5];
	for (int i = 0; i < 5; i++) {
		last_rowfirststate_calculated[i] = XOR7(aligned_firststate_row0[i], aligned_firststate_row1[i], aligned_firststate_row2[i], aligned_firststate_row3[i], aligned_firststate_row4[i], aligned_firststate_row5[i], aligned_firststate_row6[i]);
		last_row_calculated[i] = XOR7(aligned_last_row0[i], aligned_last_row1[i], aligned_last_row2[i], aligned_last_row3[i], aligned_last_row4[i], aligned_last_row5[i], aligned_last_row6[i]);
	}

	//Assign data to state
	for (int i = 0; i < 5; i++) {
		state[i][0] = XOR(temp_state[i][0], last_rowfirststate_calculated[i]);
		state[i][1] = last_row_calculated[i];
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

						   //T5
	for (int i = 0; i < 5; i++) {
		T5_regs[i][0] = XOR(state[i][0], T4_regs[i][0]);
		T5_regs[i][1] = XOR(state[i][1], T4_regs[i][1]);
	}

	//T9
	for (int i = 0; i < 5; i++) {
		T9_regs[i][0] = XOR(state[i][0], T8_regs[i][0]);
		T9_regs[i][1] = XOR(state[i][1], T8_regs[i][1]);
	}

	//T11
	for (int i = 0; i < 5; i++) {
		T11_regs[i][0] = XOR3(state[i][0], T8_regs[i][0], T2_regs[i][0]);
		T11_regs[i][1] = XOR3(state[i][1], T8_regs[i][1], T2_regs[i][1]);
	}

	//T15
	for (int i = 0; i < 5; i++) {
		T15_regs[i][0] = XOR4(state[i][0], T2_regs[i][0], T4_regs[i][0], T8_regs[i][0]);
		T15_regs[i][1] = XOR4(state[i][1], T2_regs[i][1], T4_regs[i][1], T8_regs[i][1]);
	}

	//T20
	for (int i = 0; i < 5; i++) {
		T20_regs[i][0] = XOR(T16_regs[i][0], T4_regs[i][0]);
		T20_regs[i][1] = XOR(T16_regs[i][1], T4_regs[i][1]);
	}

	//T23
	for (int i = 0; i < 5; i++) {
		T23_regs[i][0] = XOR3(T16_regs[i][0], T5_regs[i][0], T2_regs[i][0]);
		T23_regs[i][1] = XOR3(T16_regs[i][1], T5_regs[i][1], T2_regs[i][1]);
	}

	//T27
	for (int i = 0; i < 5; i++) {
		T27_regs[i][0] = XOR4(T16_regs[i][0], T8_regs[i][0], T2_regs[i][0], state[i][0]);
		T27_regs[i][1] = XOR4(T16_regs[i][1], T8_regs[i][1], T2_regs[i][1], state[i][1]);
	}

	//T28
	for (int i = 0; i < 5; i++) {
		T28_regs[i][0] = XOR3(T16_regs[i][0], T8_regs[i][0], T4_regs[i][0]);
		T28_regs[i][1] = XOR3(T16_regs[i][1], T8_regs[i][1], T4_regs[i][1]);
	}

	//T29
	for (int i = 0; i < 5; i++) {
		T29_regs[i][0] = XOR4(T16_regs[i][0], T8_regs[i][0], T4_regs[i][0], state[i][0]);
		T29_regs[i][1] = XOR4(T16_regs[i][1], T8_regs[i][1], T4_regs[i][1], state[i][1]);
	}

	//Shuffle bytes upwards due to lines 1,2,3,4,5,6 in matrix.
	for (int i = 0; i < 5; i++) {
		temp_state[i][0].m256i_u64[0] = state[i][0].m256i_u64[3];
		temp_state[i][0].m256i_u64[1] = state[i][1].m256i_u64[0];
		temp_state[i][0].m256i_u64[2] = state[i][1].m256i_u64[1];
		temp_state[i][0].m256i_u64[3] = state[i][1].m256i_u64[2];
	}
	
	//handle last line of matrix. Align all rows on the third u64
	YMM aligned_last_row0[5];
	YMM aligned_last_row1[5];
	YMM aligned_last_row2[5];
	YMM aligned_last_row3[5];
	YMM aligned_last_row4[5];
	YMM aligned_last_row5[5];
	YMM aligned_last_row6[5];
	for (int i = 0; i < 5; i++) {
		aligned_last_row0[i] = _mm256_setr_epi64x(state[i][0].m256i_u64[0],		T2_regs[i][0].m256i_u64[0],		T11_regs[i][0].m256i_u64[0], 0);
		aligned_last_row1[i] = _mm256_setr_epi64x(T2_regs[i][0].m256i_u64[1],	T5_regs[i][0].m256i_u64[1],		T20_regs[i][0].m256i_u64[1], 0);
		aligned_last_row2[i] = _mm256_setr_epi64x(T15_regs[i][0].m256i_u64[2],	T28_regs[i][0].m256i_u64[2],	T3_regs[i][0].m256i_u64[2], 0);
		aligned_last_row3[i] = _mm256_setr_epi64x(T9_regs[i][0].m256i_u64[3],	T29_regs[i][0].m256i_u64[3],	T5_regs[i][0].m256i_u64[3], 0);
		aligned_last_row4[i] = _mm256_setr_epi64x(T9_regs[i][1].m256i_u64[0],	T27_regs[i][1].m256i_u64[0],	T4_regs[i][1].m256i_u64[0], 0);
		aligned_last_row5[i] = _mm256_setr_epi64x(T15_regs[i][1].m256i_u64[1],	T23_regs[i][1].m256i_u64[1],	T29_regs[i][1].m256i_u64[1], 0);
		aligned_last_row6[i] = _mm256_setr_epi64x(T2_regs[i][1].m256i_u64[2],	T11_regs[i][1].m256i_u64[2],	state[i][1].m256i_u64[2], 0);
	}

	YMM last_row_calculated[5];
	for (int i = 0; i < 5; i++) {
		last_row_calculated[i] = XOR7(aligned_last_row0[i], aligned_last_row1[i], aligned_last_row2[i], aligned_last_row3[i], aligned_last_row4[i], aligned_last_row5[i], aligned_last_row6[i]);
	}

	//Assign data to state
	for (int i = 0; i < 5; i++) {
		state[i][0] = temp_state[i][0];
		state[i][1] = XOR(temp_state[i][1], last_row_calculated[i]);
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

						   //T5
	for (int i = 0; i < 5; i++) {
		T5_regs[i][0] = XOR(state[i][0], T4_regs[i][0]);
		T5_regs[i][1] = XOR(state[i][1], T4_regs[i][1]);
	}

	//T9
	for (int i = 0; i < 5; i++) {
		T9_regs[i][0] = XOR(state[i][0], T8_regs[i][0]);
		T9_regs[i][1] = XOR(state[i][1], T8_regs[i][1]);
	}

	//T11
	for (int i = 0; i < 5; i++) {
		T11_regs[i][0] = XOR3(state[i][0], T8_regs[i][0], T2_regs[i][0]);
		T11_regs[i][1] = XOR3(state[i][1], T8_regs[i][1], T2_regs[i][1]);
	}

	//T15
	for (int i = 0; i < 5; i++) {
		T15_regs[i][0] = XOR4(state[i][0], T2_regs[i][0], T4_regs[i][0], T8_regs[i][0]);
		T15_regs[i][1] = XOR4(state[i][1], T2_regs[i][1], T4_regs[i][1], T8_regs[i][1]);
	}

	//T23
	for (int i = 0; i < 5; i++) {
		T23_regs[i][0] = XOR3(T16_regs[i][0], T5_regs[i][0], T2_regs[i][0]);
		T23_regs[i][1] = XOR3(T16_regs[i][1], T5_regs[i][1], T2_regs[i][1]);
	}

	//T27
	for (int i = 0; i < 5; i++) {
		T27_regs[i][0] = XOR4(T16_regs[i][0], T8_regs[i][0], T2_regs[i][0], state[i][0]);
		T27_regs[i][1] = XOR4(T16_regs[i][1], T8_regs[i][1], T2_regs[i][1], state[i][1]);
	}

	//T28
	for (int i = 0; i < 5; i++) {
		T28_regs[i][0] = XOR3(T16_regs[i][0], T8_regs[i][0], T4_regs[i][0]);
		T28_regs[i][1] = XOR3(T16_regs[i][1], T8_regs[i][1], T4_regs[i][1]);
	}

	//T29
	for (int i = 0; i < 5; i++) {
		T29_regs[i][0] = XOR4(T16_regs[i][0], T8_regs[i][0], T4_regs[i][0], state[i][0]);
		T29_regs[i][1] = XOR4(T16_regs[i][1], T8_regs[i][1], T4_regs[i][1], state[i][1]);
	}

	//Shuffle bytes upwards due to lines 1,2,3,4,5,6 in matrix.
	for (int i = 0; i < 5; i++) {
		temp_state[i][0].m256i_u64[0] = state[i][0].m256i_u64[2];
		temp_state[i][0].m256i_u64[1] = state[i][0].m256i_u64[3];
		temp_state[i][0].m256i_u64[2] = state[i][1].m256i_u64[0];
		temp_state[i][0].m256i_u64[3] = state[i][1].m256i_u64[1];

		temp_state[i][1].m256i_u64[0] = temp_state[i][1].m256i_u64[2];
	}

	//handle last line of matrix. Align all rows on the third u64
	YMM aligned_last_row0[5];
	YMM aligned_last_row1[5];
	YMM aligned_last_row2[5];
	YMM aligned_last_row3[5];
	YMM aligned_last_row4[5];
	YMM aligned_last_row5[5];
	YMM aligned_last_row6[5];
	for (int i = 0; i < 5; i++) {
		aligned_last_row0[i] = _mm256_setr_epi64x(0, state[i][0].m256i_u64[0],		T2_regs[i][0].m256i_u64[0], 0);
		aligned_last_row1[i] = _mm256_setr_epi64x(0, T2_regs[i][0].m256i_u64[1],	T5_regs[i][0].m256i_u64[1], 0);
		aligned_last_row2[i] = _mm256_setr_epi64x(0, T15_regs[i][0].m256i_u64[2],	T28_regs[i][0].m256i_u64[2], 0);
		aligned_last_row3[i] = _mm256_setr_epi64x(0, T9_regs[i][0].m256i_u64[3],	T29_regs[i][0].m256i_u64[3], 0);
		aligned_last_row4[i] = _mm256_setr_epi64x(0, T9_regs[i][1].m256i_u64[0],	T27_regs[i][1].m256i_u64[0], 0);
		aligned_last_row5[i] = _mm256_setr_epi64x(0, T15_regs[i][1].m256i_u64[1],	T23_regs[i][1].m256i_u64[1], 0);
		aligned_last_row6[i] = _mm256_setr_epi64x(0, T2_regs[i][1].m256i_u64[2],	T11_regs[i][1].m256i_u64[2], 0);
	}

	YMM last_row_calculated[5];
	for (int i = 0; i < 5; i++) {
		last_row_calculated[i] = XOR7(aligned_last_row0[i], aligned_last_row1[i], aligned_last_row2[i], aligned_last_row3[i], aligned_last_row4[i], aligned_last_row5[i], aligned_last_row6[i]);
	}

	//Assign data to state
	for (int i = 0; i < 5; i++) {
		state[i][0] = temp_state[i][0];
		state[i][1] = XOR(temp_state[i][1], last_row_calculated[i]);
	}
}

void mixcolumns_A1(__m256i (*state)[2]) {
	__m256i T2_regs[5][2];
	__m256i T4_regs[5][2];
	__m256i T8_regs[5][2];
	__m256i T9_regs[5][2];
	__m256i T15_regs[5][2];

	__m256i temp_state[5][2];



	//printf("Before: \n");
	//print_state_as_hex(state);

	//T2
	T2(state, T2_regs);

	T2(T2_regs, T4_regs); //T4
	T2(T4_regs, T8_regs); //T8

						  //T9
	for (int i = 0; i < 5; i++) {
		T9_regs[i][0] = XOR(state[i][0], T8_regs[i][0]);
		T9_regs[i][1] = XOR(state[i][1], T8_regs[i][1]);
	}

	//T15
	for (int i = 0; i < 5; i++) {
		T15_regs[i][0] = XOR4(state[i][0], T2_regs[i][0], T4_regs[i][0], T8_regs[i][0]);
		T15_regs[i][1] = XOR4(state[i][1], T2_regs[i][1], T4_regs[i][1], T8_regs[i][1]);
	}

	//Shuffle bytes upwards due to lines 1,2,3,4 in matrix.
	for (int i = 0; i < 5; i++) {
		temp_state[i][0].m256i_u64[0] = state[i][0].m256i_u64[1];
		temp_state[i][0].m256i_u64[1] = state[i][0].m256i_u64[2];
		temp_state[i][0].m256i_u64[2] = state[i][0].m256i_u64[3];
		temp_state[i][0].m256i_u64[3] = state[i][1].m256i_u64[0];

		temp_state[i][1].m256i_u64[0] = temp_state[i][1].m256i_u64[1];
		temp_state[i][1].m256i_u64[1] = temp_state[i][1].m256i_u64[2];
	}

	//handle last line of matrix. Align all rows on the third u64
	YMM aligned_last_row0[5];
	YMM aligned_last_row1[5];
	YMM aligned_last_row2[5];
	YMM aligned_last_row3[5];
	YMM aligned_last_row4[5];
	YMM aligned_last_row5[5];
	YMM aligned_last_row6[5];
	for (int i = 0; i < 5; i++) {
		aligned_last_row0[i] = _mm256_setr_epi64x(0, 0, state[i][0].m256i_u64[0], 0);
		aligned_last_row1[i] = _mm256_setr_epi64x(0, 0, T2_regs[i][0].m256i_u64[1], 0);
		aligned_last_row2[i] = _mm256_setr_epi64x(0, 0, T15_regs[i][0].m256i_u64[2], 0);
		aligned_last_row3[i] = _mm256_setr_epi64x(0, 0, T9_regs[i][0].m256i_u64[3], 0);
		aligned_last_row4[i] = _mm256_setr_epi64x(0, 0, T9_regs[i][1].m256i_u64[0], 0);
		aligned_last_row5[i] = _mm256_setr_epi64x(0, 0, T15_regs[i][1].m256i_u64[1], 0);
		aligned_last_row6[i] = _mm256_setr_epi64x(0, 0, T2_regs[i][1].m256i_u64[2], 0);
	}

	YMM last_row_calculated[5];
	for (int i = 0; i < 5; i++) {
		last_row_calculated[i] = XOR7(aligned_last_row0[i], aligned_last_row1[i], aligned_last_row2[i], aligned_last_row3[i], aligned_last_row4[i], aligned_last_row5[i], aligned_last_row6[i]);
	}

	//Assign data to state
	for (int i = 0; i < 5; i++) {
		state[i][0] = temp_state[i][0];
		state[i][1] = XOR(temp_state[i][1], last_row_calculated[i]);
	}
}


void T2(__m256i (*state)[2], __m256i (*new_state)[2]) {
	//Shift the bits in each primate element one to the left (i.e. shift bits between regs). and reduce with GF(2^5) mask x^5+x^2+1.
	__m256i GF_reduce_mask0 = state[4][0];
	__m256i GF_reduce_mask1 = state[4][1];

	new_state[0][0] = XOR(_mm256_setzero_si256(), GF_reduce_mask0);
	new_state[0][1] = XOR(_mm256_setzero_si256(), GF_reduce_mask1);

	new_state[1][0] = state[0][0];
	new_state[1][1] = state[0][1];

	new_state[2][0] = XOR(state[1][0], GF_reduce_mask0);
	new_state[2][1] = XOR(state[1][1], GF_reduce_mask1);

	new_state[3][0] = state[2][0];
	new_state[3][1] = state[2][1];

	new_state[4][0] = state[3][0];
	new_state[4][1] = state[3][1];
}