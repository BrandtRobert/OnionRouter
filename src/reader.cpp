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

    return (response == "ERROR") ? false : true;
}

string Reader::relay(string ip, int port, string msg){
    int next_hop_fd = Connection::connect_to_server(ip, port);
    if (next_hop_fd < 0) {
        return "ERROR";
    }
    bool send_res = Connection::send_message(next_hop_fd, msg);
    if (send_res == false) {
        return "ERROR";
    }
    string response = Connection::receive_message(next_hop_fd);
    if (response == "") {
        return "ERROR";
    }
    return response;
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