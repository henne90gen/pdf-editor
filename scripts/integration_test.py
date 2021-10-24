import os
import sys
from typing import Optional
import multiprocessing
import queue
import test_directory

NUM_PROCESSES = 10


def integration_test(executable: str, test_suites_folder: str, test_suite_name: Optional[str]):
    test_files = []
    for directory, _, files in os.walk(test_suites_folder + "/" + test_suite_name):
        for f in files:
            if not f.endswith(".pdf"):
                continue
            test_files.append(directory + "/" + f)

    with multiprocessing.Pool(NUM_PROCESSES) as p:
        result = p.starmap(test_directory.test_file, map(lambda f: (executable, f), test_files))

    total = 0
    successful = 0
    for res in result:
        if not res:
            successful += 1
        total += 1

    print(successful, "/", total)


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
