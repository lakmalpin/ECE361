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

#define PORT "9035"   // port we're listening on

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
};

struct online* onlineq;
struct session* sessq;
//struct active_sess* actq;

void enqueue(struct user* enter){
                                                                                                             
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


void dequeue(struct user* leave){

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

void process_user_message(char *buf,int fds, char* ipstr)
{

    /*
    get command from user message (buf)
    if (command == "login"){
        verify_login(command,x,y...) <- x,y any data structures you need to modify
    }
    else if (command == "logout"){
        logout(...)
    }
    .
    .
    .
    **everyfunction except for verify_login needs to make sure the messaging client is logged into the server already
    **some of these functions need to send an ack or nack back to the sending user
    */

  char* command;

  command = strtok(buf,":");
  //printf("%s\n",command);
  char* type = strtok(NULL,":");

  if(strcmp(command,"login") == 0){

    char* log = strtok(NULL,":"); 
    char* pass = strtok(NULL,":");

    for(int i =0;i<strlen(pass);i++){
      if(isspace(pass[i]) > 0){
        //printf("%d",i);
	pass[i]='\0';
	break;
      }
    }
    
    //printf("%s pass\n",pass);
    // printf("here is from server read %s, %s\n", log, pass);
  
    if(log == NULL | pass == NULL){
      char* message = "Format for login: <username> or <password> missing\n";
      send(fds,message,strlen(message),0);
      return;
    }
    printf("here\n");
    FILE* fc = fopen("login.txt", "r");
    printf("h\n");
    if(fc == NULL){
      perror("fopen");
      return;
    }

    char*str;
    size_t len = 1024;
    char* us;
    char* passe;

    while(getline(&str,&fc) != -1){
      printf("newline\n");
      us = strtok(str," ");
      printf("us is %s\n",us);
      passe = strtok(NULL,"\n");
      for(int i =0;i<strlen(passe);i++){
	if(isspace(passe[i]) > 0){
	  //printf("%d",i);                                                                                                                         
	  passe[i]='\0';
	  break;
	}
      }
      printf("passe is %s\n",passe);
      printf("%d cmp for us, %d cmp for pas\n",strcmp(us,log),strcmp(pass,passe));
      if(strcmp(us,log) == 0 && strcmp(pass,passe) == 0){
	break;
      }
      
    }

    fclose(fc);
    
    if(onlineq->head == NULL){

      if(strcmp(us,log) == 0 && strcmp(pass,passe) == 0){
	char*message = "Successfully logged in\n";
	send(fds,message,strlen(message),0);
	struct user* new_user = (struct user*)malloc(sizeof(new_user));
	strcpy(new_user->username,us);
	//	strcpy(new_user->password,pass);
	new_user->fd = fds;
	new_user->sess_id = -1;
	strcpy(new_user->ip,ipstr);
	new_user->next = NULL;
	
	// new_user->ip = ; //need tp dp
	// new_user-port = ;
      }
    }
	else{

	  struct user* temp = onlineq->head;
	  while(temp != NULL){

	    if(strcmp(temp->username,log)==0){
	      char* message = "Already logged on\n";
	      printf("Already logged on, try a different username\n");
	      send(fds,message,strlen(message),0);
	      return;
	    }

	    temp=temp->next;
	  }

	  char*message = "Successfully logged in\n";
	  if(send(fds,message,strlen(message),0) == -1){
	    perror("send");
	    exit(1);
	  }
	  struct user* new_user = (struct user*)malloc(sizeof(new_user));
	  strcpy(new_user->username,us);
  	  //	strcpy(n
	  return;
	}
  
  
  
  
      char*message = "Username or password incorrect\n";
      if(send(fds,message,strlen(message),0) == -1){
	perror("send");
	exit(1);
      }   
      return;
  }  
  
  else if(strcmp(command,"quit") == 0){
    //should put in the main OR might not implement here

    //should use close() function
  }

  else{

    struct user* curr = onlineq->head;
    int found = 0;

    while(curr != NULL){

      if(curr->fd == fds){

	printf("%s username\n",curr->username);
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
    }  //if tou get an error its prolly this
    
    if(strcmp(command,"createsession") == 0){
    
      char* sess_id = strtok(NULL,":");
     
      for(int i =0;i<strlen(sess_id);i++){
        if(isspace(sess_id[i]) > 0){ 
          sess_id[i]='\0';
          break;
        }
      }

      int id = atoi(sess_id);
 
      if(id < 0 || id > 9){
	char* message = "Can only create sessions with ids between 0-19\n";
	send(fds,message,strlen(message),0);
	return;
      }
      //is it available?

      //struct session* sess_new = (struct session*)malloc(sizeof(struct session));

      if(sessq[id].active == 1){

	char*message = "Session already in use\n";
        send(fds,message,strlen(message),0);
	return;
      }
      
      sessq[id].users[0].fd = curr->fd;
      sessq[id].active = 1;
      sessq[id].size++;
      curr->sess_id = id;

      //enqueue the sessions

      //      enqueue_sess(sessq[sess_id]);
      
      char message[27];
      sprintf(message,"You have joined session %d\n",sess_id);
      send(fds,message,strlen(message),0);
     return;
    } 

    else if(strcmp(command,"joinsession") == 0){
    
      char* sess_id = strtok(NULL,":");

      for(int i =0;i<strlen(sess_id);i++){
        if(isspace(sess_id[i]) > 0){
          //printf("%d",i);                                                                                                                          
          sess_id[i]='\0';
          break;
        }
      }
      
      int id = atoi(sess_id);
      if(id > 9 || id < 0){
	char* message = "Can only join sessions with ids between 0-9\n";
        send(fds,message,strlen(message),0);
        return;
      }

      // struct user* session = sessq[sess_id]->users;

      if(sessq[id].active == 0){
	char* message = "Session not active\n";
        send(fds,message,strlen(message),0);
        return;
	
      }

      if(sessq[id].size == 10){
	char* message = "Session is full\n";
        send(fds,message,strlen(message),0);
        return;
      }

      for(int i =0;i<10;i++){

	if(sessq[id].users[i].fd == -1){

	  sessq[id].users[i].fd = fds;
	  sessq[id].size++;
	  curr->sess_id = id;
	  break;
	}
      }

      char* message = "Joined session\n";
      send(fds,message,strlen(message),0);
      return;
      
    }

    else if(strcmp(command,"leavesession") == 0){

      char* sess_id = strtok(NULL,":");

      for(int i =0;i<strlen(sess_id);i++){
        if(isspace(sess_id[i]) > 0){
          sess_id[i]='\0';
          break;
        }
      }

      int id = atoi(sess_id);
      if(id > 9 || id < 0){
        char* message = "Can only leave sessions with ids between 0-9\n";
        send(fds,message,strlen(message),0);
        return;
      }

      int found = 0;
      
      for(int i =0;i<10;i++){

	if(sessq[id].users[i].fd == fds){
	  found = 1;
	  sessq[id].users[i].fd = -1;
	  sessq[id].size--;
	  
	  if(sessq[id].size == 0){
	    sessq[id].active = 0;
	  }
	  
	  curr->sess_id = -1;
	  break;
	}
      }

      if(found == 0){
	
	char* message = "Can only leave sessions you're in\n";
        send(fds,message,strlen(message),0);
        return;
      }

      char message[13];
      sprintf(message,"Left session %d\n",id);
      send(fds,message,strlen(message),0);
      return;
      
    }

    else if(strcmp(command,"list") == 0){

      struct user* head = onlineq->head;
      printf("Clients online:\n");
      while(head!=NULL){
	printf("username: %s\n",head->username);
	head=head->next;
      }
      printf("Sessions available:\n");

      for(int i =0;i<10;i++){
	if(sessq[i].active == 1 && sessq[i].active < 10){
	  printf("Sessions aviable %d\n",i);
	}
      }
      return;
    }
    else if(strcmp(command,"logout") == 0){

      //if in session leave
      
      if(curr->sess_id != -1){
	int id = curr->sess_id;
	for(int i =0;i<10;i++){
	  if(sessq[id].users[i].fd == fds){
	    sessq[id].users[i].fd = -1;
	    sessq[id].size--;

	    if(sessq[id].size == 0){
	      sessq[id].active = 0;
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


      dequeue(curr);
      free(curr);
      char* mo = "Logging out\n";
      send(fds,mo,strlen(mo),0);
      
      return;
      
    }
  
    else{

      //      struct user* temp = onlineq->head;
      /*
      while(temp!= NULL){

	if(temp->fd == fds){

	  if(temp->sess_id != -1){
	    struct user* session = sessq[temp->users[temp->sess_id];
	
	    while(session!= NULL){

	      if(session->fd != fds) send(session->fd,buf,strlen(buf),0);
	  
	      session++;    
	    }
	  }
	  break;
	}
	temp=temp->next;
      }
      */

      if(curr->sess_id != -1){

	for(int i=0;i<10;i++){

	  if(sessq[curr->sess_id].users[i].fd != fds && sessq[curr->sess_id].users[i].fd != -1){

	    char* msg = strtok(NULL,"\0");
	    send(sessq[curr->sess_id].users[i].fd,msg,strlen(msg),0);
	    
	  }
	}
	return;
      }
      
      char*message = "Not a supported command, please pick one of the below:\n login\njoinsession\nlogout\ncreatesession\nlist\nquit\n or send a custom text message to someone else by joining a session.";
    
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
                        if (nbytes == 0) {
                            // connection closed
                            printf("Text Conferencing Server: socket %d hung up\n", i);
                        } else {
                            perror("recv");
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
			  printf("inside else\n");
			  //porter = ntohs(c->sin6_port);
			  inet_ntop(AF_INET6, &c->sin6_addr, ipstr,INET_ADDRSTRLEN);
		      }
		      printf("proces\n");
		      process_user_message(buf,i,ipstr);
                        printf("Data: %s\n", buf);
                        for(j = 0; j <= fdmax; j++) {
                            // send to everyone!
                            if (FD_ISSET(j, &master)) {
                                // except the listener and ourselves
                                if (j != listener && j != i) {
                                    if (send(j, buf, nbytes, 0) == -1) {
                                        perror("send");
                                    }
                                }
                            }
                        }
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!


    free(sessq);
    free(onlineq);
    
    return 0;
}
