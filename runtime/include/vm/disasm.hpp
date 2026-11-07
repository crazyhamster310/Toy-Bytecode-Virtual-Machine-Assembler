#pragma once

#include <cstdint>
#include <ostream>
#include <span>

namespace vm {

void disassemble(std::span<const uint8_t> bytecode, std::ostream& out);

}  // namespace vm
