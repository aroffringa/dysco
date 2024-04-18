#include "rftimeblockencoder.h"
#include "stochasticencoder.h"

#include <random>

namespace {
void changeChannelFactor(std::vector<RFTimeBlockEncoder::DBufferRow> &data,
                                            float *metaBuffer, size_t visIndex,
                                            double factor) {
  metaBuffer[visIndex] /= factor;
  for (RFTimeBlockEncoder::DBufferRow &row : data) row.visibilities[visIndex] *= factor;
}

}

RFTimeBlockEncoder::RFTimeBlockEncoder(size_t nPol, size_t nChannels)
    : _nPol(nPol),
      _nChannels(nChannels),
      _channelFactors(_nChannels * nPol),
      _rowFactors(),
      _ditherDist(
          dyscostman::StochasticEncoder<double>::GetDitherDistribution()) {}

RFTimeBlockEncoder::~RFTimeBlockEncoder() = default;

void RFTimeBlockEncoder::changeRowFactor(std::vector<RFTimeBlockEncoder::DBufferRow> &data,
                                            float *metaBuffer, size_t row_index,
                                            double factor) {
  metaBuffer[_nPol * _nChannels + row_index] /= factor;
  RFTimeBlockEncoder::DBufferRow &row = data[row_index];
  for(std::complex<double> v : row.visibilities)
    v *= factor;
}

void RFTimeBlockEncoder::getBestChannelIncrease(const std::vector<DBufferRow> &data, const dyscostman::StochasticEncoder<float> &gausEncoder, size_t polIndex, double& bestChannelIncrease, double& channelFactor, size_t& bestChannel) {
  bestChannelIncrease = 0.0;
  channelFactor = 1.0;
  bestChannel = 0;
  // Find the channel factor that increasest the sum of absolute values the most
  for (size_t channel = 0; channel != _nChannels*_nPol; ++channel) {
    // By how much can we increase this channel?
    double largest_component = 0.0;
    for (const DBufferRow &row : data) {
      if (row.antenna1 != row.antenna2) {
        const std::complex<double> *ptr =
            &row.visibilities[channel];
        double local_max = std::max(std::max(ptr->real(), ptr->imag()),
                                    -std::min(ptr->real(), ptr->imag()));
        if (std::isfinite(local_max) && local_max > largest_component)
          largest_component = local_max;
      }
    }
    double factor = (largest_component == 0.0)
                        ? 0.0
                        : (gausEncoder.MaxQuantity() / largest_component - 1.0);
    // How much does this increase the total?
    double thisIncrease = 0.0;
    for (const DBufferRow &row : data) {
      if (row.antenna1 != row.antenna2) {
        std::complex<double> v =
            row.visibilities[channel * _nPol + polIndex] * double(factor);
        const double absoluteValue =
            std::fabs(v.real()) + std::fabs(v.imag());
        if (std::isfinite(absoluteValue)) thisIncrease += absoluteValue;
      }
    }
    if (thisIncrease > bestChannelIncrease) {
      bestChannelIncrease = thisIncrease;
      bestChannel = channel;
      channelFactor = factor + 1.0;
    }
  }
}

void RFTimeBlockEncoder::getBestRowIncrease(const std::vector<DBufferRow> &data, const dyscostman::StochasticEncoder<float> &gausEncoder, size_t polIndex, ao::uvector<double>& maxCompPerRow, ao::uvector<double>& increasePerRow, size_t& bestRow) {
  maxCompPerRow.assign(data.size(), 0.0);
  for (size_t row_index = 0; row_index!=data.size(); ++row_index) {
    const DBufferRow &row = data[row_index];
    for (size_t channel = 0; channel != _nChannels; ++channel) {
      const std::complex<double> *ptr =
          &row.visibilities[channel * _nPol + polIndex];
      double complMax = std::max(std::max(ptr->real(), ptr->imag()),
                                  -std::min(ptr->real(), ptr->imag()));
      if (std::isfinite(complMax) && complMax > maxCompPerRow[row_index])
        maxCompPerRow[row_index] = complMax;
    }
  }
  increasePerRow.assign(data.size(), 0.0);
  for (size_t row_index = 0; row_index!=data.size(); ++row_index) {
    const DBufferRow &row = data[row_index];
    double factor = (maxCompPerRow[row_index] == 0.0)
                          ? 0.0
                          : (gausEncoder.MaxQuantity() /
                                maxCompPerRow[row_index] -
                            1.0);
    for (size_t channel = 0; channel != _nChannels; ++channel) {
      std::complex<double> v =
          row.visibilities[channel * _nPol + polIndex] * factor;
      double av = std::fabs(v.real()) + std::fabs(v.imag());
      if (std::isfinite(av)) increasePerRow[row_index] += av;
    }
  }
  bestRow = 0;
  double bestRowIncrease = 0.0;
  for (size_t row_index = 0; row_index!=data.size(); ++row_index) {
    if (increasePerRow[row_index] > bestRowIncrease) {
      bestRow = row_index;
      bestRowIncrease = increasePerRow[row_index];
    }
  }
}

void RFTimeBlockEncoder::fitToMaximum(std::vector<DBufferRow> &data, float *metaBuffer,
    const dyscostman::StochasticEncoder<float> &gausEncoder) {
  const size_t visPerRow = _nPol * _nChannels;
  // Scale channels: channels are scaled such that the maximum
  // value equals the maximum encodable value
  for (size_t visIndex = 0; visIndex != visPerRow; ++visIndex) {
    double largest_component = 0.0;
    for (const DBufferRow &row : data) {
      if (row.antenna1 != row.antenna2) {
        const std::complex<double> *ptr = &row.visibilities[visIndex];
        double local_max = std::max(std::max(ptr->real(), ptr->imag()),
                                   -std::min(ptr->real(), ptr->imag()));
        if (std::isfinite(local_max) && local_max > largest_component)
          largest_component = local_max;
      }
    }
    const double factor =
        (gausEncoder.MaxQuantity() == 0.0 || largest_component == 0.0)
            ? 1.0
            : gausEncoder.MaxQuantity() / largest_component;
    changeChannelFactor(data, metaBuffer, visIndex, factor);
  }

  ao::uvector<double> maxCompPerRow;
  ao::uvector<double> increasePerRow;
  for (size_t polIndex = 0; polIndex != _nPol; ++polIndex) {
    bool isProgressing;
    do {
      // Find the factor that increasest the sum of absolute values the most
      double bestChannelIncrease = 0.0;
      double channelFactor = 1.0;
      size_t bestChannel = 0;
      getBestChannelIncrease(data, gausEncoder, polIndex, bestChannelIncrease, channelFactor, bestChannel);

      // Do same for rows
      size_t bestRow = 0;
      getBestRowIncrease(data, gausEncoder, polIndex, maxCompPerRow, increasePerRow, bestRow);
      const double bestRowIncrease = increasePerRow[bestRow];

      // The benefit was calculated for increasing the row factor and for increasing the
      // channel factor. Select which of those two has the largest benefit and apply:
      if (bestRowIncrease > bestChannelIncrease) {
        double factor =
            (maxCompPerRow[bestRow] == 0.0)
                ? 1.0
                : (gausEncoder.MaxQuantity() / maxCompPerRow[bestRow]);
        if (factor < 1.0)
          isProgressing = false;
        else {
          isProgressing = factor > 1.01;
          changeRowFactor(data, metaBuffer, bestRow, factor);
        }
      } else {
        if (channelFactor < 1.0) {
          isProgressing = false;
        } else {
          isProgressing = channelFactor > 1.001;
          changeChannelFactor(data, metaBuffer, bestChannel * _nPol + polIndex,
                              channelFactor);
        }
      }
    } while (isProgressing);
  }
}

template <bool UseDithering>
void RFTimeBlockEncoder::encode(
    const dyscostman::StochasticEncoder<float> &gausEncoder,
    const TimeBlockEncoder::FBuffer &buffer, float *metaBuffer,
    TimeBlockEncoder::symbol_t *symbolBuffer, size_t /*antennaCount*/,
    std::mt19937 *rnd) {
  // Note that encoding is performed with doubles
  std::vector<DBufferRow> data;
  buffer.ConvertVector<std::complex<double>>(data);
  const size_t visPerRow = _nPol * _nChannels;

  // Scale channels: Normalize the RMS of the channels
  std::vector<RMSMeasurement> channelRMSes(visPerRow);
  for (const DBufferRow &row : data) {
    for (size_t i = 0; i != visPerRow; ++i)
      channelRMSes[i].Include(row.visibilities[i]);
  }
  for (DBufferRow &row : data) {
    for (size_t i = 0; i != visPerRow; ++i) {
      const double rms = channelRMSes[i].RMS();
      if (rms != 0.0) {
        row.visibilities[i] /= rms;
      }
      metaBuffer[i] = rms;
    }
  }

  // Scale rows: Scale every maximum per row to the max level.
  // Polarizations are processed separately: every polarization
  // has its own row-scaling factor.
  const double maxLevel = gausEncoder.MaxQuantity();
  for (size_t rowIndex = 0; rowIndex != data.size(); ++rowIndex) {
    DBufferRow &row = data[rowIndex];
    ao::uvector<double> maxValPerPol(_nPol, 0.0);
    for (size_t i = 0; i != visPerRow; ++i) {
      std::complex<double> v = row.visibilities[i];
      double m = std::max(std::fabs(v.real()), std::fabs(v.imag()));
      if (std::isfinite(m))
        maxValPerPol[i % _nPol] = std::max(maxValPerPol[i % _nPol], m);
    }
    for (size_t i = 0; i != visPerRow; ++i) {
      const double factor = maxValPerPol[i % _nPol] == 0.0
                                ? 1.0
                                : maxLevel / maxValPerPol[i % _nPol];
      row.visibilities[i] *= factor;
    }
    for (size_t polIndex = 0; polIndex != _nPol; ++polIndex) {
      metaBuffer[visPerRow + rowIndex * _nPol + polIndex] =
          (maxLevel == 0.0) ? 1.0 : maxValPerPol[polIndex] / maxLevel;
    }
  }

  fitToMaximum(data, metaBuffer, gausEncoder);

  symbol_t *symbolBufferPtr = symbolBuffer;
  for (const DBufferRow &row : data) {
    for (size_t i = 0; i != visPerRow; ++i) {
      if (UseDithering) {
        symbolBufferPtr[i * 2] = gausEncoder.EncodeWithDithering(
            row.visibilities[i].real(), _ditherDist(*rnd));
        symbolBufferPtr[i * 2 + 1] = gausEncoder.EncodeWithDithering(
            row.visibilities[i].imag(), _ditherDist(*rnd));
      } else {
        symbolBufferPtr[i * 2] = gausEncoder.Encode(row.visibilities[i].real());
        symbolBufferPtr[i * 2 + 1] =
            gausEncoder.Encode(row.visibilities[i].imag());
      }
    }
    symbolBufferPtr += visPerRow * 2;
  }
}

void RFTimeBlockEncoder::InitializeDecode(const float *metaBuffer, size_t nRow,
                                          size_t /*nAntennae*/) {
  _channelFactors.assign(metaBuffer, metaBuffer + _nPol * _nChannels);
  metaBuffer += _nPol * _nChannels;
  _rowFactors.assign(metaBuffer, metaBuffer + _nPol * nRow);
}

void RFTimeBlockEncoder::Decode(
    const dyscostman::StochasticEncoder<float> &gausEncoder,
    TimeBlockEncoder::FBuffer &buffer,
    const TimeBlockEncoder::symbol_t *symbolBuffer, size_t blockRow,
    size_t antenna1, size_t antenna2) {
  FBufferRow &row = buffer[blockRow];
  row.antenna1 = antenna1;
  row.antenna2 = antenna2;
  row.visibilities.resize(_nChannels * _nPol);
  std::complex<float> *destination = row.visibilities.data();
  const symbol_t *srcRowPtr = symbolBuffer + blockRow * SymbolsPerRow();
  const size_t visPerRow = _nPol * _nChannels;
  for (size_t i = 0; i != visPerRow; ++i) {
    double chFactor = _channelFactors[i];
    double factor = chFactor * _rowFactors[blockRow * _nPol + i % _nPol];
    destination->real(double(gausEncoder.Decode(*srcRowPtr)) * factor);
    ++srcRowPtr;
    destination->imag(double(gausEncoder.Decode(*srcRowPtr)) * factor);
    ++srcRowPtr;
    ++destination;
  }
}
