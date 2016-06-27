/*
Perform benchmarking the way NORX does it: 
https://github.com/norx/norx/blob/master/utils/bench.c

*/

#define _GNU_SOURCE

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Encrypt.h"
#include <time.h>
#include <Windows.h>

void benchNow();

static int bench_cmp(const void *x, const void *y)
{
	const int64_t *ix = (const int64_t *)x;
	const int64_t *iy = (const int64_t *)y;
	return *ix - *iy;
}

#if   defined(__i386__)
static unsigned long long cpucycles(void)
{
	unsigned long long result;
	__asm__ __volatile__
	(
		".byte 15;.byte 49"
		: "=A" (result)
	);
	return result;
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

void frequency()
{
	uint64_t t;
	printf("Estimating cycle counter frequency...");
	t = cpucycles();
	Sleep(1000);
	t = cpucycles() - t;
	printf("%f GHz\n", t / 1e9);
}

void bench()
{
#define BENCH_TRIALS     32
#define BENCH_MAXLEN   1536
	static unsigned char  in[4096];
	static unsigned char out[4096 + 32];
	static unsigned char  ad[4096];
	static unsigned char   n[32];
	static unsigned char   k[32];
	static unsigned long long outlen;
	static unsigned long long adlen = 0;
	static unsigned long long median[4096 + 1];
	int i, j;

	printf("#bytes  median  per byte\n");

	/* 1 ... BENCH_MAXLEN */
	for (j = 0; j <= 4096; ++j)
	{
		uint64_t cycles[BENCH_TRIALS + 1];

		for (i = 0; i <= BENCH_TRIALS; ++i)
		{
			cycles[i] = cpucycles();
			crypto_aead_encrypt(out, &outlen, in, j, ad, adlen, NULL, n, k);
		}

		for (i = 0; i < BENCH_TRIALS; ++i)
			cycles[i] = cycles[i + 1] - cycles[i];

		qsort(cycles, BENCH_TRIALS, sizeof(uint64_t), bench_cmp);
		median[j] = cycles[BENCH_TRIALS / 2];
	}

	for (j = 0; j <= BENCH_MAXLEN; j += 8)
		printf("%5d, %7.2f\n", j, (double)median[j] / j);

	printf("#2048   %6llu   %7.2f\n", median[2048], (double)median[2048] / 2048.0);
	printf("#4096   %6llu   %7.2f\n", median[4096], (double)median[4096] / 4096.0);
	printf("#long     long   %7.2f\n", (double)(median[4096] - median[2048]) / 2048.0);
}

void benchNow()
{
	frequency();
	bench();
}