import sys
import os
import subprocess


def main():
    if len(sys.argv) != 3:
        print("Wrong number of arguments")
        print("Usage: python integration_test.py [path-to-executable] [path-to-test-folder]")
        exit(1)

    executable = sys.argv[1]
    test_folder = sys.argv[2]
    for directory, b, files in os.walk(test_folder):
        for file in files:
            file_path = os.path.join(directory, file)
            if not os.path.exists(file_path):
                print("☓", file_path, "does not exist")

            process = subprocess.run([executable, "info", file_path], capture_output=True)
            if process.returncode == 0:
                print("✓", file_path)
            else:
                print("☓", file_path)
                print(process.stdout.decode("utf-8"))


if __name__ == "__main__":
    main()
