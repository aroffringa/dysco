#include "../stochasticencoder.h"

#include <boost/test/unit_test.hpp>

#include <fstream>
#include <random>
#include <sstream>

using namespace dyscostman;

const size_t bits = 4;
const bool useUniform = false;

BOOST_AUTO_TEST_SUITE(encode_example)

void make(const std::string& filenamePrefix, ao::uvector<double> values)
{
	std::mt19937 mt;
	std::uniform_int_distribution<unsigned> dist = StochasticEncoder<double>::GetDitherDistribution();
	
	double sqSum = 0.0;
	size_t count = 0;
	for(double v : values)
	{
		if(std::isfinite(v))
		{
			sqSum += v * v;
			++count;
		}
	}
	double rms = sqrt(sqSum / count);
	//GausEncoder<float> enc(1 << bits, rms);
	//double nu = 1.0;
	//GausEncoder<float> enc = GausEncoder<float>::StudentTEncoder(1 << bits, nu, rms);
	double truncVal = 2.0;
	StochasticEncoder<float> enc = StochasticEncoder<float>::TruncatedGausEncoder(1 << bits, truncVal, rms);
	
	ao::uvector<size_t> trials{1, 3, 10, 100};
	for(size_t trial : trials)
	{
		ao::uvector<double> sums(values.size(), 0.0);

		for(size_t instance=0; instance!=trial; ++instance)
		{
			for(size_t i=0; i!=values.size(); ++i)
			{
				unsigned dither = dist(mt);
				sums[i] += enc.Decode(enc.EncodeWithDithering(values[i], dither));
			}
		}
		std::ostringstream fn;
		fn << filenamePrefix << "-" << trial << ".txt";
		std::ofstream file(fn.str());
		for(size_t i=0; i!=values.size(); ++i)
		{
			file << i << '\t' << values[i] << '\t' << sums[i]/trial << '\n';
			if(trial >= 100 && std::fabs(values[i]) < enc.MaxQuantity())
				BOOST_CHECK_LT(std::fabs(sums[i]/trial - values[i]), 0.1);
		}
	}
}

constexpr size_t n=100;

BOOST_AUTO_TEST_CASE( sinus )
{
	ao::uvector<double> values(n);
	for(size_t i=0; i!=n; ++i)
	{
		double x = double(i)*2*M_PI/n;
		values[i] = sin(x);
	}
	make("sinus", values);
}

BOOST_AUTO_TEST_CASE( oneOver )
{
	ao::uvector<double> values(n);
	for(size_t i=0; i!=n ;++i)
	{
		double x = double(i)/n;
		values[i] = 1.0 / (12.0*x);
	}
	make("oneOver", values);
}

BOOST_AUTO_TEST_CASE( sinc )
{
	ao::uvector<double> values(n);
	for(size_t i=1; i!=n ;++i)
	{
		double x = double(i)/n * 2.5 * M_PI;
		values[i] = sin(x) / x;
	}
	values[0] = 1.0;
	make("sinc", values);
}

BOOST_AUTO_TEST_SUITE_END()
