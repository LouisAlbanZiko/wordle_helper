const std = @import("std");
const wh = @import("wh");

pub const WORDS: []const wh.Word = @alignCast(std.mem.bytesAsSlice(wh.Word, @embedFile("infos.bin")));

pub const SessionData = struct {
    words: []wh.Word,
    pub fn init() std.mem.Allocator.Error!SessionData {
        var session_data: SessionData = undefined;
        session_data.words = try g_alloc.dupe(wh.Word, WORDS);
        return session_data;
    }
};
pub var sessions_data: std.StringHashMap(SessionData) = undefined;
pub var g_alloc: std.mem.Allocator = undefined;

pub fn new_session() ![]const u8 {
    var buffer: [32]u8 = undefined;
    std.crypto.random.bytes(&buffer);
    const hexbuffer = std.fmt.bytesToHex(&buffer, .upper);
    const session_id = try g_alloc.dupe(u8, &hexbuffer);

    const session_data = try SessionData.init();
    try sessions_data.put(session_id, session_data);

    return session_id;
}

pub fn remove_session(session_id: []const u8) void {
    if (sessions_data.fetchRemove(session_id)) |kv| {
        g_alloc.free(kv.key);
        g_alloc.free(kv.value.words);
    }
}

pub fn get_session(session_id: []const u8) ?*SessionData {
    return sessions_data.getPtr(session_id);
}

pub fn init(alloc: std.mem.Allocator) !void {
    g_alloc = alloc;
    sessions_data = std.StringHashMap(SessionData).init(g_alloc);
}

pub fn step(session: *SessionData, guess: []const u8, pattern: []const u8) void {
    const g: wh.Guess = .{
        .word = guess.ptr,
        .pattern = pattern.ptr,
    };
    session.words = wh.filter(session.words, g);
    wh.calculate(session.words);
    wh.sort(session.words);
}

