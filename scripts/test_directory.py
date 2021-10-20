import sys
import os
import subprocess


def test_directory(executable: str, test_folder: str):
    has_failure = False
    for directory, _, files in os.walk(test_folder):
        for file in files:
            if not file.endswith(".pdf"):
                continue

            file_path = os.path.join(directory, file)
            print("->", file_path)

            process = subprocess.run(
                [executable, "info", file_path], capture_output=True, timeout=1)

            sys.stdout.write("\033[F")  # Cursor up one line
            sys.stdout.write("\033[K")  # Clear to the end of line
            if process.returncode == 0:
                print("✓", file_path)
            else:
                has_failure = True
                print("☓", file_path)
                print()
                output = process.stdout.decode("utf-8")
                parts = output.split("\n")
                for part in parts:
                    print("   ", part)

    if has_failure:
        exit(1)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Wrong number of arguments")
        print(
            "Usage: python test_directory.py [path-to-executable] [path-to-test-folder]")
        exit(1)

    executable = sys.argv[1]
    test_folder = sys.argv[2]
    test_directory(executable, test_folder)
