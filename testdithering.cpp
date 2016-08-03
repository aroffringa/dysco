#include <iostream>
#include <fstream>
#include <random>

#include "gausencoder.h"
#include "stopwatch.h"

using namespace dyscostman;

void TestGausEncoder(bool uniform)
{
	//GausEncoder<float> encoder(256, 1.0, !uniform);
	GausEncoder<float> encoder = GausEncoder<float>::StudentTEncoder(256, 1.0, 1.0);
	constexpr size_t N=1; //N=12;
	const double values[] = { -(M_PI), -(M_E), -(M_SQRT2), -1.0, -0.01, 0.0, 0.01, 1.0, M_SQRT2, M_E, M_PI, std::numeric_limits<double>::infinity() };
	long double sumsUndithered[N];
	long double sumsDithered[N];
	for(size_t j=0; j!=N; ++j) {
		sumsUndithered[j] = 0.0;
		sumsDithered[j] = 0.0;
	}
	std::mt19937 mt;
	std::uniform_int_distribution<unsigned> distribution = encoder.GetDitherDistribution();
	std::cout << "Input: ";
	for(size_t j=0; j!=N; ++j) std::cout << ' ' << values[j];
	std::cout << "\nDecoded without dithering: ";
	for(size_t j=0; j!=N; ++j) std::cout << ' ' << encoder.Decode(encoder.Encode(values[j]));
	std::cout << "\nDecoded with dithering: ";
	for(size_t j=0; j!=N; ++j) std::cout << ' ' << encoder.Decode(encoder.EncodeWithDithering(values[j], distribution(mt)));
	std::cout << '\n';
	std::ofstream file("dither-test.txt");
	for(size_t i=0; i!=10000000001; ++i)
	{
		if(i==1)
		{
			std::cout << "Undithered error average:";
			for(size_t j=0; j!=N; ++j) std::cout << ' ' << (sumsUndithered[j]/double(i))-values[j];
			std::cout << '\n';
		}
		if(i==1 || i==10 || i==100 || i==1000 || i==10000 || i==100000 || i==1000000 || i==10000000 || i==100000000 || i==1000000000 || i==10000000000)
		{
			std::cout << "Dithered error average after " << i << " x :\n";
			for(size_t j=0; j!=N; ++j) std::cout << ' ' << (sumsDithered[j]/double(i))-values[j];
			std::cout << '\n';
		}
		if(i%1000000 == 0)
		{
			file << i << '\t' << (sumsDithered[0]/double(i))-values[0] << '\n';
		}
		for(size_t j=0; j!=N; ++j)
		{
			sumsUndithered[j] += encoder.Decode(encoder.Encode(values[j]));
			sumsDithered[j] += encoder.Decode(encoder.EncodeWithDithering(values[j], distribution(mt)));
		}
	}
	std::cout << "Undithered encoding... " << std::flush;
	Stopwatch watch(true);
	for(size_t i=0; i!=1000000; ++i)
		encoder.Encode(values[i%N]);
	std::cout << 1.0/watch.Seconds() << " Mfloat/s\n";
	std::cout << "Dithered encoding... " << std::flush;
	watch.Reset(); watch.Start();
	for(size_t i=0; i!=1000000; ++i)
		encoder.EncodeWithDithering(values[i%N], distribution(mt));
	std::cout << 1.0/watch.Seconds() << " Mfloat/s\n";
}

int main(int argc, char* argv[])
{
	TestGausEncoder(false);
	//TestGausEncoder(true);
}
