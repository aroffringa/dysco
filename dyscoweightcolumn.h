#ifndef DYSCO_WEIGHT_COLUMN_H
#define DYSCO_WEIGHT_COLUMN_H

#include "threadeddyscocolumn.h"
#include "stochasticencoder.h"
#include "weightblockencoder.h"

namespace dyscostman {

class DyscoStMan;

/**
 * A column for storing compressed complex values with an approximate Gaussian distribution.
 * @author André Offringa
 */
class DyscoWeightColumn : public ThreadedDyscoColumn<float>
{
public:
	/**
	 * Create a new column. Internally called by DyscoStMan when creating a
	 * new column.
	 */
  DyscoWeightColumn(DyscoStMan* parent, int dtype) :
		ThreadedDyscoColumn(parent, dtype)
	{ }
  
	DyscoWeightColumn(const DyscoWeightColumn &source) = delete;
	
	void operator=(const DyscoWeightColumn &source) = delete;
	
  /** Destructor. */
  virtual ~DyscoWeightColumn() { shutdown(); }
	
	virtual void Prepare(DyscoDistribution distribution, DyscoNormalization normalization, double studentsTNu, double distributionTruncation) override;
	
protected:
	virtual void initializeDecode(TimeBlockBuffer<data_t>* buffer, const float* metaBuffer, size_t nRow, size_t nAntennae) final override;
	
	virtual void decode(TimeBlockBuffer<data_t>* buffer, const symbol_t* data, size_t blockRow, size_t a1, size_t a2) final override;
	
	virtual void initializeEncodeThread(void** threadData) final override
	{ }
	
	virtual void destructEncodeThread(void* threadData) final override
	{ }
	
	virtual void encode(void* threadData, TimeBlockBuffer<data_t>* buffer, float* metaBuffer, symbol_t* symbolBuffer, size_t nAntennae) final override;
	
	virtual size_t metaDataFloatCount(size_t nRows, size_t nPolarizations, size_t nChannels, size_t nAntennae) const final override
	{
		return _encoder->MetaDataFloatCount();
	}
	
	virtual size_t symbolCount(size_t nRowsInBlock, size_t nPolarizations, size_t nChannels) const final override
	{
		return _encoder->SymbolCount(nRowsInBlock);
	}
	
private:
	std::unique_ptr<WeightBlockEncoder> _encoder;
};

} // end of namespace

#endif
