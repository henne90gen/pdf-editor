import sys
import os
from dataclasses import dataclass
from pathlib import Path
import subprocess
import shutil
import urllib.request


@dataclass
class TestSuite:
    name: str
    url: str


suites = [
    TestSuite("BFOSupport", "https://github.com/bfosupport/pdfa-testsuite"),
    TestSuite("VeraPDF", "https://github.com/veraPDF/veraPDF-corpus"),
    TestSuite(
        "Isartor", "https://www.pdfa.org/wp-content/uploads/2011/08/isartor-pdfa-2008-08-13.zip"),
]


def download(suite: TestSuite):
    if os.path.exists(suite.name):
        return

    os.mkdir(suite.name)

    try:
        if "github.com" in suite.url:
            subprocess.run(["git", "clone", suite.url,
                            suite.name], capture_output=True)
            return

        p = Path(suite.url)
        file_name = p.name
        file_path = suite.name + "/" + file_name
        request = urllib.request.Request(url=suite.url, data=None, headers={
            "User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:97.0) Gecko/20100101 Firefox/97.0"
        })
        response = urllib.request.urlopen(request)
        content = response.read()
        with open(file_path, "wb") as f:
            f.write(content)

        if file_name.endswith(".zip"):
            shutil.unpack_archive(file_path, suite.name)
    except Exception as e:
        print(e)
        print("Failed to download", suite.name)
        shutil.rmtree(suite.name)


def download_test_suites(target_folder: str):
    if not os.path.exists(target_folder):
        os.mkdir(target_folder)

    if not os.path.isdir(target_folder):
        print("Failed to create target folder")
        exit(1)

    os.chdir(target_folder)

    for suite in suites:
        download(suite)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(
            "Usage: python download_test_suites.py [path-to-test-suites-target-folder]")
        exit(1)

    folder = sys.argv[1]
    download_test_suites(folder)
