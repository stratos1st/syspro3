/* inet_str_client.c: Internet stream sockets client */
#include <stdio.h>
#include <sys/types.h>	     /* sockets */
#include <sys/socket.h>	     /* sockets */
#include <netinet/in.h>	     /* internet sockets */
#include <unistd.h>          /* read, write, close */
#include <netdb.h>	         /* gethostbyaddr */
#include <stdlib.h>	         /* exit */
#include <string.h>	         /* strlen */
#include <arpa/inet.h>      //inet_ntoa
#include <pthread.h>


#include <sys/inotify.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/prctl.h>

#define LOG_ON 1

//TODO na kratai lista apo tuples kai na sbini otan lambani useroff

void perror_exit(char *message);
void kill_signal_handler(int sig);//main signal handler for SIGQUIT and SIGINT
void *rcv_child(void* newsoc);
void *send_child(void* newsoc);
int connect_to_sock(char *ipaddr ,char* sock_num);

//globals for signal handler
int server_port, server_sock,server,port;
char server_symbolicip[50],server_port_str[50];

int main(int argc, char *argv[]){
  char buf[256],tmp[500];
  struct sockaddr_in server;
  struct sockaddr *serverptr = (struct sockaddr*)&server;
  struct hostent *rem;
  pthread_t t;
  int newsock,sock;
  char symbolicip[50];



  //asigning signal handlers
  static struct sigaction act2;
  act2.sa_handler = kill_signal_handler;
  sigfillset (&(act2.sa_mask ));
  sigaction (SIGQUIT, &act2, NULL);
  sigaction (SIGINT, &act2, NULL);


//-------------------------------------create socket and connection to primary server socket
  /* Create socket */
  if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) perror_exit("socket");
  /* Find server address */
  if ((rem = gethostbyname(argv[1])) == NULL) {perror("gethostbyname");exit(1);}
  server_port = atoi(argv[2]); /*Convert port number to integer*/
  strcpy(server_port_str,argv[2]);
  server.sin_family = AF_INET;       /* Internet domain */
  memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
  server.sin_port = htons(server_port);         /* Server port */
  /* Initiate connection */
  if (connect(server_sock, serverptr, sizeof(server)) < 0) perror_exit("connect");
  printf("Connecting to %s port %d\n", argv[1], server_port);

  //find host name and ip address
  if(gethostname(buf, 256)!=0)perror_exit("gethostname");
  printf("%s\n", buf);
  struct hostent * mymachine ;
  struct in_addr ** addr_list;
  //resolve hostname to ip address
  if ( ( mymachine = gethostbyname ( buf) ) == NULL )
    printf ( " Could not resolved Name : %s \n " , buf) ;
  else{
    printf ( " Name To Be Resolved : %s \n " , mymachine -> h_name ) ;
    printf ( " Name Length in Bytes : %d \n " , mymachine -> h_length ) ;
    addr_list = ( struct in_addr **) mymachine -> h_addr_list ;
    strcpy ( server_symbolicip , inet_ntoa (* addr_list [0]) ) ;
    printf ( " %s resolved to %s \n" , mymachine -> h_name ,server_symbolicip ) ;
}


  //send ip address an port number to primary server socket
  sprintf(buf, "LOG_ON <%s, %s>",server_symbolicip,argv[3]);
  if (write(server_sock, buf, strlen(buf)+1) < 0) perror_exit("write");
  strcpy(buf,"GET_CLIENTS ");
  if (write(server_sock, buf, strlen(buf)+1) < 0) perror_exit("write");
  tmp[0]='\0';
  //rcv CLIENT_LIST
  while(read(server_sock, buf, 1) > 0){
    buf[1]='\0';
    printf("%s\n",buf);
    //pirame tin entoli
    if(buf[0]==' '){
      printf("%s\n", tmp);
      //--------------------------------------an ine log_on
      if(strcmp(tmp,"CLIENT_LIST")==0){
        int n;
        // diabazi n
        tmp[0]='\0';
        while(read(server_sock, buf, 1) > 0){
          if(buf[0]==' ')
            break;
          buf[1]='\0';
          strcat(tmp,buf);
        }
        n=atoi(tmp);
        printf(" %d\n",n );

        for(int i=0;i<n;i++){
          //diabase to proto <>
          while(read(server_sock, buf, 1) > 0){
            if(buf[0]=='<'){
              tmp[0]='\0';
              //diabase ip
              while(read(server_sock, buf, 1) > 0){
                if(buf[0]==',')
                  break;
                buf[1]='\0';
                strcat(tmp,buf);
              }
              printf("<%s,",tmp);
              tmp[0]='\0';
              //diabase port
              while(read(server_sock, buf, 1) > 0){
                if(buf[0]=='>')
                  break;
                buf[1]='\0';
                strcat(tmp,buf);
              }
              printf("%s>\n",tmp);
            }
            buf[1]='\0';
            break;
          }
        }
        break;
      }
    }
    else
      strcat(tmp,buf);
  }
  close(server_sock);


  //TODO na ftiaxni to socket pio prin
//-------------------------------------------set up my shocket
  port = atoi(argv[3]);
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
  while (1){
    /* accept connection */
    if ((newsock = accept(sock, NULL, NULL)) < 0) perror_exit("accept");
    /* Find client's address */
    //    	if ((rem = gethostbyaddr((char *) &client.sin_addr.s_addr, sizeof(client.sin_addr.s_addr), client.sin_family)) == NULL) {
    //   	    herror("gethostbyaddr"); exit(1);}
    //    	printf("Accepted connection from %s\n", rem->h_name);
    printf("Accepted connection\n");
    pthread_create(&t, NULL, rcv_child, (void *)&newsock);
    pthread_create(&t, NULL, send_child, (void *)&newsock);
    // pthread_join(t , NULL);
    // close(newsock); /* parent closes socket to client */
  }



// sleep(1);
// strcpy(buf,"END ");
// if (write(sock, buf, strlen(buf)+1) < 0) perror_exit("write");
// sleep(1);
// exit(0);


  return 0;
}

//----------------------------------------------------rcv_child
void *rcv_child(void* newsoc){
  char buf[256];


  return NULL;
}

//----------------------------------------------------send_child
void *send_child(void* newsoc){
  char buf[256];

  int newsock= *(int*)newsoc;
  char tmp[500];
  tmp[0]='\0';
  while(read(newsock, buf, 1) > 0){
    buf[1]='\0';
    printf("%s\n",buf);
    //pirame tin entoli
    if(buf[0]==' '){
      printf("%s\n", tmp);
      //--------------------------------------an ine log_on
      if(strcmp(tmp,"CLIENT_LIST")==0){
        int n;
        // diabazi n
        tmp[0]='\0';
        while(read(newsock, buf, 1) > 0){
          if(buf[0]==' ')
            break;
          buf[1]='\0';
          strcat(tmp,buf);
        }
        n=atoi(tmp);
        printf(" %d\n",n );

        for(int i=0;i<n;i++){
          //diabase to proto <>
          while(read(newsock, buf, 1) > 0){
            if(buf[0]=='<'){
              tmp[0]='\0';
              //diabase ip
              while(read(newsock, buf, 1) > 0){
                if(buf[0]==',')
                  break;
                buf[1]='\0';
                strcat(tmp,buf);
              }
              printf("<%s,",tmp);
              tmp[0]='\0';
              //diabase port
              while(read(newsock, buf, 1) > 0){
                if(buf[0]=='>')
                  break;
                buf[1]='\0';
                strcat(tmp,buf);
              }
              printf("%s>\n",tmp);
            }
            buf[1]='\0';
            break;
          }
        }
      }
    }
    else
      strcat(tmp,buf);
  }

  return NULL;
}

void perror_exit(char *message){
  perror(message);
  exit(EXIT_FAILURE);
}

//main signal handler for SIGQUIT and SIGINT
void kill_signal_handler(int sig){
  fflush(stdout);
  char buf[100];

  server_sock=connect_to_sock(server_symbolicip,server_port_str);
  sprintf(buf, "LOG_OFF <%s, %d>",server_symbolicip,port);
  printf("sending %s\n",buf );
  if (write(server_sock, buf, strlen(buf)+1) < 0) perror_exit("write");

  exit(0);
}

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
