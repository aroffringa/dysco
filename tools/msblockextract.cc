// msblockextract -- dump one time block of a Measurement Set to the flat
// binary format consumed by `dsbench`.
//
// Usage: msblockextract <input.ms> <output.bin> [blockIndex]
//
// Layout (little-endian):
//   char     magic[8] = "DYSCOBK1"
//   uint64   nAnt, nChannels, nPol, nRows
//   int32    ant1[nRows], ant2[nRows]
//   float    data[2 * nRows * nChannels * nPol]   (re,im interleaved)
//
// The visibility order matches the Dysco AF layout: within a row the
// polarization index varies fastest (index = channel * nPol + pol), which is
// also casacore's column-major DATA order for a [nPol, nChannels] column.

#include <casacore/casa/Arrays/IPosition.h>
#include <casacore/casa/BasicSL/Complex.h>
#include <casacore/tables/Tables/ArrayColumn.h>
#include <casacore/tables/Tables/ScalarColumn.h>
#include <casacore/tables/Tables/Table.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

template <class T>
void writePod(std::FILE *out, const T &value) {
  if (std::fwrite(&value, sizeof(T), 1, out) != 1)
    throw std::runtime_error("write failed");
}

}  // namespace

int main(int argc, char **argv) {
  if (argc < 3) {
    std::fprintf(stderr, "usage: msblockextract <input.ms> <output.bin> "
                         "[blockIndex]\n");
    return 1;
  }
  const std::string msPath = argv[1];
  const std::string outPath = argv[2];
  const std::size_t targetBlock = argc > 3 ? std::atoi(argv[3]) : 0;

  try {
    casacore::Table table(msPath);
    casacore::ScalarColumn<casacore::Double> timeCol(table, "TIME");
    casacore::ScalarColumn<casacore::Int> ant1Col(table, "ANTENNA1");
    casacore::ScalarColumn<casacore::Int> ant2Col(table, "ANTENNA2");
    casacore::ArrayColumn<casacore::Complex> dataCol(table, "DATA");
    if (table.nrow() == 0) throw std::runtime_error("empty table");

    const casacore::IPosition shape = dataCol.shape(0);
    if (shape.size() < 2)
      throw std::runtime_error("DATA column does not have 2 dimensions");
    const std::size_t nPol = static_cast<std::size_t>(shape[0]);
    const std::size_t nChannels = static_cast<std::size_t>(shape[1]);
    const std::size_t visPerRow = nPol * nChannels;

    // Rows are time-ordered; collect the target run of equal TIME.
    std::vector<std::size_t> rows;
    std::size_t block = 0;
    std::size_t r = 0;
    while (r != table.nrow()) {
      const double time = timeCol(r);
      std::size_t end = r + 1;
      while (end != table.nrow() && timeCol(end) == time) ++end;
      if (block == targetBlock) {
        for (std::size_t i = r; i != end; ++i) rows.push_back(i);
        break;
      }
      ++block;
      r = end;
    }
    if (rows.empty()) throw std::runtime_error("time block not found");

    const std::size_t nRows = rows.size();
    std::vector<int> ant1(nRows), ant2(nRows);
    std::size_t nAnt = 0;
    for (std::size_t i = 0; i != nRows; ++i) {
      ant1[i] = ant1Col(rows[i]);
      ant2[i] = ant2Col(rows[i]);
      nAnt = std::max<std::size_t>(
          nAnt, static_cast<std::size_t>(std::max(ant1[i], ant2[i])) + 1);
    }

    std::FILE *out = std::fopen(outPath.c_str(), "wb");
    if (out == nullptr) throw std::runtime_error("cannot open " + outPath);
    const char magic[8] = {'D','Y','S','C','O','B','K','1'};
    if (std::fwrite(magic, 8, 1, out) != 1)
      throw std::runtime_error("write failed");
    writePod<std::uint64_t>(out, nAnt);
    writePod<std::uint64_t>(out, nChannels);
    writePod<std::uint64_t>(out, nPol);
    writePod<std::uint64_t>(out, nRows);
    std::fwrite(ant1.data(), sizeof(int), nRows, out);
    std::fwrite(ant2.data(), sizeof(int), nRows, out);

    casacore::Array<casacore::Complex> rowData;
    for (std::size_t i = 0; i != nRows; ++i) {
      dataCol.get(rows[i], rowData);
      const casacore::Complex *src = rowData.data();
      for (std::size_t v = 0; v != visPerRow; ++v) {
        const float re = src[v].real();
        const float im = src[v].imag();
        std::fwrite(&re, sizeof(float), 1, out);
        std::fwrite(&im, sizeof(float), 1, out);
      }
    }
    std::fclose(out);

    std::printf(
        "wrote %s: block=%zu rows=%zu ch=%zu pol=%zu nAnt=%zu (%.1f MiB)\n",
        outPath.c_str(), targetBlock, nRows, nChannels, nPol, nAnt,
        double(nRows) * visPerRow * 8.0 / 1024 / 1024);
  } catch (const std::exception &e) {
    std::fprintf(stderr, "msblockextract: %s\n", e.what());
    return 1;
  }
  return 0;
}
