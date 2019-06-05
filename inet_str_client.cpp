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
#include <iostream>
#include <netdb.h>

#include <sys/inotify.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/prctl.h>
#include <dirent.h>

#include "linked_list.h"
#include "tuple.h"

using namespace std;

//TODO buffersz workerthreads version
//FIXME o tritos pelatis mpori na kani 2 thread gia to idio connection
//tote kolai ke den stelni stous allous
//h prospa8i na sindeth sto a,b ke termatizi anomala

void perror_exit(const char *message);// print error and exit
void kill_signal_handler(int sig);//signal handler for SIGQUIT and SIGINT
void *rcv_child(void* newsoc);//thread handles all incoming messages from my my_port
void *send_child(void* newsoc);//thread
int send_file(char* dir, int _port);//sends a file of directory
void *primary_server_messages(void* none);//thread handles initial messages to server
int connect_to_sock(char *ipaddr ,char* sock_num);//connects togiven socket
int is_file(const char *path);//returns true if path is a file
char* read_hole_file(char* file_name);// returns pointer to the contents of the file
int file_sz(char* file_name);// returns the number of characters in the file
int rcv_file(char* dir, int _port);//rcves file and creates it
void make_file_list(char* sub_dir, char* input_dir);//updates my file list
void send_file_list(int _potr);//sends files in FILE_LIST format
int get_files_no(char* sub_dir, char* input_dir);//returns number of files in input dir

//globals for signal handler
int server_port, server_sock, my_port;
char server_symbolicip[50], server_port_str[50], my_port_str[50], my_symbolicip[50];

LinkedList* list;//list holding all client tuples
LinkedList* file_list;//list holding all my file tuples (name,version)
pthread_mutex_t list_lock = PTHREAD_MUTEX_INITIALIZER;//locks list operations
char input_dir[100];//my directory
int my_files_no;

int main(int argc, char *argv[]){
  struct sockaddr_in server;
  struct sockaddr *serverptr = (struct sockaddr*)&server;
  pthread_t t;

  list = new LinkedList();
  file_list=new LinkedList();
  gethostname(my_symbolicip, sizeof(my_symbolicip));

  //asigning signal handlers
  static struct sigaction act2;
  act2.sa_handler = kill_signal_handler;
  sigfillset (&(act2.sa_mask ));
  sigaction (SIGQUIT, &act2, NULL);
  sigaction (SIGINT, &act2, NULL);

//---------------------------------------------parsing command line argumets------------------------------------------
  int opt;
  /*
  d input_dir
  p client port
  s server port
  i server ip
  */
  while((opt = getopt(argc, argv, "d:p:s:i:")) != -1) {
    switch(opt){
      case 'd':
        strcpy(input_dir,optarg);
        break;
      case 'p':
        my_port=atoi(optarg);
        strcpy(my_port_str,optarg);
        break;
      case 's':
        strcpy(server_port_str,optarg);
        server_port=atoi(optarg);
        break;
      case 'i':
        strcpy(server_symbolicip,optarg);
        break;
      default:
        std::cout << "hgchgchgc" << '\n';
        break;
    }
  }

  //record my files
  make_file_list("", input_dir);
  my_files_no=get_files_no("",input_dir);
  if(my_files_no!=1)//if input is empty
    my_files_no++;

  pthread_create(&t, NULL, primary_server_messages, NULL);
  // pthread_join(t, NULL);

//-----------------------------------------------------------set up my shocket
  int sock;
  /* Create socket */
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) perror_exit("socket");
  server.sin_family = AF_INET;       /* Internet domain */
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  server.sin_port = htons(my_port);      /* The given port */
  /* Bind socket to address */
  if (bind(sock, serverptr, sizeof(server)) < 0) perror_exit("bind");
  /* Listen for connections */
  if (listen(sock, 5) < 0) perror_exit("listen");

//------------------------------------------------------waiting for conections to my socket
//from other clients or server
  printf("Listening for connections to my port %d\n", my_port);
  int newsock;
  while (1){
    /* accept connection */
    if ((newsock = accept(sock, NULL, NULL)) < 0) perror_exit("accept");
    printf("Accepted connection to my port\n");
    pthread_create(&t, NULL, rcv_child, (void *)&newsock);
    //pthread_create(&t, NULL, send_child, (void *)&newsock);
  }


  return 0;
}

//handles all incoming messages from my port
void *rcv_child(void* newsoc){//TODO close connection when apropriate
  char buf[256];
  int newsock= *(int*)newsoc;
  printf("%lu\t\tentering rcv child %d\n",pthread_self(),newsock);
  char tmp[500];
  tmp[0]='\0';

  //reading fron connection
  while(read(newsock, buf, 1) > 0){
    buf[1]='\0';
    //printf("%s\n",buf);
    //pirame tin entoli
    if(buf[0]==' '){
      printf("%lu\t\trcved %s ",pthread_self(), tmp);
      //--------------------------------------an ine USER_ON
      if(strcmp(tmp,"USER_ON")==0){
        iptuple tmp_tuple("a","b");
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
            printf("%lu\t\t\t<%s,",pthread_self(),tmp);
            strcpy(tmp_tuple.ip,tmp);

            tmp[0]='\0';
            //diabase port
            while(read(newsock, buf, 1) > 0){
              if(buf[0]=='>')
                break;
              buf[1]='\0';
              strcat(tmp,buf);
            }
            printf("%lu\t\t\t%s>\n",pthread_self(),tmp);
            strcpy(tmp_tuple.port,tmp);
          }
          buf[1]='\0';
          break;
        }
        //updating list
        pthread_mutex_lock(&list_lock);
        list->add(tmp_tuple);
        list->print();
        pthread_mutex_unlock(&list_lock);

        sleep(1);//!!!den ine apolita sosto
        //try to connect to client
        int tmp_sock=connect_to_sock(tmp_tuple.ip,tmp_tuple.port);
        //send GET_FILE_LIST
        char aa[50];
        sprintf(aa, "GET_FILE_LIST <%s, %s>",my_symbolicip,my_port_str);
        printf("%lu\t\tsending %s\n",pthread_self(),aa );
        if (write(tmp_sock, aa, strlen(aa)) < 0) perror_exit("write");
        printf("%lu\t\tclosing connection\n",pthread_self());
        sleep(1);//!!!den ine apolita sosto
        close(tmp_sock);

        sleep(1);//!!!den ine apolita sosto
        printf("%lu\t\tclosing connection to server and terminating thread\n",pthread_self());
        close(newsock);
        return NULL;
      }
      //--------------------------------------an ine USER_OFF
      else if(strcmp(tmp,"USER_OFF")==0){
        iptuple tmp_tuple("a","b");
        //diabase to proto <>
        while(read(newsock, buf, 1) > 0){//beni mia fora
          if(buf[0]=='<'){
            tmp[0]='\0';
            //diabase ip
            while(read(newsock, buf, 1) > 0){
              if(buf[0]==',')
                break;
              buf[1]='\0';
              strcat(tmp,buf);
            }
            printf("%lu\t\t\t<%s,",pthread_self(),tmp);
            strcpy(tmp_tuple.ip,tmp);

            tmp[0]='\0';
            //diabase port
            while(read(newsock, buf, 1) > 0){
              if(buf[0]=='>')
                break;
              buf[1]='\0';
              strcat(tmp,buf);
            }
            printf("%lu\t\t\t%s>\n",pthread_self(),tmp);
            strcpy(tmp_tuple.port,tmp);
          }
          buf[1]='\0';//FIXEME mpori na 8eli 0
          break;
        }
        //updating list
        pthread_mutex_lock(&list_lock);
        list->deleten(tmp_tuple);
        list->print();
        pthread_mutex_unlock(&list_lock);

        sleep(1);//!!!den ine apolita sosto
        printf("%lu\t\tclosing connection to server and terminating thread\n",pthread_self());
        close(newsock);
        return NULL;
      }
      //--------------------------------------an ine FILE_LIST
      else if(strcmp(tmp,"FILE_LIST")==0){
        iptuple tmp_tuple("a","b");
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
        printf("%lu\t\t\t%d\n",pthread_self(),n );

        //read file_list
        LinkedList *tmp_list=new LinkedList();
        for(int i=0;i<n;i++){
          //diabase to proto <>
          while(read(newsock, buf, 1) > 0){//mpeni mono mia fora
            if(buf[0]==' ')
              continue;
            if(buf[0]=='<'){
              tmp[0]='\0';
              //diabase ip
              while(read(newsock, buf, 1) > 0){
                if(buf[0]==',')
                  break;
                buf[1]='\0';
                strcat(tmp,buf);
              }
              printf("%lu\t\t\t<%s,",pthread_self(),tmp);
              strcpy(tmp_tuple.ip,tmp);
              tmp[0]='\0';
              //diabase port
              while(read(newsock, buf, 1) > 0){
                if(buf[0]=='>')
                  break;
                buf[1]='\0';
                strcat(tmp,buf);
              }
              printf("%lu\t\t\t%s>\n",pthread_self(),tmp);
              strcpy(tmp_tuple.port,tmp);
            }
            buf[1]='\0';
            break;
          }
          tmp_list->add(tmp_tuple);
        }

        printf("%lu\t\t\t tmp_file_list:\n,",pthread_self());
        tmp_list->print();

        //get first file aka input dir of a=other client
        char aa[200];
        printf("ton perno pat pat %d\n", tmp_list->getlen());
        sprintf(aa, "GET_FILE <%s, %s>",tmp_list->get_by_index(tmp_list->getlen()-1)->ip,tmp_list->get_by_index(tmp_list->getlen()-1)->port);
        printf("%lu\t\t\tsending %s\n",pthread_self(),aa );
        if (write(newsock, aa, strlen(aa)) < 0) perror_exit("write");
        //rcv FILE_SIZE and create file
        sprintf(aa,"%s%s",input_dir,tmp_list->get_by_index(tmp_list->getlen()-1)->ip);
        rcv_file(aa, newsock);

        //get all other files
        for(int i=0;i<tmp_list->getlen()-1;i++){
          //send GET_FILE for each
          sprintf(aa, "GET_FILE <%s, %s>",tmp_list->get_by_index(i)->ip,tmp_list->get_by_index(i)->port);
          printf("%lu\t\t\tsending %s\n",pthread_self(),aa );
          if (write(newsock, aa, strlen(aa)) < 0) perror_exit("write");
          //rcv FILE_SIZE and create file
          sprintf(aa,"%s%s",input_dir,tmp_list->get_by_index(i)->ip);
          rcv_file(aa, newsock);
        }
        delete tmp_list;

        sleep(1);//!!!den ine apolita sosto
        printf("%lu\t\tclosing connection to client and terminating thread\n",pthread_self());
        close(newsock);
        return NULL;
      }
      //--------------------------------------an ine GET_FILE_LIST
      else if(strcmp(tmp,"GET_FILE_LIST")==0){
        iptuple tmp_tuple("a","b");
        //diabase  <>
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
            printf("%lu\t\t\t<%s,",pthread_self(),tmp);
            strcpy(tmp_tuple.ip,tmp);

            tmp[0]='\0';
            //diabase port
            while(read(newsock, buf, 1) > 0){
              if(buf[0]=='>')
                break;
              buf[1]='\0';
              strcat(tmp,buf);
            }
            printf("%lu\t\t\t%s>\n",pthread_self(),tmp);
            strcpy(tmp_tuple.port,tmp);
          }
          buf[1]='\0';
          break;
        }


        //try to connect to client
        int tmp_sock=connect_to_sock(tmp_tuple.ip,tmp_tuple.port);
        printf("%lu\t\tsending FILE_LIST ",pthread_self() );
        if (write(tmp_sock, "FILE_LIST ", 10) < 0) perror_exit("write");
        //send n
        char aa[50];
        sprintf(aa, "%d ",my_files_no);
        printf("%lu\t\t\t number of files %s\n",pthread_self(),aa);
        if (write(tmp_sock, aa, strlen(aa)) < 0) perror_exit("write");
        //send my input dir,version
        char file_tuple[100];
        sprintf(file_tuple, "<%s, 0>",input_dir);
        printf("sending my input dir %s\n", file_tuple);
        if(write(tmp_sock, file_tuple, strlen(file_tuple))<0 && errno!=EINTR){
          cerr<< "write send failed: "<< strerror(errno)<<endl;
          exit(1);
        }
        //send file_tuple list
        send_file_list(tmp_sock);

        for(int i=0;i<my_files_no;i++){
          //rcv GET_FILE
          tmp[0]='\0';
          while(read(tmp_sock, buf, 1) > 0){
            buf[1]='\0';
            if(buf[0]==' '){
              if(strcmp(tmp, "GET_FILE")==0){
                printf("rcved GET_FILE\n");
                //diabase to deftero <> me to filepath
                while(read(tmp_sock, buf, 1) > 0){
                  if(buf[0]=='<'){
                    tmp[0]='\0';
                    //diabase ip
                    while(read(tmp_sock, buf, 1) > 0){
                      if(buf[0]==',')
                        break;
                      buf[1]='\0';
                      strcat(tmp,buf);
                    }
                    printf("%lu\t\t\t<%s,",pthread_self(),tmp);
                    strcpy(tmp_tuple.ip,tmp);

                    tmp[0]='\0';
                    //diabase port
                    while(read(tmp_sock, buf, 1) > 0){
                      if(buf[0]=='>')
                        break;
                      buf[1]='\0';
                      strcat(tmp,buf);
                    }
                    printf("%lu\t\t\t%s>\n",pthread_self(),tmp);
                    strcpy(tmp_tuple.port,tmp);
                  }
                  buf[1]='\0';
                  break;
                }
                //must check for version and reply with FILE_UP_TODATE
                //must check if file exists and reply with FILE_NOT_FOUND

                //send file bites to client
                send_file(tmp_tuple.ip,tmp_sock);//tmp_tuple.ip holds path an tmp_tuple.port holds version
                break;
              }
            }
            else
              strcat(tmp,buf);
          }
        }

        printf("%lu\t\tclosing connection\n",pthread_self());
        sleep(1);//!!!den ine apolita sosto
        close(tmp_sock);

        sleep(1);//!!!den ine apolita sosto
        printf("%lu\t\tclosing connection to client and terminating thread\n",pthread_self());
        close(newsock);
        return NULL;
      }
      tmp[0]='\0';
    }
    else
      strcat(tmp,buf);
  }


  return NULL;
}

//thread handles initial messages to server
void *primary_server_messages(void *none){
  char buf[256],tmp[500];//tmp buffers for read and wrie
  struct sockaddr_in server;
  struct sockaddr *serverptr = (struct sockaddr*)&server;
  struct hostent *rem;

  //-------------------------------------create socket and connection to primary server socket
    /* Create socket */
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) perror_exit("socket");
    /* Find server address */
    if ((rem = gethostbyname(server_symbolicip)) == NULL) {perror("gethostbyname");exit(1);}
    server.sin_family = AF_INET;       /* Internet domain */
    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
    server.sin_port = htons(server_port);         /* Server port */
    /* Initiate connection */
    if (connect(server_sock, serverptr, sizeof(server)) < 0) perror_exit("connect");
    printf("%lu\t\tConnecting to server %s port %d\n",pthread_self(), server_symbolicip, server_port);
    //find host name and ip address
    if(gethostname(buf, 256)!=0)perror_exit("gethostname");
    struct hostent * mymachine ;
    struct in_addr ** addr_list;
    //resolve hostname to ip address
    if ( ( mymachine = gethostbyname ( buf) ) == NULL ){
      printf ( "%lu\t\tCould not resolved Name : %s \n ",pthread_self() , buf) ;
      exit(1);
    }
    else{
      addr_list = ( struct in_addr **) mymachine -> h_addr_list ;
      strcpy ( server_symbolicip , inet_ntoa (* addr_list [0]) ) ;
      printf ( "%lu\t\tserver name %s resolved to %s \n",pthread_self() , mymachine -> h_name ,server_symbolicip ) ;
  }


  //------------------------------------------------send initial messages to server socket
    /*
    1 send LOG_ON
    2 send GET_CLIENTS
    3 rcv CLIENT_LIST
    4 add clients to list
    5 open conection to new client
    6 send GET_FILE_LIST and close connection to client
    7 close connection to server port
    */
    printf("%lu\t\tsending LON_ON\n",pthread_self());
    sprintf(buf, "LOG_ON <%s, %d>",my_symbolicip,my_port);
    if (write(server_sock, buf, strlen(buf)+1) < 0) perror_exit("write");
    printf("%lu\t\tsending GET_CLIENTS\n",pthread_self());
    strcpy(buf,"GET_CLIENTS ");
    if (write(server_sock, buf, strlen(buf)+1) < 0) perror_exit("write");
    tmp[0]='\0';
    //rcv CLIENT_LIST
    printf("%lu\t\trcving from server:\n",pthread_self());
    while(read(server_sock, buf, 1) > 0){
      buf[1]='\0';
      //printf("%s\n",buf);
      //pirame tin entoli
      if(buf[0]==' '){
        printf("%lu\t\t\t%s\n",pthread_self(), tmp);
        //--------------------------------------an ine CLIENT_LIST
        if(strcmp(tmp,"CLIENT_LIST")==0){
          iptuple tmp_tuple("a","b");
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
          printf("%lu\t\t\t %d\n",pthread_self(),n );

          for(int i=0;i<n;i++){
            //diabase to proto <>
            while(read(server_sock, buf, 1) > 0){//mpeni mono mia fora
              if(buf[0]=='<'){
                tmp[0]='\0';
                //diabase ip
                while(read(server_sock, buf, 1) > 0){
                  if(buf[0]==',')
                    break;
                  buf[1]='\0';
                  strcat(tmp,buf);
                }
                printf("%lu\t\t\t<%s,",pthread_self(),tmp);
                strcpy(tmp_tuple.ip,tmp);
                tmp[0]='\0';
                //diabase port
                while(read(server_sock, buf, 1) > 0){
                  if(buf[0]=='>')
                    break;
                  buf[1]='\0';
                  strcat(tmp,buf);
                }
                printf("%lu\t\t\t%s>\n",pthread_self(),tmp);
                strcpy(tmp_tuple.port,tmp);
              }
              buf[1]='\0';
              break;
            }
            if(!(tmp_tuple==iptuple(my_symbolicip,my_port_str))){
              //update list
              pthread_mutex_lock(&list_lock);
              list->add(tmp_tuple);
              list->print();
              pthread_mutex_unlock(&list_lock);

              //try to connect to client
              int tmp_sock=connect_to_sock(tmp_tuple.ip,tmp_tuple.port);
              //send GET_FILE_LIST
              char aa[50];
              sprintf(aa, "GET_FILE_LIST <%s, %s>",my_symbolicip,my_port_str);
              printf("%lu\t\tsending %s\n",pthread_self(),aa );
              if (write(tmp_sock, aa, strlen(aa)) < 0) perror_exit("write");
              printf("%lu\t\tclosing connection\n",pthread_self());
              sleep(1);//!!!den ine apolita sosto
              close(tmp_sock);
            }
          }
          break;
        }
      }
      else
        strcat(tmp,buf);
    }
    sleep(1);//!!!den ine apolita sosto
    close(server_sock);
    printf("%lu\t\tclosing server conection\n",pthread_self());


  return NULL;
}

// print error and exit
void perror_exit(const char *message){
  perror(message);
  exit(EXIT_FAILURE);
}

//signal handler for SIGQUIT and SIGINT
void kill_signal_handler(int sig){
  fflush(stdout);
  char buf[100];

  printf("Client wants to exit\n");
  server_sock=connect_to_sock(server_symbolicip,server_port_str);
  //sending LOG_OFF to serbver
  sprintf(buf, "LOG_OFF <%s, %d>",my_symbolicip,my_port);
  printf("sending %s\n",buf );
  if (write(server_sock, buf, strlen(buf)+1) < 0) perror_exit("write");
  printf("closing connection to server\n");
  sleep(1);//!!!den ine apolita sosto
  close(server_sock);

  printf("Client exiting\n");
  exit(0);
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
    printf("gethostbyname ERROR %s - %s - %s\n",hstrerror(h_errno),ipaddr, sock_num);
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

//sends a file of directory
int send_file(char* dir, int _port){
  char tmp[500];
  int sz=0;

  printf("entering send file dir: %s \n",dir);

  //send FILE_SIZE
  printf("sending FILE_SIZE\n");
  if (write(_port, "FILE_SIZE ", 10) < 0) perror_exit("write");

  //write version
  if(write(_port, "0 ", 2)<0 && errno!=EINTR){
    cerr<< "write send failed: "<< strerror(errno)<<endl;
    return 1;
  }
  printf("sending v0\n");

  if(is_file(dir)){// if file
    //write file sz
    sz=file_sz(dir);
    if(sz==0){printf("\n\nFILE SZ ==0 !!\n\n"); exit(1);}
    sprintf(tmp, "%d ",sz);
    if(write(_port, tmp, strlen(tmp))<0 && errno!=EINTR){
      cerr<< "write send failed: "<< strerror(errno)<<endl;
      return 1;
    }
    printf("sending %s\n",tmp);

    //write file bites
    char* tmpp= read_hole_file(dir);
    printf("sending %s\n",tmpp);
    if(write(_port, tmpp, sz)<0 && errno!=EINTR){
      cerr<< "write send failed: "<< strerror(errno)<<endl;
      return 1;
    }
    free(tmpp);
  }
  else{// if directory
    printf("sending 0\n");
    if(write(_port, "0 ", 2)<0 && errno!=EINTR){
      cerr<< "write send failed: "<< strerror(errno)<<endl;
      return 1;
    }
  }

  return 0;
}

//updates my file list
void make_file_list(char* sub_dir, char* input_dir){
  DIR *d=NULL;
  char tmp[300], file_tuple[300];
  bool empty_dir=true;
  struct dirent *dir;
  char zero[]="0";
  sprintf(tmp,"%s%s",input_dir,sub_dir);

  // printf("entering send file list sub_dir: %s input_dir: %s\n",sub_dir,input_dir);
  if((d= opendir(tmp))==NULL){
    cerr<< "opendir send failed: "<< strerror(errno)<<endl;
    //return 1;
    exit(1);
  }
  //for all the files i need to send
  while ((dir = readdir(d)) != NULL){
    if(strcmp(dir->d_name,".")!=0 && strcmp(dir->d_name,"..")!=0){
      sprintf(tmp, "%s%s%s",input_dir,sub_dir,dir->d_name);
      if(is_file(tmp)){// if file
        empty_dir=false;
        //push file_name,version
        printf("pushing to file list %s %s\n", tmp,zero);
        file_list->add(iptuple(tmp,zero));
      }
      else{// if directory
        sprintf(file_tuple, "<%s%s%s, 0>",input_dir,sub_dir,dir->d_name);
        //push file_name,version
        printf("pushing to file list %s %s\n", tmp,zero);
        file_list->add(iptuple(tmp,zero));

        sprintf(tmp, "%s%s/",sub_dir,dir->d_name);
        make_file_list(tmp, input_dir);
        // printf("\t %s returned %d\n",tmp, send_file_list(tmp, input_dir));
      }
    }
  }

  if(empty_dir){//if the directory is empty
    //push file_name,version
    printf("pushing to file list %s %s\n", tmp,zero);
    file_list->add(iptuple(tmp,zero));
  }

  closedir(d);
}

//returns number of files in input dir
int get_files_no(char* sub_dir, char* input_dir){
  DIR *d=NULL;
  char tmp[300];
  bool empty_dir=true;
  int n=0;
  struct dirent *dir;
  sprintf(tmp,"%s%s",input_dir,sub_dir);

  // printf("entering send file list sub_dir: %s input_dir: %s\n",sub_dir,input_dir);
  if((d= opendir(tmp))==NULL){
    cerr<< "opendir send failed: "<< strerror(errno)<<endl;
    //return 1;
    exit(1);
  }
  //for all the files i need to send
  while ((dir = readdir(d)) != NULL){
    if(strcmp(dir->d_name,".")!=0 && strcmp(dir->d_name,"..")!=0){
      sprintf(tmp, "%s%s%s",input_dir,sub_dir,dir->d_name);
      if(is_file(tmp)){// if file
        empty_dir=false;
        n++;
        // printf("found file %s n= %d\n",tmp,n );
      }
      else{// if directory
        sprintf(tmp, "%s%s/",sub_dir,dir->d_name);
        // printf("recursive %s \n",tmp);
        n+=get_files_no(tmp, input_dir);
        n++;
        // printf("\t %s returned %d\n",tmp, send_file_list(tmp, input_dir));
      }
    }
  }

  if(empty_dir){//if the directory is empty
    // printf("found empty_dir %s\n",tmp );
    n++;


  }

  closedir(d);

  return n;
}

//sends files in FILE_LIST format
void send_file_list(int  _port){
  printf("entering send file list\n");

  char *tmp_buf;
  tmp_buf=file_list->get_string();
  if (write(_port, tmp_buf, strlen(tmp_buf)) < 0) perror_exit("write");
  printf("sended %s\n", tmp_buf);
  delete tmp_buf;

}

//rcves file and creates it
int rcv_file(char* dir, int _port){
  //---------------------------------------rcv child process
  struct stat tmp_stat;
  char tmp[200];
  char buf[2],tmp_file_bytes[200];

  printf("entering rcv file dir: %s\n",dir);

  //read FILE_SIZE
  tmp[0]='\0';
  buf[0]='\0';
  while(read(_port, buf, 1) > 0){
    if(buf[0]==' ')
      break;
    buf[1]='\0';
    strcat(tmp,buf);
  }
  printf("rcved %s\n", tmp);

  int n;
  //!!!!! DEN TO XRISIMOPIO POU8ENA
  // diabazi version
  tmp[0]='\0';
  buf[0]='\0';
  while(read(_port, buf, 1) > 0){
    if(buf[0]==' ')
      break;
    buf[1]='\0';
    strcat(tmp,buf);
  }
  n=atoi(tmp);
  printf("rcved version= %d\n",n);

  // diabazi n
  tmp[0]='\0';
  buf[0]='\0';
  while(read(_port, buf, 1) > 0){
    if(buf[0]==' ')
      break;
    buf[1]='\0';
    strcat(tmp,buf);
  }
  n=atoi(tmp);
  printf("rcved n= %d\n",n);

  //read file_list
  tmp_file_bytes[0]='\0';
  for(int i=0;i<n;i++){
    //diabase to proto byte
    if(read(_port, buf,1)<0 && errno!=EINTR){
      cerr<< "read rcv failed: "<< strerror(errno)<<endl;
      exit(1);
    }
    buf[1]='\0';
    strcat(tmp_file_bytes, buf);
  }

  if(n==0){//if directory
    printf("rcved directory %s\n",dir);
    if(stat(dir, &tmp_stat) != 0)
      if(mkdir(dir, 0766)){
        cerr <<"!!rcv can not create directory "<<dir;
        perror("\n");
        exit(1);
      }
  }
  else{//if file
    printf("rcved file %s has bytes %s\n",dir,tmp_file_bytes);
    //make file
    FILE *new_file=NULL;
    new_file=fopen(dir, "w");
    if(new_file==NULL){
      cerr <<"!rcv can not create "<<dir<<" file "<<strerror(errno)<<"\n";
      exit(1);
    }

    //write to file
    fprintf(new_file, "%s", tmp_file_bytes);
    fclose(new_file);
  }

  return 0;
}

// returns the number of characters in the file
int file_sz(char* file_name){
  int fd =-1;
  if((fd=open(file_name, O_RDONLY))<0){
    cerr<< "file_sz openfile failed: "<< strerror(errno)<<file_name<<endl;
    //return 1;
    exit(1);
  }

  int sz =lseek(fd, 0L, SEEK_END);
  close(fd);

  return sz;
}

//returns true if path is a file
int is_file(const char *path){
    struct stat path_stat;
    if(stat(path, &path_stat)!=0){
      cerr<< "is_file stat failed: "<< strerror(errno)<<path<<endl;
      //return 1;
      exit(1);
    }
    return S_ISREG(path_stat.st_mode);
}

// returns pointer to the contents of the file
char* read_hole_file(char* file_name){// TODO if return NULL
  FILE *f = fopen(file_name, "rb");
  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

  char *string = (char*) malloc(fsize + 1);
  fread(string, fsize, 1, f);
  fclose(f);

  string[fsize] = '\0';

  return string;
}
