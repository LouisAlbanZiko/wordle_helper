#include <stdint.h>
#include <math.h>
#include <string.h>

#include "helper.h"

uint64_t word_disabled(const Word word) {
	return WORD_DISABLED(word);
}

void word_disable(Word word) {
	WORD_DISABLE(word);
}

uint64_t add(uint64_t a, uint64_t b) {
	return a + b;
}

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

void state_add_guess(State *state, const char *guess, const Pattern pattern) {
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

uint64_t word_fits_state(const State *state, const Word word) {
	uint64_t found_required = 0;
	for (uint64_t i = 0; i < 5; i++) {
		char c = word[i];
		if (state->required & (0x1 << (c - 'a'))) {
			found_required |= 0x1 << (c - 'a');
		}
		if ((state->allowed & (0x1 << (c - 'a'))) == 0) {
			return 0;
		}
		if (state->letters[i].found != 0 && state->letters[i].found != c) {
			return 0;
		}
		if ((state->letters[i].allowed & (0x1 << (c - 'a'))) == 0) {
			return 0;
		}
	}
	return state->required == found_required;
}


uint64_t count_words(const State *state, const Word *words, const uint64_t word_count) {
	uint64_t count = 0;
	for (uint64_t i = 0; i < word_count; i++) {
		if (!WORD_DISABLED(words[i])) {
			count += word_fits_state(state, (const char *)&words[i]);
		}
	}
	return count;
}

double expected_info(const State *current_state, const Word *words, const uint64_t word_count, const uint64_t cword_count, const Word word_guess) {
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

						State state = *current_state;
						state_add_guess(&state, word_guess, pattern);
						
						uint64_t count = count_words(&state, words, word_count);
						if (count != 0) {
							double p = (double) count / (double) cword_count;
							info += p * log2(1 / p);
						}
					}
				}
			}
		}
	}
	return info;
}

uint64_t calculate_recommended(
	State *state,
	Word* words,
	double *infos,
	uint64_t word_count,
	uint64_t cword_count,
	const Word guess,
	const Pattern pattern
) {
	state_add_guess(state, guess, pattern);

	for (uint64_t i = 0; i < word_count; i++) {
		if (!WORD_DISABLED(words[i])) {
			if (!word_fits_state(state, words[i])) {
				WORD_DISABLE(words[i]);
				cword_count--;
			}
		}
	}
    for (uint64_t i = 0; i < word_count; i++) {
        if (!WORD_DISABLED(words[i])) {
            infos[i] = expected_info(state, words, word_count, cword_count, words[i]);
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

						Word tword;
						memcpy(&tword, &words[y], 8);
						memcpy(&words[y], &words[x], 8);
						memcpy(&words[x], &tword, 8);

					}
				}
			}
		}
	}
	return cword_count;
}

