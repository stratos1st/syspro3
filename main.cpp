#include <iostream>
#include <cstdlib>
#include "linked_list.h"
#include "tuple.h"


using namespace std;

int main(int argc, char const *argv[]){
    LinkedList* list = new LinkedList();
    for (int i = 0; i < 5; ++i){
      char tmp[10];
      sprintf(tmp, "%d",i);
      list->add(iptuple(tmp,"234"));
    }
    list->deleten(iptuple("2","234"));
    list->print();
    std::cout << "List Length: " << list->getlen() << std::endl;
    delete list;
    return 0;
}
