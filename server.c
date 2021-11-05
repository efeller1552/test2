#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>

/* Structs created based on diagrams of message formats in recitation */
struct request_mess{
    int opcode;
    int filename;
};

struct data_mess{
    int opcode;
    int block_num;
    int data[512];
};

struct ack_mess{
    int opcode;
    int block_num;
};

struct error_mess{
    int opcode;
    int error_num;
    int error_data;
};

struct tftp{
    int opcode;
    struct request_mess request_mess;
    struct data_mess data_mess;
    struct ack_mess ack_mess;
    struct error_mess error_mess;
};

int main(int argc, char *argv[]){

    char data[512];
    int sockfd;
    struct sockaddr_in client, server;
    int port = 0;
    char *char1;
    ssize_t recieved, sent;
    struct tftp tftp_message;
    struct tftp data_message_struct; 
    struct tftp tftp_from_client;
    struct tftp tftp_ack_message;
    int opcode;
    struct timeval timeval;
    char *filename;
    char *eof; 
    char *mode;
    char data_mess[512];
    FILE *file, *fixed_file;
    int datal;

    if(argc < 2){
        perror("Incorrect format");
    }

    port = strtol(argv[2], &char1, 10);
    
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = port;

    sockfd = socket(AF_INET, SOCK_DGRAM, AI_PASSIVE);
    if(sockfd == -1){
        perror("socket error");
        exit(-1);
    }

    if(bind(sockfd, (struct sockaddr *) &server, sizeof(server))==-1){
        perror("binding error");
        exit(-1);
    }

    while(1){

        //recieve a message
        recieved = recvfrom(sockfd, &tftp_message, sizeof(tftp_message), 0, (struct sockaddr *) &client, sizeof(server));
        if(recieved < 0){
            if(errno != EAGAIN){
                perror("Can't recieve message");
            }else{
                continue;
            }
        }

        if(recieved < 4){
            //fill in
        }

        opcode = ntohs(tftp_message.opcode);

        if(opcode == 1 || opcode == 3){

            if(fork()==0){

                if((sockfd = socket(AF_INET, SOCK_DGRAM, AI_PASSIVE)) < 0){
                    perror("socket error");
                    exit(-1);
                }

                timeval.tv_usec = 0;
                timeval.tv_sec = 3; 

                if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeval, sizeof(timeval))<0){
                    perror("socket error");
                    exit(-1);
                }

                filename = (char *) tftp_message.request_mess.filename;
                eof = &filename[sizeof(tftp_message) - 3];

                if(*eof != '\0'){
                    exit(-1);
                }

                mode = strchr(filename, '\0') + 1;
                opcode = ntohs(tftp_message.opcode);
                if(opcode == 1){
                    file = fopen(filename, "r");
                }else{
                    file = fopen(filename, "w");
                }
                

                if(file == NULL){
                    perror("file not found");
                    exit(-1);
                }

                if(opcode == 1){
                    
                    if(strcasecmp(mode, "netascii")==0){
                        //fixed_file = 
                    }
                    int flag = 0;
                    int num = 0;
                    while(flag == 0){
                        datal = fread(data, 1, sizeof(data), file);
                        num++;

                        if(datal < 512){
                            flag=1;
                        }

                        int i = 0;
                        while(i<10){
                            memcpy(data_mess, data, datal);
                            data_message_struct.opcode = htons(3);
                            data_message_struct.data_mess.block_num = htons(num);
                            memcpy(data_message_struct.data_mess.data, data_mess, datal);

                            sent = sendto(sockfd, &data_message_struct, datal+4, 0, (struct sockaddr *) &client, sizeof(server));
                            if(sent==-1){
                                perror("error sending");
                            }
                            if(sent < 0){
                                printf("session done");
                                exit(-1);
                            }

                            int recieve_from_client;
                            recieve_from_client = recvfrom(sockfd, &tftp_from_client, sizeof(tftp_from_client), 0, (struct sockaddr *) &client, sizeof(server));
                            if(recieve_from_client >=0){
                                if(recieve_from_client<4){
                                    //send error
                                    exit(-1);
                                }
                            }

                            if(recieve_from_client>4){
                                break;
                            }

                            if(errno!=EAGAIN){
                                exit(-1);
                            }

                            //otherwise there was no response, try again
                            i++;
                        }

                        int client_opcode;
                        client_opcode = ntohs(tftp_from_client.opcode);
                        //error opcode 
                        if(client_opcode==5){
                            //print statement
                            exit(-1);
                        }
                        //looking for ack in return
                        if(client_opcode!=4){
                            //send error
                            exit(-1);
                        }
                        int client_block;
                        client_block = ntohs(tftp_from_client.ack_mess.block_num);
                        if(client_block!=num){
                            exit(-1);
                        }
                    }
                }
                //wrq
                if(opcode==2){
                    int num = 0;
                    int flag = 0;
                    tftp_ack_message.opcode = htons(4);
                    tftp_ack_message.ack_mess.block_num = htons(num);
                    int send_ack;
                    send_ack = sendto(sockfd, &tftp_ack_message, sizeof(tftp_ack_message.ack_mess), 0, (struct sockaddr *) &client, sizeof(server));
                    if(send_ack<0){
                        exit(-1);
                    }
                    int i=0;
                    while(flag==0){
                        while(i<10){
                            recieved = recvfrom(sockfd, &tftp_from_client, sizeof(tftp_from_client), 0, (struct sockaddr *) &client, sizeof(server));
                            if(recieved>=4){
                                break;
                            }
                            if(recieved < 0){
                                if(errno != EAGAIN){
                                    perror("Can't recieve message");
                                }
                            }else if(recieved >=0){
                                if(recieved<4){
                                    exit(-1);
                                }
                            }
                            //otherwise try again
                            i++;
                        }
                        num++;
                        int dataSize = sizeof(tftp_from_client.data_mess);
                        if(recieved < dataSize){
                            flag=1;
                        }

                        int client_block;
                        client_block = ntohs(tftp_from_client.ack_mess.block_num);
                        if(client_block!=num){
                            exit(-1);
                        }
                        int client_op;
                        client_op=ntohs(tftp_from_client.opcode);
                        if(client_op!=3){
                            exit(-1);
                        }

                        dataSize = fwrite(tftp_from_client.data_mess.data, 1, dataSize-4, file);
                        if(dataSize<0){
                            perror("write error");
                            exit(-1);
                        }

                        tftp_ack_message.ack_mess.block_num = htons(num);
                        dataSize = sendto(sockfd, &tftp_ack_message, sizeof(tftp_ack_message.ack_mess), 0, (struct sockaddr *) &client, sizeof(server));
                        if(dataSize<0){
                            exit(-1);
                        }
                    }
                }
                printf("Successful transmit");
                fclose(file);
                close(sockfd);
                exit(0);
            }
        }else{
            printf("invalid request");
        }
    }
    close(sockfd);
    return 0;
}
