#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define MAX 256
#define port 61616

typedef enum Msg {
    id, type, len, end, data
} Msg;

void msgtok(char* pkt);
void connectServer(int* sockId, char* user);
void* msgRecv(void* sockIdPointer);
void* msgSend(void* sockIdPointer);
char* makeMsg(char* type, char* end, char* data);
void* UploadFile();

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
    int* sockIdPointer = malloc(sizeof(int));
    connectServer(sockIdPointer, user);

    //server로부터 받는 recv thread, client로부터 보내는 send thread 생성
    pthread_t recvId;
    pthread_create(&recvId, NULL, msgRecv, (void*) sockIdPointer);
    pthread_t sendId;
    pthread_create(&sendId, NULL, msgSend, (void*) sockIdPointer);

    //메인 함수가 thread 끝날때까지 대기
    pthread_join(recvId, NULL);
    pthread_join(sendId, NULL);
    return 0;
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

//ip, socket을 이용하여 접속을 하는 함수
void connectServer(int* sockIdPointer, char* user) {

    //나만의 port# 이용
    struct sockaddr_in serverAddr;

    //socket 할당
    if((*sockIdPointer = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        printf("Socket 생성 실패\n");
        exit(0);
    }
    else
        printf("Socket 생성 성공\n");

    serverAddr.sin_family = AF_INET;
    //local ip 127.0.0.1로 접속
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(port);

    if((connect(*sockIdPointer, (struct sockaddr*)&serverAddr, sizeof(serverAddr)))<0){
        printf("Server-Client 연결 실패\n");
        exit(0);
    }
    else
        printf("Server-Client 연결 성공\n");
    
    printf("%s님, 컴넥 소프트웨어에 오신 걸 환영합니다!\n", user);
}

//server에서 메시지를 계속 받아오는 스레드
void* msgRecv(void* sockIdPointer) {
    char* buf = malloc(sizeof(char)*MAX);
    int sockId = *(int*) sockIdPointer;

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
            strcpy(buf, makeMsg("CHAT_ANS", "END", ans));
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
                return NULL;
            }
        }
        free(buf);
    }
}

//클라이언트 스레드
void* msgSend(void* sockIdPointer) {

    int sockId = *(int*) sockIdPointer;
    char* select;
    char* buf;

    while(1) {
        //menu 창 띄우기
        printMenu();
        select = malloc(sizeof(char)*MAX);
        scanf("%s", &select);

        buf = malloc(sizeof(char)*MAX);
        memset(buf, 0x00, MAX);
        //채팅방 입장 or 채팅 보내기
        if(select == "1") {
            if(chatAuthFlag == 0) {
                printf("채팅방 입장 인증 요청을 전송합니다...\n");
                strcpy(buf, makeMsg("CHAT_REQ", "END", "-"));
                write(sockId, buf, sizeof(buf));
            }
            else if(chatAuthFlag == 1){
                printf("보낼 메시지: ");
                char* temp = malloc(sizeof(char)*MAX);
                scanf("%s", &temp);
                strcpy(buf, makeMsg("CHAT_SEND", "END", temp));
                write(sockId, buf, sizeof(buf));
                free(temp);
            }
        }
        //파일 업로드
        else if(select == "2") {
            //UploadRequest();
            UploadFile();
        }
        else if(select == "3") {
            
        }
        else if(select == "4") {
            
        }
        //접속 종료
        else if(select == "5") {
            printf("안녕히 가세요~");
            strcpy(buf, makeMsg("END_REQ", "END", "EXIT"));
            write(sockId, buf, sizeof(buf));
        }
        free(buf);
        free(select);
    }
}

//클라이언트 스레드에서 매번 메뉴를 출력하는 함수
void printMenu() {
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
}

//type, end, data를 이용하여 Msg protocol 형식으로 만드는 함수 
char* makeMsg(char* type, char* end, char* data) {
    return strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(user, "|"),type),"|"), strlen(data)),"|"), end),"|"), data);
}

//클라이언트에서 파일 업로드 선택 시 호출될 함수
void* UploadFile() {
    char* filename = malloc(sizeof(char)*MAX);
    int fp;
    //파일 이름 입력
    printf("업로드할 파일 이름: ");
    scanf("%s",&filename);

    // //현재 파일 경로
    // char* folder = malloc(sizeof(char)*MAX);
    // getcwd(folder, MAX);

    //파일 열기 시도
    if(fp = open(filename, "O_RDONLY") != -1) {
        
        //파일 크기 구하기
        struct stat st;
        stat(filename, &st);
        off_t fileSize = st.st_size;

        // fseek(fp, 0, SEEK_END);
        // int fileSize = ftell(fp);
        // fseek(fp, 0, SEEK_SET);

        int maxLength = MAX - (11 + 11 + 3 + 4 + 4);
        char* temp;
        char* buf;

        //file을 분할, 아직 파일 데이터가 남았을 때
        for(int i=0; i<fileSize/maxLength; i++) {
            //file의 일부를 받아오는 char* temp
            temp = malloc(sizeof(char)*MAX);
            read(fp, temp, maxLength);
            //temp를 msg형태로 가공한 char* buf
            buf = malloc(sizeof(char)*MAX);
            strcpy(buf, makeMsg("FILEUP_DATA","CONT",temp));
            write(socketId, buf, sizeof(buf));
            free(buf);
            free(temp);
        }
        //file의 마지막 부분
        temp = malloc(sizeof(char)*MAX);
        read(fp, temp, fileSize%maxLength);
        buf = malloc(sizeof(char)*MAX);
        strcpy(buf, makeMsg("FILEUP_DATA","END",temp));
        write(socketId, buf, sizeof(buf));
        free(temp);
        fclose(fp);
    }
    else {
        printf("%s 을(를) 찾을 수 없습니다!", *filename);
    };
    return NULL;
}
