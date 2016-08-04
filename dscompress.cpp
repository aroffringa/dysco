#include "gausencoder.h"
#include "weightencoder.h"
#include "stmanmodifier.h"
#include "dyscostman.h"
#include "dyscodistribution.h"
#include "dysconormalization.h"
#include "stopwatch.h"

#include <casacore/ms/MeasurementSets/MeasurementSet.h>

#include <casacore/tables/Tables/ArrColDesc.h>
#include <casacore/tables/Tables/ArrayColumn.h>
#include <casacore/tables/Tables/ScalarColumn.h>

using namespace dyscostman;

template<typename T>
void createDyscoStManColumn(casacore::MeasurementSet& ms, const std::string& name, const casacore::IPosition& shape, unsigned bitsPerComplex, unsigned bitsPerWeight, bool fitToMaximum, DyscoNormalization normalization, DyscoDistribution distribution, double studentsTNu, double distributionTruncation)
{
	std::cout << "Constructing new column '" << name << "'...\n";
	casacore::ArrayColumnDesc<T> columnDesc(name, "", "DyscoStMan", "DyscoStMan", shape);
	columnDesc.setOptions(casacore::ColumnDesc::Direct | casacore::ColumnDesc::FixedShape);
	
	std::cout << "Querying storage manager...\n";
	bool isAlreadyUsed;
	try {
		ms.findDataManager("DyscoStMan");
		isAlreadyUsed = true;
	} catch(std::exception &e)
	{
		std::cout << "Constructing storage manager...\n";
		DyscoStMan dataManager(bitsPerComplex, bitsPerWeight);
		switch(distribution) {
			case GaussianDistribution:
				dataManager.SetGaussianDistribution(fitToMaximum);
				break;
			case UniformDistribution:
				dataManager.SetUniformDistribution();
				break;
			case StudentsTDistribution:
				dataManager.SetStudentsTDistribution(fitToMaximum, studentsTNu);
				break;
			case TruncatedGaussianDistribution:
				dataManager.SetTruncatedGaussianDistribution(fitToMaximum, distributionTruncation);
				break;
		}
		dataManager.SetNormalization(normalization);
		std::cout << "Adding column...\n";
		ms.addColumn(columnDesc, dataManager);
		isAlreadyUsed = false;
	}
	
	// We do not want to do this within the try-catch
	if(isAlreadyUsed)
	{
		std::cout << "Adding column with existing datamanager...\n";
		ms.addColumn(columnDesc, "DyscoStMan", false);
	}
}

/**
 * Compress a given measurement set.
 * @param argc Command line parameter count
 * @param argv Command line parameters.
 */
int main(int argc, char *argv[])
{
	register_dyscostman();
	
	if(argc < 2)
	{
		std::cout <<
			"Usage: compress [-rfnormalization / -afnormalization] [-bits-per-float <n>] [-reorder] [-column <name> [-column...]] <ms>\n"
			"Defaults: \n"
			"\tbits per data val = 8\n"
			"\tbits per weight = 6\n";
		return 0;
	}
	
	DyscoDistribution distribution = GaussianDistribution;
	DyscoNormalization normalization = RFNormalization;
	bool reorder = false;
	unsigned bitsPerFloat=8, bitsPerWeight=12;
	bool fitToMaximum = false;
	double distributionTruncation = 0.0;
	
	std::vector<std::string> columnNames;
	
	int argi = 1;
	while(argi < argc && argv[argi][0]=='-')
	{
		std::string p(argv[argi]+1);
		if(p == "bits-per-float")
		{
			++argi;
			bitsPerFloat = atoi(argv[argi]);
		}
		else if(p == "bits-per-weight")
		{
			++argi;
			bitsPerWeight = atoi(argv[argi]);
		}
		else if(p == "fit-to-maximum")
		{
			fitToMaximum = true;
		}
		else if(p == "reorder")
		{
			reorder = true;
		}
		else if(p == "uniform")
		{
			distribution = UniformDistribution;
		}
		else if(p == "studentt")
		{
			distribution = StudentsTDistribution;
		}
		else if(p == "truncgaus")
		{
			++argi;
			distribution = TruncatedGaussianDistribution;
			distributionTruncation = atof(argv[argi]);
		}
		else if(p == "column")
		{
			++argi;
			columnNames.push_back(argv[argi]);
		}
		else if(p == "rfnormalization")
		{
			normalization = RFNormalization;
		}
		else if(p == "afnormalization")
		{
			normalization = AFNormalization;
		}
		else throw std::runtime_error(std::string("Invalid parameter: ") + argv[argi]);
		++argi;
	}
	
	if(columnNames.empty())
		columnNames.push_back("DATA");
	std::string msPath = argv[argi];
	
	std::cout << "Opening ms...\n";
	std::unique_ptr<casacore::MeasurementSet> ms(new casacore::MeasurementSet(msPath, casacore::Table::Update));
	
	Stopwatch watch(true);
	std::cout << "Replacing flagged values by NaNs...\n";
	for(std::string columnName : columnNames)
	{
		if(columnName != "WEIGHT_SPECTRUM")
		{
			casacore::ArrayColumn<std::complex<float>> dataCol(*ms, columnName);
			casacore::ArrayColumn<bool> flagCol(*ms, casacore::MeasurementSet::columnName(casacore::MSMainEnums::FLAG));
			casacore::Array<std::complex<float>> dataArr(dataCol.shape(0));
			casacore::Array<bool> flagArr(flagCol.shape(0));
			for(size_t row=0; row!=ms->nrow(); ++row)
			{
				dataCol.get(row, dataArr);
				flagCol.get(row, flagArr);
				casacore::Array<bool>::contiter f=flagArr.cbegin();
				bool isChanged = false;
				for(casacore::Array<std::complex<float>>::contiter d=dataArr.cbegin(); d!=dataArr.cend(); ++d)
				{
					if(*f) {
						*d = std::complex<float>(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN());
						isChanged=true;
					}
					++f;
				}
				if(isChanged)
					dataCol.put(row, dataArr);
			}
		}
	}
	std::cout << "Time taken: " << watch.ToString() << '\n';
	watch.Reset();
	watch.Start();
	
	StManModifier modifier(*ms);
	
	casacore::IPosition shape;
	bool isDataReplaced = false;
	for(std::string columnName : columnNames)
	{
		bool replaced;
		if(columnName == "WEIGHT_SPECTRUM")
			replaced = modifier.PrepareReplacingColumn<float>(columnName, "DyscoStMan", bitsPerFloat, bitsPerWeight, shape);
		else
			replaced = modifier.PrepareReplacingColumn<casacore::Complex>(columnName, "DyscoStMan", bitsPerFloat, bitsPerWeight, shape);
		isDataReplaced = replaced || isDataReplaced;
	}
	
	if(isDataReplaced) {
		for(std::string columnName : columnNames)
		{
			if(columnName == "WEIGHT_SPECTRUM")
				createDyscoStManColumn<float>(*ms, columnName, shape, bitsPerFloat, bitsPerWeight, fitToMaximum, normalization, distribution, 1.0, distributionTruncation);
			else
				createDyscoStManColumn<casacore::Complex>(*ms, columnName, shape, bitsPerFloat, bitsPerWeight, fitToMaximum, normalization, distribution, 1.0, distributionTruncation);
		}
		for(std::string columnName : columnNames)
		{
			if(columnName == "WEIGHT_SPECTRUM")
				modifier.MoveColumnData<float>(columnName);
			else
				modifier.MoveColumnData<casacore::Complex>(columnName);
		}
		if(reorder)
		{
			StManModifier::Reorder(ms, msPath);
		}
	}
	
	std::cout << "Finished. Compression time: " << watch.ToString() << "\n";
	
	return 0;
}
