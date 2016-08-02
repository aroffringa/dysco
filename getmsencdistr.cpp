#include "timeblockbuffer.h"
#include "aftimeblockencoder.h"

#include "histogram.h"
#include "gausencoder.h"

#include <casacore/ms/MeasurementSets/MeasurementSet.h>
#include <casacore/tables/Tables/ArrayColumn.h>
#include <casacore/tables/Tables/ScalarColumn.h>

#include <fstream>

using namespace casacore;

int main(int argc, char* argv[])
{
	MeasurementSet ms(argv[1]);
	ArrayColumn<std::complex<float>> dataCol(ms, MeasurementSet::columnName(MSMainEnums::DATA));
	ArrayColumn<bool> flagCol(ms, MeasurementSet::columnName(MSMainEnums::FLAG));
	ScalarColumn<int> ant1Col(ms, MeasurementSet::columnName(MSMainEnums::ANTENNA1));
	ScalarColumn<int> ant2Col(ms, MeasurementSet::columnName(MSMainEnums::ANTENNA2));
	ScalarColumn<double> timeCol(ms, MeasurementSet::columnName(MSMainEnums::TIME));
	
	IPosition shape = dataCol.shape(0);
	const size_t nPol = shape[0], nChannels = shape[1];
	
	TimeBlockBuffer<std::complex<float>> buffer(nPol, nChannels);
	AFTimeBlockEncoder encoder(nPol, nChannels, true);
	dyscostman::GausEncoder<float> gEncoder(256, 1.0);
	
	std::vector<double> extremes(nPol, 0.0);
	std::vector<RMSMeasurement> rms(nPol);
	Array<std::complex<float>> dataArr(shape);
	Array<bool> flagArr(shape);
	size_t blockRowStart = 0;
	double lastTime = timeCol(0);
	constexpr std::complex<float> cNaN(
		std::numeric_limits<float>::quiet_NaN(),
		std::numeric_limits<float>::quiet_NaN());
	
	for(size_t i=0; i!=ms.nrow(); ++i)
	{
		double t = timeCol(i);
		if(t != lastTime)
		{
			encoder.Normalize(gEncoder, buffer, ms.antenna().nrow());
			for(size_t j=0; j!=buffer.NRows(); ++j)
			{
				for(size_t ch=0; ch!=nChannels; ++ch)
				{
					for(size_t p=0; p!=nPol; ++p)
					{
						std::complex<double> vis = buffer[j].visibilities[ch*nPol + p];
						rms[p].Include(vis);
						if(std::fabs(vis.real()) > extremes[p])
							extremes[p] = std::fabs(vis.real());
						if(std::fabs(vis.imag()) > extremes[p])
							extremes[p] = std::fabs(vis.imag());
					}
				}
			}
			blockRowStart = i;
			lastTime = t;
			buffer.ResetData();
		}
		dataCol.get(i, dataArr);
		flagCol.get(i, flagArr);
		int a1 = ant1Col(i), a2 = ant2Col(i);
		for(size_t ch=0; ch!=nChannels; ++ch)
		{
			for(size_t p=0; p!=nPol; ++p)
			{
				if(flagArr.data()[ch*nPol + p])
					dataArr.data()[ch*nPol + p] = cNaN;
			}
		}
		buffer.SetData(i - blockRowStart, a1, a2, dataArr.data());
	}
	
	std::vector<Histogram> histograms;
	for(size_t p=0; p!=nPol; ++p)
	{
		std::cout << "RMS for pol " << p << " after normalization: " << rms[p].RMS() << '\n';
		std::cout << " extreme: " << extremes[p] << '\n';
		//histograms.emplace_back(-50.0*rms[p].RMS(), 50.0*rms[p].RMS(), 250);
		histograms.emplace_back(-extremes[p], extremes[p], 250);
	}
	
	blockRowStart = 0;
	lastTime = timeCol(0);
	for(size_t i=0; i!=ms.nrow(); ++i)
	{
		double t = timeCol(i);
		if(t != lastTime)
		{
			encoder.Normalize(gEncoder, buffer, ms.antenna().nrow());
			for(size_t j=0; j!=buffer.NRows(); ++j)
			{
				for(size_t ch=0; ch!=nChannels; ++ch)
				{
					for(size_t p=0; p!=nPol; ++p)
					{
						histograms[p].Include(buffer[j].visibilities[ch*nPol + p].real());
						histograms[p].Include(buffer[j].visibilities[ch*nPol + p].imag());
					}
				}
			}
			blockRowStart = i;
			lastTime = t;
			buffer.ResetData();
		}
		dataCol.get(i, dataArr);
		flagCol.get(i, flagArr);
		int a1 = ant1Col(i), a2 = ant2Col(i);
		for(size_t ch=0; ch!=nChannels; ++ch)
		{
			for(size_t p=0; p!=nPol; ++p)
			{
				if(flagArr.data()[ch*nPol + p])
					dataArr.data()[ch*nPol + p] = cNaN;
			}
		}
		buffer.SetData(i - blockRowStart, a1, a2, dataArr.data());
	}
	
	for(size_t p=0; p!=nPol; ++p)
	{
		std::ostringstream fname;
		fname << "distribution-pol" << p << ".txt";
		std::ofstream file(fname.str());
		for(size_t i=0; i!=histograms[p].size(); ++i)
			file << histograms[p].BinX(i) << '\t' << histograms[p][i] << '\n';
	}
}
