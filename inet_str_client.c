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

#define LOG_ON 1

void perror_exit(char *message);

int main(int argc, char *argv[]) {
  int             port, sock, i;
  char            buf[256];
  struct sockaddr_in server, client;
  struct sockaddr *serverptr = (struct sockaddr*)&server;
  struct hostent *rem;

  //check for arg errors
  if (argc != 3) {
    printf("Please give host name and port number\n");
    exit(1);
  }




  //create socket and connecton
  /* Create socket */
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) perror_exit("socket");
  /* Find server address */
  if ((rem = gethostbyname(argv[1])) == NULL) {perror("gethostbyname");exit(1);}
  port = atoi(argv[2]); /*Convert port number to integer*/
  server.sin_family = AF_INET;       /* Internet domain */
  memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
  server.sin_port = htons(port);         /* Server port */
  /* Initiate connection */
  if (connect(sock, serverptr, sizeof(server)) < 0) perror_exit("connect");

  printf("Connecting to %s port %d\n", argv[1], port);

  // int a=LOG_ON;
  // if (write(sock, &a, sizeof(int)) < 0) perror_exit("write");
  /* Find client's address */

//find host name and ip address
if(gethostname(buf, 256)!=0){perror_exit("gethostname");}
printf("%s\n", buf);
struct hostent * mymachine ;
char symbolicip [50];
struct in_addr ** addr_list;
if ( ( mymachine = gethostbyname ( buf) ) == NULL )
  printf ( " Could not resolved Name : %s \n " , buf) ;
else{
  printf ( " Name To Be Resolved : %s \n " , mymachine -> h_name ) ;
  printf ( " Name Length in Bytes : %d \n " , mymachine -> h_length ) ;
  addr_list = ( struct in_addr **) mymachine -> h_addr_list ;
  strcpy ( symbolicip , inet_ntoa (* addr_list [0]) ) ;
  printf ( " %s resolved to %s \n" , mymachine -> h_name , symbolicip ) ;
}
//send ip address an port number
// printf("%s %d\n",symbolicip,strlen(symbolicip)+1 );
sprintf(buf, "LOG_ON <%s, %d>",symbolicip,port);
if (write(sock, buf, strlen(buf)+1) < 0) perror_exit("write");
strcpy(buf,"END ");
if (write(sock, buf, strlen(buf)+1) < 0) perror_exit("write");
sleep(1);
exit(0);




  do {
    printf("Give input string: ");
    fgets(buf, sizeof(buf), stdin);	/* Read from stdin*/
    for(i=0; buf[i] != '\0'; i++) { /* For every char */
      /* Send i-th character */
      if (write(sock, buf + i, 1) < 0) perror_exit("write");
      /* receive i-th character transformed */
      if (read(sock, buf + i, 1) < 0) perror_exit("read");
    }
    printf("Received string: %s", buf);
  } while (strcmp(buf, "END\n") != 0); /* Finish on "end" */

  close(sock);                 /* Close socket and exit */
  return 0;
}

void perror_exit(char *message){
  perror(message);
  exit(EXIT_FAILURE);
}
