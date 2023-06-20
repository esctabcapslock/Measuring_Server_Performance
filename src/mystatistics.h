#ifndef MYSTATISTICS_H
#define MYSTATISTICS_H

#include <vector>
#include <numeric>
#include <cmath>
#include <algorithm>

// 벡터의 합을 계산하는 함수 선언
double calculateSum(const std::vector<double>& numbers);

// 벡터의 평균을 계산하는 함수 선언
double calculateMean(const std::vector<double>& numbers);

// 벡터의 표준편차를 계산하는 함수 선언
double calculateStandardDeviation(const std::vector<double>& numbers);

std::vector<double> calculateDifferences(const std::vector<double>& response_times);

double calculateMedian(const std::vector<double>& numbers);

std::pair<double, double> findMaxMin(const std::vector<double>& response_times);

#endif
