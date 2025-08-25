#include <stdint.h>
#include <math.h>

#include "wh.h"

#define WORD_LEN 5

uint64_t wh_word_fits(
	const char *word,
	const char *guess,
	const char *pattern
) {
	uint8_t required_count[26] = { 0 };
	uint8_t word_counts[26] = { 0 };
	for (uint64_t i = 0; i < WORD_LEN; i++) {
		word_counts[word[i] - 'a'] += 1; 
		switch (pattern[i]) {
			case '!':
			{
				for (uint64_t j = 0; j < WORD_LEN; j++) {
					if (guess[i] == word[j]) {
						if (guess[j] == guess[i]) {
							if (pattern[j] == '!') {
								return 0;
							}
						} else {
							return 0;
						}
					}
				}
				break;
			}
			case '?':
			{
				required_count[guess[i] - 'a'] += 1;
				if (guess[i] == word[i]) {
					return 0;
				}
				break;
			}
			case '=':
			{
				required_count[guess[i] - 'a'] += 1;
				if (guess[i] != word[i]) {
					return 0;
				}
				break;
			}
		}
	}
	for (uint64_t j = 0; j < 26; j++) {
		if (required_count[j] != 0) {
			if (word_counts[j] < required_count[j]) {
				return 0;
			}
		} 
	}
	return 1;
}

double wh_word_expected_info(
	uint64_t word_count,
	const WH_Word *words,
	const char *word
) {
	const char PV[3] = {'!', '?', '='};
	double info = 0.0;
	char pattern[6] = {PV[0]};
	pattern[5] = '\0';
	for (uint64_t a = 0; a < 3; a++) {
		pattern[0] = PV[a];
		for (uint64_t b = 0; b < 3; b++) {
			pattern[1] = PV[b];
			for (uint64_t c = 0; c < 3; c++) {
				pattern[2] = PV[c];
				for (uint64_t d = 0; d < 3; d++) {
					pattern[3] = PV[d];
					for (uint64_t e = 0; e < 3; e++) {
						pattern[4] = PV[e];

						uint64_t count = 0;
						for (uint64_t i = 0; i < word_count; i++) {
							count += wh_word_fits(words[i].str, word, pattern);
						}

						if (count != 0) {
							double p = (double) count / (double) word_count;
							double I = log2(1.0 / p);
							double pI = p * I;
							info += pI;
						}
					}
				}
			}
		}
	}
	return info;
}
uint64_t wh_filter(uint64_t word_count, WH_Word *words, WH_Guess guess) {
	uint64_t i = 0;
	while (i < word_count) {
		WH_Word *word = words + i;
		if (!wh_word_fits(word->str, guess.word, guess.pattern)) {
			word_count--;
			words[i] = words[word_count];
		} else {
			i++;
		}
	}
	return word_count;
}

void wh_calculate(uint64_t word_count, WH_Word *words) {
	for (uint64_t i = 0; i < word_count; i++) {
		words[i].info = wh_word_expected_info(word_count, words, words[i].str);
	}
}

void wh_sort(uint64_t word_count, WH_Word *words) {
	for (uint64_t y = 0; y < word_count - 1; y++) {
		for (uint64_t x = y + 1; x < word_count; x++) {
			if (words[x].info > words[y].info) {
				WH_Word temp = words[x];
				words[x] = words[y];
				words[y] = temp;
			}
		}
	}
}

