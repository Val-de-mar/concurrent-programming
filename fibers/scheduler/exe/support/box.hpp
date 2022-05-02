#pragma once

#include <cstdlib>
#include <memory>
#include <twist/stdlike/atomic.hpp>

template <typename T>
class Box {
  using ATrait = std::allocator_traits<std::allocator<T>>;

 public:
  struct Iterator {
   private:
    friend class Box<T>;
    T* ptr_;

    explicit Iterator(T* ptr) : ptr_(ptr) {
    }

   public:
    Iterator(const Iterator& other) : ptr_(other.ptr) {
    }

    Iterator& operator++() {
      ++ptr_;
      return *this;
    }
    Iterator& operator--() {
      --ptr_;
      return *this;
    }
    T& operator*() {
      return *ptr_;
    }
    T* operator->() {
      return ptr_;
    }
    bool operator==(const Iterator& other) const {
      return ptr_ == other.ptr_;
    }
  };

  explicit Box(size_t sz) : size_(sz) {
    data_.store(allocator_.allocate(sz));
  }

  [[nodiscard]] size_t Size() const {
    return size_.load();
  }

  ~Box() {
    for (size_t i = 0; i < filled_; ++i) {
      ATrait::destroy(allocator_, data_ + i);
    }
    ATrait::deallocate(allocator_, data_, size_);
  }

  template <typename... Types>
  void EmplaceBack(Types&&... args) {
    ATrait::construct(allocator_,
                      data_.load(std::memory_order_relaxed) + filled_,
                      std::forward<Types>(args)...);
    ++filled_;
  }

  T& Back() {
    return data_[filled_ - 1];
  }

  Iterator begin() {  // NOLINT
    return Iterator{data_};
  }
  Iterator end() {  // NOLINT
    return Iterator{data_.load(std::memory_order_relaxed) + size_};
  }

 private:
  twist::stdlike::atomic<T*> data_;
  std::allocator<T> allocator_;
  const twist::stdlike::atomic<size_t> size_;
  size_t filled_{0};
};
