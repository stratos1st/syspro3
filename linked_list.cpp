#include <iostream>
#include <cstdlib>
#include "linked_list.h"
#include "tuple.h"


using namespace std;

Node::Node(){
  next=NULL;
  data= new iptuple("1","2");
}

// Node::~Node(){
//   // cout<<"node del\n";
//   if(next){
//     delete next;
//   }
// }

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
  node->next = this->head;
  this->head = node;
  this->length++;
}

Node* LinkedList::find(iptuple data){
  Node *tmp=head;

  if(tmp)
    if(*tmp->data==data)
      return tmp;

  while(tmp->next){
    if(*tmp->next->data==data)
      return tmp;
    else
      tmp=tmp->next;
  }
  return NULL;
}

bool LinkedList::deleten(iptuple data){
  Node *tmp,*a;
  if((tmp=find(data))!=NULL){
    if(*tmp->next->data==data){
      a=tmp->next->next;
      delete tmp->next;
      tmp->next=a;
      length--;
      return true;
    }
    else if(tmp)
      if(*tmp->data==data){
        a=tmp->next;
        delete tmp;
        head=a;
        length--;
        return true;
      }

    printf("\n\nWTF deleten\n\n");
    return false;
  }
  return false;
}

void LinkedList::print(){
  Node* head = this->head;
  int i = 1;
  while(head){
    std::cout << i << ": " ; head->data->print();cout << std::endl;
    head = head->next;
    i++;
  }
}

int LinkedList::getlen(){
  return length;
}
