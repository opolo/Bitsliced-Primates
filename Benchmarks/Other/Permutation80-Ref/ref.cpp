// ref.cpp : main project file.

#include "api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include <float.h>
#include "primate.h"
#include "primate80.h"
#include "parameters.h"

typedef unsigned long long u64;
int cmpfunc(const void * a, const void * b);
void bench();

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

int main()
{
	printf("PRIMATEs Permutation only-80 reference implementation v1.00 \n");

	//Run only on one core
	SetThreadAffinityMask(GetCurrentThread(), 0x00000008); //Run on fourth core

	bench();

	printf("\n Done.");
	getchar();
}

void bench() {
	
	printf("Benchmarking 80bit reference permutations. \n");
	u64 start;
	u64 cpu_frequency;

	printf("Estimating cycle counter frequency... ");
	cpu_frequency = cpucycles();
	Sleep(5000);
	cpu_frequency = (cpucycles() - cpu_frequency) / 5;
	printf("%f GHz\n", cpu_frequency / 1e9);

	int iterations = 5000;
	int iterations_per_iterations = 10;

	u64 *results_p1 = (u64*)calloc(iterations, sizeof(u64));
	u64 *results_p2 = (u64*)calloc(iterations, sizeof(u64));
	u64 *results_p3 = (u64*)calloc(iterations, sizeof(u64));
	u64 *results_p4 = (u64*)calloc(iterations, sizeof(u64));

	u64 *results_inv_p1 = (u64*)calloc(iterations, sizeof(u64));
	u64 *results_inv_p2 = (u64*)calloc(iterations, sizeof(u64));
	u64 *results_inv_p3 = (u64*)calloc(iterations, sizeof(u64));
	u64 *results_inv_p4 = (u64*)calloc(iterations, sizeof(u64));

	printf("Press to start...\n");
	getchar();

	GenerateRoundConstants();
	for (int i = 0; i < iterations; i++) {
		unsigned char state[StateSize];

		if (i % (iterations / 10) == 0) {
			//One tenth through...
			printf("Progress: %i/10 through. \n", (i / (iterations / 10) + 1));
		}

		//p1
		memset(state, 0, StateSize);
		start = cpucycles();
		for (int n = 0; n < iterations_per_iterations; n++) {
			p_1(state);
		}
		results_p1[i] = cpucycles() - start;

		//p2
		memset(state, 0, StateSize);
		start = cpucycles();
		for (int n = 0; n < iterations_per_iterations; n++) {
			p_2(state);
		}
		results_p2[i] = cpucycles() - start;

		//p3
		memset(state, 0, StateSize);
		start = cpucycles();
		for (int n = 0; n < iterations_per_iterations; n++) {
			p_3(state);
		}
		results_p3[i] = cpucycles() - start;

		//p4
		memset(state, 0, StateSize);
		start = cpucycles();
		for (int n = 0; n < iterations_per_iterations; n++) {
			p_4(state);
		}
		results_p4[i] = cpucycles() - start;

		//inv_p1
		memset(state, 0, StateSize);
		start = cpucycles();
		for (int n = 0; n < iterations_per_iterations; n++) {
			p_1_inv(state);
		}
		results_inv_p1[i] = cpucycles() - start;

		//inv_p2
		memset(state, 0, StateSize);
		start = cpucycles();
		for (int n = 0; n < iterations_per_iterations; n++) {
			p_2_inv(state);
		}
		results_inv_p2[i] = cpucycles() - start;

		//inv_p3
		memset(state, 0, StateSize);
		start = cpucycles();
		for (int n = 0; n < iterations_per_iterations; n++) {
			p_3_inv(state);
		}
		results_inv_p3[i] = cpucycles() - start;

		//inv_p4
		memset(state, 0, StateSize);
		start = cpucycles();
		for (int n = 0; n < iterations_per_iterations; n++) {
			p_4_inv(state);
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
	printf("Median cycles per byte: %f \n", 1 / (5 / medianSpeed_p1));
	printf("\n");

	printf("*** P2 *** \n");
	printf("Median speed: %f \n", medianSpeed_p2);
	printf("Median cycles per byte: %f \n", 1 / (5 / medianSpeed_p2));
	printf("\n");

	printf("*** P3 *** \n");
	printf("Median speed: %f \n", medianSpeed_p3);
	printf("Median cycles per byte: %f \n", 1 / (5 / medianSpeed_p3));
	printf("\n");

	printf("*** P4 *** \n");
	printf("Median speed: %f \n", medianSpeed_p4);
	printf("Median cycles per byte: %f \n", 1 / (5 / medianSpeed_p4));
	printf("\n");

	printf("*** Inv_P1 *** \n");
	printf("Median speed: %f \n", medianSpeed_inv_p1);
	printf("Median cycles per byte: %f \n", 1 / (5 / medianSpeed_inv_p1));
	printf("\n");

	printf("*** Inv_P2 *** \n");
	printf("Median speed: %f \n", medianSpeed_inv_p2);
	printf("Median cycles per byte: %f \n", 1 / (5 / medianSpeed_inv_p2));
	printf("\n");

	printf("*** Inv_P3 *** \n");
	printf("Median speed: %f \n", medianSpeed_inv_p3);
	printf("Median cycles per byte: %f \n", 1 / (5 / medianSpeed_inv_p3));
	printf("\n");

	printf("*** Inv_P4 *** \n");
	printf("Median speed: %f \n", medianSpeed_inv_p4);
	printf("Median cycles per byte: %f \n", 1 / (5 / medianSpeed_inv_p4));
	printf("\n");
}

int cmpfunc(const void * a, const void * b)
{
	return (int)(*(u64*)a - *(u64*)b);
}