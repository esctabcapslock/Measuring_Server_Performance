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
#include "serverinfo.h"

#include <signal.h>
// #include <Eigen/Core>

// 소켓 끊김 시그널 무시
void ignoreSIGPIPE()
{
    signal(SIGPIPE, SIG_IGN);
}

namespace plt = matplotlibcpp;

constexpr int NUM_REQUESTS = 100000;  // 스래드당 전송 개수.
constexpr int NUM_CLIENTS = 4;     // 스레드의 개수
constexpr int COUNT_PER_REQ = 100; // 몇개의 요청당 시간을 기록할건인가.

std::atomic<int> completed_requests(0);
std::atomic<int> total_requests(0);
std::atomic<int> connection_failed_CNT(0);
std::mutex mutex_completed_requests;
std::mutex mutex_total_requests;
std::condition_variable cv_completed_requests;
std::vector<double> response_times;

decltype(std::chrono::high_resolution_clock::now()) start_time; // 시간계산
const std::string serverAddressStr = SERVER_ADDRESS;

void clientThread() {

    // std::cout << "[clientThread]" << std::endl;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }

    struct sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);

    // if (inet_pton(AF_INET, "[my-servre-ip]", &(server_address.sin_addr)) <= 0)
    if (inet_pton(AF_INET, serverAddressStr.c_str(), &(server_address.sin_addr)) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        close(sock);
        return;
    }

    // std::cout << "[clientThread1]" << std::endl;

    if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        close(sock);

        {
            std::unique_lock<std::mutex> lock(mutex_completed_requests);
            ++connection_failed_CNT;
        }

        return;
    }

    // Send requests to the server
    for (int i = 0; i < NUM_REQUESTS; ++i)
    {
        // Simulate some workload by sleeping for a short time
        // std::this_thread::sleep_for(std::chrono::milliseconds(1));

        // Send a GET request to the server
        std::string request = "GET /robots.txt HTTP/1.1\r\n";
        // std::string request = "GET / HTTP/1.1\r\n";
        request += "Host: " + serverAddressStr + "\r\n";
        request += "Connection: keep-alive\r\n";
        request += "User-Agent: Mozilla/5.0\r\n";
        request += "\r\n";

        // std::cout << i << "th, req:" << request << std::endl;

        send(sock, request.c_str(), request.size(), 0);

        const int bufferSize = 4096; // 적절한 버퍼 크기를 선택하세요
        char buffer[bufferSize];

        int bytesRead = recv(sock, buffer, bufferSize - 1, 0);
        if (bytesRead == -1){
            // 에러 처리
            std::cout << "no answer" << std::endl;
            break;
        }
        else {
            // 수신된 데이터를 처리합니다.
            buffer[bytesRead] = '\0'; // 문자열 종료 문자('\0')를 추가합니다.
            std::string response(buffer);

            {
                std::unique_lock<std::mutex> lock(mutex_total_requests);
                ++total_requests;
                if (total_requests % COUNT_PER_REQ == 0) {
                    std::cout << "o" << std::flush;
                    // std::cout << total_requests << std::endl;

                    auto end_time = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
                    double response_time = static_cast<double>(duration); // / (NUM_REQUESTS);
                    response_times.push_back(response_time);
                }
            }
        }
    }
    close(sock);

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

    for (int i = 0; i < NUM_CLIENTS; ++i) {
        client_threads.emplace_back(clientThread);
    }

    // Wait for all client threads to finish
    try {
        for (auto &thread : client_threads) thread.join();
    } catch (const std::exception &ex) {
        std::cout << "Exception in  thread.join();: " << std::endl;
        std::cout << ex.what() << std::endl;
    }

    // Wait for all requests to be completed
    {
        std::unique_lock<std::mutex> lock(mutex_completed_requests);
        cv_completed_requests.wait(lock, []
                                   { return completed_requests + connection_failed_CNT == NUM_CLIENTS; });
    }

    std::cout << std::endl;

    // Performance measurement
    std::cout << "Total requests: " << (NUM_REQUESTS * NUM_CLIENTS) << std::endl;
    std::cout << "Connection Cunnect sccceased: " << NUM_CLIENTS - connection_failed_CNT << "/" << NUM_CLIENTS << std::endl;
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
                            + "\nCOUNT_PER_REQ = " + std::to_string(COUNT_PER_REQ) 
                            + "\nCOUNT_PER_REQ = " + std::to_string(COUNT_PER_REQ) 
                            + "\nCOUNT_USING_SOCKET = " + std::to_string(NUM_CLIENTS - connection_failed_CNT) 
                            + "\nTrend Line: y = " + std::to_string(x(0)) + " + " + std::to_string(x(1)) + "x";

    double px = indices.front();
    double py = response_times.front() * 0.8;
    plt::text(px, py, showouttext);

    // 그래프 출력
    plt::show();

    return 0;
}