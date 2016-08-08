#include "dyscodatacolumn.h"
#include "aftimeblockencoder.h"
#include "rftimeblockencoder.h"
#include "rowtimeblockencoder.h"

namespace dyscostman {

void DyscoDataColumn::Prepare(bool fitToMaximum, DyscoDistribution distribution, DyscoNormalization normalization, double studentsTNu, double distributionTruncation)
{
	_fitToMaximum = fitToMaximum;
	_distribution = distribution;
	_studentsTNu = studentsTNu;
	_normalization = normalization;
	ThreadedDyscoColumn::Prepare(fitToMaximum, distribution, normalization, studentsTNu, distributionTruncation);
	const size_t nPolarizations = shape()[0], nChannels = shape()[1];
	
	switch(normalization) {
		case AFNormalization:
			_decoder.reset(new AFTimeBlockEncoder(nPolarizations, nChannels, _fitToMaximum));
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
			_gausEncoder.reset(new GausEncoder<float>(1 << getBitsPerSymbol(), 1.0, true));
			break;
		case UniformDistribution:
			_gausEncoder.reset(new GausEncoder<float>(1 << getBitsPerSymbol(), 1.0, false));
			break;
		case StudentsTDistribution:
			_gausEncoder.reset(new GausEncoder<float>(GausEncoder<float>::StudentTEncoder(1 << getBitsPerSymbol(), studentsTNu, 1.0)));
			break;
		case TruncatedGaussianDistribution:
			_gausEncoder.reset(new GausEncoder<float>(GausEncoder<float>::TruncatedGausEncoder(1 << getBitsPerSymbol(), distributionTruncation, 1.0)));
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
			encoder = new AFTimeBlockEncoder(nPolarizations, nChannels, _fitToMaximum);
			break;
		case RFNormalization:
			encoder = new RFTimeBlockEncoder(nPolarizations, nChannels);
			break;
		case RowNormalization:
			encoder = new RowTimeBlockEncoder(nPolarizations, nChannels);
			break;
	}
	*reinterpret_cast<ThreadData**>(threadData) = new ThreadData(encoder);
}

void DyscoDataColumn::destructEncodeThread(void* threadData)
{
	ThreadData* data = reinterpret_cast<ThreadData*>(threadData);
	delete data;
}

void DyscoDataColumn::encode(void* threadData, TimeBlockBuffer<data_t>* buffer, float* metaBuffer, symbol_t* symbolBuffer, size_t nAntennae)
{
	ThreadData& data = *reinterpret_cast<ThreadData*>(threadData);
	data.encoder->EncodeWithDithering(*_gausEncoder, *buffer, metaBuffer, symbolBuffer, nAntennae, _rnd);
}

size_t DyscoDataColumn::metaDataFloatCount(size_t nRows, size_t nPolarizations, size_t nChannels, size_t nAntennae) const
{
	return _decoder->MetaDataCount(nRows, nPolarizations, nChannels, nAntennae);
}

size_t DyscoDataColumn::symbolCount(size_t nRowsInBlock, size_t nPolarizations, size_t nChannels) const
{
	return _decoder->SymbolCount(nRowsInBlock, nPolarizations, nChannels);
}

} // end of namespace
