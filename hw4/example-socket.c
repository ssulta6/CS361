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
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    // fill up IP address
    inet_pton(addr.sin_family, "8.8.8.8", &(addr.sin_addr));
    
    // create a client TCP socket
    int client_sock = socket(addr.sin_family, SOCK_STREAM, 0);
    if(client_sock < 0) {
        perror("Creating socket failed");
        exit(1);
    }

    int conn = connect(client_sock, (struct sockaddr *) &addr, sizeof(addr));  
    if (conn < 0) {
        perror("Connecting to socket faild");
        exit(1);
    }
    
    char *msg = "tacocat";
    int sent = send(client_sock, msg, strlen(msg), 0); 
    // keep sending the remaining until it sends all
    while (sent < strlen(msg)) {
        sent = sent + send(client_sock, msg + sent, strlen(msg) - sent, 0);
    }

    /*
    char buf[100];
    int received = recv(client_sock, buf, 100, 0);
    while (received > 0) {
        buf[received] = '\0';
        printf("%s\n", buf);
        int rem = recv(client_sock, buf, 100, 0);

    }
    */
}
