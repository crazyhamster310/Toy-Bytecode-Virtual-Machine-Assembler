#include "vm/vm.hpp"

#include <algorithm>
#include <limits>

#include "vm/errors.hpp"

namespace vm {

void VirtualMachine::load_bytecode(std::span<const uint8_t> bytecode) {
  if (bytecode.size() < HEADER_SIZE) {
    throw VmException("Header is too short");
  }

  if (!std::ranges::equal(bytecode.first<4>(), MAGIC_BYTES)) {
    throw VmException("Invalid magic bytes: not a BVM1 file");
  }

  auto version = static_cast<uint16_t>(static_cast<uint32_t>(bytecode[4]) |
                                       (static_cast<uint32_t>(bytecode[5]) << 8U));
  if (version != VERSION) {
    throw VmException("Unsupported bytecode version");
  }

  bytecode_.assign(bytecode.begin() + HEADER_SIZE, bytecode.end());
  ip_ = 0;
  halted_ = false;
  stack_.reset();
  memory_.reset();
}

uint8_t VirtualMachine::fetch_u8() {
  if (ip_ >= bytecode_.size()) {
    throw VmException("Unexpected end of bytecode: expected opcode");
  }

  return bytecode_[ip_++];
}

int32_t VirtualMachine::fetch_i32() {
  if (ip_ + 4 > bytecode_.size()) {
    throw VmException("Unexpected end of bytecode: truncated int32 operand");
  }

  uint32_t uval = static_cast<uint32_t>(bytecode_[ip_]) |
                  (static_cast<uint32_t>(bytecode_[ip_ + 1]) << 8U) |
                  (static_cast<uint32_t>(bytecode_[ip_ + 2]) << 16U) |
                  (static_cast<uint32_t>(bytecode_[ip_ + 3]) << 24U);
  ip_ += 4;
  return static_cast<int32_t>(uval);
}

uint32_t VirtualMachine::fetch_u32() {
  if (ip_ + 4 > bytecode_.size()) {
    throw VmException("Unexpected end of bytecode: truncated uint32 operand");
  }

  uint32_t val = static_cast<uint32_t>(bytecode_[ip_]) |
                 (static_cast<uint32_t>(bytecode_[ip_ + 1]) << 8U) |
                 (static_cast<uint32_t>(bytecode_[ip_ + 2]) << 16U) |
                 (static_cast<uint32_t>(bytecode_[ip_ + 3]) << 24U);
  ip_ += 4;
  return val;
}

void VirtualMachine::step() {
  if (halted_) {
    throw VmException("Execution attempted on halted machine");
  }

  uint8_t raw_op = fetch_u8();
  auto opcode = static_cast<Opcode>(raw_op);

  switch (opcode) {
    case Opcode::HALT: {
      halted_ = true;
      break;
    }
    case Opcode::PUSH: {
      stack_.push(fetch_i32());
      break;
    }
    case Opcode::POP: {
      stack_.pop();
      break;
    }
    case Opcode::DUP: {
      stack_.dup();
      break;
    }
    case Opcode::SWAP: {
      stack_.swap();
      break;
    }
    case Opcode::ADD: {
      int32_t rhs = stack_.pop();
      int32_t lhs = stack_.pop();
      stack_.push(static_cast<int32_t>(static_cast<uint32_t>(lhs) + static_cast<uint32_t>(rhs)));
      break;
    }
    case Opcode::SUB: {
      int32_t rhs = stack_.pop();
      int32_t lhs = stack_.pop();
      stack_.push(static_cast<int32_t>(static_cast<uint32_t>(lhs) - static_cast<uint32_t>(rhs)));
      break;
    }
    case Opcode::MUL: {
      int32_t rhs = stack_.pop();
      int32_t lhs = stack_.pop();
      stack_.push(static_cast<int32_t>(static_cast<uint32_t>(lhs) * static_cast<uint32_t>(rhs)));
      break;
    }
    case Opcode::DIV: {
      int32_t rhs = stack_.pop();
      int32_t lhs = stack_.pop();

      if (rhs == 0) {
        throw DivisionByZeroException();
      }

      if (lhs == std::numeric_limits<int32_t>::min() && rhs == -1) {
        throw VmException("Signed arithmetic overflow in division");
      }

      stack_.push(lhs / rhs);
      break;
    }
    case Opcode::MOD: {
      int32_t rhs = stack_.pop();
      int32_t lhs = stack_.pop();

      if (rhs == 0) {
        throw DivisionByZeroException();
      }

      if (lhs == std::numeric_limits<int32_t>::min() && rhs == -1) {
        throw VmException("Signed arithmetic overflow in modulo");
      }

      stack_.push(lhs % rhs);
      break;
    }
    case Opcode::EQ: {
      int32_t rhs = stack_.pop();
      int32_t lhs = stack_.pop();
      stack_.push(lhs == rhs ? 1 : 0);
      break;
    }
    case Opcode::LT: {
      int32_t rhs = stack_.pop();
      int32_t lhs = stack_.pop();
      stack_.push(lhs < rhs ? 1 : 0);
      break;
    }
    case Opcode::GT: {
      int32_t rhs = stack_.pop();
      int32_t lhs = stack_.pop();
      stack_.push(lhs > rhs ? 1 : 0);
      break;
    }
    case Opcode::JMP: {
      auto address = static_cast<size_t>(fetch_u32());
      if (address >= bytecode_.size()) {
        throw VmException("Jump target out of bounds");
      }
      ip_ = address;
      break;
    }
    case Opcode::JZ: {
      auto address = static_cast<size_t>(fetch_u32());
      if (address >= bytecode_.size()) {
        throw VmException("Jump target out of bounds");
      }
      if (stack_.pop() == 0) {
        ip_ = address;
      }
      break;
    }
    case Opcode::PRINT: {
      out_.get() << stack_.pop() << '\n';
      break;
    }
    case Opcode::LOAD: {
      stack_.push(memory_.load(static_cast<uint32_t>(stack_.pop())));
      break;
    }
    case Opcode::STORE: {
      auto address = static_cast<uint32_t>(stack_.pop());
      int32_t value = stack_.pop();
      memory_.store(address, value);
      break;
    }
    default:
      throw InvalidOpcodeException(raw_op);
  }
}

void VirtualMachine::run() {
  while (!halted_) {
    step();
  }
}

}  // namespace vm