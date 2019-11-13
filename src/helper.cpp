#include "helper.h"

/**
 * From: http://www.cplusplus.com/articles/2wA0RXSz/ 
 */
const vector<string> Helper::explode(const string& s, const char& c)
{
	string buff{""};
	vector<string> v;
	
	for(auto n:s)
	{
		if(n != c) buff+=n; else
		if(n == c && buff != "") { v.push_back(buff); buff = ""; }
	}
	if(buff != "") v.push_back(buff);
	
	return v;
}