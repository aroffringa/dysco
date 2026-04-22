#ifndef DYSCO_RFTIME_BLOCK_ENCODER_H
#define DYSCO_RFTIME_BLOCK_ENCODER_H

#include "stochasticencoder.h"
#include "timeblockbuffer.h"
#include "uvector.h"

#include <complex>
#include <random>
#include <vector>

#include "timeblockencoder.h"

class RFTimeBlockEncoder : public TimeBlockEncoder {
 public:
  RFTimeBlockEncoder(size_t nPol, size_t nChannels);

  virtual ~RFTimeBlockEncoder() override;

  virtual void EncodeWithDithering(
      const dyscostman::StochasticEncoder<float> &gausEncoder, FBuffer &buffer,
      float *metaBuffer, symbol_t *symbolBuffer, size_t antennaCount,
      std::mt19937 &rnd) final override {
    encode<true>(gausEncoder, buffer, metaBuffer, symbolBuffer, antennaCount,
                 &rnd);
  }

  virtual void EncodeWithoutDithering(
      const dyscostman::StochasticEncoder<float> &gausEncoder, FBuffer &buffer,
      float *metaBuffer, symbol_t *symbolBuffer,
      size_t antennaCount) final override {
    encode<false>(gausEncoder, buffer, metaBuffer, symbolBuffer, antennaCount,
                  0);
  }

  virtual void InitializeDecode(const float *metaBuffer, size_t nRow,
                                size_t nAntennae) final override;

  virtual void Decode(const dyscostman::StochasticEncoder<float> &gausEncoder,
                      FBuffer &buffer, const symbol_t *symbolBuffer,
                      size_t blockRow, size_t antenna1,
                      size_t antenna2) final override;

  virtual size_t SymbolCount(size_t nRow, size_t nPol,
                             size_t nChannels) const final override {
    return nRow * nChannels * nPol * 2 /*complex*/;
  }

  virtual size_t SymbolCount(size_t nRow) const final override {
    return nRow * _nChannels * _nPol * 2 /*complex*/;
  }

  virtual size_t SymbolsPerRow() const final override {
    return _nChannels * _nPol * 2 /*complex*/;
  }

  virtual size_t MetaDataCount(size_t nRow, size_t nPol, size_t nChannels,
                               size_t /*nAntennae*/) const final override {
    return nPol * (nChannels + nRow);
  }

  /**
   * Performs the RF normalization on the data such that no value is higher than
   * the @p max_level, while trying to maximize the values as much as possible.
   *
   * The size of @p meta_buffer should be at least @ref MetaDataCount().
   */
  void Normalize(TimeBlockBuffer<std::complex<float>> &buffer,
                 size_t antennaCount, double max_level);

 private:
  /**
   * Scales the rows in the data such that every row has a value with the
   * maximum level (unless all values are 0). Polarizations within one row are
   * scaled independently, i.e. every polarization is independently maximized.
   * This function is normally called for a timeblock of data, but the data
   * array does not need to be a timeblock.
   */
  void maximizeRows(std::vector<DBufferRow> &data, float *metaBuffer,
                    double maxLevel) const;
  /**
   * Scales the channels such that every channel has a value with the maximum
   * level (unless all values are 0). Each channel consists of values with the
   * same polarizations, i.e. polarizations are scaled independently. Like @ref
   * maximizeRows() this function is normally called for one timeblock, but this
   * is not required.
   */
  void maximizeChannels(std::vector<DBufferRow> &data, float *metaBuffer,
                        double maxLevel) const;

  template <bool UseDithering>
  void encode(const dyscostman::StochasticEncoder<float> &gausEncoder,
              const FBuffer &buffer, float *metaBuffer, symbol_t *symbolBuffer,
              size_t antennaCount, std::mt19937 *rnd);

  size_t _nPol, _nChannels;

  aocommon::UVector<double> _channelFactors, _rowFactors;
  std::uniform_int_distribution<unsigned> _ditherDist;
};

#endif
