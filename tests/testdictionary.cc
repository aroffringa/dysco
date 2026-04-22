#include <boost/test/unit_test.hpp>

#include "../dictionary.h"

using namespace dyscostman;

namespace {
template<typename Function>
void TestLowerBound(Function lower_bound) {
  const std::array<float, 9> data{
    -10.0f, -9.0f, -7.0f, -0.1f, -0.0f,
    1.0f, 2.0f, 3.0f, 10.0f
  };
  Dictionary dict(9);
  std::copy(data.begin(), data.end(), dict.begin());

  Dictionary::const_iterator iter;

  iter = lower_bound(dict, std::numeric_limits<float>::lowest());
  BOOST_REQUIRE(iter != dict.end());
  BOOST_CHECK_EQUAL(dict.symbol(iter), 0u);

  iter = lower_bound(dict, -20.0f);
  BOOST_REQUIRE(iter != dict.end());
  BOOST_CHECK_EQUAL(dict.symbol(iter), 0u);

  iter = lower_bound(dict, -10.0f);
  BOOST_REQUIRE(iter != dict.end());
  BOOST_CHECK_EQUAL(dict.symbol(iter), 0u);

  iter = lower_bound(dict, -9.5f);
  BOOST_REQUIRE(iter != dict.end());
  BOOST_CHECK_EQUAL(dict.symbol(iter), 1u);

  iter = lower_bound(dict, -9.0f);
  BOOST_REQUIRE(iter != dict.end());
  BOOST_CHECK_EQUAL(dict.symbol(iter), 1u);

  iter = lower_bound(dict, -0.1f);
  BOOST_REQUIRE(iter != dict.end());
  BOOST_CHECK_EQUAL(dict.symbol(iter), 3u);

  iter = lower_bound(dict, -0.05f);
  BOOST_REQUIRE(iter != dict.end());
  BOOST_CHECK_EQUAL(dict.symbol(iter), 4u);

  iter = lower_bound(dict, 0.0f);
  BOOST_REQUIRE(iter != dict.end());
  BOOST_CHECK_EQUAL(dict.symbol(iter), 4u);

  iter = lower_bound(dict, 0.5f);
  BOOST_REQUIRE(iter != dict.end());
  BOOST_CHECK_EQUAL(dict.symbol(iter), 5u);

  iter = lower_bound(dict, 1.0f);
  BOOST_REQUIRE(iter != dict.end());
  BOOST_CHECK_EQUAL(dict.symbol(iter), 5u);

  iter = lower_bound(dict, 9.0f);
  BOOST_REQUIRE(iter != dict.end());
  BOOST_CHECK_EQUAL(dict.symbol(iter), 8u);

  iter = lower_bound(dict, 10.0f);
  BOOST_REQUIRE(iter != dict.end());
  BOOST_CHECK_EQUAL(dict.symbol(iter), 8u);

  iter = lower_bound(dict, 10.1f);
  BOOST_CHECK(iter == dict.end());

  iter = lower_bound(dict, std::numeric_limits<float>::max());
  BOOST_CHECK(iter == dict.end());
}

} // namespace

BOOST_AUTO_TEST_SUITE(dictionary)

BOOST_AUTO_TEST_CASE(fill) {
  Dictionary dict;
  BOOST_CHECK_EQUAL(dict.size(), 0);
  BOOST_CHECK(dict.begin() == dict.end());

  constexpr size_t n = 5;
  dict.resize(n);
  BOOST_CHECK_EQUAL(dict.size(), n);

  for(size_t i=0; i!=n; ++i)
    dict.begin()[i] = static_cast<float>(i*i);

  BOOST_CHECK_CLOSE_FRACTION(dict.largest_value(), (n-1)*(n-1), 1e-6);
  BOOST_CHECK_EQUAL(dict.largest_symbol(), n-1);
  BOOST_CHECK_EQUAL(dict.smallest_value(), 0);

  for(Dictionary::const_iterator iter = dict.begin(); iter != dict.end(); ++iter) {
    const Dictionary::symbol_t s = dict.symbol(iter);
    BOOST_CHECK_CLOSE_FRACTION(dict.value(s), static_cast<float>(s*s), 1e-6);
    BOOST_CHECK_CLOSE_FRACTION(dict.value(s), static_cast<float>(s*s), 1e-6);
  }
}

BOOST_AUTO_TEST_CASE(lower_bound) {
  TestLowerBound([](const Dictionary& d, float value) {
    return d.lower_bound(value);
  });
}

BOOST_AUTO_TEST_CASE(lower_bound_two_minimum) {
  TestLowerBound([](const Dictionary& d, float value) {
    return d.lower_bound_two_minimum(value);
  });
}

BOOST_AUTO_TEST_CASE(lower_bound_branchless) {
  TestLowerBound([](const Dictionary& d, float value) {
    return d.lower_bound_branchless(value);
  });
}

BOOST_AUTO_TEST_CASE(lower_bound_fast) {
  TestLowerBound([](const Dictionary& d, float value) {
    return d.lower_bound_one_minimum(value);
  });
}

BOOST_AUTO_TEST_CASE(lower_bound_slow) {
  TestLowerBound([](const Dictionary& d, float value) {
    return d.lower_bound_slow(value);
  });
}

BOOST_AUTO_TEST_SUITE_END()
