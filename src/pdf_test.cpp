#include "pdf_reader.h"

int main() {
  auto reader = pdf_reader();
  reader.read("../../test-files/blank.pdf");
}
