#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

#include "vm/errors.hpp"
#include "vm/vm.hpp"

namespace {

void print_usage(std::ostream& stream, std::string_view program_name) {
  stream << "Usage: " << program_name << " [options] <bytecode.bin>\n\n"
         << "Toy Bytecode Virtual Machine (ByteVM) runtime execution engine.\n\n"
         << "Arguments:\n"
         << "  <bytecode.bin>   Path to compiled binary file\n\n"
         << "Options:\n"
         << "  -h, --help       Display this help message and exit\n";
}

std::vector<uint8_t> read_file(const std::filesystem::path& path) {
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

}  // namespace

int main(int argc, char* argv[]) {
  const std::span<char* const> args(argv, static_cast<size_t>(argc));
  std::string_view prog_name = (argc > 0) ? args[0] : "byte-vm";

  if (args.size() == 2 &&
      (args[1] == std::string_view("-h") || args[1] == std::string_view("--help"))) {
    print_usage(std::cout, prog_name);
    return 0;
  }

  if (args.size() < 2) {
    std::cerr << "error: missing input bytecode file\n\n";
    print_usage(std::cerr, prog_name);
    return 1;
  }

  if (args.size() > 2) {
    std::cerr << "error: too many arguments provided\n\n";
    print_usage(std::cerr, prog_name);
    return 1;
  }

  try {
    std::filesystem::path filepath = args[1];
    std::vector<uint8_t> bytecode = read_file(filepath);

    vm::VirtualMachine virt;
    virt.load_bytecode(bytecode);
    virt.run();
  } catch (const vm::VmException& e) {
    std::cerr << "error: " << e.what() << '\n';
    return 1;
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << '\n';
    return 1;
  }

  return 0;
}