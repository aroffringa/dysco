#include <boost/test/unit_test.hpp>

#include <casacore/tables/Tables/ArrayColumn.h>
#include <casacore/tables/Tables/Table.h>
#include <casacore/tables/Tables/TableDesc.h>
#include <casacore/tables/Tables/SetupNewTab.h>
#include <casacore/tables/Tables/ArrColDesc.h>
#include <casacore/tables/Tables/ScaColDesc.h>

#include "../dyscostman.h"

using namespace casacore;
using namespace dyscostman;

BOOST_AUTO_TEST_SUITE(dyscostman)

casa::Record GetDyscoSpec()
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
	casacore::ScalarColumnDesc<int>
		ant1Desc("ANTENNA1"),
		ant2Desc("ANTENNA2"),
		fieldDesc("FIELD_ID"),
		dataDescIdDesc("DATA_DESC_ID");
	casacore::ScalarColumnDesc<double>
		timeDesc("TIME");
	tableDesc.addColumn(columnDesc);
	tableDesc.addColumn(ant1Desc);
	tableDesc.addColumn(ant2Desc);
  tableDesc.addColumn(fieldDesc);
  tableDesc.addColumn(dataDescIdDesc);
  tableDesc.addColumn(timeDesc);
	casacore::SetupNewTable setupNewTable("TestTable", tableDesc, casacore::Table::New);
  
	DataManagerCtor dyscoConstructor = DataManager::getCtor("DyscoStMan");
	std::unique_ptr<DataManager> dysco(dyscoConstructor("DATA_dm", GetDyscoSpec()));
  setupNewTable.bindColumn("DATA", *dysco);
	casacore::Table newTable(setupNewTable);
	
	const size_t nRow = 10;
	newTable.addRow(nRow);
	casacore::ScalarColumn<int>
		a1Col(newTable, "ANTENNA1"),
		a2Col(newTable, "ANTENNA2"),
		fieldCol(newTable, "FIELD_ID"),
		dataDescIdCol(newTable, "DATA_DESC_ID");
	for(size_t i=0; i!=nRow; ++i)
	{
		a1Col.put(i, i/2);
		a2Col.put(i, i%2);
		fieldCol.put(i, 0);
		dataDescIdCol.put(i, 0);
	}
	
	casacore::ArrayColumn<casacore::Complex> dataCol(newTable, "DATA");
	for(size_t i=0; i!=nRow; ++i)
	{
		casacore::Array<casacore::Complex> arr(shape);
		*arr.cbegin() = i;
		dataCol.put(i, arr);
	}
}

BOOST_AUTO_TEST_SUITE_END()
