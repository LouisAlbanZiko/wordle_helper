const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const helper_mod = b.addModule("helper", .{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
        .root_source_file = b.path("helper/root.zig"),
    });
    helper_mod.addCSourceFile(.{.file = b.path("helper/helper.c"), .language = .c});
    helper_mod.addIncludePath(b.path("helper"));

    const exe_name: []const u8 = "wordle_helper";

    const hermes = b.dependency("hermes", .{
        .target = target,
        .optimize = optimize,
        .web_dir = b.path("www"),
        .mod_dir = b.path("mods"),
        .exe_name = exe_name,
    });

    const index_mod = hermes.module("/index.zig");
    index_mod.addImport("helper", helper_mod);

    const words_mod = hermes.module("/words.zig");
    words_mod.addImport("helper", helper_mod);

    //_ = hermes.module("/helper.zig");
    //const mod_helper = hermes.module("/helper.zig");
    //mod_helper.addImport("helper_c", helper_c_mod);

    b.getInstallStep().dependOn(hermes.builder.getInstallStep());

    b.installFile("config.zon", "config.zon");

    const exe = hermes.artifact(exe_name);
    b.installArtifact(exe);

    const run_exe = b.addRunArtifact(exe);

    const run_step = b.step("run", "Run the application");
    run_step.dependOn(&run_exe.step);
}
