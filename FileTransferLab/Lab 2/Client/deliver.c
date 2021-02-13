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

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
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

    if (access(filename,F_OK) != 0) {
        return 1;
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