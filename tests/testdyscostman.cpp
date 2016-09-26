#include <boost/test/unit_test.hpp>

#include "../dyscostman.h"

using namespace casacore;
using namespace dyscostman;

BOOST_AUTO_TEST_SUITE(dyscostman)

BOOST_AUTO_TEST_CASE( spec )
{
  DyscoStMan dysco(8, 12);
  Record spec = dysco.dataManagerSpec();
  BOOST_CHECK_EQUAL(spec.asInt("dataBitCount"), 8);
  BOOST_CHECK_EQUAL(spec.asInt("weightBitCount"), 12);
}

BOOST_AUTO_TEST_SUITE_END()