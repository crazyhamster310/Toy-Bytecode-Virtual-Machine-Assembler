#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace vm {

class EvaluationStack {
 private:
  static constexpr size_t DEFAULT_MAX_SIZE = 16384;
  size_t max_size_;
  std::vector<int32_t> buffer_;
  size_t sp_{0};

 public:
  explicit EvaluationStack(size_t max_size = DEFAULT_MAX_SIZE)
      : max_size_(max_size), buffer_(max_size_) {}

  void push(int32_t value) {
    assert(sp_ < max_size_);
    buffer_[sp_++] = value;
  }

  int32_t pop() {
    assert(sp_ > 0);
    return buffer_[--sp_];
  }

  [[nodiscard]] int32_t peek() const {
    assert(sp_ > 0);
    return buffer_[sp_ - 1];
  }

  void dup() {
    assert(sp_ > 0 && sp_ < max_size_);
    buffer_[sp_] = buffer_[sp_ - 1];
    ++sp_;
  }

  void swap() {
    assert(sp_ >= 2);
    std::swap(buffer_[sp_ - 1], buffer_[sp_ - 2]);
  }

  [[nodiscard]] size_t size() const { return sp_; }

  [[nodiscard]] bool empty() const { return sp_ == 0; }

  [[nodiscard]] size_t capacity() const { return max_size_; }

  [[nodiscard]] std::span<const int32_t> data() const { return {buffer_.data(), sp_}; }

  void reset() { sp_ = 0; }
};

class LinearMemory {
 private:
  static constexpr size_t DEFAULT_MEMORY_SIZE = 16384;
  std::vector<int32_t> buffer_;

 public:
  explicit LinearMemory(size_t size = DEFAULT_MEMORY_SIZE) : buffer_(size, 0) {}

  [[nodiscard]] int32_t load(uint32_t address) const {
    assert(static_cast<size_t>(address) < buffer_.size());
    return buffer_[address];
  }

  void store(uint32_t address, int32_t value) {
    assert(static_cast<size_t>(address) < buffer_.size());
    buffer_[address] = value;
  }

  [[nodiscard]] size_t size() const { return buffer_.size(); }

  void reset() { std::ranges::fill(buffer_, 0); }
};

}  // namespace vm
