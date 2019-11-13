#ifndef CONNECTION_H
#define CONNECTION_H

#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>
using namespace std;

#define VERSION_NUM 1

class Connection {
public:
  string wait_for_connection();
  bool connect_to_server(string &ip, int port);
  bool send_message(string message);
  string receive_message();
  void close_connection();
  void print_listening_message();

  // For constructing server
  Connection(string port){
    this->port = port;
    this->init_server(port);
  }

  // For constructing client (do not init server)
  Connection(){
  }

  // Connection() = delete;

private:
  bool init_server(string port);
  string addr_to_string(sockaddr *addr);
  int send_data_with_error_checking(const char *data, int length);
  int receive_data_with_error_checking(const char* buffer, int length);
  int server_fd;
  int client_fd;

  string port;
};
#endif
