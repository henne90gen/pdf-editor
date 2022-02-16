FROM henne90gen/gtk:4.4

RUN mkdir -p /app/build
COPY . /app
WORKDIR /app/build
RUN rm * -rf

RUN apt-get update && apt-get install -y poppler-utils
RUN cmake .. -G Ninja -D CMAKE_BUILD_TYPE=Release -D CMAKE_C_COMPILER=/usr/bin/clang-13 -D CMAKE_CXX_COMPILER=/usr/bin/clang++-13
RUN cmake --build .
RUN ctest --output-on-failure
RUN cmake --build . --target test_files
RUN cmake --build . --target test_suite_verapdf
RUN cmake --build . --target test_suite_isartor
RUN cmake --build . --target test_suite_bfosupport
