#include <iostream>

#include "bytepacker.h"
#include "uvector.h"

using namespace dyscostman;

void testSingle(const ao::uvector<unsigned int>& data, int bitCount)
{
	int limit = (1<<bitCount);
	ao::uvector<unsigned int> trimmedData(data.size()), restoredData(data.size());
	for(size_t i=0; i!=data.size(); ++i)
		trimmedData[i] = data[i] % limit;
	ao::uvector<unsigned char> buffer(BytePacker::bufferSize(trimmedData.size(), bitCount), 0);
	
	if(false) {
		std::cout << "Tested array: [" << trimmedData[0];
		for(size_t i=1; i!=std::min<size_t>(trimmedData.size(), 32); ++i)
			std::cout << ", " << trimmedData[i];
		if(trimmedData.size() > 32) std::cout << ", ...";
		std::cout << "]\n";
	}
	
	BytePacker::pack(bitCount, buffer.data(), trimmedData.data(), trimmedData.size());
	BytePacker::unpack(bitCount, restoredData.data(), buffer.data(), restoredData.size());
	
	for(size_t i=0; i!=trimmedData.size(); ++i)
	{
		if(restoredData[i] != trimmedData[i])
			std::cout << "data[" << i << "] was incorrectly unpacked: was " << restoredData[i] << ", should be " << trimmedData[i] << '\n';
	}
}

void testCombinations(const ao::uvector<unsigned int>& data, int bitCount)
{
	std::cout << "==Testing bitrate " << bitCount << "==\n";
	for(size_t dataSize=1; dataSize!=std::min<size_t>(32u, data.size()); ++dataSize)
	{
		ao::uvector<unsigned int> resizedData(data.begin(), data.begin()+dataSize);
		testSingle(resizedData, bitCount);
	}
	testSingle(data, bitCount);
	for(size_t dataSize=1; dataSize!=std::min<size_t>(32u, data.size()-1); ++dataSize)
	{
		ao::uvector<unsigned int> resizedData(data.begin()+1, data.begin()+dataSize+1);
		testSingle(resizedData, bitCount);
	}
	ao::uvector<unsigned int> resizedData(data.begin()+1, data.end());
	testSingle(resizedData, bitCount);
}

void testAllBitRates(const ao::uvector<unsigned int>& data)
{
	const int bitrates[] = {2, 3, 4, 6, 8, 10, 12, 16};
	for(int b : bitrates)
		testCombinations(data, b);
}

int main(int argc, char* argv[])
{
	ao::uvector<unsigned int> testArray{1337, 2, 100, 0};
	for(int i=0; i!=1000; ++i)
	{
		testArray.push_back(i);
		testArray.push_back(i*37);
		testArray.push_back(i*2);
	}
	testAllBitRates(testArray);
}
