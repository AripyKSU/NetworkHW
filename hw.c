#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>

#define MAX 256

typedef enum Msg {
    id, type, len, end, data
} Msg;

void msgtok(char** msg, char* pkt);
void connectServer(int sockId);
void msgRecv(char** msg, int sockId);

int main(int argc, char* argv[]) {

    //User 이름 등록
    char* user = (char*)malloc(sizeof(char) * 16);
    strcpy(user, argv[1]);

    //msg 배열 초기화
    char** msg = (char**)malloc(sizeof(char*)*5);
    for(int i=0; i<5; i++) {
        msg[i] = (char*)malloc(sizeof(char) * MAX);
    }

    //통신 시작
    int sockId = 0;
    connetServer(sockId);

    pthread_t recvId;
    pthread_create(&recvId, NULL, msgRecv, NULL);
}

//받은 pkt에서 msg data를 tokenizer하는 함수
void msgtok(char** msg, char* pkt){
    char* temp = (char*)malloc(sizeof(char) * MAX);
    temp = strtok(pkt, "|");
    int i=0;
    while (temp != NULL) {
        free(msg[i]);
        msg[i] = (char*)malloc(sizeof(char) * MAX);
        strcpy(msg[i], temp);
        i++;
        temp = strtok(NULL, "|");
    }
    
    free(temp);
}

void connectServer(int sockId) {
    //나만의 port# 이용
    int port = 61616;
    struct sockaddr_in serverAddr;

    //socket 할당
    if((sockId = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        printf("Socket 생성 실패\n");
        exit(0);
    }
    else
        printf("Socket 생성 성공\n");

    serverAddr.sin_family = AF_INET;
    //local ip 127.0.0.1로 접속
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(port);

    if((connect(sockId, (struct sockaddr*)&serverAddr, sizeof(serverAddr)))<0){
        printf("Server-Client 연결 실패\n");
        exit(0);
    }
    else
        printf("Server-Client 연결 성공\n");
    
}

void msgRecv(char** msg, int sockId) {
    char* buf = malloc(sizeof(char)*MAX);

    while(1) {
        memset(buf, 0x00, MAX);
        read(sockId, buf, sizeof(buf));

        msgtok(msg, buf);

        //CHATTING
        if(strcmp(msg[type], "CHAT_AUTH")) {

        }
        else if(strcmp(msg[type], "CHAT_REP")) {

        }
        else if(strcmp(msg[type], "CHAT_LISTEN")) {
            
        }

        //FILEUP
        if(strcmp(msg[type], "FILEUP_REP")) {

        }
        else if(strcmp(msg[type], "FILEUP_END")) {

        }

        //FILEDOWN
        if(strcmp(msg[type], "FILEDOWN_REP")) {

        }
        else if(strcmp(msg[type], "FILEDOWN_DATA")) {

        }

        //FILELIST
        if(strcmp(msg[type], "FILELIST_REP")) {

        }

        //END
        if(strcmp(msg[type], "END_REP")) {
            if(strcmp(msg[data], "BYE")) {
                close(sockId);
                exit(0);
            }
        }
    }
}