#include "pdf_reader.h"

int main() {
    pdf::File file;
    pdf::load_from_file("../../test-files/blank.pdf", file);
}
