// The 'server.c' code goes here.
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#define PORT 9999
#include "Md5.c"  // Feel free to include any other .c files that you need in the 'Server Domain'.
//#define DEBUG
int connect_server(char* ip, int* server_socket){
    //connect_server returns client_socket and fills in server_socket
	int client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);


    // Creating socket file descriptor
    *server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (*server_socket == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int socket_status = setsockopt(*server_socket, SOL_SOCKET, SO_REUSEADDR , &opt, sizeof(opt));
    if (socket_status) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
	//address.sin_addr.s_addr = INADDR_ANY;
	//INADDRE_ANY is an IP address we use when we don't want to bind any specific ip address
    // The server IP address should be supplied as an argument when running the application.
    address.sin_addr.s_addr = inet_addr(ip);
    address.sin_port = htons(PORT);

    int bind_status = bind(*server_socket, (struct sockaddr*)&address, sizeof(address));
    if (bind_status < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    int listen_status = listen(*server_socket, 1);
    if (listen_status < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    client_socket = accept(*server_socket, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    if (client_socket < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
	
	return client_socket;
}

bool file_exists(const char * filename) {
    FILE* file;
    file = fopen(filename, "r");
    if (file != NULL) {
        #ifdef DEBUG
        printf("true\n");
        #endif

        fclose(file);
        return true;
    }
    #ifdef DEBUG
    printf("false\n");
    #endif
    return false;
}

void append(int connfd, char* buffer){
    char filePathName[100];
    bzero(filePathName, 100);
    FILE* fp;
    //check if file exists in remote directory
    //recieve[filename]
    bzero(buffer, 1024);
    recv(connfd, buffer, 1024, 0);
    #ifdef DEBUG
    printf("recieved: %s\n", buffer);
    #endif
    //buffer should contain filename
    strcat(filePathName, "./Remote Directory/");
    strcat(filePathName, buffer);

    #ifdef DEBUG
    printf("pathname: %s\n", filePathName);
    #endif

    if (file_exists(filePathName)) {
        //open for append and sent 1 for file exists
        FILE* fp = fopen(filePathName, "a");
        send(connfd, "1", strlen("1"), 0);
        //append content
        while(true){
            bzero(buffer, 1024);
            recv(connfd, buffer, 1024, 0);
            send(connfd, "DATA RECIEVED", strlen("DATA RECIEVED"), 0);
            #ifdef DEBUG
            printf("recieved: %s\n", buffer);
            #endif
            
            if(strcmp(buffer, "APPEND MODE CLOSED") == 0){
                fclose(fp);
                break;
            }else{
                //append here
                fprintf(fp, "%s", buffer);
                #ifdef DEBUG
                printf("appended: %s\n", buffer);
                #endif
            }
        }
 
    } else {
        send(connfd, "0", strlen("0"), 0);
    }

}
void download(int connfd, char* buffer){
    //recieve file path
    bzero(buffer, 1024);
    recv(connfd, buffer, 1024, 0);

    #ifdef DEBUG
    printf("recieved: %s\n", buffer);
    #endif

    //create file path
    FILE *fptr;
    char filePathName[100];
    bzero(filePathName, 100);

    strcat(filePathName, "./Remote Directory/");
    strcat(filePathName, buffer);

    #ifdef DEBUG 
    printf("filepath: %s\n", filePathName);
    #endif
    
    //if file exists
    if(file_exists(filePathName)){
        send(connfd, "1", strlen("1"), 0);
        
        int chunk_size = 1000;
        char file_chunk[chunk_size];

        fptr = fopen(filePathName,"rb");  // Open a file in read-binary mode.
        fseek(fptr, 0L, SEEK_END);  // Sets the pointer at the end of the file.
        int file_size = ftell(fptr);  // Get file size.
        // printf("Server: file size = %i bytes\n", file_size);
        fseek(fptr, 0L, SEEK_SET);  // Sets the pointer back to the beginning of the file.

        int total_bytes = 0;  // Keep track of how many bytes we read so far.
        int current_chunk_size;  // Keep track of how many bytes we were able to read from file (helpful for the last chunk).
        ssize_t sent_bytes;

        while (total_bytes < file_size){
            // Clean the memory of previous bytes.
            // Both 'bzero' and 'memset' works fine.
            bzero(file_chunk, chunk_size);
            // memset(file_chunk, '\0', chunk_size);

            // Read file bytes from file.
            current_chunk_size = fread(&file_chunk, sizeof(char), chunk_size, fptr);

            // Sending a chunk of file to the socket.
            sent_bytes = send(connfd, &file_chunk, current_chunk_size, 0);
            bzero(buffer, 1024);
            recv(connfd, buffer, 1024, 0);
            // Keep track of how many bytes we read/sent so far.
            // total_bytes = total_bytes + current_chunk_size;
            total_bytes = total_bytes + sent_bytes;
            
        }        
        
    } else {
        send(connfd, "0", strlen("0"), 0);
    }
}


int main(int argc, char *argv[])
{
	char buffer[1024];
	char* ip = argv[1];
    int server_socket;
	int connfd = connect_server(ip, &server_socket);
    
    //recieve Client Command line
    while(true){
        bzero(buffer, 1024);
	    recv(connfd, buffer, 1024, 0);
        send(connfd, "[COMMAND] Recieved", strlen("[COMMAND] Recieved"), 0);
        #ifdef DEBUG
        printf("command recieved: %s\n", buffer);
        #endif
        //*pause function not necessary here
        //mactch client command line (buffer)
        if(strcmp(buffer, "append") == 0){
            append(connfd, buffer);
        }else if(strcmp(buffer, "upload") == 0){
            //upload();
        }else if(strcmp(buffer, "download") == 0){
            download(connfd, buffer);
        }else if(strcmp(buffer, "quit") == 0){
            close(connfd);
            close(server_socket);
            exit(0);
        }
    }
    return 0;
}
