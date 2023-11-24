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

//나중에 strcmp, close, free 다 확인 하셈

void msgtok(char* pkt);
void connectServer(int* sockId, char* user);
void* msgRecv(void* sockIdPointer);
void* msgSend(void* sockIdPointer);
char* makeMsg(char* type, char* end, char* data);
void* chat(int sockId);
void* uploadFile(int sockId);
void* downloadFile(int sockId);
void* listFile(int sockId);

//msg가 바뀌면 안될때 0, 계속 받을 수 있는 상황에서 1
int msgFlag = 1;
//채팅 인증이 되면 1, 인증이 안되면 0
int chatAuthFlag = 0;
//사용자 계정 이름
char* user;
//메시지가 나눠질 공간
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

    //상호 배제 구현을 위한 lock 초기화
    lock = PTHREAD_MUTEX_INITIALIZER;

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
        if(msgFlag == 0) {
            continue;
        }
        memset(buf, 0x00, MAX);
        read(sockId, buf, sizeof(buf));
        msgtok(buf);

        //client의 처리를 기다려야 하는 msg가 올 때 대기
        if((strcmp(msg[type], "CHAT_AUTH") == 0)
            ||(strcmp(msg[type], "CHAT_REP") == 0)
            ||(strcmp(msg[type], "FILEUP_REP") == 0)
            ||(strcmp(msg[type], "FILEUP_DATA") == 0)
            ||(strcmp(msg[type], "FILEUP_END") == 0)
            ||(strcmp(msg[type], "FILEDOWN_REP") == 0)
            ||(strcmp(msg[type], "FILEDOWN_DATA") == 0)
            ||(strcmp(msg[type], "FILELIST_REP") == 0)
            ||(strcmp(msg[type], "END_REP") == 0)
        )
        msgFlag = 0;

        //END
        if(strcmp(msg[type], "END_REP") == 0) {
            if(strcmp(msg[data], "BYE") == 0) {
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
        if(strcmp(select, "1") == 0) {
            // pthread_t chatId;
            // pthread_create(&chatId, NULL, chat, sockId);
            chat(sockId);
        }
        //파일 업로드
        else if(strcmp(select, "2") == 0) {
            // pthread_t uploadId;
            // pthread_create(&uploadId, NULL, uploadFile, sockId);
            uploadFile(sockId);
        }
        else if(strcmp(select, "3") == 0) {
            // pthread_t downloadId;
            // pthread_create(&downloadId, NULL, downloadFile, sockId);
            downloadFile(sockId);
        }
        else if(strcmp(select, "4") == 0) {
            // pthread_t fileListId;
            // pthread_create(&fileListId, NULL, fileList, sockId);
            fileList(sockId);
        }
        //접속 종료
        else if(strcmp(select, "5") == 0) {
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
    printf("------------------\n");
    printf("- 1. 채팅방 입장  -\n");
    printf("- 2. 파일 업로드  -\n");
    printf("- 3. 파일 다운로드-\n");
    printf("- 4. 파일 리스트  -\n");
    printf("- 5. 접속 종료    -\n");
    printf("------------------\n");
}

//type, end, data를 이용하여 Msg protocol 형식으로 만드는 함수 
char* makeMsg(char* type, char* end, char* data) {
    char* userCpy = malloc(sizeof(char)*MAX);
    strcpy(userCpy,user);
    return strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(userCpy, "|"),type),"|"), strlen(data)),"|"), end),"|"), data);
}

void* chat(int sockId) {

    //채팅 인증 절차 (1회만 실행)
    if(chatAuthFlag == 0) {

        //chat_request
        printf("채팅방 입장 인증 요청을 전송합니다...\n");
        strcpy(buf, makeMsg("CHAT_REQ", "END", "-"));
        write(sockId, buf, sizeof(buf));

        //chat_auth
        //요청에 대한 응답이 올때까지 대기
        while(strcmp(msg[type],"CHAT_AUTH") != 0) {
            sleep(1);
            msgFlag = 1;
        }
        printf("인증을 위한 간단한 퀴즈에 답해주세요.\n");
        printf("%s ?", msg[data]);
        char* ans = malloc(sizeof(char)*16);
        scanf("%s",&ans);
        strcpy(buf, makeMsg("CHAT_ANS", "END", ans));
        write(sockId, buf, sizeof(buf));

        //chat_rep
        while(strcmp(msg[type],"CHAT_REP") != 0) {
            sleep(1);
            msgFlag = 1;
        }
        if(msg[data] == "ALLOW") {
            printf("인증이 완료되었습니다.\n");
            chatAuthFlag = 1;
        }
        else if(strcmp(msg[data],"DENY") == 0){
            printf("인증에 실패하였습니다.\n");
            printf("다시 시도해보세요.\n");
            return NULL;
        }
    }

    //메시지 보내기
    printf("보낼 메시지: ");
    char* temp = malloc(sizeof(char)*MAX);
    scanf("%s", &temp);
    strcpy(buf, makeMsg("CHAT_SEND", "END", temp));
    write(sockId, buf, sizeof(buf));
    free(temp);
    
    if(strcmp(msg[type], "CHAT_LISTEN") == 0) {
        //혹시 인증 안했을때 온다면 무시
        if(chatAuthFlag == 0);
        else if(chatAuthFlag == 1) {
            printf("%s: %s\n", msg[id], msg[data]);
        }
    }
    return NULL;
}

//클라이언트에서 파일 업로드 선택 시 호출될 함수
void* uploadFile(int sockId) {

    char* filename = malloc(sizeof(char)*MAX);
    int fp;

    //파일 이름 입력
    printf("업로드할 파일 이름: ");
    scanf("%s",&filename);

    //파일 열기 시도
    if(fp = open(filename, "O_RDONLY") != -1) {
        
        //파일 업로드 요청
        char* buf = malloc(sizeof(char)*MAX);
        printf("\"%s\" 파일 전송을 요청합니다... ",&filename);
        strcpy(buf, makeMsg("FILEUP_REQ", "END", filename));
        write(sockId, buf, sizeof(buf));

        //요청에 대한 응답이 올때까지 대기
        while(strcmp(msg[type],"FILEUP_REP") != 0) {
            sleep(1);
            msgFlag = 1;
        }
        if(strcmp(msg[data], "ALLOW") == 0) {
            printf("요청이 승인되었습니다.\n");
        }
        else if(strcmp(msg[data], "DENY") == 0) {
            printf("요청이 거부되었습니다.\n");
            return NULL;
        }

        //파일 크기 구하기
        struct stat st;
        stat(filename, &st);
        off_t fileSize = st.st_size;

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
            write(sockId, buf, sizeof(buf));
            free(buf);
            free(temp);
        }
        //file의 마지막 부분
        temp = malloc(sizeof(char)*MAX);
        read(fp, temp, fileSize%maxLength);
        buf = malloc(sizeof(char)*MAX);
        strcpy(buf, makeMsg("FILEUP_DATA","END",temp));
        write(sockId, buf, sizeof(buf));
        free(temp);
        fclose(fp);

        //파일 업로드 끝났다는 신호가 올 때까지 대기
        while(strcmp(msg[type],"FILEUP_END") != 0) {
            sleep(1);
            msgFlag = 1;
        }
        printf("파일 업로드가 완료되었습니다.\n");
    }
    else {
        printf("%s 을(를) 찾을 수 없습니다!", *filename);
    };
    return NULL;
}

//클라이언트에서 파일 업로드 선택 시 호출될 함수
void* downloadFile(int sockId) {
    char* filename = malloc(sizeof(char)*MAX);
    int fp;
    //파일 이름 입력
    printf("다운로드할 파일 이름: ");
    scanf("%s",&filename);

    //파일 다운로드 요청
    char* buf = malloc(sizeof(char)*MAX);
    printf("\"%s\" 파일 다운로드를 요청합니다... ",&filename);
    strcpy(buf, makeMsg("FILEDOWN_REQ", "END", filename));
    write(sockId, buf, sizeof(buf));

    //요청에 대한 응답이 올때까지 대기
    while(strcmp(msg[type],"FILEDOWN_REP") != 0) {
        sleep(1);
        msgFlag = 1;
    }
    if(strcmp(msg[data], "ALLOW") == 0) {
        printf("요청이 승인되었습니다.\n");
    }
    else if(strcmp(msg[data], "DENY") == 0) {
        printf("요청이 거부되었습니다. 파일 이름을 확인해보세요.\n");
        return NULL;
    }
    free(buf);
    buf = malloc(sizeof(char)*MAX);

    //파일 열기 시도
    if(fp = open(filename, O_WRONLY | O_CREAT) != -1) {
        while(1) {
            //파일 다운로드 데이터가 올 때까지 대기
            while(strcmp(msg[type],"FILEDOWN_DATA") != 0) {
                sleep(1);
                msgFlag = 1;
            }
            //파일 다운로드 일부만 왔을 때
            if(strcmp(msg[end], "CONT") == 0) {
                write(fp, msg[data], strlen(msg[data]));
                continue;
            }
            //파일 다운로드 끝부분
            if(strcmp(msg[end], "END") == 0) {
                write(fp, msg[data], strlen(msg[data]));
                strcpy(buf, makeMsg("FILEDOWN_END", "END", "-"));
                write(sockId, buf, sizeof(buf));
            }
            fclose(fp);
        }
    }
    else {
        printf("파일 열기 중 오류가 발생했습니다!\n");
    }
    return NULL;
}

void* listFile(int sockId) {
    printf("------파일 리스트------\n");
    //파일 리스트 데이터가 올 때까지 대기
    while(strcmp(msg[type],"FILELIST_REP") != 0) {
        sleep(1);
        msgFlag = 1;
    }
    //파일 리스트 일부만 왔을 때
    if(strcmp(msg[end], "CONT") == 0) {
        printf("%s",msg[data]);
        continue;
    }
    //파일 리스트 끝부분
    if(strcmp(msg[end], "END") == 0) {
        printf("%s",msg[data]);
    }
    printf("----------------------\n")
    return NULL;
}
