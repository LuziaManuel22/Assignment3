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
#include <poll.h>

// Thread Library
#include <pthread.h>


// Network Constants
const char* PORT = "8081";
const int MAX_CONNECTIONS = 5000;

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
    int client_socket[MAX_CONNECTIONS];
    struct pollfd fds[MAX_CONNECTIONS];
    int sockfd = initSocket();

        // Listen for connections
    if(listen(sockfd, MAX_CONNECTIONS) == -1) {
        printf("Error in listening\n");
        return -1;
    }

    // Initialize fds
    fds[0].fd = sockfd;
    fds[0].events = POLLIN;

    server_socket = sockfd;

    for(int i = 1; i < MAX_CONNECTIONS; i++) {
        fds[i].fd = -1;
        client_socket[i] = -1;
    }

    while (true)
    {
        int nfds = 1;
        for(int i = 1; i < MAX_CONNECTIONS; i++) {
            int tmp = client_socket[i];
            if(tmp == -1) {
               continue;
            }
            nfds++;
            fds[i].events = POLLIN;
            fds[i].fd = tmp;
        }

        int activity = poll(fds, nfds, -1);
        if(activity < 0) {
            printf("Error in poll()\n");
            return -1;
        }

        if(fds[0].revents & POLLIN) {
            int new_fd = accept(server_socket, NULL, NULL);
            if(new_fd == -1) {
                printf("Error in accepting connection\n");
                return -1;
            }
            for(int i = 1; i < MAX_CONNECTIONS; i++) {
                if(client_socket[i] == -1) {
                    // printf("New connection accepted\n");
                    client_socket[i] = new_fd;
                    break;
                }
            }
        }

        for(int i = 1; i < MAX_CONNECTIONS; i++) {
            int tmp = client_socket[i];
            if(tmp == -1) {
                continue;
            }
            if(fds[i].revents & POLLIN) {
                int n;
                status = recv(tmp, &n, sizeof(n), 0);
                if(status == -1) {
                    printf("Error in receiving data\n");
                    // close(tmp);
                    client_socket[i] = -1;
                    continue;
                }
                if(status == 0) {
                    printf("Connection closed\n");
                    // close(tmp);
                    client_socket[i] = -1;
                    continue;
                }
                // Calculate factorial
                unsigned int fact_n = fact(n);

                // printf("Factorial of %u is %u\n", n, fact_n);

                // Send data to client
                status = send(tmp, &fact_n, sizeof fact_n, 0);
                if(status == -1) {
                    printf("Error in sending data\n");
                    // close(tmp);
                    client_socket[i] = -1;
                    // return -1;
                    continue;
                }
                
            }
        }
        // printf("Waiting for connections\n");
    }

}
