import os
import sys
import subprocess
import multiprocessing

NUM_PROCESSES = 10
ACTIVATED_TESTS = {"VeraPDF": ["6.1 File structure"]}
IGNORED_TESTS = ["veraPDF test suite 6-1-12-t02-pass-a.pdf"]
RUN_WITH_VALGRIND = False


def test_file(executable: str, file_path: str) -> bool:
    for ignored in IGNORED_TESTS:
        if ignored in file_path:
            return False

    has_error = False
    try:
        cmd = []
        if RUN_WITH_VALGRIND:
            cmd.extend( [
                "valgrind",
                "--tool=memcheck",
                "--gen-suppressions=all",
                "--leak-check=full",
                "--leak-resolution=med",
                "--track-origins=yes",
                "--error-exitcode=0",
            ])

        cmd.extend([executable, "info", file_path])
        process = subprocess.run(cmd, capture_output=True, timeout=10)
    except Exception as e:
        print(e)
        has_error = True
        print(" ".join(cmd))
        print(file_path)
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


def integration_test(executable: str, test_suites_folder: str, test_suite_name: str):
    test_files = []
    for directory, _, files in os.walk(test_suites_folder + "/" + test_suite_name):
        for f in files:
            if not f.endswith(".pdf"):
                continue

            file_path = directory + "/" + f

            if "fail" in file_path:
                continue

            for activated_test in ACTIVATED_TESTS[test_suite_name]:
                if activated_test in file_path:
                    test_files.append(file_path)
                    break

    with multiprocessing.Pool(NUM_PROCESSES) as p:
        result = p.starmap(test_file, map(lambda f: (executable, f), test_files))

    total = 0
    successful = 0
    for err in result:
        if not err:
            successful += 1
        total += 1

    print(successful, "/", total)

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
