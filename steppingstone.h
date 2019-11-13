#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>
#include <string>
#include <iostream>
#include <tuple>
#include <map>

using std::string;
using std::tuple;
using std::pair;
using std::cout;
using std::cerr;
using std::make_tuple;
using std::map;
using std::get;


class SteppingStone {
public:
  void start_async_server(string port);
private:
  tuple<string, string, int> get_next_hop_strip(string chain_msg);
  int connect_to_server(string &ip, int port);
  pair<int, string> act_on_message(string msg, int source_fd, sockaddr* client_addr);

  string receive_message(int client_fd);
  int receive_data_with_error_checking(int client_fd, const char* buffer, int length);

  bool send_message(int fd, string message);
  int send_data_with_error_checking(int fd, const char* data, int length);

  int init_server(string port);
  string addr_to_string(sockaddr *client_addr);
  void print_listening_message();

  string port;
};
#endif
