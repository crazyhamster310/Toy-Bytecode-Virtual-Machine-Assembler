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
         << "  -h, --help       Display this help message and exit\n"
         << "  -t, --trace      Enable execution tracing to stderr\n";
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

  bool trace_enabled = false;
  std::filesystem::path bytecode_path;
  bool path_found = false;

  for (size_t i = 1; i < args.size(); ++i) {
    std::string_view arg = args[i];

    if (arg == "-h" || arg == "--help") {
      print_usage(std::cout, prog_name);
      return 0;
    }

    if (arg == "-t" || arg == "--trace") {
      trace_enabled = true;
    } else if (arg.starts_with('-')) {
      std::cerr << "error: unknown option '" << arg << "'\n\n";
      print_usage(std::cerr, prog_name);
      return 1;
    } else if (path_found) {
      std::cerr << "error: too many positional arguments provided\n\n";
      print_usage(std::cerr, prog_name);
      return 1;
    } else {
      bytecode_path = arg;
      path_found = true;
    }
  }

  if (!path_found) {
    std::cerr << "error: missing input bytecode file\n\n";
    print_usage(std::cerr, prog_name);
    return 1;
  }

  try {
    std::vector<uint8_t> bytecode = read_file(bytecode_path);

    vm::VirtualMachine virt({.trace = trace_enabled});
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