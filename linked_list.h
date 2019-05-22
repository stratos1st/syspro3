#ifndef LINKED_LIST
#define LINKED_LIST

#include "tuple.h"


class Node{
 public:
  Node* next;
  iptuple *data;
  Node();
  // ~Node(){}
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

};


#endif
