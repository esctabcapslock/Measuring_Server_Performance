#include "mystatistics.h"


// 벡터의 합을 계산하는 함수 구현
double calculateSum(const std::vector<double>& numbers) {
    return std::accumulate(numbers.begin(), numbers.end(), 0.0);
}

// 벡터의 평균을 계산하는 함수 구현
double calculateMean(const std::vector<double>& numbers) {
    double sum = calculateSum(numbers);
    return static_cast<double>(sum) / numbers.size();
}

// 벡터의 표준편차를 계산하는 함수 구현
double calculateStandardDeviation(const std::vector<double>& numbers) {
    double mean = calculateMean(numbers);
    double variance = 0.0;

    for (int number : numbers) {
        variance += pow(number - mean, 2);
    }
    variance /= numbers.size();

    return sqrt(variance);
}


double calculateMedian(const std::vector<double>& numbers) {
    std::vector<double> sortedNumbers = numbers;
    std::sort(sortedNumbers.begin(), sortedNumbers.end());

    size_t size = sortedNumbers.size();
    if (size % 2 == 0) {
        // 짝수 개의 원소일 경우 중앙값은 두 중간값의 평균
        double mid1 = sortedNumbers[size / 2 - 1];
        double mid2 = sortedNumbers[size / 2];
        return (mid1 + mid2) / 2.0;
    } else {
        // 홀수 개의 원소일 경우 중앙값은 중간 값
        return sortedNumbers[size / 2];
    }
}


std::vector<double> calculateDifferences(const std::vector<double>& response_times) {
    // std::vector<double> a(response_times.size());

    // if (response_times.size() > 0) {
    //     a[0] = response_times[0];
    //     for (size_t i = 1; i < response_times.size(); i++) {
    //         a[i] = response_times[i] - response_times[i - 1];
    //     }
    // }

    // 크기 줄이기
    std::vector<double> a(response_times.size()-1);

    if (response_times.size() > 0) {
        for (size_t i = 1; i < response_times.size(); i++) {
            a[i-1] = response_times[i] - response_times[i - 1];
        }
    }

    return a;
}


std::pair<double, double> findMaxMin(const std::vector<double>& response_times) {
    if (response_times.empty()) {
        // 빈 벡터인 경우
        return std::make_pair(0.0, 0.0); // 또는 다른 적절한 값을 반환할 수 있음
    }
    
    // 최댓값과 최솟값을 초기화
    double maxVal = response_times[0];
    double minVal = response_times[0];
    
    // 벡터를 순회하며 최댓값과 최솟값 갱신
    for (const auto& val : response_times) {
        maxVal = std::max(maxVal, val);
        minVal = std::min(minVal, val);
    }
    
    return std::make_pair(maxVal, minVal);
}