// The 'client.c' code goes here.
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include "Md5.c"  // Feel free to include any other .c files that you need in the 'Client Domain'.
#define PORT 9999
#define MAX_LINE_LENGTH 80

//#define DEBUG

//flag to check if appending
int APPEND_FLAG = 0;

int connect_client(char* ip)
{
    //Establishes connection with server
    //returns client socket if successful
    //returns -1 otherwise
    int client_socket;
    struct sockaddr_in serv_addr;

    //create socket
    client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (client_socket < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    //NOTE: htons() converts unsigned short integer from hostbyte order to network byte order
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // The server IP address should be supplied as an argument when running the application.
    //NOTE: inet_pton() used to convert IPV4/IPV6 address to binary 
    int addr_status = inet_pton(AF_INET, ip, &serv_addr.sin_addr);
    if (addr_status <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    int connect_status = connect(client_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if (connect_status < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    return client_socket;
}
//Helper Functions======================================================================================
void tokenize(char* input, char** args){
    const char* delim = " \n\r\t";
    char* command = strtok(input, delim);
    int i = 0;
    while(command != NULL){
        args[i] = command;
        ++i;
        command = strtok(NULL, delim);
    }
}

void getFirst(char* input, char* first){
    int i = 0;
    while(input[i] != ' ' && input[i] != '\n'){
        first[i] = input[i];
        ++i;
    }
    first[i] = '\0';
}

void appendLoop(FILE* fp, int clientfd, char* buffer){
    char input[500];
    bzero(input, 500);
    char* args[10];
    char first_word[500];
    //copy fp stream content to input
    while (fgets(input, sizeof(input), fp) != NULL){
        printf("Appending> %s", input);
        
        getFirst(input, first_word);
        if(strcmp(first_word, "close") == 0){
            send(clientfd, "APPEND MODE CLOSED", strlen("APPEND MODE CLOSED"), 0);
            bzero(buffer, 1024);
            recv(clientfd, buffer, 1024, 0);
            break;
        }else if(strcmp(first_word, "pause") == 0){
            tokenize(input, args);
            sleep(atoi(args[1]));
        }else{
            //sent_size = write(client_socket, message, strlen(message));
            //ssize_t is the return type of send and read 
            send(clientfd, input, strlen(input), 0);
            bzero(input, 500);
            bzero(buffer, 1024);
            recv(clientfd, buffer, 1024, 0);
        }
    }
}

void appendFunction(int clientfd, char* filename, FILE* fp, char* buffer){
    
    //check if file exists on remote directory 
    send(clientfd, filename, strlen(filename), 0);
    bzero(buffer, 1024);
    recv(clientfd, buffer, 1024, 0);

    //if server responds with 1 then file exists 
    //if server responds with 0 then file DNE
    if(strcmp(buffer, "1") == 0){
        //append goes into a while loop that breaks iff close is called
        appendLoop(fp, clientfd, buffer);
    }else{
        printf("File [%s] could not be found in remote directory.\n", filename);
    }
    
   
}
void downloadLoop(char* filename, int clientfd, char* buffer){
    //recv size of file
    int file_size;
    recv(clientfd, &file_size, sizeof(file_size), 0);
    file_size = ntohl(file_size);
    //create filepathname
    char filePathName[100];
    strcat(filePathName, "./Local Directory/");
    strcat(filePathName, filename);
    //create storage for chunks
    int chunk_size = 1000;
    char file_buffer[chunk_size];    
    int received_bytes;
    int total_bytes = 0;
    //open file
    FILE *fptr;
    // Opening a new file in write-binary mode to write the received file bytes into the disk using fptr.
    fptr = fopen(filePathName,"wb");

    // Keep receiving bytes until we receive the whole file.
    while (1){
        bzero(file_buffer, chunk_size);
        int remaining_bytes = file_size - total_bytes;
        if (remaining_bytes <= chunk_size){
            received_bytes = recv(clientfd, file_buffer, remaining_bytes, 0);
            fwrite(&file_buffer, sizeof(char), received_bytes, fptr);
            break;
        }
        received_bytes = recv(clientfd, file_buffer, chunk_size, 0);
        total_bytes = total_bytes + received_bytes;
        fwrite(&file_buffer, sizeof(char), received_bytes, fptr);
    }
    fclose(fptr);
}

void download(int clientfd, char* filename, char* buffer){
    //send file name over
    send(clientfd, filename, strlen(filename), 0);
    bzero(buffer, 1024);
    recv(clientfd, buffer, 1024, 0);

    //if server responds with 1 then file exists 
    //if server responds with 0 then file DNE
    if(strcmp(buffer, "1") == 0){   
        #ifdef DEBUG
        printf("Server Found File");
        #endif
        downloadLoop(filename, clientfd, buffer);
    } else {
        printf("File [%s] could not be found in remote directory.\n", filename);
    }
}
    //send download command over to server
    //recv confirmation of command
    //then send file name over to server
    //recv confirmation that file is found
    //send bytes over
void execute(char** args, FILE* fp, int clientfd, char* buffer){
    ssize_t sent_size;
    //let server know command used 
    sent_size = send(clientfd, args[0], strlen(args[0]), 0);
    //recieve[command]
    bzero(buffer, 1024);
    recv(clientfd, buffer, 1024, 0);
    
    if(strcmp(args[0], "pause") == 0){
        sleep(atoi(args[1]));
    }
    else if(strcmp(args[0], "append") == 0){
        char* filename = args[1];
        appendFunction(clientfd, filename, fp, buffer);
    }else if(strcmp(args[0], "upload") == 0){

    }else if(strcmp(args[0], "download") == 0){
        char* filename = args[1];
        download(clientfd, filename, buffer);
    }else if(strcmp(args[0], "quit") == 0){
        close(clientfd);
        fclose(fp);
        exit(0);
    }
    else {
        printf("Command [%s] is not recognized.\n", args[0]);
    }
}

int main(int argc, char **argv)
{
    char buffer[1025];
    //connecting to server --> clientfd
    int clientfd = connect_client(argv[2]);
    //Opening Command Line File
    char input[100];
    char* args[10];
    //read input file
    FILE* fp = fopen(argv[1], "r");
    if(fp == NULL){
        printf("File cant be opened");
    }
    printf("Welcome to ICS53 Online Cloud Storage.\n");

    //Iterating through Command Line File
    while (fgets(input, sizeof(input), fp) != NULL)
    {
        printf("> %s", input);
        tokenize(input, args);
        execute(args, fp, clientfd, buffer);
    }
    fclose(fp);
    return 0;
    
}

