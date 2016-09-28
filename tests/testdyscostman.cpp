#include <boost/test/unit_test.hpp>

#include <casacore/tables/Tables/Table.h>
#include <casacore/tables/Tables/TableDesc.h>
#include <casacore/tables/Tables/SetupNewTab.h>
#include <casacore/tables/Tables/ArrColDesc.h>

#include "../dyscostman.h"

using namespace casacore;
using namespace dyscostman;

BOOST_AUTO_TEST_SUITE(dyscostman)

casa::Record GetDyscoSpec() const
{
	casa::Record dyscoSpec;
	dyscoSpec.define ("distribution", "TruncatedGaussian");
	dyscoSpec.define ("normalization", "AF");
	dyscoSpec.define ("distributionTruncation", 2.0);
	dyscoSpec.define ("dataBitCount", 10);
	dyscoSpec.define ("weightBitCount", 12);
	return dyscoSpec;
}

BOOST_AUTO_TEST_CASE( spec )
{
	DyscoStMan dysco(8, 12);
	Record spec = dysco.dataManagerSpec();
	BOOST_CHECK_EQUAL(spec.asInt("dataBitCount"), 8);
	BOOST_CHECK_EQUAL(spec.asInt("weightBitCount"), 12);
}

BOOST_AUTO_TEST_CASE( name )
{
	DyscoStMan dysco1(8, 12, "withparameters");
	BOOST_CHECK_EQUAL(dysco1.dataManagerType(), "DyscoStMan");
	BOOST_CHECK_EQUAL(dysco1.dataManagerName(), "withparameters");

	DyscoStMan dysco2("testname", GetDyscoSpec());
	BOOST_CHECK_EQUAL(dysco2.dataManagerType(), "DyscoStMan");
	BOOST_CHECK_EQUAL(dysco2.dataManagerName(), "testname");
	
	std::unique_ptr<DataManager> dysco3(dysco2.clone());
	BOOST_CHECK_EQUAL(dysco3->dataManagerType(), "DyscoStMan");
	BOOST_CHECK_EQUAL(dysco3->dataManagerName(), "testname");
	
	DataManagerCtor dyscoConstructor = DataManager::getCtor("DyscoStMan");
	std::unique_ptr<DataManager> dysco4(dyscoConstructor("Constructed", GetDyscoSpec()));
	BOOST_CHECK_EQUAL(dysco4->dataManagerName(), "Constructed");
}

BOOST_AUTO_TEST_CASE( makecolumn )
{
	DyscoStMan dysco(8, 12, "mydysco");
	dysco.createDirArrColumn("DATA", casacore::DataType::TpComplex, "");
	dysco.createDirArrColumn("WEIGHT_SPECTRUM", casacore::DataType::TpFloat, "");
	dysco.createDirArrColumn("CORRECTED_DATA", casacore::DataType::TpComplex, "");
	dysco.createDirArrColumn("ANYTHING", casacore::DataType::TpComplex, "");
	BOOST_CHECK(true);
}

BOOST_AUTO_TEST_CASE( maketable )
{
	casacore::TableDesc tableDesc;
	IPosition shape(2, 1, 1);
	casacore::ArrayColumnDesc<casacore::Complex> columnDesc("DATA", "", "DyscoStMan", "", shape);
	columnDesc.setOptions(casacore::ColumnDesc::Direct | casacore::ColumnDesc::FixedShape);
	tableDesc.addColumn(columnDesc);
	casacore::SetupNewTable setupNewTable("TestTable", tableDesc, casacore::Table::New);
	casacore::Table newTable(setupNewTable);
}

BOOST_AUTO_TEST_SUITE_END()