FROM henne90gen/gtk:4.8

RUN apt-get update && apt-get dist-upgrade -y && apt-get install -y poppler-utils

RUN mkdir -p /app/build
COPY . /app
WORKDIR /app/build

# Build
RUN cmake .. -G Ninja -D CMAKE_BUILD_TYPE=Release -D CMAKE_C_COMPILER=/usr/bin/clang -D CMAKE_CXX_COMPILER=/usr/bin/clang++
RUN cmake --build .

# Test
RUN ctest --output-on-failure
RUN cmake --build . --target test_fuzzer
RUN cmake --build . --target test_suite_verapdf
RUN cmake --build . --target test_suite_isartor
RUN cmake --build . --target test_suite_bfosupport

# Package
RUN cpack -G ZIP
