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

int find_total_frag(int size)
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

void create_packet(int total_frag, int frag_no, int size, char* filename, char* filedata, char* packet){
    char t_f[100];
    char t_n[100];
    char s[100];

    sprintf(t_f, "%d", total_frag);
    sprintf(t_n, "%d", frag_no);
    sprintf(s, "%d", size);

    printf("Frag Number: %d\n", frag_no);

    strcat (packet, t_f);
    strcat (packet, ":");
    strcat (packet, t_n);
    strcat (packet, ":");
    strcat (packet, s);
    strcat (packet, ":");
    strcat (packet, filename);
    strcat (packet, ":");
    strcat (packet, filedata);
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

    int c;
    int d;
    int num_bytes = 0;
    int total_frag;
    char filedata[PACKETSIZE];
    int byte_count = 0;
    int total_byte_count = 0;
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
    file = fopen(filename,"r");
    if(file){
        while ((c = getc(file)) != EOF) {
            num_bytes = num_bytes + 1;
        }
        printf("%d\n",num_bytes);

        total_frag = find_total_frag(num_bytes);

        printf("Total Frag: %d\n",total_frag);

        fclose(file); 
    }
    else{
        return 1;
    }

    const char *packets_list[total_frag];
    char packet[PACKETSIZE + 100]; 
    file = fopen(filename, "r");

    if(file){
        while ( (d = getc(file)) != EOF ){

            filedata[byte_count] = d;
            byte_count ++;
            total_byte_count ++;

            if ((byte_count == PACKETSIZE) || (total_byte_count == num_bytes)) {
                create_packet(total_frag, packet_count, byte_count, filename, filedata, packet);
                packets_list[packet_count] = packet;
                printf("%s\n",packet);
                memset(packet, '\0', PACKETSIZE + 100);
                memset(filedata, '\0', byte_count);
                byte_count = 0;
                packet_count ++;
            }

        }
        
        fclose(file);      
    }

    char *protocol_type = strtok(userin, " ");
        
    //printf("%s\n",ftp);

    if ((numbytes = sendto(sockfd, protocol_type, strlen(protocol_type), 0, p->ai_addr, p->ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1);
    }

    freeaddrinfo(servinfo);
    
    //printf("talker: sent %d bytes to %s\n", numbytes, argv[1]);
    
    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }

    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);

    buf[numbytes] = '\0';
    printf("message from server: \"%s\"\n", buf);

    if (strcmp(buf,yes) == 0) {
        printf("A file transfer can start. \n");
    }
    else {
        return 1;
    }
    
    close(sockfd);
    
    return 0;
}