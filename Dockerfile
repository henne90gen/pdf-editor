FROM henne90gen/gtk:gtk-4.4

RUN mkdir -p /app/build
COPY . /app
WORKDIR /app/build
RUN rm * -rf

RUN cmake .. -G Ninja -D CMAKE_BUILD_TYPE=Release -D CMAKE_C_COMPILER=/usr/bin/clang-13 -D CMAKE_CXX_COMPILER=/usr/bin/clang++-13
RUN cmake --build .
RUN ctest --output-on-failure
RUN cmake --build . --target test_files
RUN cmake --build . --target test_suite_verapdf
