#include <gtest/gtest.h>

#include <pdf_reader.h>

TEST(Reader, Blank) {
    pdf::File file;
    pdf::load_from_file("../../test-files/blank.pdf", file);
    std::vector<Object *> objects = file.getAllObjects();
    ASSERT_EQ(objects.size(), 8);
}
