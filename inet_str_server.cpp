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

void *child_server(void* newsoc);//handles a connection
void perror_exit(const char *message);// print error and exit
void sigchld_handler (int sig);//handler for Reap dead children asynchronously
int connect_to_sock(char *ipaddr ,char* sock_num);//connects togiven socket

LinkedList* list;//list holding all client tuples
pthread_mutex_t list_lock = PTHREAD_MUTEX_INITIALIZER;//locks list operations


int main(int argc, char *argv[]) {
  int port, sock, newsock;
  struct sockaddr_in server;
  struct sockaddr *serverptr=(struct sockaddr *)&server;

  list = new LinkedList();

//---------------------------------------------parsing command line argumets------------------------------------------
  int opt;
  /*
  p server port
  */
  while((opt = getopt(argc, argv, "p:")) != -1) {
    switch(opt){
      case 'p':
        port=atoi(optarg);
        break;
    }
  }



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
  printf("Listening for connections to server port %d\n", port);
  while (1) {
    /* accept connection */
    if ((newsock = accept(sock, NULL, NULL)) < 0) perror_exit("accept");
    printf("Accepted connection\n");
    pthread_t t;
    pthread_create(&t, NULL, child_server, (void *)&newsock);
  }

  return 0;
}

//handles a connection
void *child_server(void *newsoc){
  printf("child thread id = %lu\n", pthread_self());

  int newsock= *(int*)newsoc;
  char buf[256],tmp[50];
  tmp[0]='\0';

  while(read(newsock, buf, 1) > 0){
    buf[1]='\0';
    printf("%s\n",buf);
    //pirame tin entoli
    if(buf[0]==' '){
      printf("rcved %s ", tmp);
      //--------------------------------------an ine LOG_ON
      if(strcmp(tmp,"LOG_ON")==0){
        iptuple tmp_tuple("a","b");
        tmp[0]='\0';
        //1 arg ip
        while(read(newsock, buf, 1) > 0){
          if(buf[0]=='<')
            continue;
          if(buf[0]==',')
            break;
          buf[1]='\0';
          strcat(tmp,buf);
        }
        printf(" <%s, ", tmp);
        strcpy(tmp_tuple.ip,tmp);

        //2 arg port
        tmp[0]='\0';
        while(read(newsock, buf, 1) > 0){
          if(buf[0]=='>')
            break;
          buf[1]='\0';
          strcat(tmp,buf);
        }
        printf(" %s>\n", tmp);
        strcpy(tmp_tuple.port,tmp);

        //send to all usrs USR_ON message
        iptuple* list_elem=NULL;
        for(unsigned int i=0;(list_elem=list->get_by_index(i))!=NULL;i++){
          sprintf(buf, "USER_ON <%s, %s>",tmp_tuple.ip,tmp_tuple.port);
          //connect to usr
          int tmp_sock=connect_to_sock(list_elem->ip,list_elem->port);
          //send USER_ON message
          if(write(tmp_sock, buf, strlen(buf)+1) < 0) perror_exit("write");
          close(tmp_sock);
          printf("sended %s\nclosing connection to client\n", buf);
        }

        //update list
        pthread_mutex_lock(&list_lock);
        list->add(tmp_tuple);
        list->print();
        pthread_mutex_unlock(&list_lock);
      }
      //--------------------------------------an ine GET_CLIENTS
      else if(strcmp(tmp,"GET_CLIENTS")==0){
        tmp[0]='\0';
        char *tmp_buf;

        printf("\nsending CLIENT_LIST ");
        if (write(newsock, "CLIENT_LIST ", 12) < 0) perror_exit("write");
        sprintf(tmp, "%d ",list->getlen());
        if (write(newsock, tmp, strlen(tmp)) < 0) perror_exit("write");
        tmp_buf=list->get_string();//TODO na min stelni ton eafto tou
        if (write(newsock, tmp_buf, strlen(tmp_buf)+1) < 0) perror_exit("write");
        printf(" %s\n",tmp_buf);
      }
      //--------------------------------------an ine LOG_OFF
      else if(strcmp(tmp,"LOG_OFF")==0){
        iptuple tmp_tuple("a","b");
        tmp[0]='\0';
        //1 arg ip
        while(read(newsock, buf, 1) > 0){
          if(buf[0]=='<')
            continue;
          if(buf[0]==',')
            break;
          buf[1]='\0';
          strcat(tmp,buf);
        }
        printf(" <%s, ", tmp);
        strcpy(tmp_tuple.ip,tmp);

        //2 arg port
        tmp[0]='\0';
        while(read(newsock, buf, 1) > 0){
          if(buf[0]=='>')
            break;
          buf[1]='\0';
          strcat(tmp,buf);
        }
        printf("%s>\n", tmp);
        strcpy(tmp_tuple.port,tmp);

        //update list
        pthread_mutex_lock(&list_lock);
        list->deleten(tmp_tuple);
        list->print();
        pthread_mutex_unlock(&list_lock);

        //send to all usrs USR_OFF message
        iptuple* list_elem=NULL;
        for(unsigned int i=0;(list_elem=list->get_by_index(i))!=NULL;i++){
          sprintf(buf, "USER_OFF <%s, %s>",tmp_tuple.ip,tmp_tuple.port);
          //connect to usr
          int tmp_sock=connect_to_sock(list_elem->ip,list_elem->port);
          //send USER_OFF message
          if(write(tmp_sock, buf, strlen(buf)+1) < 0) perror_exit("write");
          close(tmp_sock);
          printf("sended %s\nclosing connection to client\n", buf);
        }

        //end child (he loged off)
        printf("Ending child proccess\n");
        return NULL;
      }
      tmp[0]='\0';
    }
    else
      strcat(tmp,buf);

  }

  return NULL;
}

//connects togiven socket
int connect_to_sock(char *ipaddr ,char* sock_num){
  printf("triyng to connect to %s %s\n",ipaddr,sock_num);
  //create socket and connection
  int _sock,_port;
  struct sockaddr_in server;
  struct sockaddr *serverptr = (struct sockaddr*)&server;
  /* Create socket */
  if ((_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) perror_exit("socket");
  /* Find server address */
  struct hostent *rem;
  if ((rem = gethostbyname(ipaddr)) == NULL) {
    printf("%s\n",hstrerror(h_errno));
    perror("gethostbyname");
    exit(1);
  }
  memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
  _port = atoi(sock_num); /*Convert port number to integer*/
  server.sin_family = AF_INET;       /* Internet domain */
  server.sin_port = htons(_port);         /* Server port */
  /* Initiate connection */
  if (connect(_sock, serverptr, sizeof(server)) < 0) perror_exit("connect");
  printf("Connected to %s port %d\n", ipaddr, _port);

  return _sock;
}

// Wait for all dead child processes
void sigchld_handler(int sig) {
  while (waitpid(-1, NULL, WNOHANG) > 0);
}

// print error and exit
void perror_exit(const char *message) {
  perror(message);
  char buffer[256];
  char * errorMsg = strerror_r( errno, buffer, 256 ); // GNU-specific version, Linux default
  printf("Error %s", errorMsg); //return value has to be used since buffer might not be modified

  exit(EXIT_FAILURE);
}
