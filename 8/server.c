#include <stdio.h>     
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>   
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define MAXPENDING 8

pthread_mutex_t mutex;
int* sectors;
int found = 0;
int printed = 0;

typedef struct threadArgs {
    int socket;
    int sectorsCount;
} threadArgs;

void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}


void* handleListener(void *args) {
    int socket;
    pthread_detach(pthread_self());
    socket = ((threadArgs*)args)->socket;
    int sectorsCount = ((threadArgs*)args)->sectorsCount;
    free(args);
    int recvData[1];
    recvData[0] = 0;
    for (;;) {
        if (found) {
            break;
        }
        send(socket, sectors, sectorsCount * sizeof(int), 0);
        recv(socket, recvData, sizeof(recvData), 0);
        if (recvData[0] == -1) {
            break;
        }
        sleep(1);
    }
    close(socket);
}


void *listenThread(void* args) {
    int socket;
    int client_length;
    int client_socket;
    struct sockaddr_in client_addr;
    pthread_t threadId;

    pthread_detach(pthread_self());
    socket = ((threadArgs*)args)->socket;
    int sectorsCount = ((threadArgs*)args)->sectorsCount;
    free(args);

    listen(socket, MAXPENDING);

    for (;;) {
        client_length = sizeof(client_addr);
        client_socket = accept(socket, (struct sockaddr *) &client_addr, &client_length);
        printf("New connection from %s\n", inet_ntoa(client_addr.sin_addr));
        threadArgs *args = (threadArgs*) malloc(sizeof(threadArgs));
        args->socket = client_socket;
        args->sectorsCount = sectorsCount;
        if (pthread_create(&threadId, NULL, handleListener, (void*) args) != 0) DieWithError("pthread_create() failed");
    }
}

void *clientThread(void* args) {
    int socket;
    pthread_detach(pthread_self());
    socket = ((threadArgs*)args)->socket;
    int sectorsCount = ((threadArgs*)args)->sectorsCount;
    free(args);

    int sendData[2];
    int recvData[2];

    for (;;) {
        int sec = -1;
        pthread_mutex_lock(&mutex);
        for (int i = 0; i < sectorsCount; i++) {
            if (sectors[i] != 1) {
                sec = i;
                break;
            }
        }
        if (sec == -1) {
            sendData[0] = -1;
        } else {
            sendData[0] = sec;
            sendData[1] = sectors[sec] == 2;
            sectors[sec] = 1;
        }
        if (found) {
            sendData[0] = -1;
        }
        pthread_mutex_unlock(&mutex);
        send(socket, sendData, sizeof(sendData), 0);
        recv(socket, recvData, sizeof(recvData), 0);
        pthread_mutex_lock(&mutex);
        if (!found) {
            found = recvData[0];
            if (!printed) {
                printf("Pirates found trasure!\n");
                printed = 1;
            }
        }
        pthread_mutex_unlock(&mutex);
        if (found) {
            sendData[0] = -1;
            send(socket, sendData, sizeof(sendData), 0);
            break;
        }
    }

    close(socket);
}

int main(int argc, char *argv[])
{
    unsigned short port;
    unsigned short listen_port;
    int server_socket;
    int client_socket;
    int client_length;
    int lisnen_server_socket;
    struct sockaddr_in client_addr;
    struct sockaddr_in server_addr;
    struct sockaddr_in server_listen_addr;
    pthread_t threadId;
    pthread_mutex_init(&mutex, NULL);
    
    if (argc != 5)
    {
        fprintf(stderr, "Usage:  %s <Client port> <listen port> <Number of sectors> <Sector with trasure>\n", argv[0]);
        exit(1);
    }

    port = atoi(argv[1]);
    listen_port = atoi(argv[2]);
    int sectorsCount = atoi(argv[3]);
    int trasureSector = atoi(argv[4]);
    trasureSector -= 1;

    sectors = (int*) malloc(sizeof(int) * sectorsCount);
    for (int i = 0; i < sectorsCount; i++) {
        sectors[i] = 0;
    }
    sectors[trasureSector] = 2;

    if ((server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) DieWithError("socket() failed");
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;              
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) DieWithError("bind() failed");
    printf("Open socket on %s:%d\n", inet_ntoa(server_addr.sin_addr), port);

    if ((lisnen_server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) DieWithError("socket() failed");
    memset(&server_listen_addr, 0, sizeof(server_listen_addr));
    server_listen_addr.sin_family = AF_INET;              
    server_listen_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    server_listen_addr.sin_port = htons(listen_port);

    if (bind(lisnen_server_socket, (struct sockaddr *) &server_listen_addr, sizeof(server_listen_addr)) < 0) DieWithError("bind() failed");
    printf("Open socket on %s:%d\n", inet_ntoa(server_listen_addr.sin_addr), listen_port);

    threadArgs *args2 = (threadArgs*) malloc(sizeof(threadArgs));
    args2->socket = lisnen_server_socket;
    args2->sectorsCount = sectorsCount;
    if (pthread_create(&threadId, NULL, listenThread, (void*) args2) != 0) DieWithError("pthread_create() failed");

    
    listen(server_socket, MAXPENDING);
    for (;;) {
        client_length = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_length);
        printf("New connection from %s\n", inet_ntoa(client_addr.sin_addr));

        threadArgs *args = (threadArgs*) malloc(sizeof(threadArgs));
        args->socket = client_socket;
        args->sectorsCount = sectorsCount;
        if (pthread_create(&threadId, NULL, clientThread, (void*) args) != 0) DieWithError("pthread_create() failed");
    }
    free(sectors);
    pthread_mutex_destroy(&mutex);
    return 0;
}
