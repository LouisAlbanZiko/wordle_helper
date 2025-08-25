pub const c = @cImport({
    @cInclude("wh.h");
});

pub const Word = c.WH_Word;
pub const Guess = c.WH_Guess;

pub fn filter(words: []Word, guess: Guess) []Word {
    const new_len = c.wh_filter(words.len, words.ptr, guess);
    var new_words: []Word = undefined;
    new_words.ptr = words.ptr;
    new_words.len = new_len;
    return new_words;
}

pub fn calculate(words: []Word) void {
    c.wh_calculate(words.len, words.ptr);
}

pub fn sort(words: []Word) void {
    c.wh_sort(words.len, words.ptr);
}

