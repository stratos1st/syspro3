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

#include <sys/inotify.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/prctl.h>
#include <dirent.h>

#include "linked_list.h"
#include "tuple.h"

#define LOG_ON 1

using namespace std;

void perror_exit(char *message);
void kill_signal_handler(int sig);//main signal handler for SIGQUIT and SIGINT
void *rcv_child(void* newsoc);
void *send_child(void* newsoc);
int connect_to_sock(char *ipaddr ,char* sock_num, bool true_ip=false);
int is_file(const char *path);//returns true if path is a file
char* read_hole_file(char* file_name);// returns pointer to the contents of the file
int file_sz(char* file_name);// returns the number of characters in the file
int send_file(char* sub_dir, char* input_dir, int port);
int rcv_file(char* mirror_dir, int buff_len, int port);


//globals for signal handler
int server_port, server_sock,server,port;
char server_symbolicip[50],server_port_str[50];

LinkedList* list;
pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER;
char input_dir[100];

int main(int argc, char *argv[]){
  char buf[256],tmp[500];
  struct sockaddr_in server;
  struct sockaddr *serverptr = (struct sockaddr*)&server;
  struct hostent *rem;
  pthread_t t;
  int newsock,sock;
  char symbolicip[50];

  list = new LinkedList();

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
        port=atoi(optarg);
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
  printf("Connecting to %s port %d\n", symbolicip, server_port);
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


//------------------------------------------------------------send initial messages to server socket
  sprintf(buf, "LOG_ON <%s, %d>",server_symbolicip,port);
  if (write(server_sock, buf, strlen(buf)+1) < 0) perror_exit("write");
  printf("sending logon\n");
  strcpy(buf,"GET_CLIENTS ");
  if (write(server_sock, buf, strlen(buf)+1) < 0) perror_exit("write");
  printf("sending getclients\n");
  tmp[0]='\0';
  //rcv CLIENT_LIST
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
              strcpy(tmp_tuple.ip,tmp);
              tmp[0]='\0';
              //diabase port
              while(read(server_sock, buf, 1) > 0){
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
          pthread_mutex_lock(&counter_lock);
          list->add(tmp_tuple);
          list->print();
          pthread_mutex_unlock(&counter_lock);
        }
        break;
      }
    }
    else
      strcat(tmp,buf);
  }
  close(server_sock);

  //TODO na ftiaxni to socket pio prin
//-----------------------------------------------------------set up my shocket
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
  //from other clients or server
  printf("Listening for connections to port %d\n", port);
  while (1){
    /* accept connection */
    if ((newsock = accept(sock, NULL, NULL)) < 0) perror_exit("accept");
    printf("Accepted connection\n");
    pthread_create(&t, NULL, rcv_child, (void *)&newsock);
    pthread_create(&t, NULL, send_child, (void *)&newsock);
    // pthread_join(t , NULL);
    // close(newsock); /* parent closes socket to client */
  }


  return 0;
}

//----------------------------------------------------rcv_child
void *rcv_child(void* newsoc){
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
            printf("USER_ON <%s,",tmp);
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
        pthread_mutex_lock(&counter_lock);
        list->add(tmp_tuple);
        list->print();
        pthread_mutex_unlock(&counter_lock);

        server_sock=connect_to_sock(tmp_tuple.ip,tmp_tuple.port,true);
        if (write(server_sock, "ABC ", 4) < 0) perror_exit("write");

        std::cout << "send_file" << '\n';
        if(send_file("", input_dir,server_sock)!=0){
          cerr<< "send_proces failed \n";
          exit(1);
        }

      }
      //--------------------------------------an ine USER_OFF
      else if(strcmp(tmp,"USER_OFF")==0){
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
            printf("USER_OFF <%s,",tmp);
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
        pthread_mutex_lock(&counter_lock);
        list->deleten(tmp_tuple);
        list->print();
        pthread_mutex_unlock(&counter_lock);
      }
      else if(strcmp(tmp,"ABC")==0){

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
            printf("ABC <%s,",tmp);
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


        std::cout << "connecting and rcv" << '\n';
        int aaaa=1000;
        server_sock=connect_to_sock(tmp_tuple.ip,tmp_tuple.port,true);
        if(rcv_file(input_dir,aaaa,server_sock)!=0){
          cerr<< "rvc_proces failed \n";
          exit(1);
        }
      }
    }
    else
      strcat(tmp,buf);
  }



  return NULL;
}

//----------------------------------------------------send_child
void *send_child(void* newsoc){


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

int connect_to_sock(char *ipaddr ,char* sock_num, bool true_ip){
  //create socket and connection
  int sock,port;
  struct sockaddr_in server;
  struct sockaddr *serverptr = (struct sockaddr*)&server;
  struct hostent *rem;
  /* Create socket */
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) perror_exit("socket");
  if(!true_ip){
    /* Find server address */
    if ((rem = gethostbyname(ipaddr)) == NULL) {perror("gethostbyname");exit(1);}
    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
  }
  else
    memcpy(&server.sin_addr, ipaddr, strlen(ipaddr));
  port = atoi(sock_num); /*Convert port number to integer*/
  server.sin_family = AF_INET;       /* Internet domain */
  server.sin_port = htons(port);         /* Server port */
  /* Initiate connection */
  if (connect(sock, serverptr, sizeof(server)) < 0) perror_exit("connect");
  printf("Connecting to %s port %d\n", ipaddr, port);

  return sock;
}

int send_file(char* sub_dir, char* input_dir, int port){
  DIR *d=NULL;
  char tmp[300];
  bool empty_dir=true;
  struct dirent *dir;
  sprintf(tmp,"%s%s",input_dir,sub_dir);
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
        cout<<"for "<<tmp << '\n';

        //write name
        sprintf(tmp, "%s%s",sub_dir,dir->d_name);
        int sz=strlen(tmp);
        sprintf(buffer, "%d",sz);
        if(write(port, buffer, 2)<0 && errno!=EINTR){
          cerr<< "write send faileda: "<< strerror(errno)<<endl;
          return 1;
        }

        if(write(port, tmp, sz)<0 && errno!=EINTR){
          cerr<< "write send failed: "<< strerror(errno)<<endl;
          return 1;
        }

        //write file
        sprintf(tmp, "%s%s%s",input_dir,sub_dir,dir->d_name);
        sz=file_sz(tmp);
        sprintf(buffer, "%d",sz);
        if(write(port, buffer, 4)<0 && errno!=EINTR){
          cerr<< "write send failed: "<< strerror(errno)<<endl;
          return 1;
        }
        char* tmpp= read_hole_file(tmp);
        if(write(port, tmpp, sz)<0 && errno!=EINTR){
          cerr<< "write send failed: "<< strerror(errno)<<endl;
          return 1;
        }
        free(tmpp);
      }
      else{// if directory
        sprintf(tmp, "%s%s/",sub_dir,dir->d_name);
        cout<<"recursive "<<tmp<<endl;
        send_file(tmp, input_dir,port);
      }
    }
  }

  if(empty_dir){//if the directory is empty
    char buffer[9999];
    //write name
    sprintf(tmp,"%s",sub_dir);
    cout<<"\n\nempty dir"<<tmp<<"\n";
    int sz=strlen(tmp);
    sprintf(buffer, "%d",sz);
    if(write(port, buffer, 2)<0 && errno!=EINTR){
      cerr<< "write send faileda: "<< strerror(errno)<<endl;
      return 1;
    }

    if(write(port, tmp, sz)<0 && errno!=EINTR){
      cerr<< "write send failed: "<< strerror(errno)<<endl;
      return 1;
    }

    //write file
    sprintf(buffer, "%d",-1);
    if(write(port, buffer, 4)<0 && errno!=EINTR){
      cerr<< "write send failed: "<< strerror(errno)<<endl;
      return 1;
    }

  }

  //write 00 at the end
  if(strcmp(sub_dir,"")==0){
    if(write(port, "00", 2)<0 && errno!=EINTR){
      cerr<< "write send failed: "<< strerror(errno)<<endl;
      return 1;
    }
    cout<<"TELIOSA kiego\n";
  }

  closedir(d);
  close(port);

  return 0;
}

//handles all the rcv part
int rcv_file(char* mirror_dir, int buff_len, int port){
  //---------------------------------------rcv child process
  struct stat tmp_stat;
  char tmp[200];
  char buffer[9999], tmp_buff[buff_len+1],tmp_file_name[9999];
  //read name len
  if(read(port, buffer, 2)<0 && errno!=EINTR){
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
      if(read(port, tmp_buff, bytes_to_read)<0 && errno!=EINTR){
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
    if(read(port, buffer, 4)<0 && errno!=EINTR){
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
        if(read(port, tmp_buff, bytes_to_read)<0 && errno!=EINTR){
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
    if(read(port, buffer, 2)<0 && errno!=EINTR){
      cerr<< "read rcv failed: "<< strerror(errno)<<endl;
      return 1;
    }
    buffer[2]='\0';
    try {sz=atoi(buffer);/* den exi c++11 stoi(buffer, NULL, 10); */} catch (...) {cout<<"!!ERROR stoi: "<<buffer<<" "<<tmp_file_name<<"\n";exit(1);}
  }

  cout<<"rcv ended\n";

  close(port);

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
