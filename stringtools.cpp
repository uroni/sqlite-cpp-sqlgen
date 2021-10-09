/**
 * Copyright (C) Martin Raiber
 * SPDX-License-Identifier: Apache-2.0.
 */

#include <string>
#include <vector>
#include <fstream>
#include <sstream> 
#include <map>
#include <stdlib.h>

#ifndef _WIN32
#include <memory.h>
#include <stdlib.h>
#else
#include <Windows.h>
#endif

#include "stringtools.h"

using namespace std;
using namespace sqlgen;

typedef unsigned int u32;
typedef float f32;

namespace sqlgen
{

	string getafterinc(const std::string& str, const std::string& data)
	{
		size_t pos = data.find(str);
		if (pos != std::string::npos)
		{
			return data.substr(pos);
		}
		else
		{
			return "";
		}
	}

	string getafter(const std::string& str, const std::string& data)
	{
		std::string ret = getafterinc(str, data);
		ret.erase(0, str.size());
		return ret;
	}

	string getbetween(string s1, string s2, string data)
	{
		size_t off1 = data.find(s1);

		if (off1 == -1)return "";

		off1 += s1.size();

		size_t off2 = data.find(s2, off1);

		if (s2 == "\n")
		{
			size_t off3 = data.find("\r\n", off1);
			if (off3 < off2)
				off2 = off3;
		}

		if (off2 == -1)return "";

		string ret = data.substr(off1, off2 - off1);
		return ret;
	}

	string strdelete(string str, string data)
	{
		size_t off = data.find(str);
		if (off == -1)
			return data;
		data.erase(off, str.size());
		return data;
	}

	void writestring(string str, string file)
	{
		fstream out;
		out.open(file.c_str(), ios::out | ios::binary);

		out.write(str.c_str(), (int)str.size());

		out.close();
	}

	void writestring(char* str, unsigned int len, std::string file)
	{
		fstream out;
		out.open(file.c_str(), ios::out | ios::binary);

		out.write(str, len);
		out.flush();

		out.close();
	}

	string getuntil(string str, string data)
	{
		size_t off = data.find(str);
		if (off == -1)
			return "";
		return data.substr(0, off);
	}

	string getuntilinc(string str, string data)
	{
		size_t off = data.find(str);
		if (off == -1)
			return "";
		return data.substr(0, off + str.size());
	}

	string getline(int line, const string& str)
	{
		int num = 0;
		string tl;

		for (size_t i = 0; i < str.size(); i++)
		{
			if (str[i] == '\n')
			{
				if (num == line)
					break;
				num++;
			}
			if (str[i] != '\n' && str[i] != '\r' && num == line)
				tl += str[i];
		}

		return tl;
	}

	int linecount(const std::string& str)
	{
		int lines = 0;
		for (size_t i = 0; i < str.size(); i++)
		{
			if (str[i] == '\n')
				lines++;
		}
		return lines + 1;
	}

	std::string getFile(std::string filename)
	{
		std::fstream FileBin;
		FileBin.open(filename.c_str(), std::ios::in | std::ios::binary);
		if (FileBin.is_open() == false)
		{
			return "";
		}
		FileBin.seekg(0, std::ios::end);
		unsigned long FileSize = (unsigned int)std::streamoff(FileBin.tellg());
		FileBin.seekg(0, std::ios::beg);
		std::string ret;
		ret.resize(FileSize);
		FileBin.read(const_cast<char*>(ret.c_str()), FileSize);
		FileBin.close();
		return ret;
	}

	std::string getStreamFile(const std::string& fn)
	{
		std::fstream fin(fn.c_str(), std::ios::binary | std::ios::in);
		if (!fin.is_open())
			return std::string();

		std::string ret;
		while (!fin.eof())
		{
			char buf[512];
			fin.read(buf, sizeof(buf));
			ret.insert(ret.end(), buf, buf + fin.gcount());
		}
		return ret;
	}

	std::string strlower(const std::string& str)
	{
		std::string ret;
		ret.resize(str.size());
		for (size_t i = 0; i < str.size(); ++i)
		{
			ret[i] = tolower(str[i]);
		}

		return ret;
	}

	void strupper(std::string* pStr)
	{
		for (size_t i = 0; i < pStr->size(); ++i)
		{
			(*pStr)[i] = toupper((*pStr)[i]);
		}
	}

	string ExtractFileName(string fulln, string separators)
	{
		string filename;

		int off = 0;
		for (int i = (int)fulln.length() - 1; i > -1; i--)
		{
			bool separator = separators.find(fulln[i]) != string::npos;
			if (separator)
			{
				if (i < (int)fulln.length() - 1)
					break;
			}
			if (fulln[i] != 0 && !separator)
				filename = fulln[i] + filename;
		}

		return filename;
	}

	string ExtractFilePath(string fulln, string separators)
	{
		bool in = false;
		string path;
		for (int i = (int)fulln.length() - 2; i >= 0; --i)
		{
			if (separators.find(fulln[i]) != string::npos
				&& in == false)
			{
				in = true;
				continue;
			}

			if (in == true)
			{
				path = fulln[i] + path;
			}

		}

		return path;
	}


	std::string findextension(const std::string& pString)
	{
		std::string retv;
		std::string temp;

		for (int i = (int)pString.size() - 1; i >= 0; i--)
			if (pString[i] != '.')
				temp.push_back(pString[i]);
			else
				break;

		for (int i = (int)temp.size() - 1; i >= 0; i--)
			retv.push_back(temp[i]);

		return retv;
	}

	std::string replaceonce(std::string tor, std::string tin, std::string data)
	{
		int off = (int)data.find(tor);
		if (off != -1)
		{
			data.erase(off, tor.size());
			data.insert(off, tin);
		}
		return data;
	}

	void Tokenize(const std::string& str, std::vector<std::string>& tokens, std::string seps)
	{
		// one-space line for storing blank lines in the file
		std::string blankLine = "";

		// pos0 and pos1 store the scope of the current turn, i stores
		// the position of the symbol \".
		int pos0 = 0, pos1 = 0;
		while (true)
		{
			// find the next seperator
			pos1 = (int)str.find_first_of(seps, pos0);
			// find the next \" 

			// if the end is reached..
			if (pos1 == std::string::npos)
			{
				// ..push back the string to the end
				std::string nt = str.substr(pos0, str.size());
				if (nt != "")
					tokens.push_back(nt);
				break;
			}
			// if two seperators are found in a row, the file has a blank
			// line, in this case the one-space string is pushed as a token
			else if (pos1 == pos0)
			{
				tokens.push_back(blankLine);
			}
			else
				// if no match is found, we have a simple token with the range
				// stored in pos0/1
				tokens.push_back(str.substr(pos0, (pos1 - pos0)));

			// equalize pos
			pos0 = pos1;
			// increase 
			++pos1;
			// added for ini-file!
			// increase by length of seps
			++pos0;
		}
	}

	bool str_isnumber(char ch)
	{
		if (ch >= 48 && ch <= 57)
			return true;
		else
			return false;
	}

	bool isletter(char ch)
	{
		ch = toupper(ch);
		if (ch <= 90 && ch >= 65)
			return true;
		else
			return false;
	}


	bool next(const std::string& pData, const size_t& doff, const std::string& pStr)
	{
		for (size_t i = 0; i < pStr.size(); ++i)
		{
			if (i + doff >= pData.size())
				return false;
			if (pData[doff + i] != pStr[i])
				return false;
		}
		return true;
	}

	std::string greplace(std::string tor, std::string tin, std::string data)
	{
		for (size_t i = 0; i < data.size(); ++i)
		{
			if (next(data, i, tor) == true)
			{
				data.erase(i, tor.size());
				data.insert(i, tin);
				i += tin.size() - 1;
			}
		}

		return data;
	}

	int getNextNumber(const std::string& pStr, int* read)
	{
		std::string num;
		bool start = false;
		for (size_t i = 0; i < pStr.size(); ++i)
		{
			if (str_isnumber(pStr[i]))
			{
				num += pStr[i];
				start = true;
			}
			else if (start == true)
				return atoi(num.c_str());

			if (read != NULL)
				++* read;
		}

		return 0;
	}

	bool FileExists(std::string pFile)
	{
		fstream in(pFile.c_str(), ios::in);
		if (in.is_open() == false)
			return false;

		in.close();
		return true;
	}


	std::string trim(const std::string& str)
	{
		size_t startpos = str.find_first_not_of(" \r\n\t");
		size_t endpos = str.find_last_not_of(" \r\n\t");
		if ((string::npos == startpos) || (string::npos == endpos))
		{
			return "";
		}
		else
		{
			return str.substr(startpos, endpos - startpos + 1);
		}
	}

}