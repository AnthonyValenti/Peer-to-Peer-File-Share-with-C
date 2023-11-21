#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <string.h>


#define BUFLEN 100
#define PEERLENGTH 20
#define MAXPEERS 100
#define MAXCONTENT 100
#define PORT 3002
#define CONTENTLENGTH 20
#define ADDYLENGTH 16
#define PORTLENGTH 6

int main(int argc, char **argv){
    //pdu struct
    typedef struct PDU1{
        char type;
        char data[BUFLEN];
    } pdu;
    
    //peer pdu struct
    typedef struct peerPDU{
        char name [PEERLENGTH];
        char contentList [MAXCONTENT][CONTENTLENGTH];
        char address [ADDYLENGTH];
        char port [PORTLENGTH];
        int numContent;
    } peer;
    
    //local list of peers
    peer peerList [MAXPEERS];
    int i=0;
    int j=0;
    //init peer list
    while(i < MAXPEERS){
        memset(peerList[i].name, '\0', sizeof(peerList[i].name));
        memset(peerList[i].address, '\0', sizeof(peerList[i].address));
        memset(peerList[i].port, '\0', sizeof(peerList[i].port));
        peerList[i].numContent = 0;
        for (j = 0 ; j < MAXCONTENT ; j++){
            memset(peerList[i].contentList[j], '\0', sizeof(peerList[i].contentList[j]));
        }
        i++;
    }
    int numPeers = 0;
    char clientAddyString [16];
    ssize_t recvResult;
    pdu recvData, sendData; //pdus for data being received and sent
    
    //code for craeting the server socket
    int serverSock;
    serverSock = socket(PF_INET, SOCK_DGRAM, 0);
    if (serverSock < 0){
        printf("error creating socket");
        exit(1);
    }
    //making server and client address
    struct sockaddr_in serverAddy, clientAddy;
    socklen_t addySize;
    memset(&serverAddy, 0, sizeof(serverAddy));
    memset(&clientAddy, 0, sizeof(clientAddy));
    serverAddy.sin_family = AF_INET;
    serverAddy.sin_port = htons(PORT);
    serverAddy.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(serverAddy.sin_zero, '\0', sizeof (serverAddy.sin_zero));
    //binding the socket
    int temp;
    temp = bind(serverSock, (struct sockaddr*) &serverAddy, sizeof(serverAddy));
    if (temp < 0){
        printf("binding error");
        exit(1);
    }
    
    addySize = sizeof(serverAddy);
    printf("Server started awaiting requests\n");
    while (1){
        memset(recvData.data, '\0', sizeof(recvData.data));
        memset(sendData.data, '\0', sizeof(sendData.data));
        recvResult = recvfrom(serverSock, (struct PDU1 *) &recvData, sizeof(recvData), 0, (struct sockaddr *) &clientAddy, &addySize);
        inet_ntop(AF_INET, &clientAddy.sin_addr, (char *)clientAddyString, sizeof(clientAddyString));
        clientAddyString [15] = '\0';
        if (recvData.type == 'R'){
            char peerName [PEERLENGTH];
            char contentName [CONTENTLENGTH];
            char tempPort [PORTLENGTH];
            int peerExists = 0;
            int nameError = 0;
            int contentError = 0;
            memset(peerName, '\0', sizeof(peerName));
            memset(contentName, '\0', sizeof(contentName));
            memset(tempPort, '\0', sizeof(tempPort));
            int i = 0, j = 0;
            for (i = 0 ; i < PEERLENGTH ; i++){
                if (recvData.data [i] == '\0'){
                    break;
                }
                peerName [i] = recvData.data [i];
            }
            peerName [i] = '\0';
            //verify peer
            for (i = 0 ; i < MAXPEERS ; i++){
                if (strcmp(peerName, peerList[i].name) == 0){
                    if (strcmp(clientAddyString, peerList[i].address) == 0){
                        peerExists = 1;
                        break;
                    }
                    else{
                        nameError = 1;
                        break;
                    }
                }
            }
            
            i++;
            //getting the content from the data received
            for (i = i ; i < PEERLENGTH + CONTENTLENGTH ; i++){
                if (recvData.data [i] == '\0'){
                    break;
                }
                contentName [j] = recvData.data [i];
                j++;
            }
            j++;
            contentName [j] = '\0';
            int y, x;
            //check for repeating peer names
            for (x = 0 ; x < MAXPEERS ; x++){
                for (y = 0 ; y < MAXCONTENT ; y++){
                    if (strcmp(contentName, peerList[x].contentList[y]) == 0){
                        contentError = 1;
                        break;
                    }
                }
            }
            
            //cehck if peer is registered
            if (peerExists == 0){
                i++;
                j = 0;
                for (i = i ; i < PEERLENGTH + CONTENTLENGTH + PORTLENGTH ; i++)
                {
                    if (recvData.data [i] == '\0')
                    {
                        break;
                    }
                    tempPort [j] = recvData.data [i];
                    j++;
                }
                j++;
                tempPort [j] = '\0';
                printf("registration from\nPeer: %s\nFile name: %s\nPort: %s\n", peerName, contentName, tempPort);
                //name error check
                if (nameError == 1){
                    printf("error peer name conflict\n");
                    sendData.type = 'E';
                    memset(sendData.data, '\0', sizeof(sendData.data));
                    strcpy(sendData.data, "error peer name conflict\0");
                    sendto(serverSock, (struct PDU1 *) &sendData, sizeof(sendData), 0, (struct sockaddr*) &clientAddy, addySize);
                }
                //content error check
                else if (contentError == 1){
                    printf("error content already exists\n");
                    sendData.type = 'E';
                    memset(sendData.data, '\0', sizeof(sendData.data));
                    strcpy(sendData.data, "error content already exists\0");
                    sendto(serverSock, (struct PDU1 *) &sendData, sizeof(sendData), 0, (struct sockaddr*) &clientAddy, addySize);
                }
                //no errors occured respond with 'A' type PDU
                else{
                    printf("\nRegistration success\n");
                    strcpy(peerList[numPeers].name, peerName);
                    strcpy(peerList[numPeers].contentList[peerList[numPeers].numContent], contentName);
                    strcpy(peerList[numPeers].address, clientAddyString);
                    strcpy(peerList[numPeers].port, tempPort);
                    sendData.type = 'A';
                    memset(sendData.data, '\0', sizeof(sendData.data));
                    sprintf(sendData.data, "Registration success");
                    sendto(serverSock, (struct PDU1 *) &sendData, sizeof(sendData), 0, (struct sockaddr*) &clientAddy, addySize);
                    peerList[numPeers].numContent++;
                    numPeers++;
                }
            }
            else{
                //content name error check
                if (contentError == 1){
                    printf("error content already exists");
                    sendData.type = 'E';
                    memset(sendData.data, '\0', sizeof(sendData.data));
                    strcpy(sendData.data, "error content already exists\0");
                    sendto(serverSock, (struct PDU1 *) &sendData, sizeof(sendData), 0, (struct sockaddr*) &clientAddy, addySize);
                }
                //register and respond with 'A' type PDU
                else{
                    memset(peerList[i].contentList[peerList[i].numContent], '\0', sizeof(peerList[i].contentList[peerList[i].numContent]));
                    strcpy(peerList[i].contentList[peerList[i].numContent], contentName);
                    sendData.type = 'A';
                    memset(sendData.data, '\0', sizeof(sendData.data));
                    sprintf(sendData.data, "Registration success\nContent Name: %s", peerList[i].contentList[peerList[i].numContent]);
                    sendto(serverSock, (struct PDU1 *) &sendData, sizeof(sendData), 0, (struct sockaddr*) &clientAddy, addySize);
                    peerList[i].numContent++;
                }
            }
        }
        
        //handling download request
        //download request starts with 'S' type PDU to search for content and then switches to sending 'C' type PDUs
        else if (recvData.type == 'S'){
            printf("search request: %s\n", recvData.data);
            int i;
            int j;
            int length1;
            int length2;
            int found = 0;
            for (i = 0 ; i < numPeers ; i++){
                for (j = 0 ; j < numPeers ; j++){
                    if (strcmp(recvData.data, peerList[i].contentList[j]) == 0){
                        found = 1;
                        break;
                    }
                }
                if (found == 1){
                    break;
                }
            }
            //if the content is found return details to client
            if (found == 1){
                printf("\nContent Server:\n");
                printf("peer name: %s\n", peerList[i].name);
                printf("peer port: %s\n", peerList[i].port);
                char removeContent [CONTENTLENGTH];
                strcpy(removeContent, recvData.data);
                int k;
                sendData.type = 'D';
                length1 = strlen(peerList[i].address);
                length1++;
                length2 = strlen(peerList[i].port);
                length2++;
                memcpy(sendData.data, peerList[i].address, length1);
                memcpy(sendData.data + length1, peerList[i].port, length2);
                sendto(serverSock, (struct PDU1 *) &sendData, sizeof(sendData), 0, (struct sockaddr*) &clientAddy, addySize);
                memset(sendData.data, '\0', sizeof(sendData.data));
                memset(recvData.data, '\0', sizeof(recvData.data));
                recvResult = recvfrom(serverSock, (struct PDU1 *) &recvData, sizeof(recvData), 0, (struct sockaddr *) &clientAddy, &addySize);
                //making the content client the new content server after succesful download
                if (recvData.type == 'T'){
                    for (j = 0 ; j < peerList[i].numContent ; j++){
                        if (strcmp(removeContent, peerList[i].contentList[j]) == 0){
                            if (j == MAXCONTENT - 1){
                                memset(peerList[i].contentList[j], '\0', sizeof(peerList[i].contentList[j]));
                            }
                            else{
                                for (k = j ; k < MAXCONTENT ; k++){
                                    memset(peerList[i].contentList[k], '\0', sizeof(peerList[i].contentList[k]));
                                    strcpy(peerList[i].contentList[k], peerList[i].contentList[k + 1]);
                                }
                            }
                            break;
                        }
                    }
                    peerList[i].numContent--;
                    for (i = 0 ; i < numPeers ; i++){
                        if (strcmp(clientAddyString, peerList[i].address) == 0){
                            strcpy(peerList[i].contentList[peerList[i].numContent], removeContent);
                            peerList[i].numContent++;
                            break;
                        }
                    }
                }
            }
            //If content doesnt exist send an error PDU
            else{
                sendData.type = 'E';
                strcpy(sendData.data, "error, does not exists\0");
                sendto(serverSock, (struct PDU1 *) &sendData, sizeof(sendData), 0, (struct sockaddr*) &clientAddy, addySize);
            }
        }
        //handling deregistration
        else if (recvData.type == 'T'){
            memset(sendData.data, '\0', sizeof(sendData.data));
            char deregistrationFile[CONTENTLENGTH];
            strcpy(deregistrationFile, recvData.data);
            int test = 0;
            int i;
            
            //ensure the client registered to the content is the one deregistering
            for (i = 0 ; i < MAXPEERS ; i++){
                if (strcmp(clientAddyString, peerList[i].address) == 0){
                    test = 1;
                    break;
                }
            }
            //if the client attempting to deregister is not the client server of that file return an error PDU
            if (test == 0){
                sendData.type = 'E';
                strcpy(sendData.data, "error, not registered\0");
                sendto(serverSock, (struct PDU1 *) &sendData, sizeof(sendData), 0, (struct sockaddr*) &clientAddy, addySize);
            }
            //the client is the client server, can now deregister
            else{
                printf("De-registration request:\n");
                printf("peer name: %s\n", peerList[i].name);
                printf("File name: %s\n", deregistrationFile);
                int j;
                int k;
                //remove from local list
                for (j = 0 ; j < peerList[i].numContent ; j++){
                    if (strcmp(deregistrationFile, peerList[i].contentList[j]) == 0){
                        if (j == MAXCONTENT - 1){
                            memset(peerList[i].contentList[j], '\0', sizeof(peerList[i].contentList[j]));
                        }
                        else{
                            for (k = j ; k < MAXCONTENT ; k++){
                                memset(peerList[i].contentList[k], '\0', sizeof(peerList[i].contentList[k]));
                                strcpy(peerList[i].contentList[k], peerList[i].contentList[k + 1]);
                            }
                        }
                        break;
                    }
                }
                peerList[i].numContent--;
                printf("\nDe-registration success\n");
                //Send 'A' PDU to confirm deregister
                sendData.type = 'A';
                strcpy(sendData.data, "De-registration success");
                sendto(serverSock, (struct PDU1 *) &sendData, sizeof(sendData), 0, (struct sockaddr*) &clientAddy, addySize);
            }
        }
        
        //list content request
        else if (recvData.type == 'O')
        {
            printf("View request from: %s\n",clientAddyString);
            int check = 0;
            //if there are no registered files
            if (peerList[0].contentList[0][0] == '\0'){
                sendData.type = 'E';
                memset(sendData.data, '\0', sizeof(sendData.data));
                sendto(serverSock, (struct PDU1 *) &sendData, sizeof(sendData), 0, (struct sockaddr*) &clientAddy, addySize);
                check = 1;
                printf("\nerror, no files\n");
            }
            //if content exists on the network
            if (check == 0){
                int i = 0;
                int j = 0;
                int numBytes = 0;
                int contentSize = 0;
                int new = 0;
                memset(sendData.data, '\0', sizeof(sendData.data));
                //send 100 bytes at a time, last pdu sent is 'F' for final transmission
                for (i = 0 ; i < MAXPEERS ; i++){
                    for (j = 0 ; j < MAXCONTENT ; j++){
                        if (peerList[i].contentList[j][0] == '\0'){
                            break;
                        }
                        contentSize = strlen(peerList[i].contentList[j]);
                        contentSize++;
                        numBytes = numBytes + contentSize;
                        if ((100 - numBytes) >= 0){
                            if (new == 0){
                                memcpy(sendData.data, peerList[i].contentList[j], contentSize);
                                new = 1;
                            }
                            else{
                                memcpy(sendData.data + numBytes - contentSize, peerList[i].contentList[j], contentSize);
                            }
                        }
                        else{
                            sendData.type = 'O';
                            sendto(serverSock, (struct PDU1 *) &sendData, sizeof(sendData), 0, (struct sockaddr*) &clientAddy, addySize);
                            memset(sendData.data, '\0', sizeof(sendData.data));
                            memcpy(sendData.data, peerList[i].contentList[j], contentSize);
                            numBytes = contentSize;
                        }
                    }
                }
                if (sendData.data[0] != '\0'){
                    sendData.type = 'F';
                    sendto(serverSock, (struct PDU1 *) &sendData, sizeof(sendData), 0, (struct sockaddr*) &clientAddy, addySize);
                }
                printf("content list sent\n\n");
            }
        }

    }
    
    return 0;
}


