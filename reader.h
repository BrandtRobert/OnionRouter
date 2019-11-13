#ifndef READER_H
#define READER_H

#include<iostream>
#include<fstream>
#include<vector>
#include<stdlib.h>
#include<tuple>
#include<stdio.h>
#include<sstream>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>

using std::string;
using std::tuple;
using std::pair;
using std::cin;
using std::cout;
using std::cerr;
using std::vector;
using std::ifstream;
using std::getline;

// srand(time(NULL))
class Reader {
public:
    bool fetch(string url, string chaingang);
private:
    string relay(string ip, int port, string msg);
    vector<pair<string, int>> read_chain_file(string chaingang);
};
#endif