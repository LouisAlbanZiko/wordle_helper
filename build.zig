const std = @import("std");

pub fn build(b: *std.Build) void {
    const Project = enum { web, cmd };
    const project = b.option(Project, "project", "Which project to build") orelse .cmd;

    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const wh_mod = b.addModule("wh", .{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
        .root_source_file = b.path("wh/root.zig"),
    });
    wh_mod.addCSourceFile(.{.file = b.path("wh/wh.c"), .language = .c});
    wh_mod.addIncludePath(b.path("wh"));

    switch (project) {
        .web => {
            const exe_name: []const u8 = "wordle_helper";

            const hermes = b.dependency("hermes", .{
                .target = target,
                .optimize = optimize,
                .web_dir = b.path("www"),
                .mod_dir = b.path("mods"),
                .exe_name = exe_name,
            });

            const index_mod = hermes.module("/index.zig");
            index_mod.addImport("wh", wh_mod);

            const words_mod = hermes.module("/words.zig");
            words_mod.addImport("wh", wh_mod);

            b.getInstallStep().dependOn(hermes.builder.getInstallStep());

            b.installFile("config.zon", "config.zon");

            const exe = hermes.artifact(exe_name);
            b.installArtifact(exe);

            const run_exe = b.addRunArtifact(exe);

            const run_step = b.step("run", "Run the application");
            run_step.dependOn(&run_exe.step);
        },
        .cmd => {
            const exe_mod = b.addModule("wh_cmd", .{
                .target = target,
                .optimize = optimize,
                .link_libc = true,
            });
            exe_mod.addCSourceFile(.{ .file = b.path("wh/wh.c"), .language = .c });
            exe_mod.addCSourceFile(.{ .file = b.path("wh/main.c"), .language = .c });
            exe_mod.addIncludePath(b.path("wh"));

            const exe = b.addExecutable(.{
                .name = "wh_cmd",
                .root_module = exe_mod,
                .link_libc = true,
            });
            const run_exe = b.addRunArtifact(exe);

            const run_step = b.step("run", "Run the application");
            run_step.dependOn(&run_exe.step);
        },
    }


}
