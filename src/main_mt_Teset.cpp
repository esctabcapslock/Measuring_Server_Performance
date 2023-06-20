#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_SOCKETS 1000
#define SERVER_ADDR "192.168.1.3"
#define SERVER_PORT 8080

void send_http_request(int sockfd) {
    char request[256];
    snprintf(request, sizeof(request), "GET / HTTP/1.1\r\nHost: %s:%d\r\n\r\n", SERVER_ADDR, SERVER_PORT);
    if (send(sockfd, request, strlen(request), 0) == -1) {
        perror("send");
        return;
    }
}

int main() {
    printf("main");
    

    int total_sockets = 1;  // 총 소켓 개수
    int requests_per_socket = 2000;  // 각 소켓당 요청 반복 횟수

    int sockets[MAX_SOCKETS];
    int num_requests_sent = 0;
    int num_responses_received = 0;

    
    // Create sockets and connect to the server
    for (int i = 0; i < total_sockets; i++) {
        printf("Create sockets %d\n", i);
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("socket");
            return 1;
        }

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SERVER_PORT);
        if (inet_pton(AF_INET, SERVER_ADDR, &server_addr.sin_addr) <= 0) {
            perror("inet_pton");
            return 1;
        }

        if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            perror("connect");
            return 1;
        }

        sockets[i] = sockfd;
    }

   printf("sockets created \n");
    // return 0;


      // Send HTTP GET requests asynchronously
    

    // Send HTTP GET requests asynchronously
    while (num_responses_received < total_sockets * requests_per_socket) {
        fd_set read_fds;
        FD_ZERO(&read_fds);

        int max_fd = -1;
        for (int i = 0; i < total_sockets; i++) {
            FD_SET(sockets[i], &read_fds);
            if (sockets[i] > max_fd) {
                max_fd = sockets[i];
            }
        }

        fd_set write_fds;
        FD_ZERO(&write_fds);

        for (int i = 0; i < total_sockets; i++) {
            if (num_requests_sent < total_sockets * requests_per_socket) {
                FD_SET(sockets[i], &write_fds);
            }
        }

        if (select(max_fd + 1, &read_fds, &write_fds, NULL, NULL) == -1) {
            perror("select");
            return 1;
        }

        for (int i = 0; i < total_sockets; i++) {
            if (FD_ISSET(sockets[i], &read_fds)) {
                char buffer[4096];
                int num_bytes = recv(sockets[i], buffer, sizeof(buffer) - 1, 0);
                if (num_bytes <= 0) {
                    if (num_bytes == 0) {
                        // 연결이 닫힌 경우
                        printf("Socket %d closed.\n", sockets[i]);
                        num_responses_received += requests_per_socket - (num_requests_sent % requests_per_socket);
                    } else {
                        // 오류가 발생한 경우
                        perror("recv");
                    }
                    close(sockets[i]);
                } else {
                    // 응답을 받은 경우
                    buffer[15] = '\0';
                    // buffer[num_bytes] = '\0';
                    // printf("Received response from socket %d:\nnum_responses_received: %d\n", sockets[i], num_responses_received);
                    printf("%dth, Received response from socket %d (len:%d): %s\n", num_responses_received, sockets[i], num_bytes, buffer);
                    num_responses_received++;
                }
            }

            if (FD_ISSET(sockets[i], &write_fds)) {
                send_http_request(sockets[i]);
                num_requests_sent++;
            }
        }
    }

    // Close the sockets
    for (int i = 0; i < total_sockets; i++) {
        close(sockets[i]);
    }

    return 0;
}