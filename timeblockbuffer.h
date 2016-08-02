#ifndef TIME_BLOCK_BUFFER_H
#define TIME_BLOCK_BUFFER_H

#include "uvector.h"

#include <complex>
#include <vector>

template<typename data_t>
class TimeBlockBuffer
{
public:
	typedef unsigned symbol_t;
	
	TimeBlockBuffer(size_t nPol, size_t nChannels) :
		_nPol(nPol), _nChannels(nChannels)
	{ }
	
	bool Empty() const { return _data.empty(); }
	
	void resize(size_t nRows) { _data.resize(nRows); }
	
	struct DataRow
	{
		size_t antenna1, antenna2;
		std::vector<data_t> visibilities;
	};
	
	DataRow& operator[](size_t rowIndex) { return _data[rowIndex]; }
	
	void ResetData()
	{
		_data.clear();
	}
	
	void SetData(size_t blockRow, size_t antenna1, size_t antenna2, const data_t* data)
	{
		if(_data.size() <= blockRow)
			_data.resize(blockRow+1);
		DataRow& newRow = _data[blockRow];
		newRow.antenna1 = antenna1;
		newRow.antenna2 = antenna2;
		newRow.visibilities.resize(_nPol * _nChannels);
		for(size_t i=0; i!=_nPol*_nChannels; ++i)
			newRow.visibilities[i] = data[i];
	}
	
	void GetData(size_t blockRow, data_t* destination) const
	{
		const data_t* srcPtr = _data[blockRow].visibilities.data();
		memcpy(destination, srcPtr, sizeof(data_t) * _data[blockRow].visibilities.size());
	}
	
	size_t NRows() const { return _data.size(); }
	
	size_t MaxAntennaIndex() const
	{
		size_t maxAntennaIndex = 0;
		for(const DataRow& row : _data)
		{
			maxAntennaIndex = std::max(maxAntennaIndex, std::max(row.antenna1, row.antenna2));
		}
		return maxAntennaIndex;
	}
	
	const std::vector<DataRow>& GetVector() const { return _data; }
	std::vector<DataRow>& GetVector() { return _data; }
	
private:
	size_t _nPol, _nChannels;
	std::vector<DataRow> _data;
};

template class TimeBlockBuffer<std::complex<float>>;

#endif
