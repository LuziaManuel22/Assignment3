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

int main() {
    calcFactArr(); // Calculate Factorial Array

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
		return 1;
	}

    // Create a socket
    int sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if(sockfd == -1) {
        printf("Error in creating socket\n");
        return 1;
    }

    // Bind the socket to the IP and PORT
    status = bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
    if(status == -1) {
        printf("Error in binding socket\n");
        return 1;
    }

    // Listen for connections
    status = listen(sockfd, 10);
    if(status == -1) {
        printf("Error in listening\n");
        return 1;
    }
    printf("Listening for connections\n");
    // Accept a connection
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size = sizeof client_addr;

    // Initialize client_fd array
    for(int i = 0; i < MAX_CONNECTIONS; i++) {
        client_fd[i] = -1;
    }
    client_fd[0] = sockfd;

    while(true){
        FD_ZERO(&readfds);
        for(int i = 0; i < MAX_CONNECTIONS; i++) {
            if(client_fd[i] > -1) { // If client_fd[i] is a valid file descriptor
                FD_SET(client_fd[i], &readfds);
            }
        }
        
        printf("Waiting for connections\n");
        int ready = select(MAX_CONNECTIONS, &readfds, NULL, NULL, NULL);
        if(ready <= -1) {
            printf("Error in select\n");
            return 1;
        }

        if(FD_ISSET(sockfd, &readfds)) {
            int new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_size);
            if(new_fd == -1) {
                printf("Error in accepting connection\n");
                return 1;
            }
            printf("New connection accepted\n");
            for(int i = 0; i < MAX_CONNECTIONS; i++) {
                if(client_fd[i] == -1) {
                    client_fd[i] = new_fd;
                    break;
                }
            }

            if(ready > 1) continue; // If there are more than 1 ready file descriptors, then continue
        }

        for(int i = 0; i < MAX_CONNECTIONS; i++) {
            if(client_fd[i] > -1 && FD_ISSET(client_fd[i], &readfds)) {
                // Receive data from client
                unsigned int n;
                status = recv(client_fd[i], &n, sizeof(n), 0);
                if(status == -1) {
                    printf("Error in receiving data\n");
                    continue;
                }
                
                // Calculate factorial
                unsigned int fact_n = fact(n);

                // printf("Factorial of %u is %u\n", n, fact_n);

                // Send data to client
                status = send(client_fd[i], &fact_n, sizeof fact_n, 0);
                if(status == -1) {
                    printf("Error in sending data\n");
                    continue;
                }
                printf("Factorial of %u is %u\n", n, fact_n);

            }

        }


    }

    return 0;
}
