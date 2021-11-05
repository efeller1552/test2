#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>

#define NET_ASCII_MODE 1
#define OCTET_MODE 0

/* Structs created based on diagrams of message formats in recitation */
struct request_mess{
    int opcode: 1;
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
    int opcode: 5;
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
    int sockfd, new_sockfd;
    struct sockaddr_in client, server, new_server;
    int well_known_port = 0;
    char *char1;
    ssize_t recieved, sent;
    struct tftp tftp_message;
    //struct tftp data_message_struct; 
    //struct tftp tftp_from_client;
    //struct tftp tftp_ack_message;
    int opcode;
    int first_mess_opcode;
    struct timeval timeval;
    char *filename;
    char name[512];
    char *eof; 
    int mode;
    int fd =0;
    char data_mess[512];
    FILE *file, *fixed_file;
    int datal;
    int ephem_pt = 0;
    char mess_from_client[1024]={0};
    char mode_var[512];
    char nullchar = '\0';
    fd_set rset;
    struct data_mess mess_to_client;
    struct ack_mess ack_from_client;
    char data_mess_data[512];
    char help_char;
    struct addrinfo server_addr;
    struct addrinfo *server_addr_w_info, *adr_next;

    //check that both ip adress and port were used
    if(argc!=3){
        perror("Incorrect format");
    }

    //set well known port to argument given
    well_known_port = strtol(argv[2], &char1, 10);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = well_known_port;
    getaddrinfo(NULL, argv[2], &server_addr, &server_addr_w_info);

    for(adr_next = server_addr_w_info; adr_next!=NULL; adr_next->ai_next){
    //socket
        if(sockfd = socket(AF_INET, SOCK_DGRAM, adr_next->ai_protocol)==-1){
            //perror("socket error");
           // exit(-1);
           continue;
        }

        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 1, sizeof(int));
        //bind to server
        if(bind(sockfd, adr_next->ai_addr, adr_next->ai_addrlen)==-1){
            //perror("binding error");
            //exit(-1);
            close(sockfd);
            continue;
        }

        //otherwise we found the righ one so exit out
        break;
    }


    printf("server side is running");

    while(1){

        memset(&server, 0, sizeof server);
        memset(&client, 0, sizeof client);

        //recieve first RRQ from client
        ssize_t socklen = sizeof(client);
        recieved = recvfrom(sockfd, mess_from_client, sizeof(mess_from_client), 0, (struct sockaddr *) &client, socklen);
        if(recieved < 0){
            if(errno == EAGAIN){
                printf("nothing available yet, still waiting");
                continue;
            }else{
                perror("Can't recieve message");
                exit(-1);
            }
        }

        //start by getting the opcode to determine what we are doing. Opcode is always 2 bytes
        memcpy(&first_mess_opcode, &mess_from_client, 2);

        if(fork()<0){
            perror("Can't fork");
            exit(-1);
        }else{
            
            first_mess_opcode = ntohs(first_mess_opcode);
            if(first_mess_opcode==1){
                //we are in RRQ

                //now need to get filename and mode
                //bzero(filename, 512);
                int i;
                for(i=2; mess_from_client[i] != nullchar; i++){
                    filename[i-2]=mess_from_client[i];
                }

                //get mode. After the 2 bytes of opcode and 512 bytes of file
                int j;
                int new_index = 0;
                for(j=i+1; mess_from_client[j]; j++){
                    mode_var[new_index];
                    new_index++;
                }

                printf("First RRQ complete.");
                printf("mode: %s", mode_var);
                printf("file: %s", filename);

                mode = ReturnMode(mode_var);
                //TO DO: do different things with different modes

                file = fopen(filename, "r");
                if(file == NULL){
                    perror("file not found");
                    exit(-1);
                }else{
                    close(sockfd);

                    //here is where we create a new ephemeral port to use and start new socket
                    new_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
                    if(new_sockfd==-1){
                        perror("socket error");
                        exit(-1);
                    }
                    
                    new_server.sin_port = htons(ephem_pt);
                    new_server.sin_family = AF_INET;
                    new_server.sin_addr.s_addr = htonl(INADDR_ANY);

                    //new bind
                    if(bind(new_sockfd, (struct sockaddr *) &new_server, sizeof (new_server))<0){
                        perror("binding error");
                        exit(-1);
                    }

                    FD_ZERO(&rset);
                    FD_SET(fd, &rset);
                    timeval.tv_usec = 0;
                    timeval.tv_sec = 8; 

                    //we are in RRQ so we want to send data
                    int done, k;
                    done = 0;
                    //this section was highly influenced by the code included in the mp3 instructions
                    //from: UNP vol 1 1st edition, pg 499
                    char nextChar = -1;
                    int num=0;
                    while(done==0){
                        for(k=0; k<512; k++){
                            if(nextChar >=0){
                                //data_mess_data[k] = nextChar;
                                mess_to_client.data[k]=nextChar;
                                nextChar=-1;
                                continue;
                            }

                            help_char = 'c';
                            if(mode == NET_ASCII_MODE){
                                if(help_char == '\n'){
                                    help_char = '\r';
                                    nextChar = '\n';
                                }else if(help_char == '\r'){
                                    nextChar = '\0';
                                }else{
                                    nextChar = -1;
                                }
                            }
                            mess_to_client.data[i]=help_char;
                        }

                        mess_to_client.opcode = htons(3);
                        //mess_to_client.data = data_mess_data;
                        mess_to_client.block_num = htons(num);
                        int i = 0;
                        int select_val;
                        int total_size;
                        while(i<10){
                            sent = sendto(new_sockfd, (void *) &mess_to_client, sizeof(mess_to_client.opcode)+sizeof(mess_to_client.data)+i, 0, (struct sockaddr *) &client, sizeof(server));
                            if(sent==-1){
                                perror("error sending");
                            }
                            select_val = select(new_sockfd+1, &rset, NULL, NULL, &timeval);
                            if(select_val>0){
                                //need to recieve here
                                int recieve_from_client;
                                recieve_from_client = recvfrom(new_sockfd, (void *) &ack_from_client, sizeof(ack_from_client), 0, (struct sockaddr *) &client, sizeof(server));
                                if(recieve_from_client>=0){
                                    if(ntohs(ack_from_client.opcode)==4){
                                        if(ntohs(ack_from_client.block_num==num)){
                                            //done=1;
                                            break;
                                        }
                                    }
                                }

                            }else{
                                i++;
                            }
                            /*memcpy(data_mess, data, datal);
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
                            i++;*/
                        }
                        num++;
                        if(i<512){
                            done=1;
                            break;
                        }
                    }
                    fclose(file);
                    return(1);
                }

            }else if(first_mess_opcode==3){
                //BONUS: WRQ support
            }
        }
        
    }
}

int ReturnMode(char mode[512]){
    char netascii_mode[512]="netascii";
    char octet_mode[512]="octet";
    if(strcasecmp(mode, netascii_mode)==0){
        return 1;
    }else if(strcasecmp(mode, octet_mode)==0){
        return 0;
    }else{
        perror("Could not determine mode");
    }
                        
}