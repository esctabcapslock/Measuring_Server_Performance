#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <Eigen/Dense>

#include "matplotlibcpp.h"
#include "mystatistics.h"

#include <signal.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/select.h>

#include<stdio.h>
#include "serverinfo.h"

// #include <Eigen/Core>

// 소켓 끊김 시그널 무시
void ignoreSIGPIPE()
{
    signal(SIGPIPE, SIG_IGN);
}

namespace plt = matplotlibcpp;

constexpr int NUM_REQUESTS = 20000;  // 소켓당 http 전송 개수.
constexpr int NUM_THREAD = 1;     // 스레드의 개수
constexpr int NUM_SOCKET = 8;     // 스레드당 관리할 소켓의 개수
constexpr int COUNT_PER_REQ = 100; // 몇개의 요청당 시간을 기록할건인가.

std::atomic<int> completed_requests(0);
std::atomic<int> total_requests(0);
std::atomic<int> connection_failed_CNT(0);
std::atomic<int> connection_abult_CNT(0); //http응답이 강제로 끊긴경우,
std::mutex mutex_completed_requests;
std::mutex mutex_error_requests;
std::mutex mutex_total_requests;
std::condition_variable cv_completed_requests;
std::vector<double> response_times;

decltype(std::chrono::high_resolution_clock::now()) start_time; // 시간계산

// 서버설정
const std::string serverAddressStr = SERVER_ADDRESS;
// const int serverPort = 80;
const int serverPort = SERVER_PORT;
// const int serverPort = 8012;



bool sendHttpRequest(int sockfd, int cnt=-1) {
    // HTTP 요청 메시지 작성
    std::string request = "GET /robots.txt HTTP/1.1\r\nHost: " + serverAddressStr + "\r\nConnection: keep-alive\r\n\r\n";
    // std::string request = "GET /index.html HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";

    // 소켓으로 HTTP 요청 보내기
    ssize_t bytesSent = send(sockfd, request.c_str(), request.length(), 0);
    if (bytesSent < 0) {
        std::cerr << "Failed to send HTTP request " << sockfd << ":" << std::endl;
        return false;
    } else {
        // std::cerr << "Succesed to send HTTP request " << sockfd << ":" << std::endl;
        // std::cout <<  "["+std::to_string(sockfd)+"]"  << std::flush;
        
        return true;}
}

void addReqCount(){
    
    {
        std::unique_lock<std::mutex> lock(mutex_total_requests);
        ++total_requests;
        if (total_requests % COUNT_PER_REQ == 0) {
            std::cout << "o" << std::flush;

            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            double response_time = static_cast<double>(duration); // / (NUM_REQUESTS);
            response_times.push_back(response_time);
        }
    }
}

int processHttpResponse(int sockfd, int cnt = -1) {
    // std::cout << "[processHttpResponse]" << std::endl;
    // HTTP 응답 받아오기
    const int bufferSize = 4096;
    char buffer[bufferSize];
    std::string response;
    
    // while (true) {
    //     memset(buffer, 0, bufferSize);
    //     ssize_t bytesRead = recv(sockfd, buffer, bufferSize - 1, 0);
    //     if (bytesRead == 0) {

    //         // std::cerr << "socket이 강제로 닫힘" << std::endl;
    //         // return -1;
    //     }else if (bytesRead == -1){
    //         std::cerr << "Failed to receive data from socket" << std::endl;
    //         return -1;
    //     }
    //     response += buffer;
    // }

   

    while (true) {
        memset(buffer, 0, bufferSize);
        ssize_t bytesRead = recv(sockfd, buffer, bufferSize - 1, 0);
        
        if (bytesRead == 0) {
            // 소켓이 닫힘
            return -1;
        }
        else if (bytesRead == -1) {
            // 읽기 에러 발생
            std::cerr << "Failed to receive data from socket id:" + std::to_string(sockfd) << std::endl;

            {
            std::unique_lock<std::mutex> lock(mutex_error_requests);
            ++connection_failed_CNT;
            }

            return -1;
        }
        else {
            // 응답 데이터가 있음
            response += buffer;
            
            // 여기에서 필요한 응답 처리를 수행
            
            // 데이터를 모두 읽었을 경우 반복문 종료
            if (bytesRead < bufferSize - 1) {
                break;
            }
        }
    }

    if (response.size() == 0) return 0; //  응답 데이터가 없는경우.

    // std::string response_print;//
    // std::string replacedStr;
    // for (char c : response) {
    //     if (c == '\n')
    //         response_print += "\\n";
    //     else if (c == '\r')
    //         response_print += "\\r";
    //     else
    //         response_print += c;
    // }

    // std::string prefix = "!HTTP/1.1 200 OK";
    // std::string prefix2 = "!<!DOCTYPE html>";
    // std::string prefix3 = "!HTTP/1.1 500";
    // if (response.compare(0, prefix.length(), prefix) != 0 
    // && response.compare(0, prefix2.length(), prefix2) != 0
    // && response.compare(0, prefix3.length(), prefix3) != 0
    // ) {
    
    // int len = 
        // printf("?th, Received response from socket %d (len:%d): %s\n", sockfd, len, str.substr(0, 13));
        // std::cout << std::to_string(cnt) << "th Received response from socket " << sockfd << "(len:" <<  std::to_string(response.size()) << "):" << ":" << response_print << std::endl;
        //std::cout << std::to_string(cnt) << "th Received response from socket " << sockfd << "(len:" <<  std::to_string(response.size()) << "):" << ":" << response.substr(0, 13) << std::endl;
        // std::cout << "Received response from socket " << sockfd << "(len:" <<  std::to_string(response.size()) << "):" << ":" << response_print << std::endl;
        // std::cout << "Received response from socket " << sockfd << ":" << std::endl;
        // std::cout << std::to_string(response.size()) +  "=======<< ans >>==============\n\r"+response+"\n\r===========================" << std::endl;
    // }
    addReqCount();
    return 1;
    
}





int createsocket(){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    // std::cout << "[createsocket] sockfd:" << std::to_string(sockfd) << std::endl;
    if (sockfd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return -1;
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);
    if (inet_pton(AF_INET, serverAddressStr.c_str(), &(serverAddress.sin_addr)) <= 0) {
        std::cerr << "Failed to set server address" << std::endl;
        return -1;
    }

    if (connect(sockfd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        std::cerr << "Failed to connect socket" << std::endl;
        {
            std::unique_lock<std::mutex> lock(mutex_completed_requests);
            ++connection_failed_CNT;
        }
        // continue;
        return -1;
    }

    // 비차단(non-blocking) 모드로 소켓 설정
    // 그라니까 소켓에 대해 비동기처리하겠다는 뜻
    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    return sockfd;
}


void clientThread() {

    // 소켓들을 저장할 벡터 생성
    std::vector<int> sockets(NUM_SOCKET, -1);

    // 각 소켓별로 요청한 횟수 적어두기
    std::vector<int> req_count(NUM_SOCKET, NUM_REQUESTS); // 길이 NUM_SOCKET, 초기화 NUM_REQUESTS

    // 만약 만료된다면, 새로운 소켓을 할당할것인가 / 아님 버릴것인가.

    // 소켓 생성 및 연결
    for (int i = 0; i < NUM_SOCKET; ++i) {

        int sockfd = createsocket();

        // 생성된 소켓을 벡터에 저장
        sockets[i] = sockfd;
    }

    std::cout << "sockets created " << std::endl;

    for (int i = 0; i < NUM_SOCKET; ++i) {
        // 각 소켓마다 HTTP 요청 보내기
        sendHttpRequest(sockets[i]);
    }

    
    int start_for = 0;

    while (true) {
        // select()를 이용하여 비동기적으로 소켓 이벤트 처리
        fd_set readfds, writefds;
        FD_ZERO(&readfds);
        int maxfd = -1;
        for (int i = 0; i < NUM_SOCKET; ++i) {
            if (sockets[i] != -1) {
                FD_SET(sockets[i], &readfds);
                if (sockets[i] > maxfd)
                    maxfd = sockets[i];
            }
        }
        struct timeval timeout;
        timeout.tv_sec = 10;  // 타임아웃 설정. 10초
        timeout.tv_usec = 0; //  // 타임아웃 설정. 0 마이크로초

        int activity = select(maxfd + 1, &readfds, nullptr, nullptr, &timeout);
        if (activity < 0) {
            std::cerr << "Failed to select socket" << std::endl;
            break;
        }

        if (activity == 0) {
            // 타임아웃 발생
             std::cerr << "타임아웃 발생" << std::endl;
            // continue;
            return;
        }

        // std::cout << " for [소켓 읽기 확인]" << std::endl;
        for (int j = 0; j < NUM_SOCKET; ++j) {
            int i = (j + start_for)%NUM_SOCKET;
            if (sockets[i] != -1 && FD_ISSET(sockets[i], &readfds)) {
                // 소켓에서 읽을 데이터가 있음
                int ans = processHttpResponse(sockets[i], req_count[i]);

                // std::string A = std::to_string(req_count.size())+":길이/,";
                // for (auto it:req_count) A += (std::to_string(it)+",");
                // A += '|';
                // std::cout << A << std::endl;

                if (ans == 0) continue; // 걍 응답이 없는경우
                else if (ans == -1) { // 소켓 자체가 닫힌경우

                    // std::cout << "소켓 자체가 닫힌경우" << std::endl;
                    close(sockets[i]);
                    sockets[i] = createsocket();

                    if (sockets[i] == -1){
                        std::cout << "소켓 재생성 실패!" << std::endl;
                        req_count[i] = -1;
                        continue;
                    }
                    // continue;
                }

                // 아직 소켓이 살아있는 경우

                // 요청 횟수 확인.
                // std::cout << "요청수확인" + std::to_string(i) + "th / cnt:"  + std::to_string(req_count[i]) + ", "  << std::endl;
                if (req_count[i]-- > 0){
                   if (sendHttpRequest(sockets[i]), req_count[i]) {
                        continue;
                    }
                }

                // 소켓에 할당된 요청을 모두 다한 경우 & 전송상에 오류가 발생한 경우
                std::cout << "소켓에 할당된 요청을 모두 다한 경우, id:" + std::to_string(sockets[i]) << std::endl;
                close(sockets[i]);
                sockets[i] = -1;
                
            }
        }

        start_for++;

        // 모든 소켓이 닫힌 경우 반복 종료
        bool allClosed = true;
        for (int i = 0; i < NUM_SOCKET; ++i) {
            if (sockets[i] != -1) {
                allClosed = false;
                break;
            }
        }
        if (allClosed) {
            std::cout << "모든 소켓이 닫힘" << std::endl;
            break;
        }
    }

    // Increment the count of completed requests
    {
        std::unique_lock<std::mutex> lock(mutex_completed_requests);
        // response_times.push_back(response_time);
        ++completed_requests;
    }
    cv_completed_requests.notify_one();
}

int main() {
    std::cout << "start" << std::endl;

    
    // 소켓 끊겨도 무시
    ignoreSIGPIPE();

    // Start the client threads
    std::vector<std::thread> client_threads;
    start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_THREAD; ++i) {
        client_threads.emplace_back(clientThread);
    }

    // Wait for all client threads to finish
    try {
        for (auto &thread : client_threads) thread.join();
    } catch (const std::exception &ex) {
        std::cout << "Exception in  thread.join();: " << std::endl;
        std::cout << ex.what() << std::endl;
    }

    // // Wait for all requests to be completed
    // {
    //     std::unique_lock<std::mutex> lock(mutex_completed_requests);
    //     cv_completed_requests.wait(lock, []
    //                                { return completed_requests + connection_failed_CNT == NUM_THREAD*NUM_SOCKET; });
    // }


    // Performance measurement
    std::cout << std::endl;
    std::cout << "Total requests: " << total_requests << "/" << (NUM_REQUESTS * NUM_THREAD * NUM_SOCKET )  << std::endl;
    std::cout << "Connection Cunnect sccceased: " << NUM_THREAD*NUM_SOCKET - connection_failed_CNT << "/" << NUM_THREAD*NUM_SOCKET << std::endl;
    std::cout << "connection_failed_CNT: " << connection_failed_CNT << std::endl;
    response_times = calculateDifferences(response_times);

    std::cout << "Sum: " << calculateSum(response_times) << std::endl;
    std::cout << "Mean: " << calculateMean(response_times) << std::endl;
    std::cout << "Standard Deviation: " << calculateStandardDeviation(response_times) << std::endl;

    std::vector<int> indices(response_times.size());
    std::iota(indices.begin(), indices.end(), 0); // 0부터 response_times.size()-1까지의 값을 가지는 벡터 생성
    plt::plot(indices, response_times);

    // 추세선 피팅
    // Eigen::Matrix
    Eigen::MatrixXd A(indices.size(), 2);
    Eigen::VectorXd b(indices.size());
    for (size_t i = 0; i < indices.size(); ++i)
    {
        A(i, 0) = 1.0;
        A(i, 1) = indices[i];
        b(i) = response_times[i];
    }
    Eigen::VectorXd x = A.colPivHouseholderQr().solve(b);
    std::vector<double> trend_line(indices.size());
    for (size_t i = 0; i < indices.size(); ++i)
    {
        trend_line[i] = x(0) + x(1) * indices[i];
    }

    // 추세선 그래프 그리기
    plt::plot(indices, trend_line, "r--");

    //// 평균과 추세선의 식 표시
    std::string showouttext = "Mean: y = " + std::to_string(calculateMean(response_times)) 
                            + "\nsd = " + std::to_string(calculateStandardDeviation(response_times)) 
                            + "\nmedian = " + std::to_string(calculateMedian(response_times)) 
                            + "\nNUM_REQUESTS = " + std::to_string(NUM_REQUESTS) 
                            + "\nNUM_THREAD = " + std::to_string(NUM_THREAD) 
                            + "\nNUM_SOCKET = " + std::to_string(NUM_SOCKET) 
                            + "\nCOUNT_PER_REQ = " + std::to_string(COUNT_PER_REQ) 
                            + "\nTotal requests: " + std::to_string(total_requests) + "/" + std::to_string(NUM_REQUESTS * NUM_THREAD * NUM_SOCKET )
                            + "\nconnection_failed_CNT: " + std::to_string(connection_failed_CNT);
                            + "\nCOUNT_USING_SOCKET = " + std::to_string(NUM_THREAD*NUM_SOCKET - connection_failed_CNT)  + "/" + std::to_string(NUM_THREAD*NUM_SOCKET);
                            + "\nTrend Line: y = " + std::to_string(x(0)) + " + " + std::to_string(x(1)) + "x";

    double px = indices.front();
    // double py = response_times.front() * 0.8;
    std::pair<double, double> yts = findMaxMin(response_times);
    double py = (yts.first + yts.second)/2;
    plt::text(px, py, showouttext);

    // 그래프 출력
    plt::show();

    return 0;
}