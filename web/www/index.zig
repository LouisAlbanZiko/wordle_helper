const std = @import("std");
const server = @import("server");
const http = server.http;

const helper = @import("helper");

const WORDS = @import("/words.zig");

pub fn http_GET(ctx: http.Context, req: *const http.Request) std.mem.Allocator.Error!http.Response {
    if (req.cookies.get("session_id")) |session_id| {
        WORDS.remove_session(session_id);
    }

    var headers = std.ArrayList(http.Header).init(ctx.arena);
    
    const session_id = try WORDS.new_session();
    const session_id_cookie = try std.fmt.allocPrint(ctx.arena, "session_id={s};", .{session_id});
    try headers.append(http.Header{ "Set-Cookie", session_id_cookie });
    try headers.append(http.Header{ "Content-Type", @tagName(.@"text/html")});

    var help_panel = std.ArrayList(u8).init(ctx.arena);
    try std.fmt.format(help_panel.writer(), 
        \\Recommended:
        \\<ol>
        , .{});
    for (0..10) |i| {
        const w = WORDS.WORDS[WORDS.WORDS.len - 1 - i];
        try std.fmt.format(help_panel.writer(), "<li>{s}: {d}</li>", .{w.word, w.info});
    }
    try help_panel.appendSlice("</ol>");

    var body = std.ArrayList(u8).init(ctx.arena);
    try server.util.template(body.writer(), @embedFile("index.html.ignore"), .{
        .help_panel = try help_panel.toOwnedSlice(),
    });

    return http.Response{
        .code = .@"200 Ok",
        .headers = headers.items,
        .body = try body.toOwnedSlice(),
    };
}

pub fn http_POST(ctx: http.Context, req: *const http.Request) std.mem.Allocator.Error!http.Response {
    if (req.cookies.get("session_id")) |session_id| {
        if (WORDS.get_session(session_id)) |session| {
            const Guess = struct {
                word: []const u8,
                pattern: []const u8,
            };
            const json_guess = std.json.parseFromSlice(Guess, ctx.arena, req.body, .{}) catch {
                const message = "Failed to parse json request.";
                return http.Response{
                    .code = .@"400 Bad Request",
                    .headers = &[_]http.Header{
                        .{ "Content-Type", @tagName(http.ContentType.@"text/plain")},
                    },
                    .body = message,
                };
            };
            const guess = json_guess.value;
            var word: [8]u8 = undefined;
            for (0..5) |i| {
                word[i] = guess.word[i];
            }
            word[5] = 0;
            word[6] = 0;
            word[7] = 0;

            var pattern: [5]u8 = undefined;
            for (0..5) |i| {
                switch (guess.pattern[i]) {
                    '!' => {
                        pattern[i] = 0;
                    },
                    '?' => {
                        pattern[i] = 1;
                    },
                    '=' => {
                        pattern[i] = 2;
                    },
                    else => {
                        const message = "Invalid character in pattern";
                        return http.Response{
                            .code = .@"400 Bad Request",
                            .headers = &[_]http.Header{
                                .{ "Content-Type", @tagName(http.ContentType.@"text/plain")},
                            },
                            .body = message,
                        };
                    }
                }
            }
            WORDS.step(
                session,
                guess.word,
                pattern,
            );

            var res_code: http.Code = .@"200 Ok";
            var help_panel = std.ArrayList(u8).init(ctx.arena);
            if (session.cword_count == 0) {
                try std.fmt.format(help_panel.writer(), "No word found!", .{});
            } else if (session.cword_count == 1) {
                var has_word: ?[]const u8 = null;
                for (0..session.words.len) |i| {
                    const w = session.words[session.words.len - 1 - i];
                    if (helper.c.word_disabled(&w) == 0) {
                        has_word = &w;
                        break;
                    }
                }
                if (has_word) |w| {
                    try std.fmt.format(help_panel.writer(), "Word found: {s}", .{w});
                } else {
                    res_code = .@"500 Internal Server Error";
                    try std.fmt.format(help_panel.writer(), "Server Error. No word found!", .{});
                }
            } else {
                try std.fmt.format(help_panel.writer(), 
                    \\Recommended:
                    \\<ol>
                    , .{});
                
                var max_count: u64 = session.cword_count;
                if (session.cword_count > 10) {
                    max_count = session.cword_count;
                }
                var wcount: u64 = 0;
                for (0..session.words.len) |i| {
                    const w = session.words[session.words.len - 1 - i];
                    const info = session.infos[session.infos.len - 1 - i];
                    if (helper.c.word_disabled(&w) == 0) {
                        const w1: [8]u8 = @bitCast(w);
                        try std.fmt.format(help_panel.writer(), "<li>{s}: {d}</li>", .{&w1, info});
                        wcount += 1;
                        if (wcount >= max_count) {
                            break;
                        }
                    }
                }
                try help_panel.appendSlice("</ol>");
            }

            return http.Response{
                .code = res_code,
                .headers = &[_]http.Header{
                    http.Header{ "Content-Type", @tagName(http.ContentType.@"text/html") },
                },
                .body = try help_panel.toOwnedSlice(),
            };
        } else {
            const message = "Invalid session_id. Please reload the page.";
            return http.Response{
                .code = .@"400 Bad Request",
                .headers = &[_]http.Header{
                    http.Header{ "Content-Type", @tagName(http.ContentType.@"text/plain") },
                },
                .body = message,
            };

        }
    } else {
        const message = "No session_id found. Please reload the page.";
        return http.Response{
            .code = .@"400 Bad Request",
            .headers = &[_]http.Header{
                http.Header{ "Content-Type", @tagName(http.ContentType.@"text/plain") },
            },
            .body = message,
        };
    }
}

