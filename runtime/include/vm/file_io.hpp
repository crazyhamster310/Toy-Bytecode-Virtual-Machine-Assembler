#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace vm {

std::vector<uint8_t> read_binary_file(const std::filesystem::path& path);

}  // namespace vm
