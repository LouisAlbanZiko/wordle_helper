#include <stdint.h>

typedef char Word[8];

#define WORD_DISABLED(word) (word[7])
#define WORD_DISABLE(word) word[7] = 1

uint64_t word_disabled(const Word word);
void word_disable(Word word);

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

void state_init(State *state);
void state_disallow_char(State *state, char c);
void state_require_char(State *state, char c);

void letter_disallow_char(Letter *letter, char c);
void letter_found(Letter *letter, char c);

#define PATTERN_TYPE_GRAY 0
#define PATTERN_TYPE_YELLOW 1
#define PATTERN_TYPE_GREEN 2

typedef uint8_t Pattern[5];

void state_add_guess(State *state, const char *guess, const Pattern pattern);

uint64_t word_fits_state(const State *state, const Word word_u);

uint64_t count_words(const State *state, const Word *words, const uint64_t word_count);

double expected_info(const State *current_state, const Word *words, const uint64_t word_count, const uint64_t cword_count, const Word word_guess);

uint64_t calculate_recommended(
	State *state,
	Word* words,
	double *infos,
	uint64_t word_count,
	uint64_t cword_count,
	const Word guess,
	const Pattern pattern
);

