const std = @import("std");
const server = @import("server");
const http = server.http;

const words = @import("/words.zig");

pub fn http_GET(ctx: http.Context, req: *const http.Request) std.mem.Allocator.Error!http.Response {
    if (req.cookies.get("session_id")) |session_id| {
        words.remove_session(session_id);
    }

    var headers = std.ArrayList(http.Header).init(ctx.arena);
    
    const session_id = try words.new_session();
    const session_id_cookie = try std.fmt.allocPrint(ctx.arena, "session_id={s};", .{session_id});
    try headers.append(http.Header{ "Set-Cookie", session_id_cookie });
    try headers.append(http.Header{ "Content-Type", @tagName(.@"text/html")});

    var help_panel = std.ArrayList(u8).init(ctx.arena);
    try std.fmt.format(help_panel.writer(), 
        \\<h2>Recommended (10 / {d}):</h2>
        \\<ol>
        , .{words.WORDS.len});
    for (0..10) |i| {
        const w = words.WORDS[i];
        try std.fmt.format(help_panel.writer(), "<li><span class=\"word\">{s}</span><span class=\"info\">({d:.2})</span></li>", .{w.str, w.info});
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
        if (words.get_session(session_id)) |session| {
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

            words.step(
                session,
                guess.word,
                guess.pattern,
            );

            var help_panel = std.ArrayList(u8).init(ctx.arena);
            if (session.words.len == 0) {
                try std.fmt.format(help_panel.writer(), "<h2>No word found!</h2>", .{});
            } else if (session.words.len == 1) {
                try std.fmt.format(help_panel.writer(), "<h2>Word found: <span class=\"word\">{s}</span></h2>", .{session.words[0].str});
            } else {
                var max_count: u64 = 10;
                if (session.words.len < 10) {
                    max_count = session.words.len;
                }

                try std.fmt.format(help_panel.writer(), 
                    \\<h2>Recommended ({d} / {d}):</h2>
                    \\<ol>
                    , .{max_count, session.words.len});
                
                var wcount: u64 = 0;
                for (0..session.words.len) |i| {
                    const w = session.words[i];
                    try std.fmt.format(help_panel.writer(), "<li><span class=\"word\">{s}</span><span class=\"info\">({d:.2})</span></li>", .{w.str, w.info});
                    wcount += 1;
                    if (wcount >= max_count) {
                        break;
                    }
                }
                try help_panel.appendSlice("</ol>");
            }

            return http.Response{
                .code = .@"200 Ok",
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

