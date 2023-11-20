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

int main(int argc, char* argv[]) {

    //User 이름 등록
    char* user = (char*)malloc(sizeof(char) * 16);
    strcpy(user, argv[1]);

    //msg 배열 초기화
    char** msg = (char**)malloc(sizeof(char*)*5);
    for(int i=0; i<5; i++) {
        msg[i] = (char*)malloc(sizeof(char) * MAX);
    }

    /*실험용
    char* pkt = (char*)malloc(sizeof(char) * MAX); 
    strcpy(pkt, "Hello|I'm|Fine|Thank|You");
    msgtok(msg, pkt);
    for(int i=0; i<5; i++) {
        printf("%s\n",msg[i]); 
    }*/
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

void connectServer() {
    //나만의 port# 이용
    int port = 61616;
    int sockFlag;
    struct sockaddr_in serverAddr;

    //socket 할당
    if((sockFlag = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        printf("Socket 생성 실패\n");
        exit(0);
    }
    else
        printf("Socket 생성 성공\n");

    serverAddr.sin_family = AF_INET;
    //local ip 127.0.0.1로 접속
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(port);

    if((connect(sockFlag, (struct sockaddr*)&serverAddr, sizeof(serverAddr)))<0){
        printf("Server-Client 연결 실패\n");
        exit(0);
    }
    else
        printf("Server-Client 연결 성공\n");
    

}

void dataRecv(int sockFlag) {
    char* buf = malloc(sizeof(char)*MAX);

    while(1) {
        memset(buf, 0x00, MAX);
        read(sockFlag, buf, sizeof(buf));
    }
}