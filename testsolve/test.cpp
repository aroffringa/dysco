#include <iostream>
#include <vector>
#include <cmath>

void outputM(const std::vector<double>& m, size_t n)
{
  for(size_t i=0; i!=n; ++i) {
    for(size_t j=0; j!=n; ++j) {
      std::cout << m[i*n+j] << ' ';
    }
    std::cout << '\n';
  }
}

void outputV(const std::vector<double>& v)
{
  for(auto i : v)
    std::cout << i << ' ';
  std::cout << '\n';
}

int main(int argc, char* argv[])
{
  size_t n = 12;
  std::vector<double> vec = { 5, 3, 8, 4, 1, 2, 3, 4, 5, 6, M_PI };
  vec.resize(n);
  std::vector<double> matrix(n*n);
  for(size_t i=0; i!=n; ++i) {
    for(size_t j=0; j!=n; ++j) {
      if(i==j)
	matrix[i*n+j] = 0.0;
      else
	matrix[i*n+j] = vec[j]*vec[i];
    }
  }
  double corruption = 1.0;
  matrix[n*4 + 3] += corruption;
  matrix[n*1 + 0] -= corruption;
  outputM(matrix, n);

  std::vector<double> estVec(n, 1.0);
  for(size_t x=0; x!=100 && std::fabs(estVec[0.0]-5.0)>1e-4; ++x) {
    std::cout << "Iteration " << x << '\n';
    std::vector<double> nextVec(n, 0.0);
    for(size_t i=0; i!=n; ++i) {
      double weightSum = 0.0;
      for(size_t j=0; j!=n; ++j) {
	if(i!=j && estVec[j]!=0.0) {
	  double w = estVec[j];
	  // matrix / estVec, but since we weight with estVec, just:
	  nextVec[i] += matrix[i*n+j];
	  weightSum += w;
	}
      }
      nextVec[i] /= weightSum;
    }

    double maxVal = 0.0;
    for(size_t i=0; i!=n; ++i) {
      estVec[i] = nextVec[i]*0.8 + estVec[i]*0.2;
      maxVal = std::max(estVec[i], maxVal);
    }
    for(size_t i=0; i!=n; ++i) {
      if(estVec[i] < maxVal*1e-5)
	estVec[i] = 0.0;
    }
    
  }
  outputV(estVec);
}

