#include "vm/vm.hpp"

#include <algorithm>
#include <format>
#include <limits>

#include "vm/errors.hpp"

namespace {

std::string escape_char(char character) {
  switch (character) {
    case '\n':
      return "\\n";
    case '\t':
      return "\\t";
    case '\r':
      return "\\r";
    case '\0':
      return "\\0";
    case '\'':
      return "\\\'";
    case '\\':
      return "\\\\";
    default:
      break;
  }

  const auto uchar = static_cast<unsigned char>(character);
  if (std::isprint(uchar) != 0) {
    return {character};
  }
  return std::format("\\x{:02X}", uchar);
}

std::string format_stack(std::span<const int32_t> data) {
  std::string result;
  for (size_t i = 0; i < data.size(); ++i) {
    if (i > 0) {
      result += ", ";
    }
    result += std::to_string(data[i]);
  }
  return result;
}

void require_valid_divisor(int32_t lhs, int32_t rhs, size_t instr_ip, vm::Opcode opcode) {
  if (rhs == 0) [[unlikely]] {
    throw vm::VmRuntimeError(instr_ip, opcode, vm::ErrorCode::DivisionByZero, "division by zero");
  }

  if (lhs == std::numeric_limits<int32_t>::min() && rhs == -1) [[unlikely]] {
    throw vm::VmRuntimeError(instr_ip, opcode, vm::ErrorCode::SignedOverflow,
                             opcode == vm::Opcode::DIV ? "signed arithmetic overflow in division"
                                                       : "signed arithmetic overflow in modulo");
  }
}

}  // namespace

namespace vm {

void VirtualMachine::load_bytecode(std::span<const uint8_t> bytecode) {
  if (bytecode.size() < HEADER_SIZE) [[unlikely]] {
    throw VmLoadError(ErrorCode::HeaderTooShort,
                      std::format("header too short: expected at least {} bytes, got {}",
                                  HEADER_SIZE, bytecode.size()));
  }

  if (!std::ranges::equal(bytecode.first<4>(), MAGIC_BYTES)) [[unlikely]] {
    throw VmLoadError(ErrorCode::InvalidMagicBytes, "invalid magic bytes: expected 'BVM1'");
  }

  auto version = static_cast<uint16_t>(static_cast<uint32_t>(bytecode[4]) |
                                       (static_cast<uint32_t>(bytecode[5]) << 8U));
  if (version != VERSION) [[unlikely]] {
    throw VmLoadError(
        ErrorCode::UnsupportedVersion,
        std::format("unsupported bytecode version: expected {}, got {}", VERSION, version));
  }

  bytecode_.assign(bytecode.begin() + HEADER_SIZE, bytecode.end());
  ip_ = 0;
  halted_ = false;
  stack_.reset();
  memory_.reset();
}

int32_t VirtualMachine::fetch_i32(size_t instr_ip, Opcode opcode) {
  if (ip_ + 4 > bytecode_.size()) [[unlikely]] {
    throw VmRuntimeError(instr_ip, opcode, ErrorCode::TruncatedInstruction,
                         "truncated instruction: expected 4-byte operand");
  }

  uint32_t uval = static_cast<uint32_t>(bytecode_[ip_]) |
                  (static_cast<uint32_t>(bytecode_[ip_ + 1]) << 8U) |
                  (static_cast<uint32_t>(bytecode_[ip_ + 2]) << 16U) |
                  (static_cast<uint32_t>(bytecode_[ip_ + 3]) << 24U);
  ip_ += 4;
  return static_cast<int32_t>(uval);
}

uint32_t VirtualMachine::fetch_u32(size_t instr_ip, Opcode opcode) {
  if (ip_ + 4 > bytecode_.size()) [[unlikely]] {
    throw VmRuntimeError(instr_ip, opcode, ErrorCode::TruncatedInstruction,
                         "truncated instruction: expected 4-byte operand");
  }

  uint32_t val = static_cast<uint32_t>(bytecode_[ip_]) |
                 (static_cast<uint32_t>(bytecode_[ip_ + 1]) << 8U) |
                 (static_cast<uint32_t>(bytecode_[ip_ + 2]) << 16U) |
                 (static_cast<uint32_t>(bytecode_[ip_ + 3]) << 24U);
  ip_ += 4;
  return val;
}

void VirtualMachine::require_stack(size_t count, size_t instr_ip, Opcode opcode) const {
  if (stack_.size() < count) [[unlikely]] {
    throw VmRuntimeError(instr_ip, opcode, ErrorCode::StackUnderflow,
                         std::format("stack underflow: requires {} operand(s), available: {}",
                                     count, stack_.size()));
  }
}

void VirtualMachine::ensure_stack_capacity(size_t needed, size_t instr_ip, Opcode opcode) const {
  if (needed > stack_.capacity() - stack_.size()) [[unlikely]] {
    throw VmRuntimeError(instr_ip, opcode, ErrorCode::StackOverflow,
                         std::format("stack overflow: requires {} slot(s), available: {}", needed,
                                     stack_.capacity() - stack_.size()));
  }
}

void VirtualMachine::require_memory_bounds(uint32_t address, size_t instr_ip, Opcode opcode) const {
  if (static_cast<size_t>(address) >= memory_.size()) [[unlikely]] {
    throw VmRuntimeError(instr_ip, opcode, ErrorCode::MemoryOutOfBounds,
                         std::format("memory out of bounds: address 0x{:08X} exceeds size {}",
                                     address, memory_.size()));
  }
}

size_t VirtualMachine::fetch_jump_address(size_t instr_ip, Opcode opcode) {
  auto address = static_cast<size_t>(fetch_u32(instr_ip, opcode));
  if (address >= bytecode_.size()) [[unlikely]] {
    throw VmRuntimeError(
        instr_ip, opcode, ErrorCode::JumpOutOfBounds,
        std::format("jump out of bounds: target offset 0x{:08X} exceeds bytecode size {}", address,
                    bytecode_.size()));
  }
  return address;
}

void VirtualMachine::step() {
  if (halted_) [[unlikely]] {
    throw VmRuntimeError(ip_, Opcode::HALT, ErrorCode::ExecutionHalted,
                         "execution attempted on halted machine");
  }

  if (ip_ >= bytecode_.size()) [[unlikely]] {
    throw VmRuntimeError(ip_, Opcode::HALT, ErrorCode::MissingHalt,
                         "unexpected end of bytecode: missing HALT instruction");
  }

  const size_t instr_ip = ip_;
  const uint8_t raw_op = bytecode_[ip_++];
  const auto opcode = static_cast<Opcode>(raw_op);

  std::string instr_str;
  std::string effect_str;
  if (config_.trace) {
    instr_str = to_string(opcode);
  }

  switch (opcode) {
    case Opcode::HALT: {
      halted_ = true;
      break;
    }
    case Opcode::PUSH: {
      ensure_stack_capacity(1, instr_ip, opcode);
      int32_t val = fetch_i32(instr_ip, opcode);
      stack_.push(val);
      if (config_.trace) {
        instr_str = std::format("{} {}", to_string(opcode), val);
      }
      break;
    }
    case Opcode::POP: {
      require_stack(1, instr_ip, opcode);
      stack_.pop();
      break;
    }
    case Opcode::DUP: {
      require_stack(1, instr_ip, opcode);
      ensure_stack_capacity(1, instr_ip, opcode);
      stack_.dup();
      break;
    }
    case Opcode::SWAP: {
      require_stack(2, instr_ip, opcode);
      stack_.swap();
      break;
    }
    case Opcode::ADD: {
      require_stack(2, instr_ip, opcode);
      int32_t rhs = stack_.pop();
      int32_t lhs = stack_.pop();
      stack_.push(static_cast<int32_t>(static_cast<uint32_t>(lhs) + static_cast<uint32_t>(rhs)));
      break;
    }
    case Opcode::SUB: {
      require_stack(2, instr_ip, opcode);
      int32_t rhs = stack_.pop();
      int32_t lhs = stack_.pop();
      stack_.push(static_cast<int32_t>(static_cast<uint32_t>(lhs) - static_cast<uint32_t>(rhs)));
      break;
    }
    case Opcode::MUL: {
      require_stack(2, instr_ip, opcode);
      int32_t rhs = stack_.pop();
      int32_t lhs = stack_.pop();
      stack_.push(static_cast<int32_t>(static_cast<uint32_t>(lhs) * static_cast<uint32_t>(rhs)));
      break;
    }
    case Opcode::DIV: {
      require_stack(2, instr_ip, opcode);
      int32_t rhs = stack_.pop();
      int32_t lhs = stack_.pop();
      require_valid_divisor(lhs, rhs, instr_ip, opcode);
      stack_.push(lhs / rhs);
      break;
    }
    case Opcode::MOD: {
      require_stack(2, instr_ip, opcode);
      int32_t rhs = stack_.pop();
      int32_t lhs = stack_.pop();
      require_valid_divisor(lhs, rhs, instr_ip, opcode);
      stack_.push(lhs % rhs);
      break;
    }
    case Opcode::EQ: {
      require_stack(2, instr_ip, opcode);
      int32_t rhs = stack_.pop();
      int32_t lhs = stack_.pop();
      stack_.push(static_cast<int32_t>(lhs == rhs));
      break;
    }
    case Opcode::LT: {
      require_stack(2, instr_ip, opcode);
      int32_t rhs = stack_.pop();
      int32_t lhs = stack_.pop();
      stack_.push(static_cast<int32_t>(lhs < rhs));
      break;
    }
    case Opcode::GT: {
      require_stack(2, instr_ip, opcode);
      int32_t rhs = stack_.pop();
      int32_t lhs = stack_.pop();
      stack_.push(static_cast<int32_t>(lhs > rhs));
      break;
    }
    case Opcode::JMP: {
      auto address = fetch_jump_address(instr_ip, opcode);
      ip_ = address;
      if (config_.trace) {
        instr_str = std::format("{} 0x{:08X}", to_string(opcode), address);
      }
      break;
    }
    case Opcode::JZ: {
      auto address = fetch_jump_address(instr_ip, opcode);
      require_stack(1, instr_ip, opcode);
      if (stack_.pop() == 0) {
        ip_ = address;
      }
      if (config_.trace) {
        instr_str = std::format("{} 0x{:08X}", to_string(opcode), address);
      }
      break;
    }
    case Opcode::PRINT: {
      require_stack(1, instr_ip, opcode);
      auto val = stack_.pop();
      out_.get() << val << '\n';
      if (config_.trace) {
        effect_str = std::format("Out <= {}", val);
      }
      break;
    }
    case Opcode::READ: {
      ensure_stack_capacity(1, instr_ip, opcode);
      int32_t val = 0;
      if (!(in_.get() >> val)) {
        throw VmRuntimeError(instr_ip, opcode, ErrorCode::InputFailure, "input failure");
      }
      stack_.push(val);
      if (config_.trace) {
        effect_str = std::format("In => {}", val);
      }
      break;
    }
    case Opcode::PUTC: {
      require_stack(1, instr_ip, opcode);
      auto val = static_cast<char>(stack_.pop());
      out_.get().put(val);
      if (config_.trace) {
        effect_str = std::format("Out <= '{}'", escape_char(val));
      }
      break;
    }
    case Opcode::LOAD: {
      require_stack(1, instr_ip, opcode);
      auto address = static_cast<uint32_t>(stack_.pop());
      require_memory_bounds(address, instr_ip, opcode);
      auto val = memory_.load(address);
      stack_.push(val);
      if (config_.trace) {
        effect_str = std::format("Mem[{}] => {}", address, val);
      }
      break;
    }
    case Opcode::STORE: {
      require_stack(2, instr_ip, opcode);
      auto address = static_cast<uint32_t>(stack_.pop());
      require_memory_bounds(address, instr_ip, opcode);
      int32_t val = stack_.pop();
      memory_.store(address, val);
      if (config_.trace) {
        effect_str = std::format("Mem[{}] <= {}", address, val);
      }
      break;
    }
    default:
      throw VmRuntimeError(instr_ip, opcode, ErrorCode::InvalidOpcode,
                           std::format("invalid opcode: 0x{:02X}", raw_op));
  }

  if (config_.trace) {
    trace_out_.get() << std::format("[IP: 0x{:08X}] {:<16} | {:<18} | Stack: [{}]\n", instr_ip,
                                    instr_str, effect_str, format_stack(stack_.data()));
  }
}

void VirtualMachine::run() {
  while (!halted_) {
    step();
  }
}

}  // namespace vm