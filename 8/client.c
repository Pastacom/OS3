#include <stdio.h>   
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}

int main(int argc, char *argv[])
{
    int client_socket;
    struct sockaddr_in server_addr;
    unsigned short server_port;
    int recv_msg_size;
    char *server_ip;

    if (argc != 3)
    {
       fprintf(stderr, "Usage: %s <Server IP> <Server Port>\n", argv[0]);
       exit(1);
    }

    server_ip = argv[1];
    server_port = atoi(argv[2]);
    if ((client_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) DieWithError("socket() failed");
    memset(&server_addr, 0, sizeof(server_addr));  
    server_addr.sin_family      = AF_INET;        
    server_addr.sin_addr.s_addr = inet_addr(server_ip); 
    server_addr.sin_port        = htons(server_port);
    if (connect(client_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) DieWithError("connect() failed");

    int recvData[2];
    int sendData[2];
    for (;;) {
        recv(client_socket, recvData, sizeof(recvData), 0);
        if (recvData[0] == -1) break;
        printf("Pirates check sector #%d\n", recvData[0]);
        sleep(1 + rand() % 3);
        if (recvData[1]) {
            printf("Pirates found treasure!\n");
            sendData[0] = 1;
        } else {
            sendData[0] = 0;
        }
        send(client_socket, sendData, sizeof(sendData), 0);
    }
    printf("Pirates finished\n");
    close(client_socket);
    return 0;
}
