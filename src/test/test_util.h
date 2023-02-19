#pragma once

#define ASSERT_BUFFER_CONTAINS_AT(buffer, offset, str)                                                                 \
    {                                                                                                                  \
        auto s = std::string_view(str);                                                                                \
        ASSERT_EQ(s, std::string_view((char *)(buffer) + (offset), s.size()));                                         \
    }
