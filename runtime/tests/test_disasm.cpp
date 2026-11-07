#include <gtest/gtest.h>

#include <format>
#include <limits>

#include "test_helpers.hpp"
#include "vm/disasm.hpp"
#include "vm/errors.hpp"

TEST(DisasmHeaderValidationTest, BadMagicBytes) {
  std::vector<uint8_t> header = {
      'B', 'V', 'M', '2', 0x01, 0x00,
  };
  std::ostringstream oss;
  expect_vm_load_error([&] { vm::disassemble(header, oss); }, vm::ErrorCode::InvalidMagicBytes);
}

TEST(DisasmHeaderValidationTest, TruncatedHeader) {
  std::vector<uint8_t> header = {
      'B',
      'V',
  };
  std::ostringstream oss;
  expect_vm_load_error([&] { vm::disassemble(header, oss); }, vm::ErrorCode::HeaderTooShort);
}

TEST(DisasmHeaderValidationTest, WrongVersion) {
  std::vector<uint8_t> header = {
      'B', 'V', 'M', '1', 0x02, 0x00,
  };
  std::ostringstream oss;
  expect_vm_load_error([&] { vm::disassemble(header, oss); }, vm::ErrorCode::UnsupportedVersion);
}

TEST(DisasmHeaderValidationTest, ValidHeader) {
  std::vector<uint8_t> header = {
      'B', 'V', 'M', '1', 0x01, 0x00,
  };
  std::ostringstream oss;
  ASSERT_NO_THROW(vm::disassemble(header, oss));
  ASSERT_EQ(oss.view().size(), 0);
}

TEST(DisasmInvalidOpcodeTest, ThrowsOnUnknownOpcodeByte) {
  std::vector<uint8_t> code = {
      'B', 'V', 'M', '1', 0x01, 0x00, 0xFE,
  };
  std::ostringstream oss;
  expect_vm_runtime_error([&] { vm::disassemble(code, oss); }, vm::ErrorCode::InvalidOpcode, 0,
                          static_cast<vm::Opcode>(0xFE));
}

class DisasmTruncatedInstructionTest : public testing::TestWithParam<vm::Opcode> {};

TEST_P(DisasmTruncatedInstructionTest, ThrowsWhenOperandIsTruncated) {
  std::vector<uint8_t> code = {
      'B', 'V', 'M', '1', 0x01, 0x00, static_cast<uint8_t>(GetParam()), 0x42, 0x00,
  };
  std::ostringstream oss;
  expect_vm_runtime_error([&] { vm::disassemble(code, oss); }, vm::ErrorCode::TruncatedInstruction,
                          0, GetParam());
}

INSTANTIATE_TEST_SUITE_P(UnaryOperations, DisasmTruncatedInstructionTest,
                         testing::Values(vm::Opcode::PUSH, vm::Opcode::JMP, vm::Opcode::JZ));

class DisasmNullaryTest : public testing::TestWithParam<vm::Opcode> {};

TEST_P(DisasmNullaryTest, DecodesMnemonicCorrectly) {
  auto code = vm::test::BytecodeBuilder().write_opcode(GetParam()).build();
  std::ostringstream oss;
  vm::disassemble(code, oss);

  ASSERT_EQ(oss.view(), std::format("0x00000000: {}\n", vm::to_string(GetParam())));
}

INSTANTIATE_TEST_SUITE_P(NullaryOperations, DisasmNullaryTest,
                         testing::Values(vm::Opcode::HALT, vm::Opcode::POP, vm::Opcode::DUP,
                                         vm::Opcode::SWAP, vm::Opcode::ADD, vm::Opcode::SUB,
                                         vm::Opcode::MUL, vm::Opcode::DIV, vm::Opcode::MOD,
                                         vm::Opcode::EQ, vm::Opcode::LT, vm::Opcode::GT,
                                         vm::Opcode::PRINT, vm::Opcode::READ, vm::Opcode::PUTC,
                                         vm::Opcode::LOAD, vm::Opcode::STORE));

class DisasmJumpTest : public testing::TestWithParam<vm::Opcode> {};

TEST_P(DisasmJumpTest, ZeroAddress) {
  auto code = vm::test::BytecodeBuilder().write_opcode(GetParam()).write_uint32_le(0).build();
  std::ostringstream oss;
  vm::disassemble(code, oss);

  ASSERT_EQ(oss.view(), std::format("0x00000000: {} 0x00000000\n", vm::to_string(GetParam())));
}

TEST_P(DisasmJumpTest, ArbitraryAddress) {
  auto code = vm::test::BytecodeBuilder().write_opcode(GetParam()).write_uint32_le(42).build();
  std::ostringstream oss;
  vm::disassemble(code, oss);

  ASSERT_EQ(oss.view(), std::format("0x00000000: {} 0x0000002A\n", vm::to_string(GetParam())));
}

TEST_P(DisasmJumpTest, MaxUint32Address) {
  auto code = vm::test::BytecodeBuilder()
                  .write_opcode(GetParam())
                  .write_uint32_le(std::numeric_limits<uint32_t>::max())
                  .build();
  std::ostringstream oss;
  vm::disassemble(code, oss);

  ASSERT_EQ(oss.view(), std::format("0x00000000: {} 0xFFFFFFFF\n", vm::to_string(GetParam())));
}

INSTANTIATE_TEST_SUITE_P(JumpOperations, DisasmJumpTest,
                         testing::Values(vm::Opcode::JMP, vm::Opcode::JZ));

TEST(DisasmPushTest, PositiveValues) {
  auto code = vm::test::BytecodeBuilder().push(42).build();
  std::ostringstream oss;
  vm::disassemble(code, oss);

  ASSERT_EQ(oss.view(), "0x00000000: PUSH 42\n");
}

TEST(DisasmPushTest, NegativeValues) {
  auto code = vm::test::BytecodeBuilder().push(-42).build();
  std::ostringstream oss;
  vm::disassemble(code, oss);

  ASSERT_EQ(oss.view(), "0x00000000: PUSH -42\n");
}

TEST(DisasmPushTest, ZeroValues) {
  auto code = vm::test::BytecodeBuilder().push(0).build();
  std::ostringstream oss;
  vm::disassemble(code, oss);

  ASSERT_EQ(oss.view(), "0x00000000: PUSH 0\n");
}

TEST(DisasmPushTest, BoundaryValues) {
  auto code = vm::test::BytecodeBuilder()
                  .push(std::numeric_limits<int32_t>::max())
                  .push(std::numeric_limits<int32_t>::min())
                  .build();
  std::ostringstream oss;
  vm::disassemble(code, oss);

  ASSERT_EQ(oss.view(), "0x00000000: PUSH 2147483647\n0x00000005: PUSH -2147483648\n");
}

TEST(DisasmSequenceTest, AccurateOffsets) {
  auto code = vm::test::BytecodeBuilder().read().push(10).store().jmp(14).halt().build();
  std::ostringstream oss;
  vm::disassemble(code, oss);

  constexpr std::string_view expected =
      R"(0x00000000: READ
0x00000001: PUSH 10
0x00000006: STORE
0x00000007: JMP 0x0000000E
0x0000000C: HALT
)";
  ASSERT_EQ(oss.view(), expected);
}
