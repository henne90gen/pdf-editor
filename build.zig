const std = @import("std");

fn findSources(b: *std.Build, directory: []const u8) ![][]const u8 {
    var sources = std.ArrayList([]const u8).init(b.allocator);

    var dir = try std.fs.cwd().openDir(directory, .{ .iterate = true });

    var walker = try dir.walk(b.allocator);
    defer walker.deinit();

    const allowed_exts = [_][]const u8{ ".c", ".cpp" };

    while (try walker.next()) |entry| {
        if (entry.kind != std.fs.File.Kind.file) {
            continue;
        }

        const ext = std.fs.path.extension(entry.basename);
        const include_file = for (allowed_exts) |e| {
            if (std.mem.eql(u8, ext, e))
                break true;
        } else false;

        if (!include_file) {
            continue;
        }

        try sources.append(b.pathJoin(&.{ directory, entry.path }));
    }

    return sources.items;
}

fn createSpdlog(b: *std.Build, optimize: std.builtin.OptimizeMode) !*std.Build.Step.Compile {
    const spdlog = b.addSharedLibrary(.{
        .name = "spdlog",
        .target = b.host,
        .optimize = optimize,
    });
    spdlog.addCSourceFiles(.{
        .files = &[_][]const u8{
            "submodules/spdlog/src/spdlog.cpp",
            "submodules/spdlog/src/stdout_sinks.cpp",
            "submodules/spdlog/src/color_sinks.cpp",
            "submodules/spdlog/src/file_sinks.cpp",
            "submodules/spdlog/src/async.cpp",
            "submodules/spdlog/src/cfg.cpp",
            "submodules/spdlog/src/bundled_fmtlib_format.cpp",
        },
        .flags = &[_][]const u8{ "-std=c++23", "-DSPDLOG_COMPILED_LIB" },
    });
    const includePath = b.path("submodules/spdlog/include");
    spdlog.addIncludePath(includePath);
    spdlog.installHeadersDirectory(includePath, "", .{});
    spdlog.linkLibCpp();
    return spdlog;
}

fn createPdf(b: *std.Build, optimize: std.builtin.OptimizeMode, spdlog: *std.Build.Step.Compile) !*std.Build.Step.Compile {
    const pdfLib = b.addSharedLibrary(.{
        .name = "pdf",
        .target = b.host,
        .optimize = optimize,
    });
    pdfLib.linker_allow_shlib_undefined = true;
    const sources = try findSources(b, "src/lib/pdf");
    pdfLib.addCSourceFiles(.{
        .files = sources,
        .flags = &[_][]const u8{"-std=c++23"},
    });
    pdfLib.addIncludePath(b.path("src/lib"));
    pdfLib.linkLibC();
    pdfLib.linkLibCpp();
    pdfLib.linkSystemLibrary("cairo");
    pdfLib.linkSystemLibrary("zlib");
    pdfLib.linkSystemLibrary("freetype");
    pdfLib.linkLibrary(spdlog);
    pdfLib.installHeadersDirectory(b.path("src/lib/pdf"), "pdf", .{});
    return pdfLib;
}

fn createCli(b: *std.Build, optimize: std.builtin.OptimizeMode, pdfLib: *std.Build.Step.Compile, spdlog: *std.Build.Step.Compile) !*std.Build.Step.Compile {
    const cliExe = b.addExecutable(.{
        .name = "pdf-cli",
        .target = b.host,
        .optimize = optimize,
    });
    cliExe.linker_allow_shlib_undefined = true;
    cliExe.addCSourceFiles(.{
        .files = &.{"src/app/cli/main.cpp"},
        .flags = &[_][]const u8{"-std=c++20"},
    });
    cliExe.linkLibC();
    cliExe.linkLibCpp();
    cliExe.linkLibrary(pdfLib);
    cliExe.linkLibrary(spdlog);
    cliExe.linkSystemLibrary("cairo");
    cliExe.linkSystemLibrary("zlib");
    cliExe.linkSystemLibrary("freetype");
    return cliExe;
}

pub fn build(b: *std.Build) !void {
    const optimize = b.standardOptimizeOption(.{});
    const spdlog = try createSpdlog(b, optimize);
    b.installArtifact(spdlog);

    const pdf = try createPdf(b, optimize, spdlog);
    b.installArtifact(pdf);

    const cli = try createCli(b, optimize, pdf, spdlog);
    b.installArtifact(cli);
}
