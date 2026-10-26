#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace vm {

class EvaluationStack {
private:
  static constexpr size_t DEFAULT_MAX_SIZE = 16384;
  size_t max_size_;
  std::vector<int32_t> buffer_;

public:
  explicit EvaluationStack(size_t max_size = DEFAULT_MAX_SIZE) : max_size_(max_size) {
    buffer_.reserve(max_size);
  }

  void push(int32_t value);
  int32_t pop();
  [[nodiscard]] int32_t peek() const;
  void dup();
  void swap();
  [[nodiscard]] size_t size() const;
  [[nodiscard]] bool empty() const;
};

class LinearMemory {
private:
  static constexpr size_t DEFAULT_MEMORY_SIZE = 16384;
  std::vector<int32_t> buffer_;

public:
  explicit LinearMemory(size_t size = DEFAULT_MEMORY_SIZE) : buffer_(size, 0) {}

  [[nodiscard]] int32_t load(uint32_t address) const;
  void store(uint32_t address, int32_t value);
  [[nodiscard]] size_t size() const;
  void reset();
};

}  // namespace vm