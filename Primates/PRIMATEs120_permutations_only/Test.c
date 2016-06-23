#include <stdio.h>
#include "Primate.h"
#include <time.h>
#include <float.h>
#include <Windows.h>
#include <stdlib.h>

int cmpfunc(const void * a, const void * b);


void main() {
	LARGE_INTEGER start, finish;
	double cpu_frequency;
	QueryPerformanceFrequency(&start);
	cpu_frequency = (double)(start.QuadPart) / 1000.0; //Frequency: Ticks per milisecond on the system.

	printf("CPU frequency: %f \n", cpu_frequency);

	int iterations = 500'000;
	int iterations_per_iterations = 10;
		
	double *results_p1 = calloc(iterations, sizeof(double));
	double *results_p2 = calloc(iterations, sizeof(double));
	double *results_p3 = calloc(iterations, sizeof(double));
	double *results_p4 = calloc(iterations, sizeof(double));
		
	double *results_inv_p1 = calloc(iterations, sizeof(double));
	double *results_inv_p2 = calloc(iterations, sizeof(double));
	double *results_inv_p3 = calloc(iterations, sizeof(double));
	double *results_inv_p4 = calloc(iterations, sizeof(double));

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
		QueryPerformanceCounter(&start);
		for (int n = 0; n < iterations_per_iterations; n++) {
			p1(state);
		}
		QueryPerformanceCounter(&finish);
		results_p1[i] = (double)(finish.QuadPart - start.QuadPart);

		//p2
		for (int j = 0; j < 5; j++) {
			state[j][0] = _mm256_setzero_si256();
			state[j][1] = _mm256_setzero_si256();
		}
		QueryPerformanceCounter(&start);
		for (int n = 0; n < iterations_per_iterations; n++) {
			p2(state);
		}
		QueryPerformanceCounter(&finish);
		results_p2[i] = (double)(finish.QuadPart - start.QuadPart);

		//p3
		for (int j = 0; j < 5; j++) {
			state[j][0] = _mm256_setzero_si256();
			state[j][1] = _mm256_setzero_si256();
		}
		QueryPerformanceCounter(&start);
		for (int n = 0; n < iterations_per_iterations; n++) {
			p3(state);
		}
		QueryPerformanceCounter(&finish);
		results_p3[i] = (double)(finish.QuadPart - start.QuadPart);

		//p4
		for (int j = 0; j < 5; j++) {
			state[j][0] = _mm256_setzero_si256();
			state[j][1] = _mm256_setzero_si256();
		}
		QueryPerformanceCounter(&start);
		for (int n = 0; n < iterations_per_iterations; n++) {
			p4(state);
		}
		QueryPerformanceCounter(&finish);
		results_p4[i] = (double)(finish.QuadPart - start.QuadPart);

		//inv_p1
		for (int j = 0; j < 5; j++) {
			state[j][0] = _mm256_setzero_si256();
			state[j][1] = _mm256_setzero_si256();
		}
		QueryPerformanceCounter(&start);
		for (int n = 0; n < iterations_per_iterations; n++) {
			p1_inv(state);
		}
		QueryPerformanceCounter(&finish);
		results_inv_p1[i] = (double)(finish.QuadPart - start.QuadPart);

		//inv_p2
		for (int j = 0; j < 5; j++) {
			state[j][0] = _mm256_setzero_si256();
			state[j][1] = _mm256_setzero_si256();
		}
		QueryPerformanceCounter(&start);
		for (int n = 0; n < iterations_per_iterations; n++) {
			p2_inv(state);
		}
		QueryPerformanceCounter(&finish);
		results_inv_p2[i] = (double)(finish.QuadPart - start.QuadPart);

		//inv_p3
		for (int j = 0; j < 5; j++) {
			state[j][0] = _mm256_setzero_si256();
			state[j][1] = _mm256_setzero_si256();
		}
		QueryPerformanceCounter(&start);
		for (int n = 0; n < iterations_per_iterations; n++) {
			p3_inv(state);
		}
		QueryPerformanceCounter(&finish);
		results_inv_p3[i] = (double)(finish.QuadPart - start.QuadPart);

		//inv_p4
		for (int j = 0; j < 5; j++) {
			state[j][0] = _mm256_setzero_si256();
			state[j][1] = _mm256_setzero_si256();
		}
		QueryPerformanceCounter(&start);
		for (int n = 0; n < iterations_per_iterations; n++) {
			p4_inv(state);
		}
		QueryPerformanceCounter(&finish);
		results_inv_p4[i] = (double)(finish.QuadPart - start.QuadPart);
	}

	//Calculate results
	qsort(results_p1, iterations, sizeof(double), cmpfunc);
	qsort(results_p2, iterations, sizeof(double), cmpfunc);
	qsort(results_p3, iterations, sizeof(double), cmpfunc);
	qsort(results_p4, iterations, sizeof(double), cmpfunc);
	qsort(results_inv_p1, iterations, sizeof(double), cmpfunc);
	qsort(results_inv_p2, iterations, sizeof(double), cmpfunc);
	qsort(results_inv_p3, iterations, sizeof(double), cmpfunc);
	qsort(results_inv_p4, iterations, sizeof(double), cmpfunc);
	double medianSpeed_p1 = results_p1[iterations / 2] / iterations_per_iterations;
	double medianSpeed_p2 = results_p2[iterations / 2] / iterations_per_iterations;
	double medianSpeed_p3 = results_p3[iterations / 2] / iterations_per_iterations;
	double medianSpeed_p4 = results_p4[iterations / 2] / iterations_per_iterations;
	double medianSpeed_inv_p1 = results_inv_p1[iterations / 2] / iterations_per_iterations;
	double medianSpeed_inv_p2 = results_inv_p2[iterations / 2] / iterations_per_iterations;
	double medianSpeed_inv_p3 = results_inv_p3[iterations / 2] / iterations_per_iterations;
	double medianSpeed_inv_p4 = results_inv_p4[iterations / 2] / iterations_per_iterations;

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


	getchar();
}

int cmpfunc(const void * a, const void * b)
{
	return (int)(*(double*)a - *(double*)b);
}