#include "gausencoder.h"
#include "dyscodistribution.h"

using namespace dyscostman;

#include <iostream>
#include <random>

#include <casacore/ms/MeasurementSets/MeasurementSet.h>
#include <tables/Tables/ArrayColumn.h>

std::mt19937 mt;
std::uniform_int_distribution<int> dither(0, 65535);

float compress(float val, float maxVal, int roundValue)
{
	if(std::isfinite(val))
	{
		return round(val * roundValue / maxVal) * maxVal / roundValue;
	}
	else {
		return std::numeric_limits< float >::quiet_NaN();
	}
}

void simulateCompress(GausEncoder<float>& encoder, std::complex<float>* data, const bool* flag, size_t n)
{
	float maxVal = 0.0;
	for(size_t i=0; i!=n; ++i)
	{
		if(!flag[i] && std::isfinite(data[i].real()) && std::isfinite(data[i].imag()))
		{
			maxVal = std::max(std::fabs(data[i].real()), maxVal);
			maxVal = std::max(std::fabs(data[i].imag()), maxVal);
		}
	}
	// 2^(bitCount-1) - 1 options, both negative, zero and positive
	/*int roundFactor = ((1<<bitCount) - 1) / 2;
	
	//std::cout << "max=" << maxVal << ", roundFac=" << roundFactor << '\n';
	
	for(size_t i=0; i!=n; ++i)
	{
		std::complex<float>& v = data[i];
		v.real(compress(v.real(), maxVal, roundFactor));
		v.imag(compress(v.imag(), maxVal, roundFactor));
	}*/
	double factor = encoder.MaxQuantity() / maxVal;
	
	for(size_t i=0; i!=n; ++i)
	{
		unsigned int
			rsym = encoder.EncodeWithDithering(data[i].real() * factor, dither(mt)),
			isym = encoder.EncodeWithDithering(data[i].imag() * factor, dither(mt));
		data[i] = std::complex<float>(encoder.Decode(rsym)/factor, encoder.Decode(isym)/factor);
	}
}

void compressFile(const std::string& filename, int bitCount, DyscoDistribution distribution, double distributionTruncation)
{
	int quantCount = 1<<bitCount;
	GausEncoder<float> encoder(2, 1.0, false);
	switch(distribution)
	{
		case UniformDistribution:
			encoder = GausEncoder<float>(quantCount, 1.0, false);
			break;
		case GaussianDistribution:
			encoder = GausEncoder<float>(quantCount, 1.0, true);
			break;
		case TruncatedGaussianDistribution:
			encoder = GausEncoder<float>::TruncatedGausEncoder(quantCount, distributionTruncation, 1.0);
			break;
		default:
			throw std::runtime_error("Unsupported distribution");
	}
	
	casacore::MeasurementSet ms(filename, casacore::Table::Update);
	casacore::ArrayColumn<casa::Complex> dataCol(ms, casacore::MeasurementSet::columnName(casacore::MSMainEnums::DATA));
	casacore::ArrayColumn<bool> flagCol(ms, casacore::MeasurementSet::columnName(casacore::MSMainEnums::FLAG));
	casacore::IPosition shape(dataCol.shape(0));
	casacore::Array<casa::Complex> dataArr(shape);
	casacore::Array<bool> flagArr(shape);
	
	const size_t n = shape[0] * shape[1];
	
	for(size_t row=0; row!=ms.nrow(); ++row)
	{
		dataCol.get(row, dataArr);
		flagCol.get(row, flagArr);
		simulateCompress(encoder, dataArr.data(), flagArr.data(), n);
		dataCol.put(row, dataArr);
	}
}

int main(int argc, char* argv[])
{
	if(argc >= 3)
	{
		int argi = 1;
		DyscoDistribution distribution = GaussianDistribution;
		double distributionTruncation = 0.0;
		while(argv[argi][0] == '-')
		{
			std::string p(&argv[argi][1]);
			if(p == "truncgaus")
			{
				++argi;
				distribution = TruncatedGaussianDistribution;
				distributionTruncation = atof(argv[argi]);
			}
			else if(p == "uniform")
			{
				distribution = UniformDistribution;
			}
			else throw std::runtime_error("Bad parameter");
			++argi;
		}
		compressFile(argv[argi], atoi(argv[argi+1]), distribution, distributionTruncation);
	}
	else if(argc == 2)
		throw std::runtime_error("Wrong number of parameters");
	else {
		bool flag[8] = {false, false, false, false, false, false, false, false};
		const std::complex<float> dataB[8] = {0.0, 0.8, 1.5, 0.85, 0.0, -0.9, -1.5, -0.8111111};

		for(size_t bits=2; bits!=20; ++bits)
		{
			std::cout << "Bits=" << bits << '\n';
			GausEncoder<float> encoder(1<<bits, 1.0, false);
			std::complex<float> data[8] = dataB;
			simulateCompress(encoder, data, flag, 8);
			for(size_t i=0; i!=8; ++i)
				std::cout << data[i] << ' ';
			std::cout << '\n';
		}
	}
	
	return 0;
}
