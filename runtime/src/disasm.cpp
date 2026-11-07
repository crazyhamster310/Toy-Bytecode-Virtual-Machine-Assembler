#include "vm/disasm.hpp"

#include <format>
#include <functional>
#include <ostream>
#include <vector>

#include "vm/errors.hpp"
#include "vm/opcodes.hpp"

namespace {

class DisassemblyEngine {
 private:
  std::vector<uint8_t> bytecode_;
  size_t cursor_{0};
  std::reference_wrapper<std::ostream> out_;

  int32_t fetch_i32(size_t instr_ip, vm::Opcode opcode) {
    if (cursor_ + 4 > bytecode_.size()) [[unlikely]] {
      throw vm::VmRuntimeError(instr_ip, opcode, vm::ErrorCode::TruncatedInstruction,
                               "truncated instruction: expected 4-byte operand");
    }

    uint32_t uval = static_cast<uint32_t>(bytecode_[cursor_]) |
                    (static_cast<uint32_t>(bytecode_[cursor_ + 1]) << 8U) |
                    (static_cast<uint32_t>(bytecode_[cursor_ + 2]) << 16U) |
                    (static_cast<uint32_t>(bytecode_[cursor_ + 3]) << 24U);
    cursor_ += 4;
    return static_cast<int32_t>(uval);
  }

  uint32_t fetch_u32(size_t instr_ip, vm::Opcode opcode) {
    if (cursor_ + 4 > bytecode_.size()) [[unlikely]] {
      throw vm::VmRuntimeError(instr_ip, opcode, vm::ErrorCode::TruncatedInstruction,
                               "truncated instruction: expected 4-byte operand");
    }

    uint32_t val = static_cast<uint32_t>(bytecode_[cursor_]) |
                   (static_cast<uint32_t>(bytecode_[cursor_ + 1]) << 8U) |
                   (static_cast<uint32_t>(bytecode_[cursor_ + 2]) << 16U) |
                   (static_cast<uint32_t>(bytecode_[cursor_ + 3]) << 24U);
    cursor_ += 4;
    return val;
  }

  void load_bytecode(std::span<const uint8_t> bytecode) {
    if (bytecode.size() < vm::HEADER_SIZE) [[unlikely]] {
      throw vm::VmLoadError(vm::ErrorCode::HeaderTooShort,
                            std::format("header too short: expected at least {} bytes, got {}",
                                        vm::HEADER_SIZE, bytecode.size()));
    }

    if (!std::ranges::equal(bytecode.first<4>(), vm::MAGIC_BYTES)) [[unlikely]] {
      throw vm::VmLoadError(vm::ErrorCode::InvalidMagicBytes,
                            "invalid magic bytes: expected 'BVM1'");
    }

    auto version = static_cast<uint16_t>(static_cast<uint32_t>(bytecode[4]) |
                                         (static_cast<uint32_t>(bytecode[5]) << 8U));
    if (version != vm::VERSION) [[unlikely]] {
      throw vm::VmLoadError(
          vm::ErrorCode::UnsupportedVersion,
          std::format("unsupported bytecode version: expected {}, got {}", vm::VERSION, version));
    }

    bytecode_.assign(bytecode.begin() + vm::HEADER_SIZE, bytecode.end());
    cursor_ = 0;
  }

 public:
  DisassemblyEngine(std::span<const uint8_t> bytecode, std::ostream& out) : out_(out) {
    load_bytecode(bytecode);
  }

  void run() {
    while (cursor_ < bytecode_.size()) {
      const size_t instr_ip = cursor_;
      const uint8_t raw_op = bytecode_[cursor_++];
      const auto opcode = static_cast<vm::Opcode>(raw_op);

      switch (opcode) {
        case vm::Opcode::HALT:
        case vm::Opcode::POP:
        case vm::Opcode::DUP:
        case vm::Opcode::SWAP:
        case vm::Opcode::ADD:
        case vm::Opcode::SUB:
        case vm::Opcode::MUL:
        case vm::Opcode::DIV:
        case vm::Opcode::MOD:
        case vm::Opcode::EQ:
        case vm::Opcode::LT:
        case vm::Opcode::GT:
        case vm::Opcode::PRINT:
        case vm::Opcode::READ:
        case vm::Opcode::PUTC:
        case vm::Opcode::LOAD:
        case vm::Opcode::STORE: {
          out_.get() << std::format("0x{:08X}: {}\n", instr_ip, vm::to_string(opcode));
          break;
        }
        case vm::Opcode::PUSH: {
          auto val = fetch_i32(instr_ip, opcode);
          out_.get() << std::format("0x{:08X}: {} {}\n", instr_ip, vm::to_string(opcode), val);
          break;
        }
        case vm::Opcode::JMP:
        case vm::Opcode::JZ: {
          auto address = fetch_u32(instr_ip, opcode);
          out_.get() << std::format("0x{:08X}: {} 0x{:08X}\n", instr_ip, vm::to_string(opcode),
                                    address);
          break;
        }
        default:
          throw vm::VmRuntimeError(instr_ip, opcode, vm::ErrorCode::InvalidOpcode,
                                   std::format("invalid opcode: 0x{:02X}", raw_op));
      }
    }
  }
};

}  // namespace

namespace vm {

void disassemble(std::span<const uint8_t> bytecode, std::ostream& out) {
  DisassemblyEngine(bytecode, out).run();
}

}  // namespace vm
