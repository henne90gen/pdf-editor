import sys
import re
import os


def clean(s):
    # Remove invalid characters
    s = re.sub('[^0-9a-zA-Z_]', '_', s)
    # Remove leading characters until we find a letter or underscore
    s = re.sub('^[^a-zA-Z_]+', '', s)
    return s


def embed(file: str, destination_name: str):
    _, file_name = os.path.split(destination_name)
    c_identifier = clean(file_name)

    with open(file) as f:
        content = f.read()

    encodedContent = content.encode('utf-8')
    hexContent = encodedContent.hex()

    result = ""
    result += f"#include \"{file_name}.h\"\n\n"
    result += f"uint8_t {c_identifier}_data[] = {{"

    for c in range(0, len(hexContent), 2):
        if c % 16 == 0:
            result += "\n    "
        result += "0x"
        result += hexContent[c]
        result += hexContent[c + 1]
        result += ", "

    result += "\n"
    result += "};\n"
    result += f"uint32_t {c_identifier}_size = sizeof({c_identifier}_data);\n"

    c_file = destination_name + ".c"
    with open(c_file, "w+") as f:
        f.write(result)

    h_file_content = f"""\
#pragma once

#include "stdint.h"

extern uint8_t {c_identifier}_data[];
extern uint32_t {c_identifier}_size;
"""
    h_file = destination_name + ".h"
    with open(h_file, "w+") as f:
        f.write(h_file_content)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Wrong number of arguments")
        exit(1)

    file = sys.argv[1]
    destination_name = sys.argv[2]
    embed(file, destination_name)
