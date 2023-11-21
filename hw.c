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

void msgtok(char* pkt);
void connectServer(int sockId, char* user);
void msgRecv(int sockId);
void msgSend(int sockId);

int chatAuthFlag = 0;
char* user;
char** msg;

int main(int argc, char* argv[]) {

    //User 이름 등록
    user = (char*)malloc(sizeof(char) * 16);
    strcpy(user, argv[1]);

    //msg 배열 초기화
    msg = (char**)malloc(sizeof(char*)*5);
    for(int i=0; i<5; i++) {
        msg[i] = (char*)malloc(sizeof(char) * MAX);
    }

    //채팅 flag 초기화
    chatAuthFlag = 0;

    //통신 시작
    int sockId = 0;
    connectServer(sockId, user);

    pthread_t recvId;
    pthread_create(&recvId, NULL, msgRecv, NULL);
    pthread_t sendId;
    pthread_create(&sendId, NULL, msgSend, NULL);
}

//받은 pkt에서 msg data를 tokenizer하는 함수
void msgtok(char* pkt){
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

void connectServer(int sockId, char* user) {
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
    
    printf("%s님, 컴넥 소프트웨어에 오신 걸 환영합니다!\n", user);
}

//server에서 메시지를 계속 받아오는 스레드
void msgRecv(int sockId) {
    char* buf = malloc(sizeof(char)*MAX);

    while(1) {
        memset(buf, 0x00, MAX);
        read(sockId, buf, sizeof(buf));

        msgtok(buf);

        //CHATTING
        if(strcmp(msg[type], "CHAT_AUTH")) {
            printf("인증을 위한 간단한 퀴즈에 답해주세요.\n");
            printf("%s ?", msg[data]);
            char* ans = malloc(sizeof(char)*16);
            scanf("%s",&ans);
            strcpy(buf, strcat(strcat(strcat(strcat(user, "|CHAT_ANS|"), strlen(ans)), "|END|"), ans));
            write(sockId, buf, sizeof(buf));
        }
        else if(strcmp(msg[type], "CHAT_REP")) {
            if(msg[data] == "ALLOW") {
                printf("인증이 완료되었습니다.\n");
                chatAuthFlag = 1;
            }
            else if(msg[data] == "DENY"){
                printf("인증에 실패하였습니다.\n");
                printf("다시 시도해주세요.\n");
            }
        }
        else if(strcmp(msg[type], "CHAT_LISTEN")) {
            if(chatAuthFlag == 0);
            else if(chatAuthFlag == 1) {
                printf("%s: %s\n", msg[id], msg[data]);
            }
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
        free(buf);
    }
}

void msgSend(int sockId) {
    char* select = malloc(sizeof(char)*MAX);
    while(1) {
        if(chatAuthFlag == 0) {
            printf("------------------\n");
            printf("- 1. 채팅방 입장  -\n");
            printf("- 2. 파일 업로드  -\n");
            printf("- 3. 파일 다운로드-\n");
            printf("- 4. 파일 리스트  -\n");
            printf("- 5. 접속 종료    -\n");
            printf("------------------\n");
        }
        else if(chatAuthFlag == 1) {
            printf("------------------\n");
            printf("- 1. 채팅 보내기  -\n");
            printf("- 2. 파일 업로드  -\n");
            printf("- 3. 파일 다운로드-\n");
            printf("- 4. 파일 리스트  -\n");
            printf("- 5. 접속 종료    -\n");
            printf("------------------\n");
        }

        scanf("%s", &select);

        char* buf = malloc(sizeof(char)*MAX);
        memset(buf, 0x00, MAX);
        if(select == "1") {
            if(chatAuthFlag == 0) {
                printf("채팅방 입장 인증 요청을 전송합니다...\n");
                strcpy(buf, strcat(user, "|CHAT_REQ|1|END|-"));
                write(sockId, buf, sizeof(buf));
            }
            else if(chatAuthFlag == 1){
                printf("보낼 메시지: ");
                char* temp = malloc(sizeof(char)*MAX);
                scanf("%s", &temp);
                strcpy(buf, strcat(strcat(strcat(strcat(user, "|CHAT_SEND|"), strlen(temp)), "|END|"), temp));
                write(sockId, buf, sizeof(buf));
            }
        }

        free(buf);
    }
}