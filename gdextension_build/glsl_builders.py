from typing import Optional, Iterable

import os.path


def generate_inline_code(input_lines: Iterable[str], insert_newline: bool = True):
    """Take header data and generate inline code

    :param: input_lines: values for shared inline code
    :return: str - generated inline value
    """
    output = []
    for line in input_lines:
        if line:
            output.append(",".join(str(ord(c)) for c in line))
        if insert_newline:
            output.append("%s" % ord("\n"))
    output.append("0")
    return ",".join(output)


class RAWHeaderStruct:
    def __init__(self):
        self.code = ""


def include_file_in_raw_header(filename: str, header_data: RAWHeaderStruct, depth: int) -> None:
    fs = open(filename, "r")
    line = fs.readline()

    while line:
        while line.find("#include ") != -1:
            includeline = line.replace("#include ", "").strip()[1:-1]

            included_file = os.path.relpath(os.path.dirname(filename) + "/" + includeline)
            include_file_in_raw_header(included_file, header_data, depth + 1)

            line = fs.readline()

        header_data.code += line
        line = fs.readline()

    fs.close()


def build_raw_header(
    filename: str, optional_output_filename: Optional[str] = None, header_data: Optional[RAWHeaderStruct] = None
):
    header_data = header_data or RAWHeaderStruct()
    include_file_in_raw_header(filename, header_data, 0)

    if optional_output_filename is None:
        out_file = filename + ".gen.h"
    else:
        out_file = optional_output_filename

    out_file_base = out_file.replace(".glsl.gen.h", "_shader_glsl")
    out_file_base = out_file_base[out_file_base.rfind("/") + 1 :]
    out_file_base = out_file_base[out_file_base.rfind("\\") + 1 :]
    out_file_ifdef = out_file_base.replace(".", "_").upper()

    shader_template = f"""/* WARNING, THIS FILE WAS GENERATED, DO NOT EDIT */
#ifndef {out_file_ifdef}_RAW_H
#define {out_file_ifdef}_RAW_H

static const char {out_file_base}[] = {{
    {generate_inline_code(header_data.code, insert_newline=False)}
}};
#endif
"""

    with open(out_file, "w") as f:
        f.write(shader_template)


def build_raw_headers(target, source, env):
    for x in source:
        build_raw_header(filename=str(x))
