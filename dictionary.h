#ifndef DYSCO_DICTIONARY_H_
#define DYSCO_DICTIONARY_H_

#include <cassert>
#include <cstddef>

#include "uvector.h"

namespace dyscostman {

class Dictionary {
 public:
  using value_t = float;
  using iterator = value_t*;
  using const_iterator = const value_t*;
  using symbol_t = unsigned;

  Dictionary() = default;

  explicit Dictionary(size_t size) : _values(size) {}

  void reserve(size_t size) { _values.reserve(size); }

  void resize(size_t size) { _values.resize(size); }

  /**
   * Returns an iterator pointing to the first element in the dictionary
   * that is not less than (i.e. greater or equal to) value. This method assumes
   * the dictionary is sorted.
   */
  inline const_iterator lower_bound(value_t val) const {
    return lower_bound_branchless_two_minimum(val);
  }

  /**
   * Like lower_bound_branchless, but with the loop unrolled once because
   * we know the dictionary's size is always at least of size 2.
   */
  const_iterator lower_bound_branchless_two_minimum(value_t val) const {
    assert(_values.size() >= 2);

    const value_t* base = _values.data();
    const size_t n = _values.size();

    // Largest power of two <= n
    size_t step = BitFloor(n);
    const value_t* it = base;

    // This is the loop below unrolled once
    const size_t idx = std::min(step, n - 1);
    const value_t* probe = it + idx;
    if (probe < base + n && *probe < val) it = probe;
    step >>= 1;

    while (step != 0) {
      const size_t idx = std::min(step, n - 1);
      const value_t* probe = it + idx;

      // Branchless: compiler should emit cmov
      if (probe < base + n && *probe < val) it = probe;

      step >>= 1;
    }

    // Final correction: ensure we return first >= val
    return (it < base + n && *it < val) ? it + 1 : it;
  }

  /**
   * Branchless version of lower_bound. Dictionary may not be empty.
   * This is currently (2026) the fastest implementation, and is
   * about two times faster than lower_bound_two_minimum.
   */
  const_iterator lower_bound_branchless(value_t val) const {
    assert(!empty());

    const value_t* base = _values.data();
    const size_t n = _values.size();

    // Largest power of two <= n
    size_t step = BitFloor(n);
    const value_t* it = base;

    while (step != 0) {
      const size_t idx = std::min(step, n - 1);
      const value_t* probe = it + idx;

      // Branchless: compiler should emit cmov
      if (probe < base + n && *probe < val) it = probe;

      step >>= 1;
    }

    // Final correction: ensure we return first >= val
    return (it < base + n && *it < val) ? it + 1 : it;
  }

  /**
   * This implementation is an evolution of @ref lower_bound_fast(), but
   * additionally assumes the dictionary has at least two elements, avoiding one
   * comparison.
   *
   * It might be interesting to try using Eytzinger's lay-out for further
   * improvement.
   */
  const_iterator lower_bound_two_minimum(value_t val) const {
    assert(_values.size() >= 2);
    size_t p = 0, q = _values.size();
    const size_t m = (p + q) / 2;
    if (_values[m] <= val)
      p = m;
    else
      q = m;
    while (p + 1 != q) {
      const size_t l = (p + q) / 2;
      if (_values[l] <= val)
        p = l;
      else
        q = l;
    }
    return (_values[p] < val) ? (&_values[q]) : (&_values[p]);
  }

  /**
   * Returns an iterator pointing to the first element in the dictionary
   * that is not less than (i.e. greater or equal to) value.
   *
   * This implementation turns out to be slightly faster than the
   * STL implementation. It performs 10.7 MB/s, vs. 9.0 MB/s for the
   * STL. 18% faster. Using "unsigned" instead of "size_t" is 5% slower.
   * (It's not a fair STL comparison, because this implementation
   * does not check for empty vector).
   */
  const_iterator lower_bound_one_minimum(value_t val) const {
    assert(!empty());
    size_t p = 0, q = _values.size();
    while (p + 1 != q) {
      size_t m = (p + q) / 2;
      if (_values[m] <= val)
        p = m;
      else
        q = m;
    }
    return (_values[p] < val) ? (&_values[q]) : (&_values[p]);
  }

  /**
   * Below is the first failed result of an attempt to beat the STL in
   * performance. It turns out to be 13% slower for larger dictionaries,
   * compared to the STL implementation that is used in the class
   * above. It performs 7.9 MB/s. 26% compared to the 'fastest' lower_bound.
   */
  const_iterator lower_bound_slow(value_t val) const {
    assert(!empty());
    const value_t *p = _values.data(), *q = p + _values.size();
    while (p + 1 != q) {
      // This is a bit inefficient, but (p + q)/2 was not allowed, because
      // operator+(ptr,ptr) is not allowed.
      const value_t* m = p + (q - p) / 2;
      if (*m <= val)
        p = m;
      else
        q = m;
    }
    return *p < val ? q : p;
  }

  symbol_t symbol(const_iterator iter) const {
    assert(iter > begin());
    return (iter - begin());
  }

  symbol_t largest_symbol() const {
    assert(!empty());
    return _values.size() - 1;
  }

  iterator begin() { return _values.data(); }
  const_iterator begin() const { return _values.data(); }
  iterator end() { return _values.data() + _values.size(); }
  const_iterator end() const { return _values.data() + _values.size(); }
  value_t value(const_iterator iter) const { return *iter; }
  value_t value(symbol_t sym) const { return _values[sym]; }
  bool empty() const { return _values.empty(); }
  size_t size() const { return _values.size(); }

  value_t largest_value() const { return _values.back(); }
  value_t smallest_value() const { return _values.front(); }

 private:
  static size_t BitFloor(size_t n) {
    // Calculate largest power of two <= n
    // Because the stand-alone version of Dysco is to add Dysco to old
    // versions of Casacore, and that version is not C++20 compatible,
    // C++20 is not enabled when building stand-alone but will be when
    // building it as part of Casacore.
#if __cplusplus >= 202002L // Do we have C++20 ?
    return std::bit_floor(n);
#else
    return static_cast<size_t>(1) << (63 - __builtin_clzll(n));
#endif
  }
  
  aocommon::UVector<value_t> _values;
};

}  // namespace dyscostman

#endif
