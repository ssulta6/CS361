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

#define BACKLOG (10)

char root_dir[256];
char curr_dir[256];

void serve_request(int);

char * request_str = "HTTP/1.0 200 OK\r\n"
        "Content-type: text/html; charset=UTF-8\r\n\r\n";


char * index_hdr = "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\"><html>"
        "<title>Directory listing for %s</title>"
"<body>"
"<h2>Directory listing for %s</h2><hr><ul>";

// snprintf(output_buffer,4096,index_hdr,filename,filename);


char * index_body = "<li><a href=\"%s\">%s</a>";

char * index_ftr = "</ul><hr></body></html>";

/* char* parseRequest(char* request)
 * Args: HTTP request of the form "GET /path/to/resource HTTP/1.X" 
 *
 * Return: the resource requested "/path/to/resource"
 *         0 if the request is not a valid HTTP request 
 * 
 * Does not modify the given request string. 
 * The returned resource should be free'd by the caller function. 
 */
char* parseRequest(char* request) {
    //assume file paths are no more than 256 bytes + 1 for null. 
    char *buffer = malloc(sizeof(char)*257);
    memset(buffer, 0, 257);

    if(fnmatch("GET * HTTP/1.*",  request, 0)) return 0; 

    sscanf(request, "GET %s HTTP/1.", buffer);

    printf("parseRequest: %s\n", buffer);
    return buffer; 
}

// send file_fd to socket client_fd
void serve_file(int file_fd, int client_fd) {
    int bytes_read;
    char send_buf[4096];
    while(1){
        bytes_read = read(file_fd ,send_buf,4096);
        printf("read %d bytes: %s\n", bytes_read, send_buf);
        if(bytes_read == 0)
            break;
        if (bytes_read == -1) {
            printf("error %s\n", strerror(errno));
            break;
        }
        int sent = send(client_fd,send_buf,bytes_read,0);
        // if we didn't send them all, send the remainder
        while (sent < bytes_read) {
            sent = sent + send(client_fd, send_buf + sent, bytes_read - sent, 0);
        }
    }
}

// TODO
// generate an HTML page with directory listing, write it to a file,
// then send it
void serve_listing(char* dirpath) {

    // generate HTML page

    // write it to a temp file

    // send it

    return;
}

// taken from http://stackoverflow.com/a/4553053/341505
// returns true if path is directory
int is_directory(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0)
        return 0;
    return S_ISDIR(statbuf.st_mode);
}

void serve_request(int client_fd){
    int read_fd;
    int file_offset = 0;
    char client_buf[4096];
    char * requested_file;
    memset(client_buf,0,4096);
    while(1){

        file_offset += recv(client_fd,&client_buf[file_offset],4096,0);
        if(strstr(client_buf,"\r\n\r\n"))
            break;
    }
    requested_file = parseRequest(client_buf);
    send(client_fd,request_str,strlen(request_str),0);

    printf("response: %s\n", request_str);
    // take requested_file, add a . to beginning, open that file

    // now construct filepath string using curr_dir + root_dir + filename
    char filepath[8096];
    snprintf(filepath, 8096, "%s/%s%s", curr_dir, root_dir, requested_file);
    printf("filepath: %s\n", filepath);

    if (is_directory(filepath)) {
        printf("is directory!\n");
        // check if index.html exists and serve that
        char indexpath[8096];
        snprintf(indexpath, 8096, "%s/index.html", filepath);
        read_fd = open(indexpath,0 ,0);
        if (read_fd < 0) {
            // if file doesnt exist, serve a directory listing page instead
            struct stat buffer;
            if (stat(indexpath, &buffer) < 0 && errno == ENOENT) {
                serve_listing(filepath);
                printf("should serve directory listing!\n");
                return;
            } else {
                printf("can't open index.html, error: %s\n", strerror(errno));
                close(read_fd);
                return;
            }
            // the index.html will be served
        }    

        // if not directory, serve the file
    } else {
        printf("is NOT directory!\n");
        read_fd = open(filepath,0,0);
        if (read_fd < 0) { 

            // serve the 404 file if you can't find the file
            struct stat buffer;
            if (stat(filepath, &buffer) < 0 && errno == ENOENT) {
                char newpath[8096];
                snprintf(newpath, 8096, "%s/%s/404.html", curr_dir, root_dir);
                close(read_fd);
                read_fd = open(newpath, 0, 0);
                if (read_fd < 0) {
                    printf("can't open 404.html at %s, error: %s\n", newpath, strerror(errno));
                    close(read_fd);
                    return;
                }
                printf("serving 404 html!\n");
            } else {
                printf("error getting 404.html: %s\n", strerror(errno));
                close(read_fd);
                return;
            }
        }
    }
    // serve file
    serve_file(read_fd, client_fd);
    
    close(read_fd);
    return;
}


// TODO handle each incoming client in its own thread
// TODO serve directory listing when given a directory without an index file
// TODO name this web server Abashe for lulz
// TODO serve correct content header depending on file type

// Your program should take two arguments:
/* 1) The port number on which to bind and listen for connections, and
 * 2) The directory out of which to serve files.
 */
int main(int argc, char** argv) {
   
    
    /* For checking return values. */
    int retval;

    /* Read the port number from the first command line argument. */
    int port = atoi(argv[1]);

    strncpy(root_dir, argv[2], 255);
    
    // find current directory
    getcwd(curr_dir, 256);
    if (curr_dir == NULL) {
        printf("failed to get working directory error: %s\n", strerror(errno));
        exit(1);
    }
    char dir[8096];
    snprintf(dir, 8096, "%s/%s", curr_dir, root_dir);
    printf("Welcome to Abashe, Basheer's web server! We are currently running on port %d and our root directory is %s\n", port, dir);
    /* Create a socket to which clients will connect. */
    int server_sock = socket(AF_INET6, SOCK_STREAM, 0);
    if(server_sock < 0) {
        perror("Creating socket failed");
        exit(1);
    }

    /* A server socket is bound to a port, which it will listen on for incoming
     * connections.  By default, when a bound socket is closed, the OS waits a
     * couple of minutes before allowing the port to be re-used.  This is
     * inconvenient when you're developing an application, since it means that
     * you have to wait a minute or two after you run to try things again, so
     * we can disable the wait time by setting a socket option called
     * SO_REUSEADDR, which tells the OS that we want to be able to immediately
     * re-bind to that same port. See the socket(7) man page ("man 7 socket")
     * and setsockopt(2) pages for more details about socket options. */
    int reuse_true = 1;
    retval = setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &reuse_true,
            sizeof(reuse_true));
    if (retval < 0) {
        perror("Setting socket option failed");
        exit(1);
    }

    /* Create an address structure.  This is very similar to what we saw on the
     * client side, only this time, we're not telling the OS where to connect,
     * we're telling it to bind to a particular address and port to receive
     * incoming connections.  Like the client side, we must use htons() to put
     * the port number in network byte order.  When specifying the IP address,
     * we use a special constant, INADDR_ANY, which tells the OS to bind to all
     * of the system's addresses.  If your machine has multiple network
     * interfaces, and you only wanted to accept connections from one of them,
     * you could supply the address of the interface you wanted to use here. */


    struct sockaddr_in6 addr;   // internet socket address data structure
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port); // byte order is significant
    addr.sin6_addr = in6addr_any; // listen to all interfaces


    /* As its name implies, this system call asks the OS to bind the socket to
     * address and port specified above. */
    retval = bind(server_sock, (struct sockaddr*)&addr, sizeof(addr));
    if(retval < 0) {
        perror("Error binding to port");
        exit(1);
    }

    /* Now that we've bound to an address and port, we tell the OS that we're
     * ready to start listening for client connections.  This effectively
     * activates the server socket.  BACKLOG (#defined above) tells the OS how
     * much space to reserve for incoming connections that have not yet been
     * accepted. */
    retval = listen(server_sock, BACKLOG);
    if(retval < 0) {
        perror("Error listening for connections");
        exit(1);
    }

    while(1) {
        /* Declare a socket for the client connection. */
        int sock;

        /* Another address structure.  This time, the system will automatically
         * fill it in, when we accept a connection, to tell us where the
         * connection came from. */
        struct sockaddr_in remote_addr;
        unsigned int socklen = sizeof(remote_addr); 

        /* Accept the first waiting connection from the server socket and
         * populate the address information.  The result (sock) is a socket
         * descriptor for the conversation with the newly connected client.  If
         * there are no pending connections in the back log, this function will
         * block indefinitely while waiting for a client connection to be made.
         * */
        sock = accept(server_sock, (struct sockaddr*) &remote_addr, &socklen);
        if(sock < 0) {
            perror("Error accepting connection");
            exit(1);
        }

        /* At this point, you have a connected socket (named sock) that you can
         * use to send() and recv(). */

        /* ALWAYS check the return value of send().  Also, don't hardcode
         * values.  This is just an example.  Do as I say, not as I do, etc. */
        serve_request(sock);

        /* Tell the OS to clean up the resources associated with that client
         * connection, now that we're done with it. */
        close(sock);
    }

    close(server_sock);
}
