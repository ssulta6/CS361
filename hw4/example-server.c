#include <fnmatch.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>


int main(int argc, char** argv) {


    // details of where server is
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(1234);
    // fill up IP server_address
    inet_pton(server_addr.sin_family, "0.0.0.0", &(server_addr.sin_addr));
    
    // create a client TCP socket
    int server_sock = socket(server_addr.sin_family, SOCK_STREAM, 0);
    if(server_sock < 0) {
        perror("Creating socket failed");
        exit(1);
    }

    if (bind(server_sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    listen(server_sock, 5);

    struct sockaddr_in client_addr;

    while(1) {
        int newsockfd = accept(server_sock, (struct sockaddr *) &client_addr, sizeof(client_addr));

        if (newsockfd < 0) {
            perror("error on accept");
            exit(1);
        }

        char buff[256];
        memset(buff, 0, 256);

        int received;
        received = recv(newsockfd, &buff, 256, 0);
        printf("received %d bytes: %s\n", received, buff);
        if (received < 0) {
            perror("ERROR reading from socket");
            exit(1);
        }

        int sent = send(newsockfd, &buff, received, 0);
        printf("sending %d bytes: %s\n", sent, buff);
        if (sent < 0) {
            perror("ERROR writing to socket");
            exit(1);
        }
        close(newsockfd);
    }
    close(server_sock);
}
