#include "vm/file_io.hpp"

#include <fstream>

namespace vm {

std::vector<uint8_t> read_binary_file(const std::filesystem::path& path) {
  if (!std::filesystem::exists(path)) {
    throw std::runtime_error("file '" + path.string() + "' does not exist");
  }

  if (!std::filesystem::is_regular_file(path)) {
    throw std::runtime_error("'" + path.string() + "' is not a regular file");
  }

  std::ifstream ifs(path, std::ios::binary);

  if (!ifs.is_open()) {
    throw std::runtime_error("failed to open '" + path.string() +
                             "': permission denied or I/O failure");
  }

  std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(ifs)),
                              std::istreambuf_iterator<char>());
  return buffer;
}

}  // namespace vm
