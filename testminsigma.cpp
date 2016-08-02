#include "gausencoder.h"

#include <iostream>
#include <random>

using namespace dyscostman;

std::mt19937 mt;

double testSigma(double sigmaFactor)
{
	std::uniform_int_distribution<unsigned> dither(0, 65535);
	std::normal_distribution<double> norm(0.0, 1.0);
	GausEncoder<float> enc(256, sigmaFactor);
	
	double squaredErr = 0.0, absError = 0.0, biasError = 0.0;
	for(size_t i=0; i!=5000000; ++i)
	{
		double value = norm(mt);
		double recovered = enc.Decode(enc.EncodeWithDithering(value, dither(mt)));
		double err = value - recovered;
		squaredErr += err*err;
		absError += std::fabs(err);
		biasError += err;
	}
	std::cout << "sigma=" << sigmaFactor << " :\tstderr=" << squaredErr << "\tabserr=" << absError << "\tbias=" << biasError << '\n';
	return squaredErr;
	//return absError;
}

int main(int argc, char* argv[])
{
	for(double sigma=0.5; sigma<5; sigma+=0.1)
	{
		testSigma(sigma);
	}
	double upper=1.1, lower=0.9;
	while(std::fabs(upper-lower) > 1e-7)
	{
		double first = testSigma(lower+(upper-lower)/3.0);
		double second = testSigma(lower+(upper-lower)*2.0/3.0);
		if(second < first)
		{
			lower = lower+(upper-lower)/3.0;
		}
		else {
			upper = lower+(upper-lower)*2.0/3.0;
		}
	}
}
