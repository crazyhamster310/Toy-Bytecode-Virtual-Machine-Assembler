#include <gtest/gtest.h>

#include "vm/memory.hpp"

TEST(StackTest, PushPop) {
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

TEST(StackTest, Dup) {
  vm::EvaluationStack stack;
  stack.push(1);

  stack.dup();
  ASSERT_EQ(stack.size(), 2);
  ASSERT_EQ(stack.peek(), 1);

  stack.pop();
  ASSERT_EQ(stack.peek(), 1);
}

TEST(StackTest, Swap) {
  vm::EvaluationStack stack;
  stack.push(1);
  stack.push(2);

  stack.swap();
  ASSERT_EQ(stack.size(), 2);
  ASSERT_EQ(stack.peek(), 1);

  stack.pop();
  ASSERT_EQ(stack.peek(), 2);
}

TEST(StackTest, Data) {
  vm::EvaluationStack stack;
  stack.push(1);
  stack.push(2);

  ASSERT_EQ(stack.data().size(), 2);
  ASSERT_EQ(stack.data()[0], 1);
  ASSERT_EQ(stack.data()[1], 2);

  stack.pop();
  ASSERT_EQ(stack.data().size(), 1);
  ASSERT_EQ(stack.data()[0], 1);
}

TEST(StackTest, Reset) {
  vm::EvaluationStack stack;
  stack.push(1);
  stack.push(2);
  stack.reset();

  ASSERT_TRUE(stack.empty());
}
