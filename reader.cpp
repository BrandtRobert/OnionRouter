#include "reader.h"
#include "helper.cpp"

bool Reader::fetch(string url, string chainfile) {
    vector<pair<string, int>> chainfile_entries = read_chain_file(chainfile);
    int next_index = rand() % chainfile_entries.size();
    // Pop element at index
    pair<string, int> next_hop = chainfile_entries.at(next_index);
    chainfile_entries.erase(chainfile_entries.begin() + next_index);

    std::ostringstream out;
    out << url << "\n";
    out << chainfile_entries.size() << "\n";
    for (pair<string, int> entry : chainfile_entries) {
        out << entry.first << " " << std::to_string(entry.second) << "\n";
    }
    string msg(out.str());
    
    string response = relay(next_hop.first, next_hop.second, msg);
}

string Reader::relay(string ip, int port, string msg){
    sockaddr_in serv_addr;
    int sock;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        cerr << "There was an error when connecting to the server " << strerror(errno) << "\n";
        return "ERROR";
    }
    if(inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr)<=0){
        cerr << "There was an error in converting the server ip address " << strerror(errno) << "\n";
        return "ERROR";
    }
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        cerr << "Failed to connect to server " << strerror(errno) << "\n";
        return "ERROR";
    }

    uint16_t msg_length = htons(msg.size());
    if (send(sock, (char *)&msg_length, 2, 0) == -1) {
        cerr << "Send failed\n";
        return "ERROR";
    }

    const char *buf = message.c_str();
    int left_to_send = msg.size();
    int amount_sent = 0;
    while ((left_to_send - amount_sent) > 0){
        int valsent = 0;
        valsent = send(sock, buf+amount_sent, message_length-amount_sent, 0);
        if (valsent == -1){
            return "ERROR";
        }
        amount_sent += valsent;
    }
  }
}

/**
 * Returns a vector of pairs with <ipaddr, port>
 */
vector<pair<string, int>> Reader::read_chain_file(string chaingang) {
    string line;
    vector<pair<string, int>> entries;
    ifstream chainfile(chaingang);
    int line_count = 0;
    int num_lines = 0;
    if (chainfile.is_open()) {
        while(getline(chainfile, line)) {
            if (line_count == 0) {
                num_lines = std::stoi(line);
            }
            vector<string> splits{explode(line, ' ')};
            entries.push_back(std::make_pair(splits[0], std::stoi(splits[1])));
        }
    }
    return entries;
}

// /**
//  * Returns <chainfile, ip_next, port_next>
//  */
// tuple<string, string, int> Reader::get_first_hop(vector<pair<string,int>> chain_entries) {
//     int num_entries = chain_entries.size();
//     pair<string, int> rand_entry = chain_entries.at(rand() % num_entries);
// }