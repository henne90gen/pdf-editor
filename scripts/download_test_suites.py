import sys
import os
import requests
from dataclasses import dataclass
from pathlib import Path
import subprocess
import shutil


@dataclass
class TestSuite:
    name: str
    url: str


suites = [
    TestSuite("BFOSupport", "https://github.com/bfosupport/pdfa-testsuite"),
    TestSuite("VeraPDF", "https://github.com/veraPDF/veraPDF-corpus"),
    TestSuite("Isartor", "https://www.pdfa.org/wp-content/uploads/2011/08/isartor-pdfa-2008-08-13.zip"),
]


def download(suite: TestSuite):
    print(f"Downloading {suite.name}")
    if os.path.exists(suite.name):
        return

    os.mkdir(suite.name)

    if "github.com" in suite.url:
        subprocess.run(["git", "clone", suite.url, suite.name],
                       capture_output=True)
        return

    r = requests.get(suite.url)
    p = Path(suite.url)
    file_name = p.name
    file_path = suite.name + "/" + file_name
    with open(file_path, 'wb') as f:
        f.write(r.content)

    if file_name.endswith(".zip"):
        shutil.unpack_archive(file_path, suite.name)


def download_test_suites(target_folder: str):
    if not os.path.exists(target_folder):
        # TODO make sure that parent directories are created first
        os.mkdir(target_folder)

    if not os.path.isdir(target_folder):
        print("Failed to create target folder")
        exit(1)

    os.chdir(target_folder)

    print(f"Downloading into {target_folder}")
    for suite in suites:
        download(suite)


if __name__ == "__main__":
    target_folder = sys.argv[1]
    download_test_suites(target_folder)
