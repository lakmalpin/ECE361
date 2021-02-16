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


char* create_packet(int total_frag, int frag_no, int size, char* filename, char* filedata){

    char *t_f = (char*) malloc(sizeof(char));
    char *f_n = (char*) malloc(sizeof(char));
    char *s = (char*) malloc(sizeof(char));

    char colon = ':';
    size_t s_colon = sizeof(colon);

    char *packet = (char*) malloc(3*sizeof(char) + 4*s_colon + strlen(filename) + size*(sizeof(size)));
    packet[0] = '\0';

    sprintf(t_f, "%d", total_frag);

    // printf("T_F: %d\n",sizeof(*t_f));
    // printf("F_N: %d\n",sizeof(*f_n));
    // printf("S: %d\n",sizeof(*s));

    sprintf(f_n, "%d", frag_no);
    sprintf(s, "%d", size);
  
    strcat (packet, t_f);
    strcat (packet, ":");
    strcat (packet, f_n);
    strcat (packet, ":");
    strcat (packet, s);
    strcat (packet, ":");
    strcat (packet, filename);
    strcat (packet, ":");
    memcpy (packet + strlen(packet), filedata, sizeof(size)*size);

    free(t_f);
    free(f_n);
    free(s);
    
    printf("%s\n", packet);

    return packet;
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
    char filedata[PACKETSIZE];
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
    char *buffer;
    long filelen;

    file = fopen(filename,"rb");
    fseek(file,0,SEEK_END);
    filelen = ftell(file);
    rewind(file);

    buffer = (char *)malloc(filelen * sizeof(char));
    fread(buffer, filelen, 1 ,file);
    fclose(file);

    total_frag = find_total_frag(filelen);
    printf("Total Frag: %d\n",total_frag);

    //char* example_packet = create_packet(3,1,1000,"hello.txt","Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Aenean commodo ligula eget dolor. Aenean massa. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Donec quam felis, ultricies nec, pellentesque eu, pretium quis, sem. Nulla consequat massa quis enim. Donec pede justo, fringilla vel, aliquet nec, vulputate eget, arcu. In enim justo, rhoncus ut, imperdiet a, venenatis vitae, justo. Nullam dictum felis eu pede mollis pretium. Integer tincidunt. Cras dapibus. Vivamus elementum semper nisi. Aenean vulputate eleifend tellus. Aenean leo ligula, porttitor eu, consequat vitae, eleifend ac, enim. Aliquam lorem ante, dapibus in, viverra quis, feugiat a, tellus. Phasellus viverra nulla ut metus varius laoreet. Quisque rutrum. Aenean imperdiet. Etiam ultricies nisi vel augue. Curabitur ullamcorper ultricies nisi. Nam eget dui. Etiam rhoncus. Maecenas tempus, tellus eget condimentum rhoncus, sem quam semper libero, sit amet adipiscing sem neque sed ipsum. N");

    //printf("Size of packet: %d\n", sizeof(example_packet));
    //char* packets_list = malloc(total_frag*(3*sizeof(int)+4*sizeof(char)+strlen(filename)+1000*sizeof(char)));
    char* packets_list[total_frag];

    int count = 0;

    for (count = 0; count < filelen; count ++){
        filedata[byte_count] = buffer[count];
        byte_count ++;

        if ((byte_count == PACKETSIZE) || (count == (filelen - 1))) {
            //printf("File data: %s\n", filedata);
            char *packet = create_packet(total_frag, packet_count, byte_count, filename, filedata);
            packets_list[packet_count-1] = malloc (sizeof *packets_list[packet_count-1]);
            packets_list[packet_count-1] = packet;
            memset(filedata, '\0', byte_count);
            //printf("%s\n",packets_list[packet_count-1]);
            byte_count = 0;
            packet_count ++;
        }
    }

    
    char *protocol_type = strtok(userin, " ");
        
    //printf("%s\n",ftp);

    int a = 0;

    // for (a = 0; a < total_frag; a++){

    //     if ((numbytes = sendto(sockfd, packets_list[a], strlen(packets_list[a]), 0, p->ai_addr, p->ai_addrlen)) == -1) {
    //         perror("talker: sendto");
    //         exit(1);
    //     }

    //     addr_len = sizeof their_addr;
    //     if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
    //         perror("recvfrom");
    //         exit(1);
    //     }

    //     if (strcmp(buf,yes) == 0) {
    //     printf("Acknowledgment Recieved! \n");
    //     }
    //     else {
    //         return 1;
    //     }

    // }
    
    freeaddrinfo(servinfo);
    close(sockfd);
    
    return 0;
}