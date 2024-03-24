#pragma once

#include <string>
#include <vector>

namespace sqlgen
{

	std::string getafter(const std::string& str, const std::string& data);
	std::string getafterinc(const std::string& str, const std::string& data);
	std::string getafter(const std::string& str, const std::string& data);
	std::string getafterinc(const std::string& str, const std::string& data);
	std::string getbetween(std::string s1, std::string s2, std::string data);
	std::string strdelete(std::string str, std::string data);
	void writestring(std::string str, std::string file);
	void writestring(char* str, unsigned int len, std::string file);
	std::string getuntil(std::string str, std::string data);
	std::string getuntil(std::string str, std::string data);
	std::string getuntilinc(std::string str, std::string data);
	std::string getline(int line, const std::string& str);
	int linecount(const std::string& str);
	std::string getFile(std::string filename);
	std::string getStreamFile(const std::string& fn);
	std::string ExtractFileName(std::string fulln, std::string separators = "/\\");
	std::string ExtractFilePath(std::string fulln, std::string separators = "/\\");
	std::string trim(const std::string& str);
	void Tokenize(const std::string& str, std::vector<std::string>& tokens, std::string seps);
	std::string strlower(const std::string& str);
	std::string greplace(std::string tor, std::string tin, std::string data);
	bool next(const std::string& pData, const size_t& doff, const std::string& pStr);
}

