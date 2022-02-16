import os
import sys
import subprocess
import multiprocessing

NUM_PROCESSES = 10
ACTIVATED_TESTS = {
    "VeraPDF": [
        "6.1 File structure",
        "6.2 Graphics",
        "6.3 Annotations",
        "6.6 Metadata",
        "6.6 Action",
        "6.7 Metadata",
        "6.9 Embedded files",
        "7.1 General",
        "7.2 Text",
        "7.11 Embedded files",
        "7.21 Fonts",
        "Undefined",
    ],
    "Isartor": [],
    "BFOSupport": ["BFOSupport"],
}
IGNORED_TESTS = [
    "veraPDF test suite 6-1-12-t02-pass-a.pdf",
    "veraPDF test suite 6-7-3-t01-pass-a.pdf",
    "7.1-t02-pass-b.pdf",
    "7.1-t09-pass-a.pdf",
    "7.1-t03-pass-b.pdf",
    "7.1-t06-pass-a.pdf",
    "7.1-t05-pass-a.pdf",
    "7.21.3.3-t02-pass-a.pdf",
    "7.21.4.2-t01-pass-a.pdf",
    "7.21.4.2-t02-pass-a.pdf",
    "7.21.3.1-t01-pass-d.pdf",
    "7.21.4.1-t01-pass-a.pdf",
    "7.21.7-t01-pass-a.pdf",
    "7.21.6-t02-pass-d.pdf",
    "7.21.7-t01-pass-b.pdf",
    "7.2-t29-pass-f.pdf",
    "7.2-t03-pass-a.pdf",
    "7.2-t03-pass-b.pdf",
    "7.2-t03-pass-c.pdf",
    "7.2-t29-pass-d.pdf",
    "7.2-t15-pass-a.pdf",
    "veraPDF test suite 6-6-3-t01-pass-d.pdf",
    "veraPDF test suite 6-6-3-t01-pass-c.pdf",
    "pdfa2-6-8-bfo-t01-pass.pdf",
    "pdfa2-6-8-bfo-t02-pass.pdf",
    "veraPDF test suite 6-3-1-t01-pass-b.pdf",
    "veraPDF test suite 6-3-1-t01-pass-d.pdf",
]
RUN_WITH_VALGRIND = False


def run_test_on_file(executable: str, file_path: str) -> bool:
    has_error = False
    cmd = []
    if RUN_WITH_VALGRIND:
        cmd.extend([
            "valgrind",
            "--tool=memcheck",
            "--gen-suppressions=all",
            "--leak-check=full",
            "--leak-resolution=med",
            "--track-origins=yes",
            "--error-exitcode=0",
        ])

    cmd.extend([executable, "info", file_path])

    try:
        process = subprocess.run(cmd, capture_output=True, timeout=2)
    except Exception as e:
        print(e)
        has_error = True
        print(" ".join(cmd))
        print()
        sys.stdout.flush()
        return has_error

    if not has_error:
        if "fail" in file_path:
            has_error = process.returncode == 0
        else:
            has_error = process.returncode != 0

    if has_error:
        print(file_path)
        output = process.stdout.decode("utf-8")
        parts = output.split("\n")
        for part in parts:
            print("   ", part)
        sys.stdout.flush()
    return has_error


def is_ignored(file_path: str):
    for ignored_test in IGNORED_TESTS:
        if ignored_test in file_path:
            return True

    return False


def integration_test(executable: str, test_suites_folder: str, test_suite_name: str):
    test_files = []
    num_possible_tests = 0
    num_explicitly_ignored_test = 0
    not_tested_files = []
    for directory, _, files in os.walk(test_suites_folder + "/" + test_suite_name):
        for f in files:
            if not f.endswith(".pdf"):
                continue

            file_path = directory + "/" + f

            if "fail" in file_path:
                continue

            num_possible_tests += 1

            if is_ignored(file_path):
                num_explicitly_ignored_test += 1
                continue

            should_add = False
            for activated_test in ACTIVATED_TESTS[test_suite_name]:
                if activated_test in file_path:
                    should_add = True
                    break

            if not should_add:
                not_tested_files.append(file_path)
            else:
                test_files.append(file_path)

    with multiprocessing.Pool(NUM_PROCESSES) as p:
        result = p.starmap(run_test_on_file, map(lambda f: (executable, f), test_files))

    total = 0
    successful = 0
    for err in result:
        if not err:
            successful += 1
        total += 1

    # print("Files not tested:")
    # for ignored_test_file in not_tested_files:
    #     print(ignored_test_file)

    progress = 100 * successful / num_possible_tests
    print(successful, "/", total,
          f" ({progress:.2f}% of {num_possible_tests} test files, {num_explicitly_ignored_test} ignored explicitly)")

    if successful != total:
        exit(1)


if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Wrong number of arguments")
        print(
            "Usage: python integration_test.py [path-to-executable] [path-to-test-suites-folder] [test-suite-name]")
        exit(1)

    exe = sys.argv[1]
    folder = sys.argv[2]
    suite = sys.argv[3]
    integration_test(exe, folder, suite)
