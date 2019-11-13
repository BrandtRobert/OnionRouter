#include "steppingstone.h"

tuple<string, string, int> SteppingStone::get_next_hop_strip(string chain_msg){
  return make_tuple("Done", "localhost", 8888);
}

int SteppingStone::connect_to_server(string &ip, int port){
  sockaddr_in serv_addr;
  int sock;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    cerr << "There was an error when connecting to the server " << strerror(errno) << "\n";
    return -1;
  }
  if(inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr)<=0){
    cerr << "There was an error in converting the server ip address " << strerror(errno) << "\n";
    return -1;
  }
  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
    cerr << "Failed to connect to server " << strerror(errno) << "\n";
    return -1;
  }
  return sock;
}

pair<int, string> SteppingStone::act_on_message(string msg, int source_fd, sockaddr* client_addr){
  tuple<string,string,int> res = get_next_hop_strip(msg);
  string new_chain = get<0>(res);
  string hostname = get<1>(res);
  int port = get<2>(res);
  int nextfd;

  hostent* next_hop_addrs = gethostbyname(hostname.c_str());
  if (!next_hop_addrs) {
    cerr << "An error has occured in gethostbyname " << strerror(errno) << "\n";
    return make_tuple(-1, "");
  }
  string host_addr(inet_ntoa(*(struct in_addr*)next_hop_addrs->h_addr));

  if ((nextfd = connect_to_server(host_addr, port)) < 0) {
    return make_tuple(-1, "");
  }
  // send(nextfd, new_chain.c_str(), new_chain.length(), 0);

  return std::make_pair(nextfd, new_chain);
}

void SteppingStone::start_async_server(string port) {
  int server_fd = init_server(port);
  if (server_fd < 0) {
    return;
  }
  this->port = port;

  int max_fd = server_fd;
  int new_fd;
  socklen_t addrlen;
  timeval timeout;
  timeout.tv_sec = 5;

  sockaddr_in client_addr;
  addrlen = sizeof(client_addr);

  fd_set master_fds;
  fd_set read_fds;
  fd_set write_fds;

  FD_ZERO(&master_fds);
  FD_ZERO(&read_fds);
  FD_ZERO(&write_fds);

  FD_SET(server_fd, &master_fds);

  map<int, string> outbox;

  print_listening_message();
  while (true) {
    read_fds = master_fds;
    int sel_return = select(max_fd+1, &read_fds, &write_fds, NULL, NULL);
    if (sel_return < 0) {
      cerr << strerror(errno) << "\n";
    }
    for (int i = 0; i <= max_fd; i++) {
      if (FD_ISSET(i, &read_fds)) {
        if (i == server_fd) {
          if ((new_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen)) == -1) {
            cerr << "There was an error when accepting a new SteppingStone" << strerror(errno) << '\n';
          } else {
            FD_SET(new_fd, &master_fds);
            if(new_fd > max_fd) {
              max_fd = new_fd;
            }
            cout << addr_to_string((struct sockaddr *)&client_addr) << "\n";
          }
        } else {
          string msg = receive_message(i);
          if (msg == "") {
            cout << "Error when reading socket.\n";
            FD_CLR(i, &master_fds);
            continue;
          } else {
            pair<int, string> fd_msg = act_on_message(msg, i, (sockaddr*)&client_addr);
            int response_fd = fd_msg.first;
            if (response_fd == -1) {
              // server not available pick another?
              continue;
            }
            outbox.insert(fd_msg);
            if (response_fd > max_fd) {
              max_fd = response_fd;
            }
            FD_SET(response_fd, &write_fds);
          }
        }
      } else if (FD_ISSET(i, &write_fds)) {
        string response = outbox[i];
        bool send_result = send_message(i, response);
        outbox.erase(i);
        FD_CLR(i, &write_fds);
      }
    }
  }
}

int SteppingStone::send_data_with_error_checking(int fd, const char* data, int length){
  int valsent = 0;
  valsent = send(fd, data, length, 0);
  if (valsent == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      cerr << "socket has timed out on write\n";
    } else {
      cerr << "There was an error when writing data: " << strerror(errno) << "\n";
    }
  }
  return valsent;
}

bool SteppingStone::send_message(int fd, string message){
  int amount_sent = 0;
  int message_length = message.length();

  // uint16_t version_num = htons(VERSION_NUM);
  // uint16_t length_network_order = htons(message_length);
  //
  // if (send_data_with_error_checking((char*)&version_num, 2) == -1){
  //   return false;
  // }
  //
  // if (send_data_with_error_checking((char*)&length_network_order, 2) == -1){
  //   return false;
  // }

  const char *buf = message.c_str();
  while ((message_length - amount_sent) > 0){
    int valsent = 0;
    valsent = send_data_with_error_checking(fd, buf+valsent, message_length-amount_sent);
    if (valsent == -1){
      return false;
    }
    amount_sent += valsent;
  }
  return true;
}

int SteppingStone::receive_data_with_error_checking(int client_fd, const char* buffer, int length){
  int valread = 0;
  valread = recv(client_fd, (void*)buffer, length, 0);
  if (valread == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      cerr << "Socket has timed out on read\n";
    } else {
      cerr << "There was an error when reading data: " << strerror(errno) << "\n";
    }
  } else if (valread == 0) {
      cerr << "Connection closed by client\n";
      return -1;
  }
  return valread;
}

string SteppingStone::receive_message(int client_fd){
  int REC_BUFFER_SIZE = 1024;
  char buffer[1024] = {0};
  // uint16_t version_num;
  // uint16_t message_length;

  // if (receive_data_with_error_checking((char *) &version_num, 2) == -1){
  //   return "";
  // }
  // if (receive_data_with_error_checking((char *) &message_length, 2) == -1){
  //   return "";
  // }

  // version_num = ntohs(version_num);
  // message_length = ntohs(message_length);

  // if (version_num != VERSION_NUM) {
  //   send_message("Unrecognized Version Number!\n");
  //   return "";
  // }
  //
  // if (message_length > REC_BUFFER_SIZE ) {
  //   send_message("Message too long!\n");
  //   return "";
  // }

  int valread = receive_data_with_error_checking(client_fd, buffer, REC_BUFFER_SIZE);
  if (valread == -1) {
    return "";
  }
  // int left_to_read = message_length;
  // while (left_to_read > 0) {
  //   int valread = 0;
  //   valread = receive_data_with_error_checking(buffer, left_to_read);
  //   if (valread == -1) {
  //     return "";
  //   }
  //   left_to_read -= valread;
  // }
  return string(buffer);
}

int SteppingStone::init_server(string port){
  struct addrinfo hints, *res;
  int opt = 1;
  int server_fd;

  memset(&hints, 0, sizeof hints);
  // hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  // hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
  hints.ai_flags = INADDR_ANY;

  getaddrinfo(NULL, port.c_str(), &hints, &res);

  // Creating socket file descriptor
  if ((server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == 0){
      cerr << "socket failed\n";
      return -1;
  }
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))){
      cerr << "set socketopt: " << strerror(errno) << "\n";
      return -1;
  }
  // Forcefully attaching socket to the port
  if (bind(server_fd, res->ai_addr, res->ai_addrlen)){
      cerr << "bind failed: " << strerror(errno) << "\n";
      return -1;
  }
  freeaddrinfo(res);
  if (listen(server_fd, 5) < 0){
    cerr << "listening error " << strerror(errno) << "\n";
    return -1;
  }
  return server_fd;
}

void SteppingStone::print_listening_message(){
  ifaddrs *ip_addrs, *p;
  getifaddrs(&ip_addrs);
  for (p = ip_addrs; p != NULL; p = p->ifa_next){
    sockaddr *interface_addr = p->ifa_addr;
    // cout << "Interface: " << addr_to_string(interface_addr) << "\n";
    if (interface_addr->sa_family == AF_INET && string(p->ifa_name) == "en0"){
      printf("Waiting for a connection on %s port %s\n",
              addr_to_string(interface_addr).c_str(), this->port.c_str());
    }
  }
  freeifaddrs(ip_addrs);
}

string SteppingStone::addr_to_string(sockaddr *client_addr) {
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
