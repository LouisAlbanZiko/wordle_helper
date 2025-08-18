#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define WORD_DISABLED(word) (word & 0x80000000)
#define WORD_DISABLE(word) word |= 0x80000000

#define ALLOWED_ALL 0xFFFFFFFF
typedef struct {
	uint32_t allowed;
	char found;
} Letter;

typedef struct {
	uint32_t required;
	uint32_t allowed;
	Letter letters[5];
} State;

void state_init(State *state) {
	state->required = 0;
	state->allowed = ALLOWED_ALL;
	for (uint64_t i = 0; i < 5; i++) {
		state->letters[i].allowed = ALLOWED_ALL;
		state->letters[i].found = 0;
	}
}

void state_disallow_char(State *state, char c) {
	state->allowed &= ~(0x1 << (c - 'a'));
}

void state_require_char(State *state, char c) {
	state->required |= 0x1 << (c - 'a');
}

void letter_disallow_char(Letter *letter, char c) {
	letter->allowed &= ~(0x1 << (c - 'a'));
}

void letter_found(Letter *letter, char c) {
	letter->found = c;
}

#define PATTERN_TYPE_GRAY 0
#define PATTERN_TYPE_YELLOW 1
#define PATTERN_TYPE_GREEN 2

typedef uint8_t Pattern[5];

void state_add_guess(const uint64_t u_guess, const Pattern pattern, State *state) {
	const char *guess = (const char *)&u_guess;
	for (uint64_t i = 0; i < 5; i++) {
		switch (pattern[i]) {
			case PATTERN_TYPE_GRAY:
			{
				state_disallow_char(state, guess[i]);
				break;
			}
			case PATTERN_TYPE_YELLOW:
			{
				letter_disallow_char(&state->letters[i], guess[i]);
				state_require_char(state, guess[i]);
				break;
			}
			case PATTERN_TYPE_GREEN:
			{
				letter_found(&state->letters[i], guess[i]);
				break;
			}
			default:
			{
				break;
			}
		}
	}
}

uint64_t word_fits_state(const uint64_t word_u, const State state, uint64_t debug) {
	uint64_t found_required = 0;
	const char *word = (const char *)&word_u;
	for (uint64_t i = 0; i < 5; i++) {
		char c = word[i];
		if (debug) printf("checking '%c'\n", c);
		if (state.required & (0x1 << (c - 'a'))) {
			found_required |= 0x1 << (c - 'a');
		}
		if ((state.allowed & (0x1 << (c - 'a'))) == 0) {
			if (debug) printf("\tnot allowed in state\n");
			return 0;
		}
		if (state.letters[i].found != 0 && state.letters[i].found != c) {
			if (debug) printf("\tLetter found '%c' != '%c'\n", state.letters[i].found, c);
			return 0;
		}
		if ((state.letters[i].allowed & (0x1 << (c - 'a'))) == 0) {
			if (debug) printf("\tnot allowed in letter\n");
			return 0;
		}
	}
	return state.required == found_required;
}


uint64_t count_words(const uint64_t *words, const uint64_t word_count, const State state, uint64_t debug) {
	uint64_t count = 0;
	for (uint64_t i = 0; i < word_count; i++) {
		if (!WORD_DISABLED(words[i])) {
			if (debug) printf("word: '%s'\n", (char *)(words + i));
			count += word_fits_state(words[i], state, debug);
		}
	}
	return count;
}

double expected_info(const uint64_t *words, const uint64_t word_count, const uint64_t cword_count, uint64_t word_guess, State current_state, uint64_t debug) {
	double info = 0.0;
	Pattern pattern = {0, 0, 0, 0, 0};
	for (uint64_t a = 0; a < 3; a++) {
		pattern[0] = a;
		for (uint64_t b = 0; b < 3; b++) {
			pattern[1] = b;
			for (uint64_t c = 0; c < 3; c++) {
				pattern[2] = c;
				for (uint64_t d = 0; d < 3; d++) {
					pattern[3] = d;
					for (uint64_t e = 0; e < 3; e++) {
						pattern[4] = e;

						State state = current_state;
						state_add_guess(word_guess, pattern, &state);
						
						uint64_t count = count_words(words, word_count, state, 0);
						if (count != 0) {
							double p = (double) count / (double) cword_count;
							if (debug) printf("p: %f = %llu / %llu, calcinfo: %f\n", p, count, cword_count, (p * log2(1 / p)));
							info += p * log2(1 / p);
						}
					}
				}
			}
		}
	}
	return info;
}

int main(int argc, const char *argv[]) {
	uint64_t *words = NULL;
	uint64_t word_count = 0;
	{
		FILE *w = fopen("words.txt", "r");
		fseek(w, 0, SEEK_END);
		uint32_t size = ftell(w);
		fseek(w, 0, SEEK_SET);
	
		word_count = size / 7;
		words = malloc(sizeof(uint64_t) * size);

		uint64_t i = 0;
		uint64_t buffer = 0;
		while (fgets((char *)&buffer, 8, w) != NULL) {
			words[i] = buffer & 0x000000FFFFFFFFFF;
			i++;
		}

		fclose(w);
	}
	printf("word_count=%llu\n", word_count);
	
	uint64_t cword_count = word_count;
	State state;
	state_init(&state);

	double *infos = (double *)malloc(sizeof(double) * word_count);
	FILE *file = fopen("infos.txt", "r");
	if (file != NULL) {
		for (uint64_t i = 0; i < word_count; i++) {
			fscanf(file, "%s %lf\n", (char *)(words + i), infos + i);
			//printf("\t%s %f\n", (char *)(words + i), infos[i]);
		}
		fclose(file);
	} else {
		printf("infos.txt not found.\nRecalculating...\n");
		FILE *fout = fopen("infos.txt", "w");
		for (uint64_t i = 0; i < word_count; i++) {
			uint64_t word = words[i];
			double info = expected_info(words, word_count, cword_count, word, state, 0);
			printf("\t%s: %f\n", (const char *)&word, info);
			infos[i] = info;
		}
		for (uint64_t y = 0; y < word_count - 1; y++) {
			for (uint64_t x = y + 1; x < word_count; x++) {
				if (infos[y] > infos[x]) {
					double tinfo = infos[y];
					infos[y] = infos[x];
					infos[x] = tinfo;
					uint64_t tword = words[y];
					words[y] = words[x];
					words[x] = tword;
				}
			}
		}
		for (uint64_t i = 0; i < word_count; i++) {
			fprintf(fout, "%s %f\n", (const char *)(words + i), infos[i]);
		}
		fclose(fout);
	}

	do {
		printf("recomended (%llu/%llu):\n", (cword_count > 10) ? 10 : cword_count, cword_count);
		uint64_t pi = 0;
		for (uint64_t i = 0; i < word_count && pi < 10; i++) {
			if (!WORD_DISABLED(words[word_count - i - 1])) {
				printf("\t%llu. %s %f\n", i, (char *)&words[word_count - i - 1], infos[word_count - i - 1]);
				pi++;
			}
		}
    	
		uint64_t input_ok;
    	
		uint64_t word;
		Pattern pattern;
		do {
			input_ok = 1;
    	
			uint64_t wpattern;
			printf("Enter the guess and result:\n");
			scanf("%s %s", (char *)&word, (char *)&wpattern);
			
			for (uint64_t i = 0; i < 5; i++) {
				char c = ((char *)&wpattern)[i];
				if (c == '!') {
					pattern[i] = PATTERN_TYPE_GRAY;
				} else if (c == '?') {
					pattern[i] = PATTERN_TYPE_YELLOW;
				} else if (c == '=') {
					pattern[i] = PATTERN_TYPE_GREEN;
				} else {
					printf("Wrong char '%c' in pattern.\n", c);
					input_ok = 0;
				}
			}
		} while (!input_ok);
    	
		state_add_guess(word, pattern, &state);
		for (uint64_t i = 0; i < word_count; i++) {
			if (!WORD_DISABLED(words[i])) {
				if (!word_fits_state(words[i], state, 0)) {
					WORD_DISABLE(words[i]);
					cword_count--;
				}
			}
		}
		for (uint64_t i = 0; i < word_count; i++) {
			if (!WORD_DISABLED(words[i])) {
				uint64_t word = words[i];
				double info = expected_info(words, word_count, cword_count, word, state, 0);
				//printf("\t%s: %f\n", (const char *)&word, info);
				infos[i] = info;
			}
		}
		for (uint64_t y = 0; y < word_count - 1; y++) {
			if (!WORD_DISABLED(words[y])) {
				for (uint64_t x = y + 1; x < word_count; x++) {
					if (!WORD_DISABLED(words[x])) {
						if (infos[y] > infos[x]) {
							double tinfo = infos[y];
							infos[y] = infos[x];
							infos[x] = tinfo;
							uint64_t tword = words[y];
							words[y] = words[x];
							words[x] = tword;
						}
					}
				}
			}
		}
	} while (cword_count > 1);

	if (cword_count == 0) {
		printf("No word found.");
	} else {
		for (uint64_t i = 0; i < word_count; i++) {
			if (!WORD_DISABLED(words[word_count - i - 1])) {
				printf("Word Found: %llu. %s %f\n", i, (char *)&words[word_count - i - 1], infos[word_count - i - 1]);
				break;
			}
		}
	}

	free(infos);
	free(words);
}


