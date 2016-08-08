#include "aftimeblockencoder.h"
#include "rftimeblockencoder.h"
#include "rowtimeblockencoder.h"
#include "stopwatch.h"
#include "gausencoder.h"

#include "dysconormalization.h"

#include <iostream>
#include <random>

using namespace dyscostman;

DyscoNormalization blockNormalization = AFNormalization;

std::unique_ptr<TimeBlockEncoder> CreateEncoder(size_t nPol, size_t nChan)
{
	switch(blockNormalization)
	{
		default:
		case RFNormalization:
			return std::unique_ptr<TimeBlockEncoder>(new RFTimeBlockEncoder(nPol, nChan));
		case AFNormalization:
			return std::unique_ptr<TimeBlockEncoder>(new AFTimeBlockEncoder(nPol, nChan, true));
		case RowNormalization:
			return std::unique_ptr<TimeBlockEncoder>(new RowTimeBlockEncoder(nPol, nChan));
	}
}

void TestSimpleExample()
{
	const size_t nAnt = 4, nChan = 1, nPol = 2, nRow = (nAnt*(nAnt+1)/2);
	
	TimeBlockBuffer<std::complex<float>> buffer(nPol, nChan);
	
	std::complex<float> data[nChan*nPol];
	data[0] = 99.0; data[1] = 99.0;
	buffer.SetData(0, 0, 0, data);
	data[0] = 10.0; data[1] = std::complex<double>(9.0, 1.0);
	buffer.SetData(1, 0, 1, data);
	data[0] = 8.0; data[1] = std::complex<double>(7.0, 2.0);
	buffer.SetData(2, 0, 2, data);
	data[0] = 6.0; data[1] = std::complex<double>(5.0, 3.0);
	buffer.SetData(3, 0, 3, data);
	data[0] = 99.0; data[1] = 99.0;
	buffer.SetData(4, 1, 1, data);
	data[0] = 4.0; data[1] = std::complex<double>(3.0, 4.0);
	buffer.SetData(5, 1, 2, data);
	data[0] = 2.0; data[1] = std::complex<double>(1.0, 5.0);
	buffer.SetData(6, 1, 3, data);
	data[0] = 99.0; data[1] = 99.0;
	buffer.SetData(7, 2, 2, data);
	data[0] = 0.0; data[1] = std::numeric_limits<float>::quiet_NaN();
	buffer.SetData(8, 2, 3, data);
	data[0] = 99.0; data[1] = 99.0;
	buffer.SetData(9, 3, 3, data);
	
	GausEncoder<float> gausEncoder(256, 1.0, true);
	std::unique_ptr<TimeBlockEncoder> encoder = CreateEncoder(nPol, nChan);
	
	size_t metaDataCount = encoder->MetaDataCount(nRow, nPol, nChan, nAnt);
	std::cout << "Meta data size: " << metaDataCount << '\n';
	size_t symbolCount = encoder->SymbolCount(nRow);
	ao::uvector<float> metaBuffer(metaDataCount);
	ao::uvector<TimeBlockEncoder::symbol_t> symbolBuffer(symbolCount);
	
	encoder->EncodeWithoutDithering(gausEncoder, buffer, metaBuffer.data(), symbolBuffer.data(), nAnt);
	TimeBlockBuffer<std::complex<float>> out(nPol, nChan);
	out.resize(nRow);
	std::unique_ptr<TimeBlockEncoder> decoder = CreateEncoder(nPol, nChan);
	decoder->InitializeDecode(metaBuffer.data(), nRow, nAnt);
	size_t rIndex = 0;
	for(size_t ant1=0; ant1!=nAnt; ++ant1) {
		for(size_t ant2=ant1; ant2!=nAnt; ++ant2) {
			decoder->Decode(gausEncoder, out, symbolBuffer.data(), rIndex, ant1, ant2);
			++rIndex;
		}
	}
	std::complex<float> dataOut[nChan];
	std::cout << "Output (XX\tYY)\tInput (XX\tYY)\n";
	for(size_t row=0; row!=nRow; row++)
	{
		out.GetData(row, dataOut);
		std::cout << dataOut[0] << '\t' << dataOut[1] << '\t';
		buffer.GetData(row, dataOut);
		std::cout << dataOut[0] << '\t' << dataOut[1] << '\n';
	}
}

void TestTimeBlockEncoder()
{
	const size_t nAnt = 50, nChan = 64, nPol = 4, nRow = (nAnt*(nAnt+1)/2);
	
	TimeBlockBuffer<std::complex<float>> buffer(nPol, nChan);
	std::mt19937 rnd;
	std::normal_distribution<float> dist;
	GausEncoder<float> gausEncoder(256, 1.0, false);
	std::uniform_int_distribution<unsigned> dither = gausEncoder.GetDitherDistribution();
	
	dist(rnd);
	
	ao::uvector<std::complex<float>> data(nChan * nPol);
	std::vector<ao::uvector<std::complex<float>>> allData;
	
	RMSMeasurement unscaledRMS;
	double factorSum = 0.0;
	size_t factorCount = 0;
	size_t blockRow = 0;
	for(size_t a1=0; a1!=nAnt; ++a1)
	{
		for(size_t a2=a1; a2!=nAnt; ++a2)
		{
			for(size_t ch=0; ch!=nChan; ++ch)
			{
				for(size_t p=0; p!=nPol; ++p)
				{
					std::complex<float> rndVal(dist(rnd), dist(rnd));
					//std::complex<float> rndVal(1.0, 0.0);
					float f = 1.0;
					//float f = float(a1+1) * float(a2+1) * float(p+1);
					factorSum += f;
					factorCount++;
					data[p + ch*nPol] = rndVal * f;
					
					std::complex<float> encodedVal(
						gausEncoder.Decode(gausEncoder.EncodeWithDithering(rndVal.real(), dither(rnd))),
						gausEncoder.Decode(gausEncoder.EncodeWithDithering(rndVal.imag(), dither(rnd)))
					);
					unscaledRMS.Include(encodedVal - rndVal);
				}
			}
			buffer.SetData(blockRow, a1, a2, data.data());
			allData.push_back(data);
			++blockRow;
		}
	}
	
	std::unique_ptr<TimeBlockEncoder> encoder = CreateEncoder(nPol, nChan);

	const size_t nIter = 25;
	Stopwatch watch(true);
	ao::uvector<float> metaBuffer(encoder->MetaDataCount(nRow, nPol, nChan, nAnt));
	ao::uvector<unsigned> symbolBuffer(encoder->SymbolCount(nAnt*(nAnt+1)/2));
	
	std::cout << "Baselines: " << nAnt*(nAnt+1)/2 << ", rows in encoder: " << buffer.NRows() << '\n';
	
	std::cout << "Encoding... " << std::flush;
	for(size_t i=0; i!=nIter; ++i)
		encoder->EncodeWithDithering(gausEncoder, buffer, metaBuffer.data(), symbolBuffer.data(), nAnt, rnd);
	
	double dataSize = double(nIter) * nRow * nChan * nPol;
	std::cout << "Speed: " << 1e-6*dataSize / watch.Seconds() << " Mvis/sec\n";
	
	std::cout << "Decoding... " << std::flush;
	watch.Reset();
	watch.Start();
	for(size_t i=0; i!=nIter; ++i)
	{
		encoder->InitializeDecode(metaBuffer.data(), nRow, nAnt);
		blockRow = 0;
		for(size_t a1=0; a1!=nAnt; ++a1)
		{
			for(size_t a2=a1; a2!=nAnt; ++a2)
			{
				ao::uvector<std::complex<float>> dataOut(nChan * nPol);
				encoder->Decode(gausEncoder, buffer, symbolBuffer.data(), blockRow, a1, a2);
				buffer.GetData(blockRow, dataOut.data());
				++blockRow;
			}
		}
	}
	std::cout << "Speed: " << 1e-6*dataSize / watch.Seconds() << " Mvis/sec\n";
	
	RMSMeasurement mEncoded, mZero;
	size_t index = 0;
	encoder->InitializeDecode(metaBuffer.data(), nRow, nAnt);
	blockRow = 0;
	for(size_t a1=0; a1!=nAnt; ++a1)
	{
		for(size_t a2=a1; a2!=nAnt; ++a2)
		{
			ao::uvector<std::complex<float>> dataOut(nChan * nPol);
			encoder->Decode(gausEncoder, buffer, symbolBuffer.data(), blockRow, a1, a2);
			buffer.GetData(blockRow, dataOut.data());
			if(a1 < 5 && a2 < 5)
				std::cout << dataOut[0].real() << ' ';
			
			ao::uvector<std::complex<float>>& dataIn = allData[index];
			for(size_t i=0; i!=nPol*nChan; ++i)
			{
				mEncoded.Include(dataOut[i] - dataIn[i]);
				mZero.Include(dataIn[i]);
			}
			
			++index;
			++blockRow;
		}
		if(a1 < 5)
			std::cout << '\n';
	}
	std::cout << "RMS of error: " << mEncoded.RMS() << '\n';
	std::cout << "RMS of data: " << mZero.RMS() << '\n';
	std::cout << "Gaussian encoding error for unscaled values: " << unscaledRMS.RMS() << '\n';
	std::cout << "Average factor: " << factorSum / factorCount << '\n';
	std::cout << "Effective RMS of error: " << mEncoded.RMS() / (factorSum / factorCount) << " ( " << (mEncoded.RMS() / (factorSum / factorCount)) / unscaledRMS.RMS() << " x theoretical)\n";
	std::cout.flush();
}

int main(int argc, char* argv[])
{
	std::cout << " === Row normalization ===\n";
	blockNormalization = RowNormalization;
	TestSimpleExample();
	TestTimeBlockEncoder();
	
	std::cout << " === AF normalization ===\n";
	blockNormalization = AFNormalization;
	TestSimpleExample();
	TestTimeBlockEncoder();
	
	std::cout << " === RF normalization ===\n";
	blockNormalization = RFNormalization;
	TestSimpleExample();
	TestTimeBlockEncoder();
}
