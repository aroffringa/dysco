#ifndef DYSCO_STOCHASTIC_ENCODER_H
#define DYSCO_STOCHASTIC_ENCODER_H

#include <algorithm>
#include <cmath>
#include <cstring>

#include "dictionary.h"

namespace dyscostman {

/**
 * Lossy encoder for stochastic values.
 *
 * This encoder can encode a numeric value represented by a floating point
 * number (float, double, ...) into an integer value with a given limit.
 * It can be made to be the least-square error quantization for some
 * distributions.
 *
 * Encoding and decoding have asymetric time complexity / speeds, as decoding
 * is easier than encoding. Decoding is a single indexing into an array, thus
 * extremely fast and with constant time complexity. Encoding is a binary
 * search through the quantizaton values, thus takes O(log quantcount).
 * Typical performance of encoding is 100 MB/s.
 *
 * If the values are encoded into a number of bits which are not divisible by
 * eight, the BytePacker class can be used to pack the values.
 *
 * @author André Offringa (offringa@gmail.com)
 */
template <typename ValueType = float>
class StochasticEncoder {
 public:
  /**
   * Construct encoder for given dictionary size and Gaussian stddev.
   * This constructor initializes the lookup table, and is therefore
   * quite slow.
   * @param quantCount The number of quantization levels, i.e., the dictionary
   * size. Normally this is 2^bitcount. One quantity will be saved for storing
   * non-finite values (NaN and infinites).
   * @param stddev The standard deviation of the data. The closer this value is
   * to the real stddev, the more accurate the encoder will be.
   * @param gaussianMapping Used for testing with non-gaussian distributions.
   */
  StochasticEncoder(size_t quantCount, ValueType stddev,
                    bool gaussianMapping = true);

  static StochasticEncoder StudentTEncoder(size_t quantCount, double nu,
                                           double rms) {
    StochasticEncoder<ValueType> encoder(quantCount);
    encoder.initializeStudentT(nu, rms);
    return encoder;
  }

  static StochasticEncoder TruncatedGausEncoder(size_t quantCount, double trunc,
                                                double rms) {
    StochasticEncoder<ValueType> encoder(quantCount);
    encoder.initializeTruncatedGaussian(trunc, rms);
    return encoder;
  }

  /**
   * Unsigned integer type used for representing the encoded symbols.
   */
  typedef unsigned symbol_t;

  /**
   * Template type used for representing floating point values that
   * are to be encoded.
   */
  typedef ValueType value_t;

  /**
   * Get the quantized symbol for the given floating point value.
   * This method is implemented with a binary search, so takes
   * O(log N), with N the dictionary size (2^bitcount).
   * Use Decode() on the returned symbol to get
   * the decoded value.
   * @param value Floating point value to be encoded.
   */
  symbol_t Encode(ValueType value) const {
    if (std::isfinite(value))
      return _encDictionary.symbol(_encDictionary.lower_bound(value));
    else
      return QuantizationCount() - 1;
  }

  static std::uniform_int_distribution<unsigned> GetDitherDistribution() {
    return std::uniform_int_distribution<unsigned>(0, ((1u << 31) - 1));
  }

  /**
   * Get the quantized symbol for the given floating point value.
   * Dithering is applied, which will cause the average error to
   * converge to zero, assuming the error is uniformly distributed.
   * This method is implemented with a binary search, so takes
   * O(log N), with N the dictionary size (2^bitcount).
   * Use Decode() on the returned symbol to get
   * the decoded value.
   * @param value Floating point value to be encoded.
   * @param ditherValue The dithering value to apply.
   */
  symbol_t EncodeWithDithering(ValueType value, unsigned ditherValue) const {
    if (std::isfinite(value)) {
      const typename Dictionary::const_iterator lowerBound =
          _decDictionary.lower_bound(value);
      if (lowerBound == _decDictionary.begin())
        return _decDictionary.symbol(lowerBound);
      if (lowerBound == _decDictionary.end())
        return _decDictionary.symbol(lowerBound - 1);
      const ValueType rightValue = _decDictionary.value(lowerBound);
      const ValueType leftValue = _decDictionary.value(lowerBound - 1);

      ValueType ditherMark =
          ValueType(1u << 31) * (value - leftValue) / (rightValue - leftValue);
      if (ditherMark > ditherValue)
        return _decDictionary.symbol(lowerBound);
      else
        return _decDictionary.symbol(lowerBound - 1);
    } else {
      return _encDictionary.size();
    }
  }

  /**
   * Will return the right boundary of the given symbol.
   * The right boundary is the smallest value that would not be
   * quantized to the given symbol anymore. If no such boundary
   * exists, 0.0 is returned.
   */
  value_t RightBoundary(symbol_t symbol) const {
    if (symbol != _encDictionary.size())
      return _encDictionary.value(symbol);
    else
      return 0.0;
  }

  /**
   * Get the centroid value that belongs to the given symbol.
   * @param symbol Symbol to be decoded
   * @returns The best estimate of the original value.
   */
  ValueType Decode(symbol_t symbol) const {
    return _decDictionary.value(symbol);
  }

  size_t QuantizationCount() const { return _decDictionary.size() + 1; }

  ValueType MaxQuantity() const { return _decDictionary.largest_value(); }

  ValueType MinQuantity() const { return _decDictionary.smallest_value(); }

 private:
  explicit StochasticEncoder(size_t quantCount)
      : _encDictionary(quantCount - 1), _decDictionary(quantCount - 1) {}

  void initializeStudentT(double nu, double rms);

  void initializeTruncatedGaussian(double truncationValue, double rms);

  typedef long double num_t;

  static num_t cumulative(num_t x);
  static num_t invCumulative(num_t c, num_t err = num_t(1e-13));

  Dictionary _encDictionary;
  Dictionary _decDictionary;
};

}  // namespace dyscostman

#endif
