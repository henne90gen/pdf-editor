import sys
import os
import subprocess


def test_file(executable: str, file_path: str) -> bool:
    """Returns true on failure"""
    if not file_path.endswith(".pdf"):
        return False

    print("->", file_path)

    try:
        process = subprocess.run(
            [executable, "info", file_path], capture_output=True, timeout=1)
    except:
        return True

    sys.stdout.write("\033[F")  # Cursor up one line
    sys.stdout.write("\033[K")  # Clear to the end of line
    if process.returncode == 0:
        print("✓", file_path)
        return False

    print("☓", file_path)
    print()
    output = process.stdout.decode("utf-8")
    parts = output.split("\n")
    for part in parts:
        print("   ", part)

    return True


def test_directory(executable: str, test_folder: str):
    has_failure = False
    for directory, _, files in os.walk(test_folder):
        for file in files:
            file_path = os.path.join(directory, file)
            test_file(executable, file_path)

    if has_failure:
        exit(1)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Wrong number of arguments")
        print(
            "Usage: python test_directory.py [path-to-executable] [path-to-test-folder]")
        exit(1)

    exe = sys.argv[1]
    folder = sys.argv[2]
    test_directory(exe, folder)
