#include<iostream>
#include<vector>
#include "mystatistics.h"

int main(){
    std::vector<double> response_times = {10, 20, 30, 40, 50}; // 예시 데이터
    double sum = calculateSum(response_times);
    std::cout << "Sum: " << sum << std::endl;
}