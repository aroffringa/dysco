#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include "uvector.h"

class Histogram
{
public:
	Histogram(double minVal, double maxVal, size_t binCount=100) : _bins(binCount, 0), _min(minVal), _max(maxVal)
	{ }
	
	void Include(double value)
	{
		_bins[getBin(value)]++;
	}
	
	double BinX(size_t index) const
	{
		return _min + index * (_max - _min) / _bins.size();
	}
	
	size_t operator[](size_t index) const { return _bins[index]; }
	
	size_t size() const { return _bins.size(); }
	
private:
	size_t getBin(double value) const
	{
		if(value < _min)
			return 0;
		else if(value > _max)
			return _bins.size()-1;
		else
			return round(double(_bins.size()-1) * (value - _min) / (_max - _min));
	}
	
	ao::uvector<size_t> _bins;
	double _min, _max;
};

#endif
