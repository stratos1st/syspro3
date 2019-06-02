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

void perror_exit(const char *message);// print error and exit
void kill_signal_handler(int sig);//signal handler for SIGQUIT and SIGINT
void *rcv_child(void* newsoc);//thread handles all incoming messages from my my_port
void *send_child(void* newsoc);//thread
int send_file(char* sub_dir, char* input_dir, int _port);//handles sending protocol
int rcv_file(char* mirror_dir, int buff_len, int _port);//handles rcving protocol
int connect_to_sock(char *ipaddr ,char* sock_num);//connects togiven socket
int is_file(const char *path);//returns true if path is a file
char* read_hole_file(char* file_name);// returns pointer to the contents of the file
int file_sz(char* file_name);// returns the number of characters in the file

//globals for signal handler
int server_port, server_sock, my_port;
char server_symbolicip[50], server_port_str[50], my_port_str[50], my_symbolicip[50];

LinkedList* list;//list holding all client tuples
pthread_mutex_t list_lock = PTHREAD_MUTEX_INITIALIZER;//locks list operations
char input_dir[100];//my directory


int main(int argc, char *argv[]){
  char buf[256],tmp[500];//tmp buffers for read and wrie
  struct sockaddr_in server;
  struct sockaddr *serverptr = (struct sockaddr*)&server;
  struct hostent *rem;
  pthread_t t;
  char symbolicip[50];

  list = new LinkedList();
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
        strcpy(symbolicip,optarg);
        break;
      default:
        std::cout << "hgchgchgc" << '\n';
        break;
    }
  }

//-------------------------------------create socket and connection to primary server socket
  /* Create socket */
  if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) perror_exit("socket");
  /* Find server address */
  if ((rem = gethostbyname(symbolicip)) == NULL) {perror("gethostbyname");exit(1);}
  server.sin_family = AF_INET;       /* Internet domain */
  memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
  server.sin_port = htons(server_port);         /* Server port */
  /* Initiate connection */
  if (connect(server_sock, serverptr, sizeof(server)) < 0) perror_exit("connect");
  printf("Connecting to server %s port %d\n", symbolicip, server_port);
  //find host name and ip address
  if(gethostname(buf, 256)!=0)perror_exit("gethostname");
  printf("%s\n", buf);
  struct hostent * mymachine ;
  struct in_addr ** addr_list;
  //resolve hostname to ip address
  if ( ( mymachine = gethostbyname ( buf) ) == NULL ){
    printf ( " Could not resolved Name : %s \n " , buf) ;
    exit(1);
  }
  else{
    addr_list = ( struct in_addr **) mymachine -> h_addr_list ;
    strcpy ( server_symbolicip , inet_ntoa (* addr_list [0]) ) ;
    printf ( "server name %s resolved to %s \n" , mymachine -> h_name ,server_symbolicip ) ;
}


//------------------------------------------------send initial messages to server socket
  /*
  1 send LOG_ON
  2 send GET_CLIENTS
  3 rcv CLIENT_LIST
  4 add clients to list
  5 close connection to server port
  */
  printf("sending LON_ON\n");
  sprintf(buf, "LOG_ON <%s, %d>",my_symbolicip,my_port);
  if (write(server_sock, buf, strlen(buf)+1) < 0) perror_exit("write");
  printf("sending GET_CLIENTS\n");
  strcpy(buf,"GET_CLIENTS ");
  if (write(server_sock, buf, strlen(buf)+1) < 0) perror_exit("write");
  tmp[0]='\0';
  //rcv CLIENT_LIST
  printf("rcving from server:\n");
  while(read(server_sock, buf, 1) > 0){
    buf[1]='\0';
    printf("%s\n",buf);
    //pirame tin entoli
    if(buf[0]==' '){
      printf("%s\n", tmp);
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
        printf(" %d\n",n );

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
              printf("<%s,",tmp);
              strcpy(tmp_tuple.ip,tmp);
              tmp[0]='\0';
              //diabase port
              while(read(server_sock, buf, 1) > 0){
                if(buf[0]=='>')
                  break;
                buf[1]='\0';
                strcat(tmp,buf);
              }
              printf("%s> ",tmp);
              strcpy(tmp_tuple.port,tmp);
            }
            buf[1]='\0';
            break;
          }
          printf("\n");
          //update list
          pthread_mutex_lock(&list_lock);
          list->add(tmp_tuple);
          list->print();
          pthread_mutex_unlock(&list_lock);
        }
        break;
      }
    }
    else
      strcat(tmp,buf);
  }
  close(server_sock);
  printf("closing server conection\n");

  //FIXME na ftiaxni to socket pio prin
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
  printf("\nentering rcv child\n");
  char buf[256];
  int newsock= *(int*)newsoc;
  char tmp[500];
  tmp[0]='\0';

  //reading fron connection
  while(read(newsock, buf, 1) > 0){
    buf[1]='\0';
    printf("%s\n",buf);
    //pirame tin entoli
    if(buf[0]==' '){
      printf(" rcved %s ", tmp);
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
            printf(" <%s,",tmp);
            strcpy(tmp_tuple.ip,tmp);

            tmp[0]='\0';
            //diabase port
            while(read(newsock, buf, 1) > 0){
              if(buf[0]=='>')
                break;
              buf[1]='\0';
              strcat(tmp,buf);
            }
            printf("%s>\n",tmp);
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

        sleep(1);//FIXME
        int tmp_sock=connect_to_sock(tmp_tuple.ip,tmp_tuple.port);
        printf("sending ABC and files\n" );
        if (write(tmp_sock, "ABC ", 4) < 0) perror_exit("write");

        if(send_file("", input_dir,tmp_sock)!=0){
          cerr<< "send_proces failed \n";
          exit(1);
        }

        tmp[0]='\0';
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
            printf(" <%s,",tmp);
            strcpy(tmp_tuple.ip,tmp);

            tmp[0]='\0';
            //diabase port
            while(read(newsock, buf, 1) > 0){
              if(buf[0]=='>')
                break;
              buf[1]='\0';
              strcat(tmp,buf);
            }
            printf("%s>\n",tmp);
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
        tmp[0]='\0';
      }
      //--------------------------------------an ine
      else if(strcmp(tmp,"ABC")==0){
        int aaaa=1000;
        if(rcv_file(input_dir,aaaa,newsock)!=0){
          cerr<< "rvc_proces failed \n";
          exit(1);
        }
        tmp[0]='\0';
      }
    }
    else
      strcat(tmp,buf);
  }


  return NULL;
}

//thread handles all incoming messages from my port
void *send_child(void* newsoc){


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

//handles sending protocol
int send_file(char* sub_dir, char* input_dir, int _port){
  DIR *d=NULL;
  char tmp[300];
  bool empty_dir=true;
  struct dirent *dir;
  sprintf(tmp,"%s%s",input_dir,sub_dir);

  printf("entering send files sub_dir: %s input_dir: %s\n",sub_dir,input_dir);
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
        char buffer[9999];
        printf("for %s \n",tmp);

        //write name
        sprintf(tmp, "%s%s",sub_dir,dir->d_name);
        int sz=strlen(tmp);
        sprintf(buffer, "%d",sz);
        if(write(_port, buffer, 2)<0 && errno!=EINTR){
          cerr<< "write send faileda: "<< strerror(errno)<<endl;
          return 1;
        }

        if(write(_port, tmp, sz)<0 && errno!=EINTR){
          cerr<< "write send failed: "<< strerror(errno)<<endl;
          return 1;
        }

        //write file
        sprintf(tmp, "%s%s%s",input_dir,sub_dir,dir->d_name);
        sz=file_sz(tmp);
        sprintf(buffer, "%d",sz);
        if(write(_port, buffer, 4)<0 && errno!=EINTR){
          cerr<< "write send failed: "<< strerror(errno)<<endl;
          return 1;
        }
        char* tmpp= read_hole_file(tmp);
        if(write(_port, tmpp, sz)<0 && errno!=EINTR){
          cerr<< "write send failed: "<< strerror(errno)<<endl;
          return 1;
        }
        free(tmpp);
      }
      else{// if directory
        sprintf(tmp, "%s%s/",sub_dir,dir->d_name);
        printf("recursive %s \n",tmp);
        send_file(tmp, input_dir,_port);
      }
    }
  }

  if(empty_dir){//if the directory is empty
    char buffer[9999];
    //write name
    sprintf(tmp,"%s",sub_dir);
    printf("\n\n empty dir %s \n",tmp);
    int sz=strlen(tmp);
    sprintf(buffer, "%d",sz);
    if(write(_port, buffer, 2)<0 && errno!=EINTR){
      cerr<< "write send faileda: "<< strerror(errno)<<endl;
      return 1;
    }

    if(write(_port, tmp, sz)<0 && errno!=EINTR){
      cerr<< "write send failed: "<< strerror(errno)<<endl;
      return 1;
    }

    //write file
    sprintf(buffer, "%d",-1);
    if(write(_port, buffer, 4)<0 && errno!=EINTR){
      cerr<< "write send failed: "<< strerror(errno)<<endl;
      return 1;
    }

  }

  //write 00 at the end
  if(strcmp(sub_dir,"")==0){
    if(write(_port, "00", 2)<0 && errno!=EINTR){
      cerr<< "write send failed: "<< strerror(errno)<<endl;
      return 1;
    }
    printf("sending files ended\n");
  }

  closedir(d);

  return 0;
}

//handles rcving protocol
int rcv_file(char* mirror_dir, int buff_len, int _port){
  printf("entering rcv files mirror_dir: %s\n",mirror_dir);
  //---------------------------------------rcv child process
  struct stat tmp_stat;
  char tmp[200];
  char buffer[9999], tmp_buff[buff_len+1],tmp_file_name[9999];
  //read name len
  if(read(_port, buffer, 2)<0 && errno!=EINTR){
    cerr<< "read rcv failed: "<< strerror(errno)<<endl;
    return 1;
  }
  buffer[2]='\0';
  int sz;
  try {sz=atoi(buffer);/* den exi c++11 stoi(buffer, NULL, 10); */} catch (...) {cout<<"!!ERROR stoi: "<<buffer<<" "<<tmp_file_name<<"\n";exit(1);}
  int left_to_read, bytes_to_read;

  while(sz!=0){
    //read name
    buffer[0]='\0';
    left_to_read=sz;
    while(left_to_read>0){
      if(left_to_read-buff_len<0)
        bytes_to_read=left_to_read;
      else
        bytes_to_read=buff_len;
      if(read(_port, tmp_buff, bytes_to_read)<0 && errno!=EINTR){
        cerr<< "read rcv failed: "<< strerror(errno)<<endl;
        return 1;
      }
      tmp_buff[bytes_to_read]='\0';
      strcat(buffer,tmp_buff);
      left_to_read-=bytes_to_read;
    }
    strcpy(tmp_file_name,buffer);//coppy name for later

    //make dir
    sprintf(tmp, "%s",tmp_file_name);
    char *tmpp=strrchr(tmp,'/');
    if(tmpp!=NULL){// if not in the starting directory 1_mirror
      *tmpp='\0';
      char tmp2[100];
      sprintf(tmp2, "%s/%s",mirror_dir,tmp);
      if(stat(tmp2, &tmp_stat) != 0)
        if(mkdir(tmp2, 0766)){
          cerr <<"!!rcv can not create directory "<<tmp2<<" for file "<<tmp_file_name;perror("\n");
          return 1;
        }
    }

    //read file len
    if(read(_port, buffer, 4)<0 && errno!=EINTR){
      cerr<< "read rcv failed: "<< strerror(errno)<<endl;
      return 1;
    }
    buffer[4]='\0';
    try {sz=atoi(buffer);/* den exi c++11 stoi(buffer, NULL, 10); */} catch (...) {cout<<"!!ERROR stoi: "<<buffer<<" "<<tmp_file_name<<"\n";exit(1);}
    fflush(stdout);

    if(sz!=-1){//if it is a file
      //make file
      FILE *new_file=NULL;
      sprintf(tmp, "%s/%s",mirror_dir,tmp_file_name);
      new_file=fopen(tmp, "w");
      if(new_file==NULL){
        cerr <<"!rcv can not create "<<tmp<<" file "<<strerror(errno)<<"\n";
        return 1;
      }

      //read file
      buffer[0]='\0';
      left_to_read=sz;
      while(left_to_read>0){
        if(left_to_read-buff_len<0)
          bytes_to_read=left_to_read;
        else
          bytes_to_read=buff_len;
        if(read(_port, tmp_buff, bytes_to_read)<0 && errno!=EINTR){
          cerr<< "read rcv failed: "<< strerror(errno)<<endl;
          return 1;
        }
        tmp_buff[bytes_to_read]='\0';
        strcat(buffer,tmp_buff);
        left_to_read-=bytes_to_read;
      }

      //write to file
      fprintf(new_file, "%s", buffer);
      fclose(new_file);
    }
    else{//if it is a directory
      sprintf(tmp, "%s/%s",mirror_dir,strchr(tmp_file_name+2,'/')+1);
      //cout<<"ine directory "<<tmp<<endl;
      if(stat(tmp, &tmp_stat) != 0)
        if(mkdir(tmp, 0766)){
          cerr <<"!rcv can not create lone directory "<<tmp<<"\n";
          return 1;
        }
    }

    //read next name or 00
    if(read(_port, buffer, 2)<0 && errno!=EINTR){
      cerr<< "read rcv failed: "<< strerror(errno)<<endl;
      return 1;
    }
    buffer[2]='\0';
    try {sz=atoi(buffer);/* den exi c++11 stoi(buffer, NULL, 10); */} catch (...) {cout<<"!!ERROR stoi: "<<buffer<<" "<<tmp_file_name<<"\n";exit(1);}
  }

  printf("rcving files ended\n");

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
