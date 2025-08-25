#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "wh.h"

void *file_load(const char *path, uint64_t item_size, uint64_t *out_item_count);

int main(int argc, char *argv[]) {
	uint64_t infos_count;
	WH_Word *infos = file_load("infos.bin", sizeof(WH_Word), &infos_count);

	uint64_t word_count = 0;
	WH_Word *words = NULL;
	if (infos == NULL) {
		uint64_t words_str_size;
		char *words_str = file_load("words.txt", sizeof(char), &words_str_size);

		word_count = words_str_size / (7);

		words = malloc(sizeof(WH_Word) * word_count);
		for (uint64_t i = 0; i < word_count; i++) {
			for (uint64_t j = 0; j < 5; j++) {
				words[i].str[j] = words_str[i * 7 + j];
			}
			words[i]._null = '\0';
			words[i].info = 0.0;
		}

		printf("Recalculating...\n");
		wh_calculate(word_count, words);

		printf("Sorting...\n");
		wh_sort(word_count, words);

		printf("Caching results...\n");
		FILE *fout = fopen("infos.bin", "wb");
		uint64_t written = fwrite(words, sizeof(WH_Word), word_count, fout); 
		if (written != word_count) {
			printf("Written count(%llu) is different from word_count(%llu)\n", written, word_count);
		}
		fclose(fout);
	} else {
		printf("Found infos.bin, loading cache...\n");
		word_count = infos_count;
		words = (WH_Word *)infos;
	}


	do {
		{
			uint64_t recommended_count = 10;
			if (word_count < 10) recommended_count = word_count;
			printf("Recommended ( %llu / %llu ):\n", recommended_count, word_count);
			for (uint64_t i = 0; i < recommended_count; i++) {
				printf("%llu. %s %.2f\n", i + 1, words[i].str, words[i].info);
			}
		}
		char word[6];
		char pattern[6];
		{
			uint64_t input_ok;
			do {
				input_ok = 1;

				printf("Enter the word and pattern:\n");
				int res = scanf("%5s %5s", word, pattern);
				
				if (res != 2 || strlen(word) != 5 || strlen(pattern) != 5) {
					printf("Input NOT ok.\n");
					input_ok = 0;
				}
			} while (!input_ok);
		}

		WH_Guess guess = {
			.word = word,
			.pattern = pattern
		};
		printf("Filtering...\n");
		word_count = wh_filter(word_count, words, guess);
		printf("Done. New word count %llu.\n", word_count);

		printf("Calculating...\n");
		wh_calculate(word_count, words);

		printf("Sorting...\n");
		wh_sort(word_count, words);

		printf("Done.\n");
	} while (word_count > 1);

	if (word_count == 0) {
		printf("No word found.\n");
	} else {
		printf("Word found: %s\n", words[0].str);
	}

	free(words);
}

void *file_load(const char *path, uint64_t item_size, uint64_t *out_item_count) {
	FILE *f = fopen(path, "rb");
	if (f == NULL) {
		return NULL;
	}

	fseek(f, 0, SEEK_END);
	uint64_t size = ftell(f);
	fseek(f, 0, SEEK_SET);

	uint64_t item_count = size / item_size;
	if (item_count * item_size != size) {
		printf("%llu * %llu != %llu\n", item_count, item_size, size);
		fclose(f);
		return NULL;
	}

	char *buffer = malloc(size + 1);
	fread(buffer, item_size, item_count, f);
	buffer[size] = '\0';

	fclose(f);

	if (out_item_count != NULL) {
		*out_item_count = item_count;
	}
	return buffer;
}
