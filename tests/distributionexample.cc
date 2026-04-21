#include "../aftimeblockencoder.h"
#include "../rftimeblockencoder.h"

#include <boost/test/unit_test.hpp>

#include <fstream>
#include <random>
#include <sstream>

BOOST_AUTO_TEST_SUITE(distribution_example)

BOOST_AUTO_TEST_CASE(uniform_file) {
  std::mt19937_64 rnd;
  std::uniform_int_distribution<unsigned char> distribution(0, 3);
  std::normal_distribution<double> gaus_distribution(127.5, 25);
  std::ofstream file("random.bin");
  constexpr size_t kBufferSize = 10 * 1024 * 1024;
  constexpr size_t kFileSize = 10 * 1024 * 1024;
  std::vector<unsigned char> buffer(kBufferSize);
  for(size_t i=0; i!=kFileSize / kBufferSize; ++i) {
    for(unsigned char& value : buffer) {
      value = distribution(rnd)*16;

      //value = std::clamp<double>(gaus_distribution(rnd), 0.0, 255.0);
      //if(distribution(rnd) < 25)
      //  value = 255;
    }
    file.write(reinterpret_cast<char*>(buffer.data()), buffer.size());
  }
}

BOOST_AUTO_TEST_CASE(rf) {
  constexpr size_t kNAntenna = 75;
  constexpr size_t kNChannels = 16;
  constexpr size_t kNRowsPerTimeblock = kNAntenna * (kNAntenna-1) / 2;
  constexpr size_t kNRepeats = 100;
  constexpr bool use_rf = true;

  std::vector<TimeBlockEncoder::DBufferRow> rows(kNRowsPerTimeblock);
  for(TimeBlockEncoder::DBufferRow& row : rows) {
    row.visibilities.resize(kNChannels);
  }

  std::unique_ptr<TimeBlockEncoder> encoder_ptr;
  if(use_rf) {
    encoder_ptr = std::make_unique<RFTimeBlockEncoder>(1, kNChannels);
  } else {
    encoder_ptr = std::make_unique<AFTimeBlockEncoder>(1, kNChannels, true);
  }
  TimeBlockEncoder& encoder = *encoder_ptr;

  std::vector<float> meta_buffer(encoder.MetaDataCount(kNRowsPerTimeblock, 1, kNChannels, kNAntenna));

  std::mt19937_64 rnd;
  std::normal_distribution distribution(0.0, 100.0);
  //std::uniform_real_distribution<double> distribution(-100.0, 100.0);

  size_t kNBins = 251;
  std::vector<size_t> histogram(kNBins, 0);

  for(size_t i=0; i!=kNRepeats; ++i) {
    auto row_iterator = rows.begin();
    for(size_t antenna_1 = 0; antenna_1 != kNAntenna; ++ antenna_1) {
      for(size_t antenna_2 = antenna_1 + 1; antenna_2 != kNAntenna; ++ antenna_2) {
        TimeBlockEncoder::DBufferRow& row = *row_iterator;
        for(std::complex<double>& value : row.visibilities) {
          value = {distribution(rnd), distribution(rnd)};
        }
        row.antenna1 = antenna_1;
        row.antenna2 = antenna_2;
        ++row_iterator;
      }
    }
    BOOST_CHECK_EQUAL(row_iterator - rows.begin(), kNRowsPerTimeblock);

    encoder.Normalize(rows, meta_buffer.data(), kNAntenna, 1.0);

    for(TimeBlockEncoder::DBufferRow& row : rows) {
      for(const std::complex<double>& value : row.visibilities) {
        BOOST_CHECK_LE(value.real(), 1.0);
        BOOST_CHECK_LE(value.imag(), 1.0);
        BOOST_CHECK_GE(value.real(), -1.0);
        BOOST_CHECK_GE(value.imag(), -1.0);
        const size_t real_index = value.real() * (kNBins-1) * 0.5 + (kNBins-1) * 0.5;
        const size_t imag_index = value.imag() * (kNBins-1) * 0.5 + (kNBins-1) * 0.5;
        histogram[std::clamp<size_t>(real_index, 0, histogram.size()-1)]++;
        histogram[std::clamp<size_t>(imag_index, 0, histogram.size()-1)]++;
      }
    }
  }

  std::ofstream output(use_rf ? "rf-distribution.txt" : "af-distribution.txt");
  const size_t kNValues = 2 * kNRowsPerTimeblock * kNChannels * kNRepeats;
  for(size_t i=0; i!=kNBins; ++i) {
    const double value = i * 2.0 / (kNBins-1) - 1.0;
    output << value << "\t" << static_cast<double>(histogram[i])/kNValues << '\n';
  }
}


BOOST_AUTO_TEST_SUITE_END()
