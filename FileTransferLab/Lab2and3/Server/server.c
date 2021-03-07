#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BACKLOG 10 // how many pending connections queue will hold

#define MAXBUFLEN 1500

/*

ip addr - get ip address of ug machine

//used to prep the socket address structure for subsequent use

struct addrinfo {
    int                 ai_flags;       // AI_PASSIVE, AI_CANONNAME, etc.
    int                 ai_family;      // AF_INET, AF_INET6, AF_UNSPEC
    int                 ai_socktype;    // SOCK_STREAM, SOCK_DGRAM
    int                 ai_protocol;    // use 0 for "any"
    size_t              ai_addrlen;     // size of ai_addr in bytes
    struct sockaddr     *ai_addr;       // struct sockaddr_in or _in6
    char                *ai_canonname;  // full canonical hostname
    struct addrinfo     *ai_next;       // linked list, next node
};

// holds socket address information for many types of sockets

struct sockaddr {
    unsigned short      sa_family;      // address family, AF_xxx; IPv4 or IPv6
    char                sa_data[14];    // 14 bytes of protocol address; destination address and port number for socket
};

// (IPv4 only--see struct sockaddr_in6 for IPv6)
//sockaddr_in is a parralel strucutre to sockaddr and is to be used with IPv4
//this structure makes it easy to reference elements of the socket address
//pointer to sockaddr_in can be cast to pointer sockaddr and vice-versa

struct sockaddr_in {
    short int           sin_family;     // Address family, AF_INET
    unsigned short int  sin_port;       // Port number; must be in Network Byte Order (use htons())
    struct in_addr      sin_addr;       // Internet address
    unsigned char       sin_zero[8];    // Same size as struct sockaddr
};


// (IPv4 only--see struct in6_addr for IPv6)
// Internet address (a structure for historical reasons)
// To reference 4 byte IP use sockaddr_in.sin_addr.s_addr

struct in_addr {
    uint32_t            s_addr;         // that's a 32-bit int (4 bytes)
};

//parallel structure to sockaddr_in and is used to see if address family is IPv4 or IPv6, then you can 
// typecast to appropriate sockaddr_in

struct sockaddr_storage {
    sa_family_t         ss_family;      // address family

    // all this is padding, implementation specific, ignore it:
    char                __ss_pad1[_SS_PAD1SIZE];
    int64_t             __ss_align;
    char                __ss_pad2[_SS_PAD2SIZE];
};

*/

// get sockaddr, IPv4 or IPv6:
//AF_INET = address family that is used to designate type of address the socket can communicate with (IPv4)
//according to the address family, typecasts the sockaddr pointer to a different type, then returns the
// address of the sockaddr_in type.
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

uint8_t* deserialize_packet(char *buffer, int *packet_size, int *file_size, char* file_name){

    const char colon[2] = ":";

    char *temp = buffer;
    uint8_t *data = malloc(2000);
    int data_index = 0;
    int colon_count = 0;
    int buffer_index = 0;

    while (*buffer){
        if (':'== *buffer){
            
            colon_count ++;

            if (colon_count == 1){
                *file_size = atoi(data);
                memset(data, 0, 2000);
                data_index = 0;
            }
            else if (colon_count == 2){
                memset(data, 0, 2000);
                data_index = 0;
            }
            else if (colon_count == 3){
                *packet_size = atoi(data);
                memset(data, 0, 2000);
                data_index = 0;
            }
            else if (colon_count ==4){
                memcpy(file_name, data, strlen(data)+1);
                memset(data, 0, 2000);
                data_index = 0;
                buffer_index ++;
                break;
            }
        }

        else {
            data[data_index] = *buffer;
            data_index ++;
        }
        
        buffer_index ++;
        buffer++;
    }

    buffer = temp;

    memcpy(data,buffer + buffer_index, *packet_size);

    // printf("pray to god: \n");
    // int c;
    // for (c = 0; c < *packet_size; c++){
    //     printf("%02X",data[c]);
    // }
    // printf("\n");

    return data;
    
}

int main(int argc, char *argv[])
{
    int sockfd;
    struct addrinfo hints; //points to a struct addrinfo that is already filled with relevant info
    struct addrinfo *servinfo; //points to a linked list of addrinfo structs, each containing a struct sockaddr
    struct addrinfo *p;
    int status;
    int numbytes;
    struct sockaddr_storage their_addr;
    uint8_t buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];
    char ip4[INET_ADDRSTRLEN];
    char ftp[] = "ftp";

    //checks execution command has 1 input parameter
    if (argc != 2) {
        fprintf(stderr,"server: port not specified\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints); //make sure struct is empty
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4, currently set to do not care IPv4 or IPv6
    hints.ai_socktype = SOCK_DGRAM; //UDP stream sockets
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me

    //if there is no error, servinfo will point to a linked list of addrinfo structs, each containing a struct sockaddr
    // getaddrinfo() is used to translate a domain name to an IP address. (uses DNS protocol)
    // returns a list since a host can have many ways to contact it (multiple protocols, multiple socket types, etc.)
    if ((status = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) { //if getaddrinfo() returns non-zero, print out error
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }

    // loop through all the results and bind to the first we can
    // first make sure a socket descriptor can be assigned to the given values
    // then check if you can bind the created socket with a port on the server
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("sever: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind socket\n");
        return 2;
    }

    //&((struct sockaddr_in *)p->ai_addr)->sin_addr - type cast from struct sockaddr to struct sockaddr_in 
    //printf("ip: %s\n", inet_ntop(AF_INET, &((struct sockaddr_in *)p->ai_addr)->sin_addr, ip4, sizeof ip4));

    freeaddrinfo(servinfo);

    printf("server: waiting for message...\n");

    //use their_addr (sockaddr_storage) since we do not know if client is using IPv4 or IPv6
    //recieve first packet from client
    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }

    //send acknowledgment to client
    sendto(sockfd, "yes", strlen("yes"), 0, (struct sockaddr *)&their_addr, addr_len);

    //deserialize first packet and determine number of packets to come from header.
    int packet_size = 0;
    int file_size = 0;
    char file_name[100];
    uint8_t* data = deserialize_packet(buf, &packet_size, &file_size, file_name);
    memset(buf, '\0', packet_size);
    
    printf("Packet 1 Recieved! \n");

    char* packets_list[file_size];
    int packet_sizes[file_size];

    packets_list[0] = malloc(packet_size+1);
    memcpy(packets_list[0],data, packet_size+1);
    packet_sizes[0] = packet_size; 

    int i = 0;

    for (i = 0; i < file_size-1; i++){
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }

        char* data = deserialize_packet(buf, &packet_size, &file_size, file_name);
        memset(buf, '\0', packet_size);
        packets_list[i+1] = malloc(packet_size+1);
        memcpy(packets_list[i+1],data, packet_size+1);
        packet_sizes[i+1] = packet_size;

        printf("Packet %d Recieved! \n", i+2);

        sendto(sockfd, "yes", strlen("yes"), 0, (struct sockaddr *)&their_addr, addr_len);

    }

    FILE *file;

    file = fopen(file_name, "wb");

    for (i = 0; i < file_size; i++){

        fwrite(packets_list[i], 1, packet_sizes[i], file);

    }
    fclose (file);

    close(sockfd);

    return 0;
}