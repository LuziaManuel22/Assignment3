#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Include Socket Programming Library
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/epoll.h>

// Thread Library
#include <pthread.h>


// Network Constants
const char* PORT = "8081";
const int MAX_CONNECTIONS = 1024;

struct thread_args {
    int new_fd;
    int sockfd;
};


unsigned int factArr[21]; // Factorial Array

void calcFactArr() {
    factArr[0] = 1;
    for (int i = 1; i < 21; i++) {
        factArr[i] = factArr[i - 1] * i;
    }
}

unsigned int fact(int n) {
    if(n < 0) n = 0;
    if(n > 20) n = 20;
    return factArr[n];
}

int initSocket() {
    // Socket Programming : Server
    // Fill the information about the server
    int status;
    struct addrinfo hints, *servinfo, *p;

    int client_fd[MAX_CONNECTIONS];
    fd_set readfds;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        printf("getaddrinfo: %s\n", gai_strerror(status));
        return -1;
    }

    // Create a socket
    int sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if(sockfd == -1) {
        printf("Error in creating socket\n");
        return -1;
    }

    // Bind the socket to the IP and PORT
    if(bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        printf("Error in binding\n");
        return -1;
    }

    return sockfd;


}

int main() {
    calcFactArr(); // Calculate Factorial Array

    // Socket Programming : Server
    // Fill the information about the server : Going to use poll() instead of select()
    int status;
    int server_socket;
    struct epoll_event events[MAX_CONNECTIONS];
    int sockfd = initSocket();

        // Listen for connections
    if(listen(sockfd, MAX_CONNECTIONS) == -1) {
        printf("Error in listening\n");
        return -1;
    }

    // Create epoll instance
    int epollfd = epoll_create1(0);
    if(epollfd == -1) {
        printf("Error in epoll_create1()\n");
        return -1;
    }

    // Add server socket to epoll
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = sockfd;
    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &event) == -1) {
        printf("Error in epoll_ctl()\n");
        return -1;
    }

    while (true)
    {
        int num_fds = epoll_wait(epollfd, events, MAX_CONNECTIONS, -1);
        if(num_fds == -1) {
            printf("Error in epoll_wait()\n");
            return -1;
        }

        for(int i = 0; i < num_fds; i++){
            if(events[i].data.fd == sockfd) {
                // Accept new connection
                struct sockaddr_storage client_addr;
                socklen_t addr_size = sizeof client_addr;
                int new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_size);
                if(new_fd == -1) {
                    printf("Error in accept()\n");
                    return -1;
                }

                // Add new_fd to epoll
                event.events = EPOLLIN;
                event.data.fd = new_fd;
                if(epoll_ctl(epollfd, EPOLL_CTL_ADD, new_fd, &event) == -1) {
                    printf("Error in epoll_ctl()\n");
                    return -1;
                }

            } else {
                // Handle data from client
                int new_fd = events[i].data.fd;
                unsigned int n;
                int status = recv(new_fd, &n, sizeof n, 0);
                if(status == -1) {
                    printf("Error in receiving data\n");
                    continue;
                }

                // Calculate factorial
                unsigned int fact_n = fact(n);

                // printf("Factorial of %u is %u\n", n, fact_n);

                // Send data to client
                status = send(new_fd, &fact_n, sizeof fact_n, 0);
                if(status == -1) {
                    printf("Error in sending data\n");
                    continue;
                }

            }

        }
    }

}
