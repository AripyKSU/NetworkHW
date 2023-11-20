#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>

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
        msg[i] = (char*)malloc(sizeof(char) * 256);
    }

    /*실험용
    char* pkt = (char*)malloc(sizeof(char) * 256); 
    strcpy(pkt, "Hello|I'm|Fine|Thank|You");
    msgtok(msg, pkt);
    for(int i=0; i<5; i++) {
        printf("%s\n",msg[i]); 
    }*/
}

//받은 pkt에서 msg data를 tokenizer하는 함수
void msgtok(char** msg, char* pkt){
    char* temp = (char*)malloc(sizeof(char) * 256);
    temp = strtok(pkt, "|");
    int i=0;
    while (temp != NULL) {
        free(msg[i]);
        msg[i] = (char*)malloc(sizeof(char) * 256);
        strcpy(msg[i], temp);
        i++;
        temp = strtok(NULL, "|");
    }
    free(temp);
}