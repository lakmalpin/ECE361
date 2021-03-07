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
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#define MAXBUFLEN 100
#define PACKETSIZE 1000

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//finds the total number of packets needed when total number of bytes are sent
int find_total_frag(long size)
{
    int count = 1000;

    if (size%count == 0){
        return (size/1000);
    }

    while ( (size%count != size) ){
        count = count + 1000;
    }

    return (count/1000);
}

//returns the filedata and packetsize to the pointers sent in as parameters
uint8_t* serialize_packet(uint8_t total_frag, uint8_t packet_count, int byte_count, char *filename, uint8_t *filedata, int *packet_size){
    uint8_t packet[2000];
    int packet_index;

    memset(packet, 0, 2000);

    //first parts of the packets are integers, so sprintf can be used to append the packet
    packet_index = sprintf(packet, "%d", total_frag);
    packet[packet_index] = ':';
    packet_index ++;
    packet_index = packet_index + sprintf(packet+packet_index, "%d", packet_count);
    packet[packet_index] = ':';
    packet_index++;
    packet_index = packet_index + sprintf(packet+packet_index, "%d", byte_count);
    packet[packet_index] = ':';
    packet_index++;
    
    //filename is a string and is appended by character
    int a;
    for (a = 0; a < strlen(filename);a++){
        packet[packet_index + a] = filename[a];
    }

    packet_index = packet_index + strlen(filename);
    packet[packet_index] = ':';
    packet_index++;

    memcpy (packet + packet_index, filedata, byte_count);
    packet_index = packet_index + byte_count;
    
    //create return packet with correct size
    uint8_t* return_packet = malloc(packet_index); 
    memset(return_packet, 0, packet_index);
    memcpy(return_packet, packet, packet_index+1);

    *packet_size = packet_index;

    memset(packet, 0, 2000);

    return return_packet;

}

int main(int argc, char *argv[])
{
    int sockfd;
    struct addrinfo hints;
    struct addrinfo *servinfo, *p;
    int rv;
    int numbytes;
    char userin[20];
    char ftp[] = "ftp";

    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];
    char yes[] = "yes";

    int num_bytes = 0;
    int total_frag;
    uint8_t filedata[PACKETSIZE];
    int byte_count = 0;
    int packet_count = 1;

    struct timeval stop, start;
    
    if (argc != 3) {
        fprintf(stderr,"usage: talker hostname message\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
    }

    printf("Please input a message: ");
    fgets(userin,20,stdin);
    char *filename = strtok(userin, " ");
    filename = strtok(NULL, " ");
    filename[strlen(filename)-1] = '\0';

    FILE *file;
    uint8_t *buffer;
    long filelen;

    file = fopen(filename,"rb");
    fseek(file,0,SEEK_END);
    filelen = ftell(file);
    rewind(file);

    buffer = (uint8_t *)malloc(filelen * sizeof(uint8_t));
    fread(buffer, filelen, 1 ,file);
    fclose(file);

    total_frag = find_total_frag(filelen);
    uint8_t* packets_list[total_frag];

    int count = 0;
    int packet_size = 0;
    int packet_size_list[total_frag];

    //assemble packets in 1000 byte sequences and append to packets_list
    for (count = 0; count < filelen; count ++){
        filedata[byte_count] = buffer[count];
        byte_count ++;

        if ((byte_count == PACKETSIZE) || (count == (filelen - 1))) {
            
            uint8_t* packet = serialize_packet(total_frag, packet_count, byte_count, filename, filedata, &packet_size);
            packets_list[packet_count-1] = malloc (packet_size+1);
            memcpy(packets_list[packet_count-1],packet, packet_size+1);
            memset(filedata, '\0', byte_count);
            packet_size_list[packet_count-1] = packet_size+1;

            byte_count = 0;
            packet_count ++;
        }
    }

    int a=0;
    for (a = 0; a < total_frag; a++){

        gettimeofday(&start, NULL);

        if ((numbytes = sendto(sockfd, packets_list[a], packet_size_list[a], 0, p->ai_addr, p->ai_addrlen)) == -1) {
            perror("talker: sendto");
            exit(1);
        }

        memset(buf, 0, 100);
        addr_len = sizeof their_addr;
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }

        gettimeofday(&stop, NULL);


        if (strcmp(buf,yes) == 0) {
            printf("Packet %d Sent Successfully! \n", a+1);
            printf("RTT: %lu ms\n", (stop.tv_sec - start.tv_sec) * 1000 + stop.tv_usec - start.tv_usec);
        }
        else {
            printf("%s\n", buf);
            return 1;
        }

    }
    
    freeaddrinfo(servinfo);
    close(sockfd);
    
    return 0;
}