#include <gtest/gtest.h>

#include "vm/errors.hpp"
#include "vm/memory.hpp"

TEST(MemoryTest, TestNullInit) {
  vm::LinearMemory memory(8);
  for (size_t i = 0; i < memory.size(); ++i) {
    ASSERT_EQ(memory.load(static_cast<uint32_t>(i)), 0);
  }
}

TEST(MemoryTest, TestLoadStore) {
  vm::LinearMemory memory;
  memory.store(0, 1);
  ASSERT_EQ(memory.load(0), 1);
}

TEST(MemoryTest, TestNearEnd) {
  vm::LinearMemory memory(2);
  ASSERT_NO_THROW(memory.store(1, 1));
  ASSERT_NO_THROW(static_cast<void>(memory.load(1)));
}

TEST(MemoryTest, TestOutOfBounds) {
  vm::LinearMemory memory(2);
  ASSERT_THROW(memory.store(2, 1), vm::MemoryOutOfBoundsException);
  ASSERT_THROW(static_cast<void>(memory.load(2)), vm::MemoryOutOfBoundsException);
}

TEST(MemoryTest, TestReset) {
  vm::LinearMemory memory(8);
  memory.store(0, 1);
  memory.reset();
  for (size_t i = 0; i < memory.size(); ++i) {
    ASSERT_EQ(memory.load(static_cast<uint32_t>(i)), 0);
  }
}