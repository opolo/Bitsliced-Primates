#include <stdio.h>
#include "Primate.h"
#include <time.h>
#include <float.h>
#include <Windows.h>
#include <stdlib.h>

int cmpfunc(const void * a, const void * b);
void benchmark120bit();
void benchmark80bit();

#if (_MSC_VER == 1900)
static unsigned long long cpucycles(void)
{
	return __rdtsc();
}
#elif defined(__x86_64__)
static unsigned long long cpucycles(void)
{
	unsigned long long result;
	__asm__ __volatile__
	(
		".byte 15;.byte 49\n"
		"shlq $32,%%rdx\n"
		"orq %%rdx,%%rax"
		: "=a" (result) ::  "%rdx"
	);
	return result;
}
#endif

void main() {
	
	printf("PRIMATEs permutation only V1.00 \n");

	//Run only on one core
	SetThreadAffinityMask(GetCurrentThread(), 0x00000008); //Run on fourth core

	benchmark80bit();
	benchmark120bit();

	getchar();
	
}

void benchmark120bit() {
	u64 start;
	u64 cpu_frequency;
	printf("Testing 120-bit performance. \n");
	printf("Estimating cycle counter frequency... ");
	cpu_frequency = cpucycles();
	Sleep(5000);
	cpu_frequency = (cpucycles() - cpu_frequency) / 5;
	printf("%f GHz\n", cpu_frequency / 1e9);

	int iterations = 50000;
	int iterations_per_iterations = 10;

	u64 *results_p1 = calloc(iterations, sizeof(u64));
	u64 *results_p2 = calloc(iterations, sizeof(u64));
	u64 *results_p3 = calloc(iterations, sizeof(u64));
	u64 *results_p4 = calloc(iterations, sizeof(u64));

	u64 *results_inv_p1 = calloc(iterations, sizeof(u64));
	u64 *results_inv_p2 = calloc(iterations, sizeof(u64));
	u64 *results_inv_p3 = calloc(iterations, sizeof(u64));
	u64 *results_inv_p4 = calloc(iterations, sizeof(u64));

	for (int i = 0; i < iterations; i++) {
		YMM state[5][2];

		if (i % (iterations / 10) == 0) {
			//One tenth through...
			printf("Progress: %i/10 through. \n", (i / (iterations / 10) + 1));
		}

		//p1
		for (int j = 0; j < 5; j++) {
			state[j][0] = _mm256_setzero_si256();
			state[j][1] = _mm256_setzero_si256();
		}
		start = cpucycles();
		for (int n = 0; n < iterations_per_iterations; n++) {
			p1(state);
		}
		results_p1[i] = cpucycles() - start;

		//p2
		for (int j = 0; j < 5; j++) {
			state[j][0] = _mm256_setzero_si256();
			state[j][1] = _mm256_setzero_si256();
		}
		start = cpucycles();
		for (int n = 0; n < iterations_per_iterations; n++) {
			p2(state);
		}
		results_p2[i] = cpucycles() - start;

		//p3
		for (int j = 0; j < 5; j++) {
			state[j][0] = _mm256_setzero_si256();
			state[j][1] = _mm256_setzero_si256();
		}
		start = cpucycles();
		for (int n = 0; n < iterations_per_iterations; n++) {
			p3(state);
		}
		results_p3[i] = cpucycles() - start;

		//p4
		for (int j = 0; j < 5; j++) {
			state[j][0] = _mm256_setzero_si256();
			state[j][1] = _mm256_setzero_si256();
		}
		start = cpucycles();
		for (int n = 0; n < iterations_per_iterations; n++) {
			p4(state);
		}
		results_p4[i] = cpucycles() - start;

		//inv_p1
		for (int j = 0; j < 5; j++) {
			state[j][0] = _mm256_setzero_si256();
			state[j][1] = _mm256_setzero_si256();
		}
		start = cpucycles();
		for (int n = 0; n < iterations_per_iterations; n++) {
			p1_inv(state);
		}
		results_inv_p1[i] = cpucycles() - start;

		//inv_p2
		for (int j = 0; j < 5; j++) {
			state[j][0] = _mm256_setzero_si256();
			state[j][1] = _mm256_setzero_si256();
		}
		start = cpucycles();
		for (int n = 0; n < iterations_per_iterations; n++) {
			p2_inv(state);
		}
		results_inv_p2[i] = cpucycles() - start;

		//inv_p3
		for (int j = 0; j < 5; j++) {
			state[j][0] = _mm256_setzero_si256();
			state[j][1] = _mm256_setzero_si256();
		}
		start = cpucycles();
		for (int n = 0; n < iterations_per_iterations; n++) {
			p3_inv(state);
		}
		results_inv_p3[i] = cpucycles() - start;

		//inv_p4
		for (int j = 0; j < 5; j++) {
			state[j][0] = _mm256_setzero_si256();
			state[j][1] = _mm256_setzero_si256();
		}
		start = cpucycles();
		for (int n = 0; n < iterations_per_iterations; n++) {
			p4_inv(state);
		}
		results_inv_p4[i] = cpucycles() - start;
	}

	//Calculate results
	qsort(results_p1, iterations, sizeof(u64), cmpfunc);
	qsort(results_p2, iterations, sizeof(u64), cmpfunc);
	qsort(results_p3, iterations, sizeof(u64), cmpfunc);
	qsort(results_p4, iterations, sizeof(u64), cmpfunc);
	qsort(results_inv_p1, iterations, sizeof(u64), cmpfunc);
	qsort(results_inv_p2, iterations, sizeof(u64), cmpfunc);
	qsort(results_inv_p3, iterations, sizeof(u64), cmpfunc);
	qsort(results_inv_p4, iterations, sizeof(u64), cmpfunc);
	double medianSpeed_p1 = (double)results_p1[iterations / 2] / (double)iterations_per_iterations;
	double medianSpeed_p2 = (double)results_p2[iterations / 2] / (double)iterations_per_iterations;
	double medianSpeed_p3 = (double)results_p3[iterations / 2] / (double)iterations_per_iterations;
	double medianSpeed_p4 = (double)results_p4[iterations / 2] / (double)iterations_per_iterations;
	double medianSpeed_inv_p1 = (double)results_inv_p1[iterations / 2] / (double)iterations_per_iterations;
	double medianSpeed_inv_p2 = (double)results_inv_p2[iterations / 2] / (double)iterations_per_iterations;
	double medianSpeed_inv_p3 = (double)results_inv_p3[iterations / 2] / (double)iterations_per_iterations;
	double medianSpeed_inv_p4 = (double)results_inv_p4[iterations / 2] / (double)iterations_per_iterations;

	//Output results:
	printf("Iterations: %i \n\n", iterations);

	printf("*** P1 *** \n");
	printf("Median speed: %f \n", medianSpeed_p1);
	printf("Median cycles per byte: %f \n", 1 / (40 / medianSpeed_p1));
	printf("\n");

	printf("*** P2 *** \n");
	printf("Median speed: %f \n", medianSpeed_p2);
	printf("Median cycles per byte: %f \n", 1 / (40 / medianSpeed_p2));
	printf("\n");

	printf("*** P3 *** \n");
	printf("Median speed: %f \n", medianSpeed_p3);
	printf("Median cycles per byte: %f \n", 1 / (40 / medianSpeed_p3));
	printf("\n");

	printf("*** P4 *** \n");
	printf("Median speed: %f \n", medianSpeed_p4);
	printf("Median cycles per byte: %f \n", 1 / (40 / medianSpeed_p4));
	printf("\n");

	printf("*** Inv_P1 *** \n");
	printf("Median speed: %f \n", medianSpeed_inv_p1);
	printf("Median cycles per byte: %f \n", 1 / (40 / medianSpeed_inv_p1));
	printf("\n");

	printf("*** Inv_P2 *** \n");
	printf("Median speed: %f \n", medianSpeed_inv_p2);
	printf("Median cycles per byte: %f \n", 1 / (40 / medianSpeed_inv_p2));
	printf("\n");

	printf("*** Inv_P3 *** \n");
	printf("Median speed: %f \n", medianSpeed_inv_p3);
	printf("Median cycles per byte: %f \n", 1 / (40 / medianSpeed_inv_p3));
	printf("\n");

	printf("*** Inv_P4 *** \n");
	printf("Median speed: %f \n", medianSpeed_inv_p4);
	printf("Median cycles per byte: %f \n", 1 / (40 / medianSpeed_inv_p4));
	printf("\n");
}

void benchmark80bit() {
	u64 start;
	u64 cpu_frequency;

	printf("Testing 80-bit performance. \n");
	printf("Estimating cycle counter frequency... ");
	cpu_frequency = cpucycles();
	Sleep(5000);
	cpu_frequency = (cpucycles() - cpu_frequency) / 5;
	printf("%f GHz\n", cpu_frequency / 1e9);

	int iterations = 50000;
	int iterations_per_iterations = 10;

	u64 *results_p1 = calloc(iterations, sizeof(u64));
	u64 *results_p2 = calloc(iterations, sizeof(u64));
	u64 *results_p3 = calloc(iterations, sizeof(u64));
	u64 *results_p4 = calloc(iterations, sizeof(u64));

	u64 *results_inv_p1 = calloc(iterations, sizeof(u64));
	u64 *results_inv_p2 = calloc(iterations, sizeof(u64));
	u64 *results_inv_p3 = calloc(iterations, sizeof(u64));
	u64 *results_inv_p4 = calloc(iterations, sizeof(u64));

	for (int i = 0; i < iterations; i++) {
		YMM state[5][2];

		if (i % (iterations / 10) == 0) {
			//One tenth through...
			printf("Progress: %i/10 through. \n", (i / (iterations / 10) + 1));
		}

		//p1
		for (int j = 0; j < 5; j++) {
			state[j][0] = _mm256_setzero_si256();
			state[j][1] = _mm256_setzero_si256();
		}
		start = cpucycles();
		for (int n = 0; n < iterations_per_iterations; n++) {
			p1_80bit(state);
		}
		results_p1[i] = cpucycles() - start;

		//p2
		for (int j = 0; j < 5; j++) {
			state[j][0] = _mm256_setzero_si256();
			state[j][1] = _mm256_setzero_si256();
		}
		start = cpucycles();
		for (int n = 0; n < iterations_per_iterations; n++) {
			p2_80bit(state);
		}
		results_p2[i] = cpucycles() - start;

		//p3
		for (int j = 0; j < 5; j++) {
			state[j][0] = _mm256_setzero_si256();
			state[j][1] = _mm256_setzero_si256();
		}
		start = cpucycles();
		for (int n = 0; n < iterations_per_iterations; n++) {
			p3_80bit(state);
		}
		results_p3[i] = cpucycles() - start;

		//p4
		for (int j = 0; j < 5; j++) {
			state[j][0] = _mm256_setzero_si256();
			state[j][1] = _mm256_setzero_si256();
		}
		start = cpucycles();
		for (int n = 0; n < iterations_per_iterations; n++) {
			p4_80bit(state);
		}
		results_p4[i] = cpucycles() - start;

		//inv_p1
		for (int j = 0; j < 5; j++) {
			state[j][0] = _mm256_setzero_si256();
			state[j][1] = _mm256_setzero_si256();
		}
		start = cpucycles();
		for (int n = 0; n < iterations_per_iterations; n++) {
			p1_inv_80bit(state);
		}
		results_inv_p1[i] = cpucycles() - start;

		//inv_p2
		for (int j = 0; j < 5; j++) {
			state[j][0] = _mm256_setzero_si256();
			state[j][1] = _mm256_setzero_si256();
		}
		start = cpucycles();
		for (int n = 0; n < iterations_per_iterations; n++) {
			p2_inv_80bit(state);
		}
		results_inv_p2[i] = cpucycles() - start;

		//inv_p3
		for (int j = 0; j < 5; j++) {
			state[j][0] = _mm256_setzero_si256();
			state[j][1] = _mm256_setzero_si256();
		}
		start = cpucycles();
		for (int n = 0; n < iterations_per_iterations; n++) {
			p3_inv_80bit(state);
		}
		results_inv_p3[i] = cpucycles() - start;

		//inv_p4
		for (int j = 0; j < 5; j++) {
			state[j][0] = _mm256_setzero_si256();
			state[j][1] = _mm256_setzero_si256();
		}
		start = cpucycles();
		for (int n = 0; n < iterations_per_iterations; n++) {
			p4_inv_80bit(state);
		}
		results_inv_p4[i] = cpucycles() - start;
	}

	//Calculate results
	qsort(results_p1, iterations, sizeof(u64), cmpfunc);
	qsort(results_p2, iterations, sizeof(u64), cmpfunc);
	qsort(results_p3, iterations, sizeof(u64), cmpfunc);
	qsort(results_p4, iterations, sizeof(u64), cmpfunc);
	qsort(results_inv_p1, iterations, sizeof(u64), cmpfunc);
	qsort(results_inv_p2, iterations, sizeof(u64), cmpfunc);
	qsort(results_inv_p3, iterations, sizeof(u64), cmpfunc);
	qsort(results_inv_p4, iterations, sizeof(u64), cmpfunc);
	double medianSpeed_p1 = (double)results_p1[iterations / 2] / (double)iterations_per_iterations;
	double medianSpeed_p2 = (double)results_p2[iterations / 2] / (double)iterations_per_iterations;
	double medianSpeed_p3 = (double)results_p3[iterations / 2] / (double)iterations_per_iterations;
	double medianSpeed_p4 = (double)results_p4[iterations / 2] / (double)iterations_per_iterations;
	double medianSpeed_inv_p1 = (double)results_inv_p1[iterations / 2] / (double)iterations_per_iterations;
	double medianSpeed_inv_p2 = (double)results_inv_p2[iterations / 2] / (double)iterations_per_iterations;
	double medianSpeed_inv_p3 = (double)results_inv_p3[iterations / 2] / (double)iterations_per_iterations;
	double medianSpeed_inv_p4 = (double)results_inv_p4[iterations / 2] / (double)iterations_per_iterations;

	//Output results:
	printf("Iterations: %i \n\n", iterations);

	printf("*** P1 *** \n");
	printf("Median speed: %f \n", medianSpeed_p1);
	printf("Median cycles per byte: %f \n", 1 / (40 / medianSpeed_p1));
	printf("\n");

	printf("*** P2 *** \n");
	printf("Median speed: %f \n", medianSpeed_p2);
	printf("Median cycles per byte: %f \n", 1 / (40 / medianSpeed_p2));
	printf("\n");

	printf("*** P3 *** \n");
	printf("Median speed: %f \n", medianSpeed_p3);
	printf("Median cycles per byte: %f \n", 1 / (40 / medianSpeed_p3));
	printf("\n");

	printf("*** P4 *** \n");
	printf("Median speed: %f \n", medianSpeed_p4);
	printf("Median cycles per byte: %f \n", 1 / (40 / medianSpeed_p4));
	printf("\n");

	printf("*** Inv_P1 *** \n");
	printf("Median speed: %f \n", medianSpeed_inv_p1);
	printf("Median cycles per byte: %f \n", 1 / (40 / medianSpeed_inv_p1));
	printf("\n");

	printf("*** Inv_P2 *** \n");
	printf("Median speed: %f \n", medianSpeed_inv_p2);
	printf("Median cycles per byte: %f \n", 1 / (40 / medianSpeed_inv_p2));
	printf("\n");

	printf("*** Inv_P3 *** \n");
	printf("Median speed: %f \n", medianSpeed_inv_p3);
	printf("Median cycles per byte: %f \n", 1 / (40 / medianSpeed_inv_p3));
	printf("\n");

	printf("*** Inv_P4 *** \n");
	printf("Median speed: %f \n", medianSpeed_inv_p4);
	printf("Median cycles per byte: %f \n", 1 / (40 / medianSpeed_inv_p4));
	printf("\n");
}

int cmpfunc(const void * a, const void * b)
{
	return (int)(*(u64*)a - *(u64*)b);
}