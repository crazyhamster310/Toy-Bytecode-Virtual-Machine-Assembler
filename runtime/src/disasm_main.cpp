#include <filesystem>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

#include "vm/disasm.hpp"
#include "vm/errors.hpp"
#include "vm/file_io.hpp"

namespace {

void print_usage(std::ostream& stream, std::string_view program_name) {
  stream << "Usage: " << program_name << " [options] <bytecode.bin>\n\n"
         << "Toy Bytecode Virtual Machine (ByteVM) disassembler.\n\n"
         << "Arguments:\n"
         << "  <bytecode.bin>   Path to compiled binary file\n\n"
         << "Options:\n"
         << "  -h, --help       Display this help message and exit\n";
}

}  // namespace

int main(int argc, char* argv[]) {
  const std::span<char* const> args(argv, static_cast<size_t>(argc));
  std::string_view prog_name = (argc > 0) ? args[0] : "byte-disasm";

  std::filesystem::path bytecode_path;
  bool path_found = false;

  for (size_t i = 1; i < args.size(); ++i) {
    std::string_view arg = args[i];

    if (arg == "-h" || arg == "--help") {
      print_usage(std::cout, prog_name);
      return 0;
    }

    if (arg.starts_with('-')) {
      std::cerr << "error: unknown option '" << arg << "'\n\n";
      print_usage(std::cerr, prog_name);
      return 1;
    }

    if (path_found) {
      std::cerr << "error: too many positional arguments provided\n\n";
      print_usage(std::cerr, prog_name);
      return 1;
    }

    bytecode_path = arg;
    path_found = true;
  }

  if (!path_found) {
    std::cerr << "error: missing input bytecode file\n\n";
    print_usage(std::cerr, prog_name);
    return 1;
  }

  try {
    std::vector<uint8_t> bytecode = vm::read_binary_file(bytecode_path);

    vm::disassemble(bytecode, std::cout);
  } catch (const vm::VmException& e) {
    std::cerr << "error: " << e.what() << '\n';
    return 1;
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << '\n';
    return 1;
  }

  return 0;
}
