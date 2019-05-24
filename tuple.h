#ifndef IPTUPLE
#define IPTUPLE

class iptuple{
private:

public:
  char ip[100],port[50];

  iptuple(const char *_ip, const char *_port);

  int get_ip()const;
  int get_port()const;
  const char* get_ip_str()const;
  const char* get_port_str()const;
  void print();
  bool operator==(const iptuple& rhs);

};

#endif
