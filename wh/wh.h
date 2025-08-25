#ifndef _WH_H_
#define _WH_H_

#include <stdint.h>

typedef struct {
	const char *word;
	const char *pattern;
} WH_Guess;

typedef struct {
	char str[5];
	char _null;
	double info;
} WH_Word;

uint64_t wh_filter(uint64_t word_count, WH_Word *words, WH_Guess guess);
void wh_calculate(uint64_t word_count, WH_Word *words);
void wh_sort(uint64_t word_count, WH_Word *words);

#endif
