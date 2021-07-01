# PDF Editor

## Useful Links

- [PDF Specification](https://www.adobe.com/content/dam/acom/en/devnet/pdf/pdf_reference_archive/pdf_reference_1-7.pdf)

## Development

### Unix

- install gtkmm3
- run cmake
    - `mkdir build && cd build`
    - `cmake .. -G Ninja`
    - `cmake --build .`

### Windows

- install MSYS2
- install dependencies inside MSYS2 shell
    - mingw-w64-x86_64-toolchain
    - mingw-w64-x86_64-gtkmm3
    - mingw-w64-x86_64-clang
- add `<msys2-install-dir>/mingw64/bin` to PATH
- run cmake
    - `mkdir build && cd build`
    - `cmake .. -G Ninja -D CMAKE_C_COMPILER=clang -D CMAKE_CXX_COMPILER=clang++`
    - `cmake --build .`
