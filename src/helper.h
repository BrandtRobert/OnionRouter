#ifndef HELPER_H
#define HELPER_H


#include<string>
#include<vector>
using std::string;
using std::vector;

class Helper {
public:
    static const vector<string> explode(const string& s, const char& c);
};

#endif