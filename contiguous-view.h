#pragma once

#include <cassert>
#include <cstddef>
#include <iterator>
#include <type_traits>

inline constexpr size_t dynamic_extent = -1;

namespace details {
template <size_t Size>
struct storage {
  constexpr storage(size_t) {}

  size_t size() const {
    return Size;
  }
};

template <>
struct storage<dynamic_extent> {
  constexpr storage(size_t count) : _size(count) {}

  size_t size() const {
    return _size;
  }

private:
  size_t _size;
};
} // namespace details

template <typename T, size_t Extent = dynamic_extent>
class contiguous_view {
public:
  using value_type = std::remove_const_t<T>;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;

  using iterator = pointer;
  using const_iterator = const_pointer;

public:
  contiguous_view() noexcept : st(0), _data(nullptr) {}

  template <typename It>
  explicit(Extent != dynamic_extent) contiguous_view(It first, size_t count)
      : st(count),
        _data(std::to_address(first)) {
    if constexpr (Extent != dynamic_extent) {
      assert(count == Extent);
    }
  }

  template <typename It>
  explicit(Extent != dynamic_extent) contiguous_view(It first, It last)
      : contiguous_view(first, static_cast<size_t>(last - first)) {
    assert(first <= last);
  }

  contiguous_view(const contiguous_view& other) noexcept = default;

  template <typename U, size_t N>
  explicit(Extent != dynamic_extent && N == dynamic_extent) contiguous_view(
      const contiguous_view<U, N>& other,
      std::enable_if_t<Extent == dynamic_extent || N == dynamic_extent || Extent == N>* = nullptr) noexcept
      : contiguous_view(other.data(), other.size()) {}

  contiguous_view& operator=(const contiguous_view& other) noexcept = default;

  pointer data() const noexcept {
    return _data;
  }

  size_t size() const noexcept {
    return st.size();
  }

  size_t size_bytes() const noexcept {
    return sizeof(T) * size();
  }

  bool empty() const noexcept {
    return size() == 0;
  }

  iterator begin() const noexcept {
    return data();
  }

  const_iterator cbegin() const noexcept {
    return data();
  }

  iterator end() const noexcept {
    return begin() + size();
  }

  const_iterator cend() const noexcept {
    return begin() + size();
  }

  reference operator[](size_t idx) const {
    assert(idx < size());
    return *(data() + idx);
  }

  reference front() const {
    return *begin();
  }

  reference back() const {
    return *(end() - 1);
  }

  contiguous_view<T, dynamic_extent> subview(size_t offset, size_t count = dynamic_extent) const {
    assert(offset <= size() && (count == dynamic_extent || count <= size() - offset));
    return contiguous_view<T, dynamic_extent>(data() + offset, count == dynamic_extent ? size() - offset : count);
  }

  template <size_t Offset, size_t Count = dynamic_extent>
  auto subview() const {
    if constexpr (Extent != dynamic_extent) {
      static_assert(Offset <= Extent && (Count == dynamic_extent || Count <= Extent - Offset));
    } else {
      assert(Offset <= size() && (Count == dynamic_extent || Count <= size() - Offset));
    }

    if constexpr (Count == dynamic_extent) {
      if constexpr (Extent == dynamic_extent) {
        return contiguous_view<T, dynamic_extent>(data() + Offset, size() - Offset);
      } else {
        return contiguous_view<T, Extent - Offset>(data() + Offset, size() - Offset);
      }
    } else {
      return contiguous_view<T, Count>(data() + Offset, Count);
    }
  }

  template <size_t Count>
  contiguous_view<T, Count> first() const {
    if constexpr (Extent != dynamic_extent) {
      static_assert(Count <= Extent);
    } else {
      assert(Count <= size());
    }
    return contiguous_view<T, Count>(begin(), Count);
  }

  contiguous_view<T, dynamic_extent> first(size_t count) const {
    assert(count <= size());
    return contiguous_view<T, dynamic_extent>(begin(), count);
  }

  template <size_t Count>
  contiguous_view<T, Count> last() const {
    if constexpr (Extent != dynamic_extent) {
      static_assert(Count <= Extent);
    } else {
      assert(Count <= size());
    }
    return contiguous_view<T, Count>(end() - Count, Count);
  }

  contiguous_view<T, dynamic_extent> last(size_t count) const {
    assert(count <= size());
    return contiguous_view<T, dynamic_extent>(end() - count, count);
  }

private:
  using as_bytes_return_type = std::conditional_t<std::is_const_v<T>, const std::byte, std::byte>;
  static constexpr size_t as_bytes_return_size =
      (Extent == dynamic_extent ? dynamic_extent : sizeof(T) / sizeof(std::byte) * Extent);

public:
  auto as_bytes() const {
    return contiguous_view<as_bytes_return_type, as_bytes_return_size>(reinterpret_cast<as_bytes_return_type*>(data()),
                                                                       size_bytes());
  }

private:
  [[no_unique_address]] details::storage<Extent> st;

  T* _data;
};
