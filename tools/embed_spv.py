#!/usr/bin/env python3

import struct
import sys

def main() -> int:
    if len(sys.argv) != 4:
        print(f"usage: {sys.argv[0]} <input.spv> <output.cpp> <VarName>", file=sys.stderr)
        return 1
    src, dst, var = sys.argv[1], sys.argv[2], sys.argv[3]
    with open(src, "rb") as f:
        data = f.read()
    if len(data) == 0 or len(data) % 4 != 0:
        print(f"error: {src}: SPIR-V size must be a non-zero multiple of 4", file=sys.stderr)
        return 1
    words = struct.unpack(f"<{len(data) // 4}I", data)
    with open(dst, "w") as out:
        out.write('#include <cstddef>\n')
        out.write('#include <cstdint>\n\n')
        out.write(f"extern const uint32_t {var}[] = {{\n")
        for i in range(0, len(words), 4):
            chunk = words[i : i + 4]
            out.write("    " + ", ".join(f"0x{w:08x}u" for w in chunk) + ",\n")
        out.write("};\n")
        out.write(f"extern const std::size_t {var}WordCount = {len(words)};\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
