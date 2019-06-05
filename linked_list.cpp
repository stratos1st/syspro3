#include <iostream>
#include <cstdlib>
#include <string.h>
#include "linked_list.h"
#include "tuple.h"


using namespace std;

Node::Node(){
  next=NULL;
  prev=NULL;
  data= new iptuple("1","2");
}

Node::~Node(){
  delete data;
}

LinkedList::LinkedList(){
  this->length = 0;
  this->head = NULL;
}

LinkedList::~LinkedList(){
  Node *tmp=head,*a;
  while(tmp){
    a=tmp->next;
    delete tmp;
    tmp=a;
    cout<<"node deleted\n";
  }
  cout << "LIST DELETED\n";
}

void LinkedList::add(iptuple data){
  Node* node = new Node();
  node->data = new iptuple(data.get_ip_str(),data.get_port_str());
  if(!head){
    node->prev=NULL;
    node->next=NULL;
    head=node;
  }
  else{
    node->next = head;
    head->prev=node;
    node->prev=NULL;
    head = node;
  }
  length++;
}

Node* LinkedList::find(iptuple data){
  Node *tmp=head;

  while(tmp){
    if(*tmp->data==data)
      return tmp;
    else
      tmp=tmp->next;
  }
  return NULL;
}

bool LinkedList::deleten(iptuple data){
  Node *tmp;
  if((tmp=find(data))!=NULL){
    if(tmp->prev)
      tmp->prev->next=tmp->next;
    if(tmp->next)
      tmp->next->prev=tmp->prev;
    if(tmp==head)
      head=tmp->next;
    delete tmp;
    tmp=NULL;
    length--;
    return true;
  }
  return false;
}

void LinkedList::print(){
  Node* head = this->head;
  int i = 0;
  cout<<"LIST:\n";
  while(head){
    cout << i << ": " ;
    head->data->print();
    cout << endl;
    head = head->next;
    i++;
  }
}

char* LinkedList::get_string(){
  Node* head = this->head;
  char* ans=new char[999];
  ans[0]='\0';

  while(head){
    strcat(ans,head->data->get_string());
    head = head->next;
  }
  if(length==0)
    strcat(ans,"");

  return ans;
}

int LinkedList::getlen(){
  return length;
}

iptuple* LinkedList::get_by_index(unsigned int index){
  Node* ans=head;
  if(index>=length)
    return NULL;
  else
    for(unsigned int i=0;i<index;i++)
      ans=ans->next;
  return ans->data;
}
