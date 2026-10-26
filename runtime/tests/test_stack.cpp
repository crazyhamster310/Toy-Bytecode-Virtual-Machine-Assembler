#include <gtest/gtest.h>

#include "vm/errors.hpp"
#include "vm/memory.hpp"

TEST(StackTest, TestPushPop) {
  vm::EvaluationStack stack;
  ASSERT_EQ(stack.size(), 0);
  ASSERT_TRUE(stack.empty());

  stack.push(1);
  stack.push(2);

  ASSERT_EQ(stack.size(), 2);
  ASSERT_FALSE(stack.empty());
  ASSERT_EQ(stack.peek(), 2);

  stack.pop();
  ASSERT_EQ(stack.size(), 1);
  ASSERT_EQ(stack.peek(), 1);

  stack.pop();
  ASSERT_EQ(stack.size(), 0);
  ASSERT_TRUE(stack.empty());
}

TEST(StackTest, TestDup) {
  vm::EvaluationStack stack;
  stack.push(1);

  stack.dup();
  ASSERT_EQ(stack.size(), 2);
  ASSERT_EQ(stack.peek(), 1);

  stack.pop();
  ASSERT_EQ(stack.peek(), 1);
}

TEST(StackTest, TestSwap) {
  vm::EvaluationStack stack;
  stack.push(1);
  stack.push(2);

  stack.swap();
  ASSERT_EQ(stack.size(), 2);
  ASSERT_EQ(stack.peek(), 1);

  stack.pop();
  ASSERT_EQ(stack.peek(), 2);
}

TEST(StackTest, TestStackUnderflow) {
  vm::EvaluationStack stack;

  EXPECT_THROW(stack.pop(), vm::StackUnderflowException);
  EXPECT_THROW(static_cast<void>(stack.peek()), vm::StackUnderflowException);
  EXPECT_THROW(stack.dup(), vm::StackUnderflowException);
  EXPECT_THROW(stack.swap(), vm::StackUnderflowException);

  stack.push(1);
  EXPECT_THROW(stack.swap(), vm::StackUnderflowException);
}

TEST(StackTest, TestStackOverflow) {
  vm::EvaluationStack stack(1);
  stack.push(1);

  EXPECT_THROW(stack.push(1), vm::StackOverflowException);
  EXPECT_THROW(stack.dup(), vm::StackOverflowException);

  EXPECT_EQ(stack.size(), 1);
  EXPECT_EQ(stack.peek(), 1);
}
