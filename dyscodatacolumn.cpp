#include "dyscodatacolumn.h"
#include "aftimeblockencoder.h"
#include "rftimeblockencoder.h"
#include "rowtimeblockencoder.h"

namespace dyscostman {

void DyscoDataColumn::Prepare(DyscoDistribution distribution, DyscoNormalization normalization, double studentsTNu, double distributionTruncation)
{
	_distribution = distribution;
	_studentsTNu = studentsTNu;
	_normalization = normalization;
	ThreadedDyscoColumn::Prepare(distribution, normalization, studentsTNu, distributionTruncation);
	const size_t nPolarizations = shape()[0], nChannels = shape()[1];
	
	switch(normalization) {
		case AFNormalization:
			_decoder.reset(new AFTimeBlockEncoder(nPolarizations, nChannels, true));
			break;
		case RFNormalization:
			_decoder.reset(new RFTimeBlockEncoder(nPolarizations, nChannels));
			break;
		case RowNormalization:
			_decoder.reset(new RowTimeBlockEncoder(nPolarizations, nChannels));
			break;
	}
	
	switch(distribution) {
		case GaussianDistribution:
			_gausEncoder.reset(new StochasticEncoder<float>(1 << getBitsPerSymbol(), 1.0, true));
			break;
		case UniformDistribution:
			_gausEncoder.reset(new StochasticEncoder<float>(1 << getBitsPerSymbol(), 1.0, false));
			break;
		case StudentsTDistribution:
			_gausEncoder.reset(new StochasticEncoder<float>(StochasticEncoder<float>::StudentTEncoder(1 << getBitsPerSymbol(), studentsTNu, 1.0)));
			break;
		case TruncatedGaussianDistribution:
			_gausEncoder.reset(new StochasticEncoder<float>(StochasticEncoder<float>::TruncatedGausEncoder(1 << getBitsPerSymbol(), distributionTruncation, 1.0)));
			break;
	}
}

void DyscoDataColumn::initializeDecode(TimeBlockBuffer<data_t>* buffer, const float* metaBuffer, size_t nRow, size_t nAntennae)
{
	_decoder->InitializeDecode(metaBuffer, nRow, nAntennae);
}

void DyscoDataColumn::decode(TimeBlockBuffer<data_t>* buffer, const unsigned int* data, size_t blockRow, size_t a1, size_t a2)
{
	_decoder->Decode(*_gausEncoder, *buffer, data, blockRow, a1, a2);
}

void DyscoDataColumn::initializeEncodeThread(void** threadData)
{
	const size_t nPolarizations = shape()[0], nChannels = shape()[1];
	TimeBlockEncoder* encoder = 0;
	switch(_normalization) {
		case AFNormalization:
			encoder = new AFTimeBlockEncoder(nPolarizations, nChannels, true);
			break;
		case RFNormalization:
			encoder = new RFTimeBlockEncoder(nPolarizations, nChannels);
			break;
		case RowNormalization:
			encoder = new RowTimeBlockEncoder(nPolarizations, nChannels);
			break;
	}
	ThreadData* newThreadData = new ThreadData(encoder);
	// Seed every thread from a random number
	if(_randomize)
		newThreadData->rnd.seed(_rnd());
	else
		std::cout << "Warning: New thread NOT seeded.\n";
	*reinterpret_cast<ThreadData**>(threadData) = newThreadData;
}

void DyscoDataColumn::destructEncodeThread(void* threadData)
{
	ThreadData* data = reinterpret_cast<ThreadData*>(threadData);
	delete data;
}

void DyscoDataColumn::encode(void* threadData, TimeBlockBuffer<data_t>* buffer, float* metaBuffer, symbol_t* symbolBuffer, size_t nAntennae)
{
	ThreadData& data = *reinterpret_cast<ThreadData*>(threadData);
	data.encoder->EncodeWithDithering(*_gausEncoder, *buffer, metaBuffer, symbolBuffer, nAntennae, data.rnd);
}

size_t DyscoDataColumn::metaDataFloatCount(size_t nRows, size_t nPolarizations, size_t nChannels, size_t nAntennae) const
{
	return _decoder->MetaDataCount(nRows, nPolarizations, nChannels, nAntennae);
}

size_t DyscoDataColumn::symbolCount(size_t nRowsInBlock, size_t nPolarizations, size_t nChannels) const
{
	return _decoder->SymbolCount(nRowsInBlock, nPolarizations, nChannels);
}

size_t DyscoDataColumn::defaultThreadCount() const
{
	if(!_randomize)
	{
		std::cout << "Warning: using only one thread to avoid randomizing the results.\n";
		return 1;
	}
	else {
		return ThreadedDyscoColumn::defaultThreadCount();
	}
}

} // end of namespace
