pub const c = @cImport({
    @cInclude("helper.h");
});

pub fn add(a: u64, b: u64) u64 {
    return c.add(a, b);
}

pub const State = c.State;
pub const Word = c.Word;

pub fn state_init() State {
    var state: State = undefined;
    c.state_init(&state);
    return state;
}

pub fn calculate_recommended(
    state: *State,
    words: []Word,
    infos: []f64,
    word_count: u64,
    guess: []const u8,
    pattern: [5]u8,
) u64 {
    var wguess: c.Word = undefined;
    for (0..5) |i| {
        wguess[i] = guess[i];
    }
    for (5..8) |i| {
        wguess[i] = 0;
    }
    return c.calculate_recommended(
        state,
        words.ptr,
        infos.ptr,
        words.len,
        word_count,
        &wguess,
        &pattern,
    );
}

