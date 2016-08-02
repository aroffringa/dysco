#include "rmscollector.h"

#include <casacore/tables/Tables/ArrayColumn.h>
#include <casacore/tables/Tables/ScalarColumn.h>

#include <cmath>
#include <vector>
#include <iostream>

using std::size_t;

namespace dyscostman {

RMSCollector::RMSCollector(size_t antennaCount, size_t fieldCount)
: _antennaCount(antennaCount), _fieldCount(fieldCount)
{
	const size_t mSize = matrixSize();
	_counts = new size_t[mSize];
	_squaredSum = new long double[mSize];
	for(size_t i=0; i!=mSize; ++i)
	{
		_counts[i] = 0;
		_squaredSum[i] = 0.0;
	}
}

RMSCollector::~RMSCollector()
{
	delete[] _squaredSum;
	delete[] _counts;
}

RMSTable RMSCollector::GetRMSTable()
{
	RMSTable table(_antennaCount, _fieldCount);
	size_t index = 0;
	for(size_t fieldId=0; fieldId!=_fieldCount; ++fieldId)
	{
		for(size_t antenna2=0; antenna2!=_antennaCount; ++antenna2)
		{
			for(size_t antenna1=0; antenna1!=_antennaCount; ++antenna1)
			{
				size_t count = _counts[index];
				long double squaredSum = _squaredSum[index];
				
				if(count != 0)
				{
					long double rms = sqrtl(squaredSum / (long double) count);
					table.Value(antenna1, antenna2, fieldId) = static_cast<RMSTable::rms_t>(rms);
				} else {
					table.Value(antenna1, antenna2, fieldId) = 0.0;
				}
				
				++index;
			}
		}
	}
	
	return table;
}

RMSTable RMSCollector::GetMeanTable()
{
	RMSTable table(_antennaCount, _fieldCount);
	size_t index = 0;
	for(size_t fieldId=0; fieldId!=_fieldCount; ++fieldId)
	{
		for(size_t antenna2=0; antenna2!=_antennaCount; ++antenna2)
		{
			for(size_t antenna1=0; antenna1!=_antennaCount; ++antenna1)
			{
				size_t count = _counts[index];
				long double sum = _squaredSum[index];
				
				if(count != 0)
				{
					long double mean = sum / (long double) count;
					table.Value(antenna1, antenna2, fieldId) = static_cast<RMSTable::rms_t>(mean);
				} else {
					table.Value(antenna1, antenna2, fieldId) = 0.0;
				}
				
				++index;
			}
		}
	}
	
	return table;
}

void RMSCollector::Summarize(bool rms)
{
	size_t count = _antennaCount;
	if(rms)
		std::cout << "RMS";
	else
		std::cout << "Mean";
	if(count > 8)
	{
		count = 8;
		std::cout << " of baselines for first " << count << " antennas of field 0:\n";
	} else {
		std::cout << " of baselines in field 0:\n";
	}
	
	RMSTable table;
	if(rms)
		table = GetRMSTable();
	else
		table = GetMeanTable();
	std::cout.precision(3);
	for(size_t a2=0; a2!=count; ++a2)
	{
		for(size_t a1=0; a1!=count; ++a1)
		{
			RMSTable::rms_t rms = table.Value(a1, a2, 0);
			std::cout << rms << '\t';
		}
		std::cout << '\n';
	}
}

void RMSCollector::countUnflaggedSamples(casacore::MeasurementSet& ms, std::vector<size_t> &unflaggedDataCounts)
{
	casacore::ROScalarColumn<int>
		ant1Col(ms, casacore::MeasurementSet::columnName(casacore::MSMainEnums::ANTENNA1)),
		ant2Col(ms, casacore::MeasurementSet::columnName(casacore::MSMainEnums::ANTENNA2)),
		fieldIdCol(ms, casacore::MeasurementSet::columnName(casacore::MSMainEnums::FIELD_ID));
	casacore::ROArrayColumn<bool>
		flagCol(ms, casacore::MeasurementSet::columnName(casacore::MSMainEnums::FLAG));
		
	casacore::IPosition shape(flagCol.shape(0));
	casacore::Array<bool> flagData(shape);
		
	// Count the number of unflagged samples per baseline
	for(size_t rowIndex = 0; rowIndex!=ms.nrow(); ++rowIndex)
	{
		const int
			antenna1 = ant1Col(rowIndex),
			antenna2 = ant2Col(rowIndex),
			fieldId = fieldIdCol(rowIndex);
		const size_t ind = index(antenna1, antenna2, fieldId);
		flagCol.get(rowIndex, flagData);
		for(casacore::Array<bool>::const_contiter i=flagData.cbegin(); i!=flagData.cend(); ++i)
		{
			if(*i) ++unflaggedDataCounts[ind];
		}
	}
}

void RMSCollector::CollectRMS(casacore::MeasurementSet& ms, const std::string &dataColumn)
{
	std::cout << "Counting flags in measurement set...\n";
	casacore::ROScalarColumn<int>
		ant1Col(ms, casacore::MeasurementSet::columnName(casacore::MSMainEnums::ANTENNA1)),
		ant2Col(ms, casacore::MeasurementSet::columnName(casacore::MSMainEnums::ANTENNA2)),
		fieldIdCol(ms, casacore::MeasurementSet::columnName(casacore::MSMainEnums::FIELD_ID));
	casacore::ROArrayColumn<bool>
		flagCol(ms, casacore::MeasurementSet::columnName(casacore::MSMainEnums::FLAG));
	casacore::ROArrayColumn<casacore::Complex>
		dataCol(ms, dataColumn);
	
	casacore::IPosition shape(flagCol.shape(0));
	casacore::Array<bool> flagData(shape);
	
	std::vector<size_t> unflaggedDataCounts(matrixSize());
	countUnflaggedSamples(ms, unflaggedDataCounts);
	
	// Calculate RMSes per baseline
	std::cout << "Calculating RMS per baseline...\n";
	casacore::Array<casacore::Complex> visData(shape);
	for(size_t rowIndex = 0; rowIndex!=ms.nrow(); ++rowIndex)
	{
		const int
			antenna1 = ant1Col(rowIndex),
			antenna2 = ant2Col(rowIndex),
			fieldId = fieldIdCol(rowIndex);
		const size_t ind = index(antenna1, antenna2, fieldId);
		flagCol.get(rowIndex, flagData);
		dataCol.get(rowIndex, visData);
		// If there's no unflagged data for this antenna, we would like to get a RMS for the *flagged* data instead.
		bool useFlags = unflaggedDataCounts[ind] != 0;
		if(useFlags)
		{
			casacore::Array<bool>::const_contiter flagPtr = flagData.cbegin();
			for(casacore::Array<casacore::Complex>::const_contiter dataPtr=visData.cbegin(); dataPtr!=visData.cend(); ++dataPtr)
			{
				if(!*flagPtr)
				{
					_counts[ind] += 2;
					_squaredSum[ind] += dataPtr->real()*dataPtr->real() + dataPtr->imag()*dataPtr->imag();
				}
				++flagPtr;
			}
		} else {
			_counts[ind] += 2*(visData.cend() - visData.cbegin());
			for(casacore::Array<casacore::Complex>::const_contiter dataPtr=visData.cbegin(); dataPtr!=visData.cend(); ++dataPtr)
			{
				_squaredSum[ind] += dataPtr->real()*dataPtr->real() + dataPtr->imag()*dataPtr->imag();
			}
		}
	}
}

template<bool forRMS>
void RMSCollector::collectDifference(casacore::MeasurementSet &lhs, casacore::MeasurementSet &rhs, const std::string &dataColumn)
{
	std::cout << "Counting flags in measurement set...\n";
	casacore::ROScalarColumn<int>
		ant1Col(lhs, casacore::MeasurementSet::columnName(casacore::MSMainEnums::ANTENNA1)),
		ant2Col(lhs, casacore::MeasurementSet::columnName(casacore::MSMainEnums::ANTENNA2)),
		fieldIdCol(lhs, casacore::MeasurementSet::columnName(casacore::MSMainEnums::FIELD_ID));
	casacore::ROArrayColumn<bool>
		flagCol(lhs, casacore::MeasurementSet::columnName(casacore::MSMainEnums::FLAG));
	casacore::ROArrayColumn<casacore::Complex>
		lhsDataCol(lhs, dataColumn),
		rhsDataCol(rhs, dataColumn);
	
	casacore::IPosition shape(flagCol.shape(0));
	casacore::Array<bool> flagData(shape);
	
	std::vector<size_t> unflaggedDataCounts(matrixSize());
	countUnflaggedSamples(lhs, unflaggedDataCounts);
	
	// Calculate RMSes per baseline
	std::cout << "Calculating RMS per baseline...\n";
	casacore::Array<casacore::Complex> lhsVisData(shape), rhsVisData(shape);
	for(size_t rowIndex = 0; rowIndex!=lhs.nrow(); ++rowIndex)
	{
		const int
			antenna1 = ant1Col(rowIndex),
			antenna2 = ant2Col(rowIndex),
			fieldId = fieldIdCol(rowIndex);
		const size_t ind = index(antenna1, antenna2, fieldId);
		flagCol.get(rowIndex, flagData);
		lhsDataCol.get(rowIndex, lhsVisData);
		rhsDataCol.get(rowIndex, rhsVisData);
		bool useFlags = unflaggedDataCounts[ind] != 0;
		casacore::Array<bool>::const_contiter flagPtr = flagData.cbegin();
		casacore::Array<casacore::Complex>::const_contiter rhsDataPtr=rhsVisData.cbegin();
		for(casacore::Array<casacore::Complex>::const_contiter lhsDataPtr=lhsVisData.cbegin(); lhsDataPtr!=lhsVisData.cend(); ++lhsDataPtr)
		{
			if(!*flagPtr || !useFlags)
			{
				_counts[ind] += 2;
				long double
					diffR = rhsDataPtr->real() - lhsDataPtr->real(),
					diffI = rhsDataPtr->imag() - lhsDataPtr->imag();
				if(forRMS)
				{
					_squaredSum[ind] += diffR*diffR + diffI*diffI;
				} else {
					_squaredSum[ind] += diffR + diffI;
				}
			}
			++flagPtr;
			++rhsDataPtr;
		}
	}
}

void RMSCollector::CollectDifferenceRMS(casacore::MeasurementSet& lhs, casacore::MeasurementSet& rhs, const string& dataColumn)
{
	collectDifference<true>(lhs, rhs, dataColumn);
}

void RMSCollector::CollectDifferenceMean(casacore::MeasurementSet& lhs, casacore::MeasurementSet& rhs, const string& dataColumn)
{
	collectDifference<false>(lhs, rhs, dataColumn);
}

} //end of namespace
