#include<stdlib.h>
#include <chrono>
#include <thread>
int main(){
    while(true){
         // 일시적으로 대기
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}