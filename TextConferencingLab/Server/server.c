#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>

#define PORT "9035"   // port we're listening on

/*
Fix command message vs. regular message
Fix - you can createsession with same name
List: Should list which session the clients are in.
Quit: something abt this I forgot what we need to fix.
Additional Feature 1: client can join multiple sessions
Additional Feature 2: Client timeout after some time


*/

struct user{

    char* username;
    char* ip; //might not need
    char* port; //might not need
    int fd;
    int sess_id;
    struct user* next;

};

struct online{

    struct user* head;
    //struct session* sessions;
    int size;
  
};

struct actives{

    int sess_id;
    int fd;

};

struct session{
  
    struct actives* users;//max possible sessions = 10, 10 users max for each
    int sess_id;
    int active;
    int size;
    char* name;

};

struct online* onlineq;
struct session* sessq;

void enqueue(struct user* enter, struct online* onlineq){
                                                                                                             
    struct user* old_head = onlineq->head;

    if(onlineq->head == NULL){
        onlineq->head = enter;
        enter->next = NULL;
        onlineq->size= 1;                                                                                                                    
        return;
    }

    if(old_head->next == NULL){
        old_head->next = enter;
        enter->next = NULL;
        onlineq->size = 2;                                                                                                                    
        return;
    }

      while(old_head!= NULL){

          if(old_head->next == NULL){
              old_head->next = enter;
              enter->next = NULL;
              onlineq->size+=1;                                                                                                                   
              return;
          }

          old_head = old_head->next;
      }
  }


  void dequeue(struct user* leave, struct online* onlineq){

    if(leave == NULL) return;

    struct user* find = onlineq->head;

    //only one element                                                                                                                              
    if(find->next == NULL){
        onlineq->head = onlineq->head->next;
        return;
    }

    //head of queue                                                                                                                                 
    if(onlineq->head->username == leave->username){
        onlineq->head = onlineq->head->next;
        find->next= NULL;
        onlineq->size-=1;
        return;
    }

    else{
        struct user*prev;
        //for specific one that is not the head                                                                                                       
        while(find != NULL){
            if(find->username == leave->username){
                prev->next = find->next;
                find->next = NULL;
                onlineq->size-=1;
                return;
            }
            prev = find;
            find = find->next;
        }                                                                                                                  
    }
                                                                                                                                    
}


/*
void enqueue_sess(struct session* enter){
  struct user* old_head = actq->head;

  if(actq->head == NULL){
    actq->head = enter;
    enter->next = NULL;
    actq->size= 1;
    return;
  }

  if(actq->next == NULL){
    old_head->next = enter;
    enter->next = NULL;
    actq->size = 2;
    return;
  }

    while(old_head!= NULL){

      if(old_head->next == NULL){
        old_head->next = enter;
        enter->next = NULL;
        actq->size+=1;
        return;
      }

      old_head = old_head->next;
    }
}
*/

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void process_user_message(char *buf,int fds, char* ipstr, struct online* onlineq, struct session* sessq)
{

  char userIDs [3][10] = {
        "user1",
        "user2",
        "user3"
    };

  char passwords [3][10] = {
        "pass1",
        "pass2",
        "pass3"
    };

  char* command;
  char com[50];
  

  command = strtok(buf,":");
  strcpy(com,command);
  char* type = strtok(NULL,":");

  if(strcmp(com,"/login") == 0){
    
    char* log = strtok(NULL,":");
    char logstr[50];
    strcpy(logstr,log);

    struct user* temp = onlineq->head;
    while(temp != NULL)
    {
      if (temp->fd == fds){
        char *message = "You are already logged in\n";
        send(fds,message,strlen(message),0);
        return;
      }
      temp = temp->next;

    }


    char* pass = strtok(NULL,":");
    char passtr[50];
    strcpy(passtr,pass);
  
  if(onlineq->head == NULL){
    int i;
    for (i = 0; i < 3; i++)
    {

      if(strcmp(userIDs[i],logstr) == 0 && strcmp(passtr,passwords[i]) == 0)
      {

        struct user* new_user = (struct user*)malloc(sizeof(new_user));
        new_user->username = malloc(sizeof(char)*100);
        new_user->ip = malloc(sizeof(char*));
        //new_user->next = malloc(sizeof(struct user*));
        new_user->username = strdup(userIDs[i]);
        //strcpy(new_user->username,userIDs[i]);
        //	strcpy(new_user->password,pass);
        new_user->fd = fds;
        new_user->sess_id = -1;
        new_user->ip = strdup(ipstr);
        new_user->next = NULL;
        enqueue(new_user, onlineq);
        char*message = "Successfully logged in\n";
        send(fds,message,strlen(message),0);
        return;
      }
    }
  }

	else{
	  struct user* temp = onlineq->head;
	  while(temp != NULL){

	    if(strcmp(temp->username,logstr)==0){
	      char* message = "Already logged on\n";
	      //printf("Already logged on, try a different username\n");
	      send(fds,message,strlen(message),0);
        return;
	    }

	    temp=temp->next;
	  }

    int i;
    for (i = 0; i < 3; i++)
    {
      if(strcmp(userIDs[i],logstr) == 0 && strcmp(passtr,passwords[i]) == 0)
      {
        struct user* new_user = (struct user*)malloc(sizeof(new_user));
        new_user->username = malloc(sizeof(char*));
        new_user->ip = malloc(sizeof(char*));
        new_user->username = strdup(userIDs[i]);
        new_user->fd = fds;
        new_user->sess_id = -1;
        new_user->ip = strdup(ipstr);
        new_user->next = NULL;
        enqueue(new_user, onlineq);
        char*message = "Successfully logged in\n";
        if(send(fds,message,strlen(message),0) == -1){
          perror("send");
          exit(1);
        }
	      return;
      }
    }
      char*message = "Username or password incorrect\n";
      if(send(fds,message,strlen(message),0) == -1){
	        perror("send");
	        exit(1);
      }   
      return;
    }  
  }
  
  // else if(strcmp(com,"/quit") == 0){
  //   //should put in the main OR might not implement here

  //   //should use close() function
  // }

  else{

    struct user* curr = onlineq->head;
    int found = 0;

    while(curr != NULL){

      if(curr->fd == fds){
        found = 1;
        break; //so that we know which one we talking about
      }

      curr=curr->next;

    }

    if(found == 0){
      char* message = "You need to login first\n";
      if(send(fds,message,strlen(message),0) == -1){
        perror("send");
        exit(1);
      }
      return;
    }  //if tou get an error its prolly this
    
    if(strcmp(com,"/createsession") == 0){
    
      char* sess_id = strtok(NULL,":");
      sess_id = strtok(NULL,":");
      sess_id = strdup(sess_id);
      sess_id[strcspn(sess_id, "\n")] = 0;

      if (curr->sess_id != -1)
      {
        char*message = "You are already in a session. Please leave session first.\n";
        send(fds,message,strlen(message),0);
        return;
      }

      int counter = 0;
      while(counter < 10) 
      {
        if (sessq[counter].name != NULL)
        {
          if(strcmp(sessq[counter].name,sess_id) == 0)
          {
              char*message = "Session already in use\n";
              send(fds,message,strlen(message),0);
              return;
          }
        }
        else
        {
          if (sessq[counter].active == 0)
          {
              sessq[counter].name = sess_id;
              sessq[counter].users[0].fd = curr->fd;
              sessq[counter].active = 1;
              sessq[counter].size++;
              curr->sess_id = counter;
              char message[27];
              sprintf(message,"You have joined session %s\n",sess_id);
              send(fds,message,strlen(message),0);
              return;
          }
        }

        counter++;
      }

      char*message = "No sessions available\n";
      send(fds,message,strlen(message),0);
      return;
      

      // if(sessq[id].active == 1){

      //   char*message = "Session already in use\n";
      //         send(fds,message,strlen(message),0);
      //   return;
      // }
      
      //enqueue the sessions

      //      enqueue_sess(sessq[sess_id]);
      
    } 

    else if(strcmp(com,"/joinsession") == 0){
    
      char* sess_id = strtok(NULL,":");
      sess_id = strtok(NULL,":");
      

      int counter = 0;

      while (counter < 10)
      {
        if (sessq[counter].name != NULL){
          if(strcmp(sessq[counter].name, sess_id) == 0 && sessq[counter].size != 10 && sessq[counter].active == 1){
              for(int i = 0; i < 10; i++){
                  if(sessq[counter].users[i].fd == -1){
                      sessq[counter].users[i].fd = fds;
                      sessq[counter].size++;
                      curr->sess_id = counter;
                      char* message = "Joined session\n";
                      send(fds,message,strlen(message),0);
                      return;
                  }
              }
          }
        }
        counter ++;
      }
        
      char* message = "Session is not available\n";
      send(fds,message,strlen(message),0);
      return;  
    }

    else if(strcmp(com,"/leavesession") == 0){

      int id = curr->sess_id;

      if(curr->sess_id != -1){
        for(int i =0;i<10;i++){
          if(sessq[id].users[i].fd == fds){
            sessq[id].users[i].fd = -1;
            sessq[id].size--;

            if(sessq[id].size == 0){
              sessq[id].active = 0;
              memset(sessq[id].name, 0, strlen(sessq[id].name));
            }
            
            curr->sess_id = -1;
            char message[13];
            sprintf(message,"Left session %s\n",sessq[id].name);
            send(fds,message,strlen(message),0);
            memset(message, 0, strlen(message));
            return;
          }
        }
      }

      else{
        char* message = "Can only leave sessions you're in\n";
        send(fds,message,strlen(message),0);
        return;
      }
      
    }

    else if(strcmp(com,"/list") == 0){


      struct user* head = onlineq->head;
      char message[1000];
      memset(message, 0, 1000);

      strcpy(message, "Clients online: ");

      char users[500];
      while(head!=NULL){
        sprintf(users, "%s, ", head->username);
        strcat(message, users);
        head=head->next;
      }
      strcat(message, "\nSessions available: ");



      char sessions[250];
      memset(sessions, 0, 250);
      for(int i =0;i<10;i++){
        if(sessq[i].active == 1 && sessq[i].size < 10){
          sprintf(sessions," %s, ",sessq[i].name);
          strcat(message,sessions);
          memset(sessions, 0, 250);
          
        }
      }
      send(fds, message, strlen(message),0);
      return;
    }

    else if((strcmp(com,"/logout") == 0) || (strcmp(com,"/quit") == 0)){

      //if in session leave
      
      if(curr->sess_id != -1){
        int id = curr->sess_id;
        for(int i =0;i<10;i++){
          if(sessq[id].users[i].fd == fds){
            sessq[id].users[i].fd = -1;
            sessq[id].size--;

            if(sessq[id].size == 0){
              sessq[id].active = 0;
              sessq[id].name = NULL;
            }
            
            curr->sess_id = -1;
            char message[13];
            sprintf(message,"Left session %d\n",id);
            send(fds,message,strlen(message),0);
            break;
          }
        }
      }

      //now time to log out of sys
      dequeue(curr, onlineq);
      free(curr);
      char* mo = "Logging out\n";
      send(fds,mo,strlen(mo),0);
      return;
      
    }
  
    else{


      if(curr->sess_id != -1){

        for(int i=0;i<10;i++){

          if(sessq[curr->sess_id].users[i].fd != fds && sessq[curr->sess_id].users[i].fd != -1){

            char* msg = strtok(NULL,"\0");
            send(sessq[curr->sess_id].users[i].fd,msg,strlen(msg),0);
            memset(msg, 0, strlen(msg));
            
          }
        }
	      return;
      }
      
      char*message = "Not a supported command";
    
      send(fds,message,strlen(message),0);
      return;

    }
  }
}

int main(void)
{
    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

    int listener;     // listening socket descriptor
    int newfd;        // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;

    char buf[256];    // buffer for client data
    char welcome_message[40] = "Welcome to the Text Conferencing Server";
    int nbytes;

    char remoteIP[INET6_ADDRSTRLEN];

    int yes=1;        // for setsockopt() SO_REUSEADDR, below
    int i, j, rv;

    struct addrinfo hints, *ai, *p;

    struct online* onlineq=(struct online*)malloc(sizeof(struct online));
    onlineq->head = NULL;
    printf("Struct: %d\n",onlineq->head != NULL);
    struct session* sessq =(struct session*)malloc(sizeof(struct session)*10);

    for(int j = 0;j<10;j++){

     struct actives* g = (struct actives*)malloc(sizeof(struct actives)*10);

     for(int k=0;k<10;k++){
       g[k].fd = -1;
       g[k].sess_id= -1;
     }
     
     sessq[j].users = g;
     sessq[j].active = 0;
     sessq[j].sess_id=j;
     sessq[j].size = 0;
    }
    
    onlineq->size = 0;
    
    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
    
    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) { 
            continue;
        }
        
        // lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    // if we got here, it means we didn't get bound
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }

    //Inital Setup complete -----

    char s[INET6_ADDRSTRLEN];

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
        s, sizeof s);
    printf("Text Conferencing Server: Now active at %s\n", s);

    freeaddrinfo(ai); // all done with this

    printf("Text Conferencing Server: waiting for connections...\n");

    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }
    
    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one
    
    // main loop
    for(;;) {
        read_fds = master; // copy it
        //select monitors sets of file descriptors (readfds,writefds,exceptds) and returns the file descriptors 
        //that are ready for some sort of interaction (reading or writing).
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                //there is a new connection waiting on the server port
                if (i == listener) {
                    // handle new connections
                    addrlen = sizeof remoteaddr;
                    //accept returns a brand new fd for the incoming connection, while the initial fd that 
                    //is the server's remains untouched.
                    newfd = accept(listener,
                        (struct sockaddr *)&remoteaddr,
                        &addrlen);

                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
                        printf("Text Conferencing Server: new connection from %s on "
                            "socket %d\n",
                            inet_ntop(remoteaddr.ss_family,
                                get_in_addr((struct sockaddr*)&remoteaddr),
                                remoteIP, INET6_ADDRSTRLEN),
                            newfd);
                        send(newfd, welcome_message, strlen(welcome_message),0);
                        break;
                    }
                } else {
                    // handle data from a client
                    if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes != 0) {
                            printf("Text Conferencing Server: socket %d Quit\n", i);
                        }
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    } else {
		      struct sockaddr_storage addr;
		      socklen_t len = sizeof addr;
		      char ipstr[INET6_ADDRSTRLEN];
		      //int porter;

		      int suc = getpeername(i,(struct sockaddr*)&addr,&len);

		      if(addr.ss_family == AF_INET){
			  struct sockaddr_in *c = (struct sockaddr_in *)&addr;
			  //  porter = ntohs(c->sin_port);
			  //printf("after porter %s\n",c->sin_port);
			  inet_ntop(AF_INET, &c->sin_addr, ipstr, INET_ADDRSTRLEN);
			  //printf("%c\n",ef);
		      }
		      else { // AF_INET6
			  struct sockaddr_in6 *c = (struct sockaddr_in6 *)&addr;
			  //printf("inside else\n");
			  //porter = ntohs(c->sin6_port);
			  inet_ntop(AF_INET6, &c->sin6_addr, ipstr,INET_ADDRSTRLEN);
		      }
		      //printf("proces\n");
		      process_user_message(buf,i,ipstr, onlineq, sessq);
                        //printf("Data: %s\n", buf);
                        // for(j = 0; j <= fdmax; j++) {
                        //     // send to everyone!
                        //     if (FD_ISSET(j, &master)) {
                        //         // except the listener and ourselves
                        //         if (j != listener && j != i) {
                        //             if (send(j, buf, nbytes, 0) == -1) {
                        //                 perror("send");
                        //             }
                        //         }
                        //     }
                        // }
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!


    free(sessq);
    free(onlineq);
    
    return 0;
}
