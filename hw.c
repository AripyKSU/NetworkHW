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
#define port 45000

//you need -lpthread in gcc compile

typedef enum Msg {
    id, type, len, end, data
} Msg;

void msgtok(char* pkt);
void connectServer(char* user);
void* msgRecv();
void* msgSend();
char* makeMsg(char* type, char* end, char* data);
void* chat(int sockId);
void uploadFile(int sockId);
void downloadFile(int sockId);
void listFile(int sockId);
void printMenu();

//prevent msg changing if important msg is received
int msgFlag = 1;
//user name
char* user;
//save message
char** msg;
//socket ID
int sockId;
//whether you enter chatting room
int chatFlag = 0;

int main(int argc, char* argv[]) {

    //User name copy
    user = (char*)malloc(sizeof(char) * 16);
    strcpy(user, argv[1]);

    //msg array initialization
    msg = (char**)malloc(sizeof(char*)*5);
    for(int i=0; i<5; i++) {
        msg[i] = (char*)malloc(sizeof(char) * MAX);
    }

    //start communication
    connectServer(user);

    //create server msg recv thread, client msg send thread
    pthread_t recvId;
    pthread_create(&recvId, NULL, msgRecv, NULL);
    pthread_t sendId;
    pthread_create(&sendId, NULL, msgSend, NULL);

    //prevent main fininshing while thread is not closed
    pthread_join(recvId, NULL);
    pthread_join(sendId, NULL);

    for(int i=0; i<5; i++) {
        free(msg[i]);
    }
    free(msg);
    free(user);

    return 0;
}

//msg tokenizer
void msgtok(char* pkt){

    char* temp = (char*)malloc(sizeof(char) * MAX);
    temp = strtok(pkt, "|");

    int i=0;
    while (temp != NULL) {
        memset(msg[i], 0x00, MAX);
        strcpy(msg[i], temp);
        i++;
        temp = strtok(NULL, "|");
    }

    free(temp);
}

//connet with ip, socket
void connectServer(char* user) {

    struct sockaddr_in serverAddr;

    //allocate socket
    if((sockId = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        printf("Fail to create Socket\n");
        exit(0);
    }
    else
        printf("Success to create Socket\n");

    serverAddr.sin_family = AF_INET;
    //local ip 127.0.0.1
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(port);

    if((connect(sockId, (struct sockaddr*)&serverAddr, sizeof(serverAddr)))<0){
        printf("Fail to connect Server-Client\n");
        exit(0);
    }
    else
        printf("Success to connect Server-Client\n");
    
    printf("Welcome %s!\n", user);
}

//msg recv thread from server
void* msgRecv() {
    char buf[MAX];

    while(1) {
        if(msgFlag == 0) {
            continue;
        }
        memset(buf, 0x00, MAX);
        read(sockId, buf, MAX);
        msgtok(buf);

        //if you are in chat room, chat_listen msg will print
        if(strcmp(msg[type], "CHAT_LISTEN") == 0 && chatFlag == 1) {
            printf("%s: %s\n", msg[id], msg[data]);
            //prevent use duplicate message
            memset(msg[type],0x00,MAX);
        }

        //wait if you receive msg which need client to reply 
        if((strcmp(msg[type], "CHAT_AUTH") == 0)
            ||(strcmp(msg[type], "CHAT_REP") == 0)
            ||(strcmp(msg[type], "FILEUP_REP") == 0)
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

    }
}

//client thread
void* msgSend() {

    char* select;
    char* buf;

    while(1) {
        //print Menu
        printMenu();
        select = malloc(sizeof(char)*MAX);
        scanf("%s", select);

        buf = malloc(sizeof(char)*MAX);
        memset(buf, 0x00, MAX);

        //Enter chat room
        if(strcmp(select, "1") == 0) {
            chat(sockId);
        }
        //Upload File
        else if(strcmp(select, "2") == 0) {
            uploadFile(sockId);
        }
        //Download File
        else if(strcmp(select, "3") == 0) {
            downloadFile(sockId);
        }
        //List File
        else if(strcmp(select, "4") == 0) {
            listFile(sockId);
        }
        //End connection
        else if(strcmp(select, "5") == 0) {
            printf("See you~\n");
            strcpy(buf, makeMsg("END_REQ", "END", "EXIT"));
            write(sockId, buf, strlen(buf));
            free(buf);
            free(select);
            break;
        }
        free(buf);
        free(select);
    }
}

//print menu in client thread
void printMenu() {
    printf("---------------------\n");
    printf("- 1. Enter ChatRoom -\n");
    printf("- 2. Upload File    -\n");
    printf("- 3. Download File  -\n");
    printf("- 4. List File      -\n");
    printf("- 5. End Connection -\n");
    printf("---------------------\n");
}

//Make msg with type, end, data
char* makeMsg(char* type, char* end, char* data) {
    char* userCpy = malloc(sizeof(char)*MAX);
    strcpy(userCpy,user);
    char* len = malloc(sizeof(char)*5);
    sprintf(len,"%d", (int)strlen(data));
    return strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(userCpy, "|"),type),"|"), len),"|"), end),"|"), data);
}

void* chat(int sockId) {

    char* buf = malloc(sizeof(char)*MAX);
    memset(buf, 0x00, MAX);

    //chat_request
    printf("Sending Chat Request to Server...\n");
    strcpy(buf, makeMsg("CHAT_REQ", "END", "-"));
    write(sockId, buf, strlen(buf));

    //chat_auth
    //Wait for reply
    while(strcmp(msg[type],"CHAT_AUTH") != 0) {
        sleep(1);
        msgFlag = 1;
    }
    //User reply Quiz
    printf("Answer Quiz for Authentication.\n");
    printf("%s", msg[data]);
    char* ans = malloc(sizeof(char)*16);
    scanf("%s", ans);
    strcpy(buf, makeMsg("CHAT_ANS", "END", ans));
    write(sockId, buf, strlen(buf));
    free(ans);

    //chat_rep
    while(strcmp(msg[type],"CHAT_REP") != 0) {
        sleep(1);
        msgFlag = 1;
    }
    if(msg[data] == "ALLOW") {
        printf("Complete Auth.\n");
    }
    else if(strcmp(msg[data],"DENY") == 0){
        printf("Fail to Auth.\n");
        printf("Please Retry.\n");
        return NULL;
    }

    chatFlag = 1;
    printf("Write Message and press Enter. If you want to exit, type \"exit\".\n");
    while(1) {
        //Write message from user
        char* temp = malloc(sizeof(char)*MAX);
        scanf("%s", temp);

        if(strcmp(temp, "exit") == 0) {
            printf("Exit the Chatting room.\n");
            chatFlag = 0;
            free(temp);
            break;
        }

        strcpy(buf, makeMsg("CHAT_SEND", "END", temp));
        write(sockId, buf, strlen(buf));
        free(temp);
        
    }
    return NULL;
}

//UploadFile
void uploadFile(int sockId) {

    char* filename = malloc(sizeof(char)*MAX);
    int fp;
    
    //Enter Filename
    printf("File name to upload: ");
    scanf("%s", filename);

    chdir("/home/s2019112555/");

    //Try open file
    if((fp = open(filename, O_RDONLY)) > 0) {
        
        //Request file upload to server
        char* buf = malloc(sizeof(char)*MAX);
        printf("Request uploading File \"%s\"... ", filename);
        strcpy(buf, makeMsg("FILEUP_REQ", "END", filename));
        write(sockId, buf, strlen(buf));

        //Wait for repling from server
        while(strcmp(msg[type],"FILEUP_REP") != 0) {
            sleep(1);
            msgFlag = 1;
        }
        if(strcmp(msg[data], "ALLOW") == 0) {
            printf("Allow Upload.\n");
        }
        else if(strcmp(msg[data], "DENY") == 0) {
            printf("Deny Upload.\n");
            return;
        }

        //File Size Calculate
        struct stat st;
        stat(filename, &st);
        int fileSize = (int)st.st_size;

        int maxLength = MAX - (11 + 11 + 3 + 4 + 4);
        //char* temp for uploading file
        char temp[fileSize+1];
        memset(temp, 0x00, fileSize+1);
        read(fp, temp, fileSize);

        //Dividing File, send file with cont
        for(int i=0; i<fileSize/maxLength; i++) {
            memset(buf, 0x00, MAX);
            strncpy(buf, (temp+(i*maxLength)), maxLength); 
            strcpy(buf, makeMsg("FILEUP_DATA","CONT", buf));
            write(sockId, buf, strlen(buf));
        }
        //Last part of file
        memset(buf, 0x00, MAX);
        strncpy(buf, (temp + (fileSize/maxLength*maxLength)), fileSize%maxLength); 
        strcpy(buf, makeMsg("FILEUP_DATA","END", buf));
        write(sockId, buf, strlen(buf));

        close(fp);

        //Wait for repling
        while(strcmp(msg[type],"FILEUP_END") != 0) {
            sleep(1);
            msgFlag = 1;
        }
        printf("Complete Uploading File.\n");
    }
    else {
        printf("Can't find File \"%s\"!", filename);
    };
    return;
}

//Download File
void downloadFile(int sockId) {

    char* filename = malloc(sizeof(char)*MAX);
    int fp;

    //Enter File Name
    printf("File Name: ");
    scanf("%s",filename);

    chdir("/home/s2019112555/");

    //Request Download file
    char* buf = malloc(sizeof(char)*MAX);
    printf("Request Download file \"%s\"... ",filename);
    strcpy(buf, makeMsg("FILEDOWN_REQ", "END", filename));
    write(sockId, buf, strlen(buf));

    //Wait for repling
    while(strcmp(msg[type],"FILEDOWN_REP") != 0) {
        sleep(1);
        msgFlag = 1;
    }
    if(strcmp(msg[data], "ALLOW") == 0) {
        printf("Allow Download.\n");
    }
    else if(strcmp(msg[data], "DENY") == 0) {
        printf("Deny Download, please check filename.\n");
        return;
    }
    memset(buf, 0x00, MAX);

    //Try to open file
    if((fp = open(filename, O_WRONLY | O_CREAT)) != -1) {
        while(1) {
            //Wait for replying data
            while(strcmp(msg[type],"FILEDOWN_DATA") != 0) {
                sleep(1);
                msgFlag = 1;
            }
            //Data with cont
            if(strcmp(msg[end], "CONT") == 0) {
                write(fp, msg[data], strlen(msg[data]));
                //Prevent duplicate msg
                memset(msg[type],0x00,MAX);
                continue;
            }
            //End of file
            if(strcmp(msg[end], "END") == 0) {
                write(fp, msg[data], strlen(msg[data]));
                strcpy(buf, makeMsg("FILEDOWN_END", "END", "-"));
                write(sockId, buf, strlen(buf));
            }
            printf("Download Complete!\n");
            close(fp);
            break;
        }
    }
    else {
        printf("Error while opening file!\n");
    }
    return;
}

void listFile(int sockId) {
    //Request file list
    char* buf = malloc(sizeof(char)*MAX);
    printf("Request file list...\n");
    strcpy(buf, makeMsg("FILELIST_REQ", "END", "-"));
    write(sockId, buf, strlen(buf));

    printf("------File List------\n");
    while(1){
        //Wait for replying
        while(strcmp(msg[type],"FILELIST_REP") != 0) {
            sleep(1);
            msgFlag = 1;
        }
        //Data with cont
        if(strcmp(msg[end], "CONT") == 0) {
            printf("%s",msg[data]);
            //Prevent duplicate msg
            memset(msg[type],0x00,MAX);
            continue;
        }
        //End of data
        if(strcmp(msg[end], "END") == 0) {
            printf("%s",msg[data]);
            break;
        }
    }
    printf("----------------------\n");
    return;
}
