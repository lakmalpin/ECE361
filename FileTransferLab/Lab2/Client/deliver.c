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

#define MAXBUFLEN 100
#define PACKETSIZE 1000

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

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

char* serialize_packet(uint8_t total_frag, uint8_t packet_count, int byte_count, char *filename, uint8_t *filedata, int *packet_size){
    char packet[2000];
    int packet_index;
    int a = 0;

    memset(packet, 0, 2000);

    packet_index = sprintf(packet, "%d", total_frag);
    packet[packet_index] = ':';
    packet_index ++;
    packet_index = packet_index + sprintf(packet+packet_index, "%d", packet_count);
    packet[packet_index] = ':';
    packet_index++;
    packet_index = packet_index + sprintf(packet+packet_index, "%d", byte_count);
    packet[packet_index] = ':';
    packet_index++;
    
    for (a = 0; a < strlen(filename);a++){
        packet[packet_index + a] = filename[a];
    }
    packet_index = packet_index + strlen(filename);
    packet[packet_index] = ':';
    packet_index++;

    // for (a = 0; a < (byte_count); a++){
    //     memcpy (&packet[a+packet_index], &filedata[a], 1);
    // }

    memcpy (packet + packet_index, filedata, byte_count);

    //printf("byte count: %d\n", byte_count);
    //printf("packet_index: %d\n", packet_index);

    packet_index = packet_index + byte_count;

    //printf("final packet_index: %d\n", packet_index);
    
    char* return_packet = malloc(packet_index);
    
    memset(return_packet, 0, packet_index);

    //return_packet = packet;

    memcpy(return_packet, packet, packet_index+1);
    
    //printf("packet DATA: %s\n", return_packet);

    *packet_size = packet_index;

    memset(packet, 0, 2000);

    //printf("Serialized Packet: %s\n", return_packet);

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
    //printf("Total Frag: %d\n",total_frag);

    //printf("File length: %d\n", filelen);

    long i;

    for (i = 0; i < filelen; i++){
        //printf("%d",buffer[i]);
    }

    char* packets_list[total_frag];
    

    uint8_t test_total_frag = 2;
    uint8_t test_packet_count = 2;
    int test_byte_count = 250;

    //char* x = serialize_packet(test_total_frag, test_packet_count, test_byte_count, filename, buffer);
    //free(x);
    

    int count = 0;
    int packet_size = 0;
    int packet_size_list[total_frag];

    for (count = 0; count < filelen; count ++){
        filedata[byte_count] = buffer[count];
        byte_count ++;

        if ((byte_count == PACKETSIZE) || (count == (filelen - 1))) {
            //printf("%s\n", filedata);
            char* packet = serialize_packet(total_frag, packet_count, byte_count, filename, filedata, &packet_size);
            packets_list[packet_count-1] = malloc (packet_size+1);
            //packets_list[packet_count-1] = packet;
            memcpy(packets_list[packet_count-1],packet, packet_size+1);
            memset(filedata, '\0', byte_count);
            packet_size_list[packet_count-1] = packet_size+1;
            //printf("Packet Size List: %d\n",packet_size_list[packet_count-1]);
            byte_count = 0;
            packet_count ++;
        }
    }

    
    char *protocol_type = strtok(userin, " ");
        
    //printf("%s\n",ftp);

    int a=0;
    for (a = 0; a < total_frag; a++){

        if ((numbytes = sendto(sockfd, packets_list[a], packet_size_list[a], 0, p->ai_addr, p->ai_addrlen)) == -1) {
            perror("talker: sendto");
            exit(1);
        }

        addr_len = sizeof their_addr;
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }

        if (strcmp(buf,yes) == 0) {
        printf("Packet %d Recieved! \n", a+1);
        }
        else {
            return 1;
        }

    }
    
    freeaddrinfo(servinfo);
    close(sockfd);
    
    return 0;
}