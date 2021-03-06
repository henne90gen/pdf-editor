FROM henne90gen/gtk:4.4

RUN mkdir -p /app/build
COPY . /app
WORKDIR /app/build

RUN apt-get update && apt-get install -y poppler-utils

# Build
RUN cmake .. -G Ninja -D CMAKE_BUILD_TYPE=Release -D CMAKE_C_COMPILER=/usr/bin/clang-13 -D CMAKE_CXX_COMPILER=/usr/bin/clang++-13
RUN cmake --build .

# Test
RUN ctest --output-on-failure
RUN cmake --build . --target test_fuzzer
RUN cmake --build . --target test_suite_verapdf
RUN cmake --build . --target test_suite_isartor
RUN cmake --build . --target test_suite_bfosupport

# Package
RUN cpack -G ZIP
