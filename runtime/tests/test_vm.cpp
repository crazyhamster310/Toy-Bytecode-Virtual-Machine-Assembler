#include <gtest/gtest.h>

#include <concepts>
#include <source_location>

#include "vm/errors.hpp"
#include "vm/opcodes.hpp"
#include "vm/vm.hpp"

namespace vm::test {

class BytecodeBuilder {
 private:
  std::vector<uint8_t> bytes_;

  BytecodeBuilder& write_uint32_le(uint32_t value) {
    bytes_.push_back(static_cast<uint8_t>(value & 0xFFU));
    bytes_.push_back(static_cast<uint8_t>((value >> 8U) & 0xFFU));
    bytes_.push_back(static_cast<uint8_t>((value >> 16U) & 0xFFU));
    bytes_.push_back(static_cast<uint8_t>((value >> 24U) & 0xFFU));
    return *this;
  }

  BytecodeBuilder& write_int32_le(int32_t value) {
    return write_uint32_le(static_cast<uint32_t>(value));
  }

 public:
  [[nodiscard]] std::vector<uint8_t> build() const {
    std::vector<uint8_t> result;
    result.reserve(HEADER_SIZE + bytes_.size());
    std::ranges::copy(MAGIC_BYTES, std::back_inserter(result));
    result.push_back(static_cast<uint8_t>(static_cast<uint32_t>(VERSION) & 0xFFU));
    result.push_back(static_cast<uint8_t>((static_cast<uint32_t>(VERSION) >> 8U) & 0xFFU));
    std::ranges::copy(bytes_, std::back_inserter(result));
    return result;
  }

  BytecodeBuilder& write_opcode(const Opcode& opcode) {
    bytes_.push_back(static_cast<uint8_t>(opcode));
    return *this;
  }

  BytecodeBuilder& push(int32_t value) { return write_opcode(Opcode::PUSH).write_int32_le(value); }

  BytecodeBuilder& halt() { return write_opcode(Opcode::HALT); }
  BytecodeBuilder& pop() { return write_opcode(Opcode::POP); }
  BytecodeBuilder& dup() { return write_opcode(Opcode::DUP); }
  BytecodeBuilder& swap() { return write_opcode(Opcode::SWAP); }

  BytecodeBuilder& add() { return write_opcode(Opcode::ADD); }
  BytecodeBuilder& sub() { return write_opcode(Opcode::SUB); }
  BytecodeBuilder& mul() { return write_opcode(Opcode::MUL); }
  BytecodeBuilder& div() { return write_opcode(Opcode::DIV); }
  BytecodeBuilder& mod() { return write_opcode(Opcode::MOD); }

  BytecodeBuilder& eq() { return write_opcode(Opcode::EQ); }
  BytecodeBuilder& lt() { return write_opcode(Opcode::LT); }
  BytecodeBuilder& gt() { return write_opcode(Opcode::GT); }

  BytecodeBuilder& print() { return write_opcode(Opcode::PRINT); }
  BytecodeBuilder& read() { return write_opcode(Opcode::READ); }
  BytecodeBuilder& putc() { return write_opcode(Opcode::PUTC); }

  BytecodeBuilder& load() { return write_opcode(Opcode::LOAD); }
  BytecodeBuilder& store() { return write_opcode(Opcode::STORE); }

  BytecodeBuilder& jmp(uint32_t address) {
    return write_opcode(Opcode::JMP).write_uint32_le(address);
  }

  BytecodeBuilder& jz(uint32_t address) {
    return write_opcode(Opcode::JZ).write_uint32_le(address);
  }
};

}  // namespace vm::test

namespace {

void expect_vm_runtime_error(std::invocable auto&& func, vm::ErrorCode expected_code,
                             size_t expected_ip, vm::Opcode expected_opcode,
                             const std::source_location& loc = std::source_location::current()) {
  testing::ScopedTrace trace(loc.file_name(), static_cast<int>(loc.line()),
                             "expect_vm_runtime_error");

  try {
    func();
    FAIL() << "Expected VmRuntimeError with code " << static_cast<int>(expected_code);
  } catch (const vm::VmRuntimeError& err) {
    EXPECT_EQ(err.code(), expected_code);
    EXPECT_EQ(err.ip(), expected_ip);
    EXPECT_EQ(err.opcode(), expected_opcode);
  } catch (const std::exception& e) {
    FAIL() << "Expected VmRuntimeError, but caught: " << e.what();
  }
}

void expect_vm_load_error(std::invocable auto&& func, vm::ErrorCode expected_code,
                          const std::source_location& loc = std::source_location::current()) {
  testing::ScopedTrace trace(loc.file_name(), static_cast<int>(loc.line()), "expect_vm_load_error");

  try {
    func();
    FAIL() << "Expected VmLoadError with code " << static_cast<int>(expected_code);
  } catch (const vm::VmLoadError& err) {
    EXPECT_EQ(err.code(), expected_code);
  } catch (const std::exception& e) {
    FAIL() << "Expected VmLoadError, but caught: " << e.what();
  }
}

}  // namespace

TEST(HeaderValidationTest, BadMagicBytes) {
  std::vector<uint8_t> header = {
      'B', 'V', 'M', '2', 0x01, 0x00,
  };
  expect_vm_load_error([&] { vm::VirtualMachine().load_bytecode(header); },
                       vm::ErrorCode::InvalidMagicBytes);
}

TEST(HeaderValidationTest, TruncatedHeader) {
  std::vector<uint8_t> header = {
      'B',
      'V',
  };
  expect_vm_load_error([&] { vm::VirtualMachine().load_bytecode(header); },
                       vm::ErrorCode::HeaderTooShort);
}

TEST(HeaderValidationTest, WrongVersion) {
  std::vector<uint8_t> header = {
      'B', 'V', 'M', '1', 0x02, 0x00,
  };
  expect_vm_load_error([&] { vm::VirtualMachine().load_bytecode(header); },
                       vm::ErrorCode::UnsupportedVersion);
}

TEST(HeaderValidationTest, ValidHeader) {
  std::vector<uint8_t> header = {
      'B', 'V', 'M', '1', 0x01, 0x00,
  };
  ASSERT_NO_THROW(vm::VirtualMachine().load_bytecode(header));
}

class TruncatedImmediateTest : public testing::TestWithParam<vm::Opcode> {};

TEST_P(TruncatedImmediateTest, ThrowsWhenOperandIsTruncated) {
  std::vector<uint8_t> code = {
      'B', 'V', 'M', '1', 0x01, 0x00, static_cast<uint8_t>(GetParam()), 0x42, 0x00,
  };
  vm::VirtualMachine virt;
  virt.load_bytecode(code);
  expect_vm_runtime_error([&] { virt.run(); }, vm::ErrorCode::TruncatedInstruction, 0, GetParam());
}

INSTANTIATE_TEST_SUITE_P(UnaryOperations, TruncatedImmediateTest,
                         testing::Values(vm::Opcode::PUSH, vm::Opcode::JMP, vm::Opcode::JZ));

TEST(SystemIoTest, InvalidOpcode) {
  std::vector<uint8_t> code = {
      'B', 'V', 'M', '1', 0x01, 0x00, 0xFE,
  };
  vm::VirtualMachine virt;
  virt.load_bytecode(code);
  expect_vm_runtime_error([&] { virt.run(); }, vm::ErrorCode::InvalidOpcode, 0,
                          static_cast<vm::Opcode>(0xFE));
}

TEST(SystemIoTest, MissingHaltThrows) {
  auto code = vm::test::BytecodeBuilder().push(1).build();

  vm::VirtualMachine virt;
  virt.load_bytecode(code);
  expect_vm_runtime_error([&] { virt.run(); }, vm::ErrorCode::MissingHalt, 5, vm::Opcode::HALT);
}

TEST(SystemIoTest, HaltStopsExecution) {
  auto code = vm::test::BytecodeBuilder().halt().push(1).build();

  vm::VirtualMachine virt;
  virt.load_bytecode(code);
  virt.run();

  ASSERT_TRUE(virt.is_halted());
  ASSERT_TRUE(virt.stack().empty());
}

TEST(SystemIoTest, PrintInstruction) {
  auto code = vm::test::BytecodeBuilder()
                  .push(1)
                  .print()
                  .push(-1)
                  .print()
                  .push(std::numeric_limits<int32_t>::max())
                  .print()
                  .push(std::numeric_limits<int32_t>::min())
                  .print()
                  .halt()
                  .build();

  std::ostringstream oss;
  vm::VirtualMachine virt(vm::VmConfig(), oss);
  virt.load_bytecode(code);

  virt.run();
  ASSERT_TRUE(virt.stack().empty());
  ASSERT_EQ(oss.view(), "1\n-1\n2147483647\n-2147483648\n");
}

TEST(SystemIoTest, ReadInstruction) {
  auto code = vm::test::BytecodeBuilder().read().halt().build();

  std::istringstream iss("1\n");
  std::ostringstream oss;
  vm::VirtualMachine virt(vm::VmConfig(), oss, iss);
  virt.load_bytecode(code);
  virt.run();

  ASSERT_EQ(virt.stack().size(), 1);
  ASSERT_EQ(virt.stack().peek(), 1);
}

TEST(SystemIoTest, InputFailureOnEmpty) {
  auto code = vm::test::BytecodeBuilder().read().halt().build();

  std::istringstream iss("");
  std::ostringstream oss;
  vm::VirtualMachine virt(vm::VmConfig(), oss, iss);
  virt.load_bytecode(code);
  expect_vm_runtime_error([&] { virt.run(); }, vm::ErrorCode::InputFailure, 0, vm::Opcode::READ);
}

TEST(SystemIoTest, InputFailureOnNotNumber) {
  auto code = vm::test::BytecodeBuilder().read().halt().build();

  std::istringstream iss("abba\n");
  std::ostringstream oss;
  vm::VirtualMachine virt(vm::VmConfig(), oss, iss);
  virt.load_bytecode(code);
  expect_vm_runtime_error([&] { virt.run(); }, vm::ErrorCode::InputFailure, 0, vm::Opcode::READ);
}

TEST(SystemIoTest, StackOverflowRead) {
  auto code = vm::test::BytecodeBuilder().push(1).read().halt().build();
  std::istringstream iss("42\n");
  std::ostringstream oss;
  vm::VirtualMachine virt(vm::VmConfig{.stack_size = 1}, oss, iss);
  virt.load_bytecode(code);

  ASSERT_NO_THROW(virt.step());
  expect_vm_runtime_error([&] { virt.step(); }, vm::ErrorCode::StackOverflow, 5, vm::Opcode::READ);
}

TEST(SystemIoTest, PutcInstruction) {
  auto code = vm::test::BytecodeBuilder().push(65).putc().halt().build();

  std::ostringstream oss;
  vm::VirtualMachine virt(vm::VmConfig(), oss);
  virt.load_bytecode(code);
  virt.run();

  ASSERT_EQ(oss.str(), "A");
}

TEST(LifecycleTest, StateResetOnReload) {
  auto code = vm::test::BytecodeBuilder().push(0).push(1).store().push(1).halt().build();

  vm::VirtualMachine virt;
  virt.load_bytecode(code);
  virt.run();

  virt.load_bytecode(code);
  ASSERT_FALSE(virt.is_halted());
  ASSERT_EQ(virt.ip(), 0);
  ASSERT_TRUE(virt.stack().empty());
  ASSERT_EQ(virt.memory().load(0), 0);
}

TEST(LifecycleTest, StepAfterHaltThrows) {
  auto code = vm::test::BytecodeBuilder().halt().build();

  vm::VirtualMachine virt;
  virt.load_bytecode(code);
  virt.run();
  expect_vm_runtime_error([&] { virt.step(); }, vm::ErrorCode::ExecutionHalted, 1,
                          vm::Opcode::HALT);
}

TEST(LifecycleTest, ExecutionWithoutBytecodeThrows) {
  ASSERT_THROW(vm::VirtualMachine().step(), vm::VmException);
}

class UnaryUnderflowTest : public testing::TestWithParam<vm::Opcode> {};

TEST_P(UnaryUnderflowTest, ThrowsOnEmptyStack) {
  auto code = vm::test::BytecodeBuilder().write_opcode(GetParam()).halt().build();

  vm::VirtualMachine virt;
  virt.load_bytecode(code);
  expect_vm_runtime_error([&] { virt.run(); }, vm::ErrorCode::StackUnderflow, 0, GetParam());
}

INSTANTIATE_TEST_SUITE_P(UnaryOperations, UnaryUnderflowTest,
                         testing::Values(vm::Opcode::DUP, vm::Opcode::LOAD, vm::Opcode::POP,
                                         vm::Opcode::PRINT, vm::Opcode::PUTC));

class BinaryUnderflowTest : public testing::TestWithParam<vm::Opcode> {};

TEST_P(BinaryUnderflowTest, ThrowsOnEmptyStack) {
  auto code = vm::test::BytecodeBuilder().write_opcode(GetParam()).halt().build();

  vm::VirtualMachine virt;
  virt.load_bytecode(code);
  expect_vm_runtime_error([&] { virt.run(); }, vm::ErrorCode::StackUnderflow, 0, GetParam());
}

TEST_P(BinaryUnderflowTest, ThrowsOnSingleOperand) {
  auto code = vm::test::BytecodeBuilder().push(42).write_opcode(GetParam()).halt().build();

  vm::VirtualMachine virt;
  virt.load_bytecode(code);
  expect_vm_runtime_error([&] { virt.run(); }, vm::ErrorCode::StackUnderflow, 5, GetParam());
}

INSTANTIATE_TEST_SUITE_P(BinaryOperations, BinaryUnderflowTest,
                         testing::Values(vm::Opcode::ADD, vm::Opcode::DIV, vm::Opcode::EQ,
                                         vm::Opcode::GT, vm::Opcode::LT, vm::Opcode::MOD,
                                         vm::Opcode::MUL, vm::Opcode::STORE, vm::Opcode::SUB,
                                         vm::Opcode::SWAP));

TEST(StackOperationsTest, PushPop) {
  auto code = vm::test::BytecodeBuilder().push(10).push(20).pop().halt().build();
  vm::VirtualMachine virt;
  virt.load_bytecode(code);

  virt.step();
  ASSERT_EQ(virt.ip(), 5);
  ASSERT_EQ(virt.stack().size(), 1);
  ASSERT_EQ(virt.stack().peek(), 10);

  virt.step();
  ASSERT_EQ(virt.stack().size(), 2);
  ASSERT_EQ(virt.stack().peek(), 20);

  virt.step();
  ASSERT_EQ(virt.ip(), 11);
  ASSERT_EQ(virt.stack().size(), 1);
  ASSERT_EQ(virt.stack().peek(), 10);
}

TEST(StackOperationsTest, PushNegativeAndBoundaries) {
  auto code = vm::test::BytecodeBuilder()
                  .push(-1)
                  .push(std::numeric_limits<int32_t>::min())
                  .push(std::numeric_limits<int32_t>::max())
                  .halt()
                  .build();

  vm::VirtualMachine virt;
  virt.load_bytecode(code);

  virt.step();
  ASSERT_EQ(virt.stack().peek(), -1);

  virt.step();
  ASSERT_EQ(virt.stack().peek(), std::numeric_limits<int32_t>::min());

  virt.step();
  ASSERT_EQ(virt.stack().peek(), std::numeric_limits<int32_t>::max());
}

TEST(StackOperationsTest, Dup) {
  auto code = vm::test::BytecodeBuilder().push(1).dup().pop().halt().build();

  vm::VirtualMachine virt;
  virt.load_bytecode(code);

  virt.step();
  virt.step();
  ASSERT_EQ(virt.ip(), 6);
  ASSERT_EQ(virt.stack().size(), 2);
  ASSERT_EQ(virt.stack().peek(), 1);

  virt.step();
  ASSERT_EQ(virt.stack().peek(), 1);
}

TEST(StackOperationsTest, Swap) {
  auto code = vm::test::BytecodeBuilder().push(1).push(2).swap().pop().halt().build();

  vm::VirtualMachine virt;
  virt.load_bytecode(code);

  virt.step();
  virt.step();
  virt.step();
  ASSERT_EQ(virt.ip(), 11);
  ASSERT_EQ(virt.stack().size(), 2);
  ASSERT_EQ(virt.stack().peek(), 1);

  virt.step();
  ASSERT_EQ(virt.stack().peek(), 2);
}

TEST(StackOperationsTest, StackOverflowPush) {
  auto code = vm::test::BytecodeBuilder().push(1).push(2).halt().build();

  vm::VirtualMachine virt({.stack_size = 1});
  virt.load_bytecode(code);

  ASSERT_NO_THROW(virt.step());
  expect_vm_runtime_error([&] { virt.step(); }, vm::ErrorCode::StackOverflow, 5, vm::Opcode::PUSH);
}

TEST(StackOperationsTest, StackOverflowDup) {
  auto code = vm::test::BytecodeBuilder().push(1).dup().halt().build();
  vm::VirtualMachine virt({.stack_size = 1});
  virt.load_bytecode(code);

  ASSERT_NO_THROW(virt.step());
  expect_vm_runtime_error([&] { virt.step(); }, vm::ErrorCode::StackOverflow, 5, vm::Opcode::DUP);
}

struct ArithmeticParam {
  int32_t a;
  int32_t b;
  vm::Opcode op;
  int32_t expected;
};

class ArithmeticTableTest : public testing::TestWithParam<ArithmeticParam> {};

TEST_P(ArithmeticTableTest, ComputesCorrectResult) {
  const auto& [lhs, rhs, op, expected] = GetParam();
  auto code = vm::test::BytecodeBuilder().push(lhs).push(rhs).write_opcode(op).halt().build();

  vm::VirtualMachine virt;
  virt.load_bytecode(code);
  virt.run();

  ASSERT_EQ(virt.stack().size(), 1);
  ASSERT_EQ(virt.stack().peek(), expected);
}

INSTANTIATE_TEST_SUITE_P(
    AllArithmetic, ArithmeticTableTest,
    testing::Values(
        // ADD
        ArithmeticParam{.a = 2, .b = 3, .op = vm::Opcode::ADD, .expected = 5},
        ArithmeticParam{.a = -2, .b = -3, .op = vm::Opcode::ADD, .expected = -5},
        ArithmeticParam{.a = -5, .b = 5, .op = vm::Opcode::ADD, .expected = 0},

        // SUB
        ArithmeticParam{.a = 20, .b = 6, .op = vm::Opcode::SUB, .expected = 14},
        ArithmeticParam{.a = 6, .b = 20, .op = vm::Opcode::SUB, .expected = -14},

        // MUL
        ArithmeticParam{.a = 3, .b = 4, .op = vm::Opcode::MUL, .expected = 12},
        ArithmeticParam{.a = -3, .b = 4, .op = vm::Opcode::MUL, .expected = -12},

        // DIV
        ArithmeticParam{.a = 20, .b = 6, .op = vm::Opcode::DIV, .expected = 3},
        ArithmeticParam{.a = -7, .b = 2, .op = vm::Opcode::DIV, .expected = -3},

        // MOD
        ArithmeticParam{.a = 20, .b = 6, .op = vm::Opcode::MOD, .expected = 2},
        ArithmeticParam{.a = -7, .b = 2, .op = vm::Opcode::MOD, .expected = -1}));

class DivisionFaultTest : public testing::TestWithParam<vm::Opcode> {};

TEST_P(DivisionFaultTest, DivisionByZeroThrows) {
  auto code = vm::test::BytecodeBuilder().push(1).push(0).write_opcode(GetParam()).halt().build();

  vm::VirtualMachine virt;
  virt.load_bytecode(code);
  expect_vm_runtime_error([&] { virt.run(); }, vm::ErrorCode::DivisionByZero, 10, GetParam());
}

TEST_P(DivisionFaultTest, SignedOverflowThrows) {
  auto code = vm::test::BytecodeBuilder()
                  .push(std::numeric_limits<int32_t>::min())
                  .push(-1)
                  .write_opcode(GetParam())
                  .halt()
                  .build();

  vm::VirtualMachine virt;
  virt.load_bytecode(code);
  expect_vm_runtime_error([&] { virt.run(); }, vm::ErrorCode::SignedOverflow, 10, GetParam());
}

INSTANTIATE_TEST_SUITE_P(DivisionOperations, DivisionFaultTest,
                         testing::Values(vm::Opcode::DIV, vm::Opcode::MOD));

struct ComparisonParam {
  int32_t a;
  int32_t b;
  vm::Opcode op;
  int32_t expected;
};

class ComparisonTableTest : public testing::TestWithParam<ComparisonParam> {};

TEST_P(ComparisonTableTest, EvaluatesCorrectBooleanResult) {
  const auto& [lhs, rhs, op, expected] = GetParam();
  auto code = vm::test::BytecodeBuilder().push(lhs).push(rhs).write_opcode(op).halt().build();

  vm::VirtualMachine virt;
  virt.load_bytecode(code);
  virt.run();

  ASSERT_EQ(virt.stack().size(), 1);
  ASSERT_EQ(virt.stack().peek(), expected);
}

INSTANTIATE_TEST_SUITE_P(
    ComparisonOperations, ComparisonTableTest,
    testing::Values(
        // EQ
        ComparisonParam{.a = 5, .b = 5, .op = vm::Opcode::EQ, .expected = 1},
        ComparisonParam{.a = 5, .b = 3, .op = vm::Opcode::EQ, .expected = 0},
        ComparisonParam{.a = -5, .b = -5, .op = vm::Opcode::EQ, .expected = 1},
        ComparisonParam{.a = 0, .b = 0, .op = vm::Opcode::EQ, .expected = 1},
        ComparisonParam{.a = 0, .b = 1, .op = vm::Opcode::EQ, .expected = 0},
        ComparisonParam{.a = std::numeric_limits<int32_t>::min(),
                        .b = std::numeric_limits<int32_t>::min(),
                        .op = vm::Opcode::EQ,
                        .expected = 1},
        ComparisonParam{.a = std::numeric_limits<int32_t>::max(),
                        .b = std::numeric_limits<int32_t>::max(),
                        .op = vm::Opcode::EQ,
                        .expected = 1},
        ComparisonParam{.a = std::numeric_limits<int32_t>::min(),
                        .b = std::numeric_limits<int32_t>::max(),
                        .op = vm::Opcode::EQ,
                        .expected = 0},

        // LT
        ComparisonParam{.a = 2, .b = 5, .op = vm::Opcode::LT, .expected = 1},
        ComparisonParam{.a = 5, .b = 2, .op = vm::Opcode::LT, .expected = 0},
        ComparisonParam{.a = 5, .b = 5, .op = vm::Opcode::LT, .expected = 0},
        ComparisonParam{.a = -5, .b = 2, .op = vm::Opcode::LT, .expected = 1},
        ComparisonParam{.a = 2, .b = -5, .op = vm::Opcode::LT, .expected = 0},
        ComparisonParam{.a = -10, .b = -5, .op = vm::Opcode::LT, .expected = 1},
        ComparisonParam{.a = -5, .b = -10, .op = vm::Opcode::LT, .expected = 0},
        ComparisonParam{.a = 0, .b = 1, .op = vm::Opcode::LT, .expected = 1},
        ComparisonParam{.a = -1, .b = 0, .op = vm::Opcode::LT, .expected = 1},
        ComparisonParam{.a = std::numeric_limits<int32_t>::min(),
                        .b = std::numeric_limits<int32_t>::max(),
                        .op = vm::Opcode::LT,
                        .expected = 1},
        ComparisonParam{.a = std::numeric_limits<int32_t>::max(),
                        .b = std::numeric_limits<int32_t>::min(),
                        .op = vm::Opcode::LT,
                        .expected = 0},

        // GT
        ComparisonParam{.a = 5, .b = 2, .op = vm::Opcode::GT, .expected = 1},
        ComparisonParam{.a = 2, .b = 5, .op = vm::Opcode::GT, .expected = 0},
        ComparisonParam{.a = 5, .b = 5, .op = vm::Opcode::GT, .expected = 0},
        ComparisonParam{.a = 2, .b = -5, .op = vm::Opcode::GT, .expected = 1},
        ComparisonParam{.a = -5, .b = 2, .op = vm::Opcode::GT, .expected = 0},
        ComparisonParam{.a = -5, .b = -10, .op = vm::Opcode::GT, .expected = 1},
        ComparisonParam{.a = -10, .b = -5, .op = vm::Opcode::GT, .expected = 0},
        ComparisonParam{.a = 1, .b = 0, .op = vm::Opcode::GT, .expected = 1},
        ComparisonParam{.a = 0, .b = -1, .op = vm::Opcode::GT, .expected = 1},
        ComparisonParam{.a = std::numeric_limits<int32_t>::max(),
                        .b = std::numeric_limits<int32_t>::min(),
                        .op = vm::Opcode::GT,
                        .expected = 1},
        ComparisonParam{.a = std::numeric_limits<int32_t>::min(),
                        .b = std::numeric_limits<int32_t>::max(),
                        .op = vm::Opcode::GT,
                        .expected = 0}));

TEST(ControlFlowTest, Jump) {
  auto code = vm::test::BytecodeBuilder().jmp(10).push(999).push(1).halt().build();

  vm::VirtualMachine virt;
  virt.load_bytecode(code);
  virt.run();

  ASSERT_EQ(virt.stack().size(), 1);
  ASSERT_EQ(virt.stack().peek(), 1);
}

TEST(ControlFlowTest, JumpZero) {
  auto code = vm::test::BytecodeBuilder().push(0).jz(15).push(999).push(1).halt().build();

  vm::VirtualMachine virt;
  virt.load_bytecode(code);
  virt.run();

  ASSERT_EQ(virt.stack().size(), 1);
  ASSERT_EQ(virt.stack().peek(), 1);
}

TEST(ControlFlowTest, JumpZeroNotTaken) {
  auto code = vm::test::BytecodeBuilder().push(2).jz(15).push(999).push(1).halt().build();

  vm::VirtualMachine virt;
  virt.load_bytecode(code);
  virt.run();

  ASSERT_EQ(virt.stack().size(), 2);
  ASSERT_EQ(virt.stack().peek(), 1);
}

TEST(ControlFlowTest, JumpZeroUnderflow) {
  auto code = vm::test::BytecodeBuilder().jz(5).halt().build();

  vm::VirtualMachine virt;
  virt.load_bytecode(code);
  expect_vm_runtime_error([&] { virt.run(); }, vm::ErrorCode::StackUnderflow, 0, vm::Opcode::JZ);
}

TEST(ControlFlowTest, JumpOutOfBounds) {
  auto code = vm::test::BytecodeBuilder().jmp(999).push(1).halt().build();

  vm::VirtualMachine virt;
  virt.load_bytecode(code);
  expect_vm_runtime_error([&] { virt.run(); }, vm::ErrorCode::JumpOutOfBounds, 0, vm::Opcode::JMP);
}

TEST(MemoryOperationsTest, StoreLoad) {
  auto code = vm::test::BytecodeBuilder().push(1).push(5).store().push(5).load().halt().build();

  vm::VirtualMachine virt;
  virt.load_bytecode(code);
  virt.run();
  ASSERT_EQ(virt.stack().peek(), 1);
  ASSERT_EQ(virt.memory().load(5), 1);
}

TEST(MemoryOperationsTest, MemoryOutOfBounds) {
  auto code = vm::test::BytecodeBuilder().push(1).push(32768).store().halt().build();

  vm::VirtualMachine virt({.memory_size = 1});
  virt.load_bytecode(code);
  expect_vm_runtime_error([&] { virt.run(); }, vm::ErrorCode::MemoryOutOfBounds, 10,
                          vm::Opcode::STORE);

  code = vm::test::BytecodeBuilder().push(1000).load().halt().build();
  virt.load_bytecode(code);
  expect_vm_runtime_error([&] { virt.run(); }, vm::ErrorCode::MemoryOutOfBounds, 5,
                          vm::Opcode::LOAD);
}

TEST(MemoryOperationsTest, NegativeAddressOutOfBounds) {
  auto code = vm::test::BytecodeBuilder().push(1).push(-1).store().halt().build();

  vm::VirtualMachine virt;
  virt.load_bytecode(code);
  expect_vm_runtime_error([&] { virt.run(); }, vm::ErrorCode::MemoryOutOfBounds, 10,
                          vm::Opcode::STORE);

  code = vm::test::BytecodeBuilder().push(-1).load().halt().build();
  virt.load_bytecode(code);
  expect_vm_runtime_error([&] { virt.run(); }, vm::ErrorCode::MemoryOutOfBounds, 5,
                          vm::Opcode::LOAD);
}

TEST(IntegrationLoopTest, LoopAccumulator) {
  auto code = vm::test::BytecodeBuilder()
                  .push(0)
                  .push(0)
                  .store()
                  .push(3)
                  .push(1)
                  .store()
                  .push(1)
                  .load()
                  .jz(75)
                  .push(1)
                  .load()
                  .push(0)
                  .load()
                  .add()
                  .push(0)
                  .store()
                  .push(1)
                  .load()
                  .push(1)
                  .sub()
                  .push(1)
                  .store()
                  .jmp(22)
                  .push(0)
                  .load()
                  .halt()
                  .build();

  vm::VirtualMachine virt;
  virt.load_bytecode(code);
  virt.run();

  ASSERT_TRUE(virt.is_halted());
  ASSERT_EQ(virt.memory().load(0), 6);
  ASSERT_EQ(virt.memory().load(1), 0);
  ASSERT_EQ(virt.stack().size(), 1);
  ASSERT_EQ(virt.stack().peek(), 6);
}