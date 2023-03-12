# PDF Editor

## Useful Links

- [PDF Specification](https://www.adobe.com/content/dam/acom/en/devnet/pdf/pdf_reference_archive/pdf_reference_1-7.pdf)

## CLI

Available Commands:

- info
- delete-page
- images
- text
- embed
- extract

## Development

### Unix

- install gtk4 and gtkmm4
- run cmake
    - `mkdir build && cd build`
    - `cmake .. -G Ninja`
    - `cmake --build .`

### Windows

- install MSYS2
- install dependencies inside MSYS2 shell using pacman
    - `pacman -S mingw-w64-x86_64-toolchain`
    - `pacman -S mingw-w64-x86_64-gtkmm4`
    - `pacman -S mingw-w64-x86_64-clang`
- add `<msys2-install-dir>/mingw64/bin` to PATH
- run cmake
    - `mkdir build && cd build`
    - `cmake .. -G Ninja -D CMAKE_C_COMPILER=clang -D CMAKE_CXX_COMPILER=clang++`
    - `cmake --build .`

## Test Files

| PDF Standard | Hint        | Links                                                                                                                         |
|--------------|-------------|-------------------------------------------------------------------------------------------------------------------------------|
| PDF/VT       | Large files | https://www.pdfa.org/resource/cal-poly-pdfvt-test-suite/                                                                      |
| PDF/A-1      |             | https://www.pdfa.org/resource/isartor-test-suite/ https://www.pdfa.org/wp-content/uploads/2011/08/isartor-pdfa-2008-08-13.zip |
| PDF/UA       |             | https://www.pdfa.org/resource/pdfua-reference-suite/                                                                          |
| PDF/A        | Many files  | https://www.pdfa.org/resource/verapdf-test-suite/ https://github.com/veraPDF/veraPDF-corpus                                   |
|              |             | https://github.com/bfosupport/pdfa-testsuite                                                                                  |

## Fuzzing

Running existing fuzzing test cases: `cmake --build . --target test_fuzzer`
Running fuzzer: `cmake --build . --target fuzzer && ./src/test/fuzzer ../fuzzing-corpus`

## Ideas

- edit text (vertical or horizontal)
- (re)move text
- (re)move images
