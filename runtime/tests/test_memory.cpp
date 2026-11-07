#include <gtest/gtest.h>

#include <limits>

#include "vm/memory.hpp"

TEST(MemoryTest, NullInit) {
  vm::LinearMemory memory(8);
  for (size_t i = 0; i < memory.size(); ++i) {
    ASSERT_EQ(memory.load(static_cast<uint32_t>(i)), 0);
  }
}

TEST(MemoryTest, LoadStore) {
  vm::LinearMemory memory;
  memory.store(0, 1);
  ASSERT_EQ(memory.load(0), 1);
}

TEST(MemoryTest, NearEnd) {
  vm::LinearMemory memory(2);
  ASSERT_NO_THROW(memory.store(1, 1));
  ASSERT_NO_THROW(static_cast<void>(memory.load(1)));
}

TEST(MemoryTest, Reset) {
  vm::LinearMemory memory(8);
  memory.store(0, 1);
  memory.reset();
  for (size_t i = 0; i < memory.size(); ++i) {
    ASSERT_EQ(memory.load(static_cast<uint32_t>(i)), 0);
  }
}

TEST(MemoryTest, StoringSignedExtremes) {
  vm::LinearMemory memory;
  memory.store(0, std::numeric_limits<int32_t>::min());
  memory.store(1, std::numeric_limits<int32_t>::max());
  memory.store(2, -1);
  ASSERT_EQ(memory.load(0), std::numeric_limits<int32_t>::min());
  ASSERT_EQ(memory.load(1), std::numeric_limits<int32_t>::max());
  ASSERT_EQ(memory.load(2), -1);
}
