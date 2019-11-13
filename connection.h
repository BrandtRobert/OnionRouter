#ifndef CONNECTION_H
#define CONNECTION_H

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

using std::string;
using std::cout;
using std::cerr;

class Connection {
public:
    static int connect_to_server(string &ip, int port);
    static string receive_message(int client_fd);
    static bool send_message(int fd, string message);
    static int init_server(string port);
    static string addr_to_string(sockaddr *client_addr);
    static void print_listening_message();
private:
    static int receive_data_with_error_checking(int client_fd, const char* buffer, int length);
    static int send_data_with_error_checking(int fd, const char* data, int length);
}
#endif