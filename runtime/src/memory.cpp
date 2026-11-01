#include "vm/memory.hpp"

#include <algorithm>
#include <utility>

#include "vm/errors.hpp"

namespace vm {

void EvaluationStack::push(int32_t value) {
  if (size() >= max_size_) {
    throw StackOverflowException();
  }
  buffer_.push_back(value);
}

int32_t EvaluationStack::pop() {
  int32_t ret = peek();
  buffer_.pop_back();
  return ret;
}

int32_t EvaluationStack::peek() const {
  if (empty()) {
    throw StackUnderflowException();
  }
  return buffer_.back();
}

void EvaluationStack::dup() {
  if (size() >= max_size_) {
    throw StackOverflowException();
  }
  buffer_.push_back(peek());
}

void EvaluationStack::swap() {
  size_t buf_size = size();
  if (buf_size < 2) {
    throw StackUnderflowException();
  }
  std::swap(buffer_[buf_size - 1], buffer_[buf_size - 2]);
}

size_t EvaluationStack::size() const { return buffer_.size(); }

bool EvaluationStack::empty() const { return buffer_.empty(); }

void EvaluationStack::reset() { buffer_.clear(); }

int32_t LinearMemory::load(uint32_t address) const {
  if (static_cast<size_t>(address) >= size()) {
    throw MemoryOutOfBoundsException(static_cast<size_t>(address));
  }
  return buffer_[address];
}

void LinearMemory::store(uint32_t address, int32_t value) {
  if (static_cast<size_t>(address) >= size()) {
    throw MemoryOutOfBoundsException(static_cast<size_t>(address));
  }
  buffer_[address] = value;
}

size_t LinearMemory::size() const { return buffer_.size(); }

void LinearMemory::reset() { std::ranges::fill(buffer_, 0); }

}  // namespace vm
