import os
import sys
import subprocess
import multiprocessing

NUM_PROCESSES = 10
ACTIVATED_TESTS = {"VeraPDF": ["6.1 File structure"]}


def test_file(executable: str, file_path: str) -> bool:
    has_error = False
    try:
        process = subprocess.run([executable, "info", file_path], capture_output=True, timeout=1)
    except:
        has_error = True

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
