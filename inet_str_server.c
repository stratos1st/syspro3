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

//TODO na stelni useroff


using namespace std;

LinkedList* list;
pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER;

void *child_server(void* newsoc);
void perror_exit(char *message);
void sigchld_handler (int sig);
int connect_to_sock(char *ipaddr ,char* sock_num);


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
    cout<<"a\n";
    /* accept connection */
    if ((newsock = accept(sock, NULL, NULL)) < 0) perror_exit("accept");
    /* Find client's address */
    //    	if ((rem = gethostbyaddr((char *) &client.sin_addr.s_addr, sizeof(client.sin_addr.s_addr), client.sin_family)) == NULL) {
    //   	    herror("gethostbyaddr"); exit(1);}
    //    	printf("Accepted connection from %s\n", rem->h_name);
    printf("Accepted connection\n");
    pthread_t t;
    pthread_create(&t, NULL, child_server, (void *)&newsock);
    // pthread_join(t , NULL);
    // close(newsock); /* parent closes socket to client */
  }

  return 0;
}

void *child_server(void *newsoc){
  printf("thread id = %d\n", pthread_self());

  int newsock= *(int*)newsoc;
  char buf[256],tmp[50];
  tmp[0]='\0';

  while(read(newsock, buf, 1) > 0){
    buf[1]='\0';
    printf("%s\n",buf);
    //pirame tin entoli
    if(buf[0]==' '){
      printf("%s\n", tmp);
      //--------------------------------------an ine LOG_ON
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

        iptuple* list_elem=NULL;
        for(unsigned int i=0;(list_elem=list->get_by_index(i))!=NULL;i++){
          sprintf(buf, "USER_ON <%s, %d>",tmp_tuple.ip,tmp_tuple.port);
          int tmp_sock=connect_to_sock(list_elem->ip,list_elem->port);
          if(write(tmp_sock, buf, strlen(buf)+1) < 0) perror_exit("write");
          close(tmp_sock);
          printf("%s\n", buf);
        }

        pthread_mutex_lock(&counter_lock);
        list->add(tmp_tuple);
        list->print();
        pthread_mutex_unlock(&counter_lock);
      }
      //--------------------------------------an ine GET_CLIENTS
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
      //--------------------------------------an ine LOG_OFF
      else if(strcmp(tmp,"LOG_OFF")==0){
        iptuple tmp_tuple("a","b");
        tmp[0]='\0';
        printf("loged off with ");
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
        list->deleten(tmp_tuple);
        list->print();
        pthread_mutex_unlock(&counter_lock);

        iptuple* list_elem=NULL;
        for(unsigned int i=0;(list_elem=list->get_by_index(i))!=NULL;i++){
          sprintf(buf, "USER_OFF <%s, %s>",tmp_tuple.ip,tmp_tuple.port);
          int tmp_sock=connect_to_sock(list_elem->ip,list_elem->port);
          if(write(tmp_sock, buf, strlen(buf)+1) < 0) perror_exit("write");
          close(tmp_sock);
          printf("%s\n", buf);
        }

        return NULL;
      }
      // else if(strcmp(tmp, "END")==0){
      //   printf("ENDED\n");
      //   break;
      // }
      tmp[0]='\0';
    }
    else
      strcat(tmp,buf);

  }

  printf("Closing connection .\n");
  close(newsock);	  /* Close socket */

  return NULL;
}

//TODO sinal handler gia exit na sbini mnimi

int connect_to_sock(char *ipaddr ,char* sock_num){
  //create socket and connection
  int sock,port;
  struct sockaddr_in server;
  struct sockaddr *serverptr = (struct sockaddr*)&server;
  struct hostent *rem;
  /* Create socket */
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) perror_exit("socket");
  /* Find server address */
  if ((rem = gethostbyname(ipaddr)) == NULL) {perror("gethostbyname");exit(1);}
  port = atoi(sock_num); /*Convert port number to integer*/
  server.sin_family = AF_INET;       /* Internet domain */
  memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
  server.sin_port = htons(port);         /* Server port */
  /* Initiate connection */
  if (connect(sock, serverptr, sizeof(server)) < 0) perror_exit("connect");
  printf("Connecting to %s port %d\n", ipaddr, port);

  return sock;
}

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
