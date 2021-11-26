#pragma once

#define ASSERT_BUFFER_CONTAINS_AT(buffer, str, offset)                                                                 \
    {                                                                                                                  \
        auto s = std::string_view(str);                                                                                \
        ASSERT_EQ(s, std::string_view((buffer) + (offset), s.size()));                                                 \
    }
