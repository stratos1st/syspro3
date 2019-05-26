/*inet_str_server.c: Internet stream sockets server */
#include <iostream>
#include <stdio.h>
#include <sys/wait.h>	     /* sockets */
#include <sys/types.h>	     /* sockets */
#include <sys/socket.h>	     /* sockets */
#include <netinet/in.h>	     /* internet sockets */
#include <netdb.h>	         /* gethostbyaddr */
#include <unistd.h>	         /* fork */
#include <stdlib.h>	         /* exit */
#include <ctype.h>	         /* toupper */
#include <signal.h>          /* signal */
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "linked_list.h"
#include "tuple.h"

using namespace std;

LinkedList* list;
pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER;

void *child_server(void* newsoc);
void perror_exit(char *message);
void sigchld_handler (int sig);

int main(int argc, char *argv[]) {
  int port, sock, newsock;
  struct sockaddr_in server, client;
  socklen_t clientlen;
  struct sockaddr *serverptr=(struct sockaddr *)&server;
  struct sockaddr *clientptr=(struct sockaddr *)&client;
  struct hostent *rem;
  //check for arg errors
  if (argc != 2) {
    printf("Please give port number\n");
    exit(1);
  }
  port = atoi(argv[1]);
  list = new LinkedList();



//-------------------------------------------------------opening ports initializing
  /* Reap dead children asynchronously */
  signal(SIGCHLD, sigchld_handler);
  /* Create socket */
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) perror_exit("socket");
  server.sin_family = AF_INET;       /* Internet domain */
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  server.sin_port = htons(port);      /* The given port */
  /* Bind socket to address */
  if (bind(sock, serverptr, sizeof(server)) < 0) perror_exit("bind");
  /* Listen for connections */
  if (listen(sock, 5) < 0) perror_exit("listen");

//------------------------------------------------------waiting for conections
  printf("Listening for connections to port %d\n", port);
  while (1) {
    /* accept connection */
    if ((newsock = accept(sock, NULL, NULL)) < 0) perror_exit("accept");
    /* Find client's address */
    //    	if ((rem = gethostbyaddr((char *) &client.sin_addr.s_addr, sizeof(client.sin_addr.s_addr), client.sin_family)) == NULL) {
    //   	    herror("gethostbyaddr"); exit(1);}
    //    	printf("Accepted connection from %s\n", rem->h_name);
    printf("Accepted connection\n");
    pthread_t t;
    pthread_create(&t, NULL, child_server, (void *)&newsock);
    pthread_join(t , NULL);
    // close(newsock); /* parent closes socket to client */
  }

  return 0;
}

void *child_server(void *newsoc) {
  int newsock= *(int*)newsoc;
  char buf[256],tmp[50];
  tmp[0]='\0';

  while(read(newsock, buf, 1) > 0){
    buf[1]='\0';
    printf("%s\n",buf);
    //pirame tin entoli
    if(buf[0]==' '){
      printf("%s\n", tmp);
      //--------------------------------------an ine log_on
      if(strcmp(tmp,"LOG_ON")==0){
        iptuple tmp_tuple("a","b");
        tmp[0]='\0';
        printf("loged on with ");
        //1 arg ip
        while(read(newsock, buf, 1) > 0){
          if(buf[0]=='<')
            continue;
          if(buf[0]==',')
            break;
          buf[1]='\0';
          strcat(tmp,buf);
        }
        printf(" ip %s and", tmp);
        strcpy(tmp_tuple.ip,tmp);

        //2 arg port
        tmp[0]='\0';
        while(read(newsock, buf, 1) > 0){
          if(buf[0]=='>')
            break;
          buf[1]='\0';
          strcat(tmp,buf);
        }
        printf(" port %s\n", tmp);
        strcpy(tmp_tuple.port,tmp);

        pthread_mutex_lock(&counter_lock);
        list->add(tmp_tuple);
        tmp[0]='\0';
        list->print();
        pthread_mutex_unlock(&counter_lock);
      }
      else if(strcmp(tmp,"GET_CLIENTS")==0){
        tmp[0]='\0';
        char *tmp_buf;

        if (write(newsock, "CLIENT_LIST ", 12) < 0) perror_exit("write");
        sprintf(tmp, "%d ",list->getlen());
        if (write(newsock, tmp, strlen(tmp)) < 0) perror_exit("write");
        tmp_buf=list->get_string();
        cout<<tmp_buf<<endl;
        if (write(newsock, tmp_buf, strlen(tmp_buf)+1) < 0) perror_exit("write");

      }
      else if(strcmp(tmp, "END")==0){
        printf("ENDED\n" );
        break;
      }
    }
    else
      strcat(tmp,buf);

  }

  printf("Closing connection.\n");
  close(newsock);	  /* Close socket */
}

//TODO sinal handler gia exit na sbini mnimi

/* Wait for all dead child processes */
void sigchld_handler (int sig) {
  while (waitpid(-1, NULL, WNOHANG) > 0);
}

void perror_exit(char *message) {
  perror(message);
  char buffer[256];
  char * errorMsg = strerror_r( errno, buffer, 256 ); // GNU-specific version, Linux default
  printf("Error %s", errorMsg); //return value has to be used since buffer might not be modified

  exit(EXIT_FAILURE);
}
