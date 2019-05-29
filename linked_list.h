#ifndef LINKED_LIST
#define LINKED_LIST

#include "tuple.h"


class Node{
 public:
  Node *next,*prev;
  iptuple *data;
  Node();
  ~Node();
  char* get_string();
};

class LinkedList{
private:
  int length;
  Node* head;
public:
  LinkedList();
  ~LinkedList();
  void add(iptuple data);
  void print();
  Node* find(iptuple data);
  bool deleten(iptuple data);
  int getlen();
  char* get_string();
  iptuple* get_by_index(unsigned int index);

};


#endif
