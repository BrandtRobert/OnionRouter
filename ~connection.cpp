#include <iostream>
#include <cstring>
#include "connection.h"
#include <string.h>
extern int errno;

bool Connection::connect_to_server(string &ip, int port){
  sockaddr_in serv_addr;
  int sock;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    cerr << "There was an error when connecting to the server " << strerror(errno) << "\n";
    return false;
  }
  if(inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr)<=0){
    cerr << "There was an error in converting the server ip address " << strerror(errno) << "\n";
    return false;
  }
  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
    cerr << "Failed to connect to server " << strerror(errno) << "\n";
    return false;
  }
  this->client_fd = sock;
  return true;
}

string Connection::wait_for_connection(){
  int new_socket;
  sockaddr info;
  socklen_t addrlen = sizeof(struct sockaddr);
  print_listening_message();
  if (listen(server_fd, 5) < 0){
    cerr << "listening error " << strerror(errno) << "\n";
    return "";
  }

  if ((new_socket = accept(this->server_fd, (struct sockaddr *) &info, (socklen_t *)&addrlen))<0){
    cerr << "accept error " << strerror(errno) << "\n";
    return "";
  }
  this->client_fd = new_socket;
  return this->addr_to_string(&info);
}

int Connection::receive_data_with_error_checking(const char* buffer, int length){
  int valread = 0;
  valread = recv(this->client_fd, (void*)buffer, length, 0);
  if (valread == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      cerr << "Socket has timed out on read\n";
    } else {
      cerr << "There was an error when reading data: " << strerror(errno) << "\n";
    }
  } else if (valread == 0) {
      cerr << "Connection to host lost\n";
      return -1;
  }
  return valread;
}

string Connection::receive_message(){
  int REC_BUFFER_SIZE = 1024
  char buffer[REC_BUFFER_SIZE] = {0};
  uint16_t version_num;
  uint16_t message_length;

  if (receive_data_with_error_checking((char *) &version_num, 2) == -1){
    return "";
  }
  if (receive_data_with_error_checking((char *) &message_length, 2) == -1){
    return "";
  }

  version_num = ntohs(version_num);
  message_length = ntohs(message_length);

  if (version_num != VERSION_NUM) {
    send_message("Unrecognized Version Number!\n");
    return "";
  }

  if (message_length > REC_BUFFER_SIZE ) {
    send_message("Message too long!\n");
    return "";
  }

  int left_to_read = message_length;
  while (left_to_read > 0) {
    int valread = 0;
    valread = receive_data_with_error_checking(buffer, left_to_read);
    if (valread == -1) {
      return "";
    }
    left_to_read -= valread;
  }
  return string(buffer);
}

int Connection::send_data_with_error_checking(const char* data, int length){
  int valsent = 0;
  valsent = send(this->client_fd, data, length, 0);
  if (valsent == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      cerr << "socket has timed out on write\n";
    } else {
      cerr << "There was an error when writing data: " << strerror(errno) << "\n";
    }
  }
  return valsent;
}

bool Connection::send_message(string message){
  int amount_sent = 0;
  int message_length = message.length();

  uint16_t version_num = htons(VERSION_NUM);
  uint16_t length_network_order = htons(message_length);

  if (send_data_with_error_checking((char*)&version_num, 2) == -1){
    return false;
  }

  if (send_data_with_error_checking((char*)&length_network_order, 2) == -1){
    return false;
  }

  const char *buf = message.c_str();
  while ((message_length - amount_sent) > 0){
    int valsent = 0;
    valsent = send_data_with_error_checking(buf+amount_sent, message_length-amount_sent);
    if (valsent == -1){
      return false;
    }
    amount_sent += valsent;
  }
  return true;
}

bool Connection::init_server(string port){
  struct addrinfo hints, *res;
  int server_fd;
  int opt = 1;

  struct timeval timeout;
  timeout.tv_sec = 5;
  timeout.tv_usec = 0;

  memset(&hints, 0, sizeof hints);
  // hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

  getaddrinfo(NULL, port.c_str(), &hints, &res);

  // Creating socket file descriptor
  if ((server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == 0){
      cerr << "socket failed\n";
      return false;
  }
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))){
      cerr << "set socketopt: " << strerror(errno) << "\n";
      return false;
  }

  if (setsockopt (server_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0){
      cerr << "set socketopt: " << strerror(errno) << "\n";
      return false;
  }
  if (setsockopt (server_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0){
      cerr << "set socketopt: " << strerror(errno) << "\n";
      return false;
  }
  // Forcefully attaching socket to the port
  if (bind(server_fd, res->ai_addr, res->ai_addrlen)){
      cerr << "bind failed: " << strerror(errno) << "\n";
      return false;
  }
  this->server_fd = server_fd;
  freeaddrinfo(res);
  return true;
}

void Connection::print_listening_message(){
  ifaddrs *ip_addrs, *p;
  getifaddrs(&ip_addrs);
  for (p = ip_addrs; p != NULL; p = p->ifa_next){
    sockaddr *interface_addr = p->ifa_addr;
    // cout << "Interface: " << addr_to_string(interface_addr) << "\n";
    if (interface_addr->sa_family == AF_INET && string(p->ifa_name) == "eno1"){
      printf("Waiting for a connection on %s port %s\n",
              addr_to_string(interface_addr).c_str(), this->port.c_str());
    }
  }
  freeifaddrs(ip_addrs);
}

string Connection::addr_to_string(sockaddr *client_addr) {
  char str[INET6_ADDRSTRLEN] = {0};
  if (client_addr->sa_family == AF_INET) {
    sockaddr_in *sin = reinterpret_cast<sockaddr_in*>(client_addr);
    inet_ntop(AF_INET, &sin->sin_addr, str, INET6_ADDRSTRLEN);
  } else if (client_addr->sa_family == AF_INET6){
    sockaddr_in6 *sin = reinterpret_cast<sockaddr_in6*>(client_addr);
    inet_ntop(AF_INET6, &sin->sin6_addr, str, INET6_ADDRSTRLEN);
  }
  return string(str);
}

void Connection::close_connection(){
  close(this->client_fd);
}
