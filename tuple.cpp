#include <iostream>
#include <cstdlib>
#include <string.h>
#include "tuple.h"

using namespace std;

iptuple::iptuple(const char *_ip,const char *_port){
  strcpy(ip,_ip);
  strcpy(port,_port);
}

int iptuple::get_ip()const{
  return atoi(ip);
}

int iptuple::get_port()const{
  return atoi(port);
}

const char* iptuple::get_ip_str()const{
  return ip;
}

const char* iptuple::get_port_str()const{
  return port;
}

void iptuple::print(){
  cout<<ip<<" "<<port;
}

char* iptuple::get_string(){
  char *ans=new char[500];
  sprintf(ans,"<%s, %s>",ip,port);
  return ans;
}

bool iptuple::operator==(const iptuple& rhs){
    return (get_ip()==rhs.get_ip()) && (get_port()==rhs.get_port());
}
