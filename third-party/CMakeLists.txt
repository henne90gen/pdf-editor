add_library(md5_reference hash/md5_reference.c)

add_executable(mddriver hash/mddriver.c)
target_link_libraries(mddriver md5_reference)

add_library(sha1_reference hash/sha1_reference.c)

add_executable(sha1test hash/sha1test.c)
target_link_libraries(sha1test sha1_reference)
