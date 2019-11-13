#include "steppingstone.h"

tuple<string, string, int> SteppingStone::get_next_hop_strip(string chain_msg){
  return make_tuple("Done", "localhost", 8888);
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

  if ((nextfd = Connection::connect_to_server(host_addr, port)) < 0) {
    return make_tuple(-1, "");
  }
  // send(nextfd, new_chain.c_str(), new_chain.length(), 0);

  return std::make_pair(nextfd, new_chain);
}

void SteppingStone::start_async_server(string port) {
  int server_fd = Connection::init_server(port);
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

  Connection::print_listening_message(port);
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
            cout << Connection::addr_to_string((struct sockaddr *)&client_addr) << "\n";
          }
        } else {
          string msg = Connection::receive_message(i);
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
        bool send_result = Connection::send_message(i, response);
        outbox.erase(i);
        FD_CLR(i, &write_fds);
      }
    }
  }
}
