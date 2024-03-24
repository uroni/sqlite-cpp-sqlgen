/**
 * Copyright (C) Martin Raiber
 * SPDX-License-Identifier: Apache-2.0.
 */

#include "sqlgen.h"
#include "stringtools.h"
#include <regex>
#include <iostream>
#include <map>
#include "Database.h"
#include "DatabaseQuery.h"

using namespace sqlgen;

namespace sqlgen
{

struct GenConfig
{
	std::string tab = "\t";
	std::string newline = "\r\n";
	std::string query_type = "IQuery";
	std::string cursor_type = "IDatabaseCursor";
};

enum CPPFileTokenType
{
	CPPFileTokenType_Code,
	CPPFileTokenType_Comment
};

struct CPPToken
{
	CPPToken(const std::string& data, CPPFileTokenType type)
		: data(data), type(type)
	{
	}

	std::string data;
	CPPFileTokenType type;
};

enum TokenizeState
{
	TokenizeState_None,
	TokenizeState_CommentMultiline,
	TokenizeState_CommentSingleline
};

std::vector<CPPToken> tokenizeFileRegex(std::string &cppfile)
{
	//So much for that. Causes stack overflows
	std::regex find_comments("(/\\*(\\S|\\s)*?\\*/)|(//.*)", std::regex::ECMAScript);
	auto comments_begin=std::regex_iterator<std::string::iterator>(cppfile.begin(), cppfile.end(), find_comments);
	auto comments_end=std::regex_iterator<std::string::iterator>();

	std::vector<CPPToken> tokens;
	size_t lastPos=0;
	for(auto i=comments_begin;i!=comments_end;++i)
	{
		auto m=*i;
		size_t pos=m.position(0);
		if(lastPos<pos)
		{
			tokens.push_back(CPPToken(cppfile.substr(lastPos, pos-lastPos), CPPFileTokenType_Code));
			lastPos=pos;
		}
		tokens.push_back(CPPToken(m.str(), CPPFileTokenType_Comment));
		lastPos+=m.length();
	}
	if(lastPos<cppfile.size())
	{
		tokens.push_back(CPPToken(cppfile.substr(lastPos), CPPFileTokenType_Code));
	}

	return tokens;
}

std::vector<CPPToken> tokenizeFile(std::string &cppfile)
{
	std::string cdata;
	std::vector<CPPToken> tokens;
	int state = 0;
	for (size_t i = 0; i < cppfile.size(); ++i)
	{
		char ch = cppfile[i];

		cdata += ch;

		switch (state)
		{
		case 0:
			if (ch == '/')
			{
				state = 1;
			}
			break;
		case 1:
			if (ch == '*')
			{
				state = 2;

				if (cdata.size() > 2)
				{
					tokens.push_back(CPPToken(cdata.substr(0, cdata.size() - 2), CPPFileTokenType_Code));
				}
				cdata = "/*";
			}
			else if (ch == '/')
			{
				state = 3;
				if (cdata.size() > 2)
				{
					tokens.push_back(CPPToken(cdata.substr(0, cdata.size() - 2), CPPFileTokenType_Code));
				}
				cdata = "//";
			}
			else
			{
				state = 0;
			}
			break;
		case 2:
			if (ch == '*')
			{
				state = 4;
			}
			break;
		case 3:
			if (ch == '\n')
			{
				state = 0;
				if (cdata.size() > 2 && cdata[cdata.size() - 2] == '\r')
				{
					tokens.push_back(CPPToken(cdata.substr(0, cdata.size() - 2), CPPFileTokenType_Comment));
					cdata = "\r\n";
				}
				else
				{
					tokens.push_back(CPPToken(cdata.substr(0, cdata.size() - 1), CPPFileTokenType_Comment));
					cdata = "\n";
				}
				cdata = "\n";
			}
			break;
		case 4:
			if (ch == '/')
			{
				state = 0;
				tokens.push_back(CPPToken(cdata, CPPFileTokenType_Comment));
				cdata.clear();
			}
			else if(ch!='*')
			{
				state = 2;
			}
			break;
		}
	}

	if (!cdata.empty())
	{
		tokens.push_back(CPPToken(cdata, CPPFileTokenType_Code));
	}

	return tokens;
}

struct AnnotatedCode
{
	AnnotatedCode(std::map<std::string, std::string> annotations, std::string code)
		: annotations(annotations), code(code)
	{
	}

	AnnotatedCode(std::string code)
		: code(code)
	{
	}

	std::map<std::string, std::string> annotations;
	std::string code;
};

std::string cleanup_annotation(const std::string& annotation)
{
	int state=0;
	std::string ret;
	for(size_t i=0;i<annotation.size();++i)
	{
		if(state==0)
		{
			if(annotation[i]=='\n' || annotation[i]=='\r')
			{
				state=1;
			}
		}
		else if(state==1)
		{
			if(annotation[i]!=' ' && annotation[i]!='*' && annotation[i]!='\n' && annotation[i]!='\t' )
			{
				state=0;
			}
		}

		if(annotation[i]=='\n')
		{
			ret+=" ";
		}

		if(state==0)
		{
			ret+=annotation[i];
		}
	}

	return ret;
};

std::string extractFirstFunction(const std::string& data)
{
	int c=0;
	bool was_in_function=false;
	for(size_t i=0;i<data.size();++i)
	{
		if(data[i]=='{') ++c;
		if(data[i]=='}') --c;

		if(c>0)
		{
			was_in_function=true;
		}

		if(c==0 && was_in_function)
		{
			return data.substr(0, i+1);
		}
	}

	return std::string();
}

std::map<std::string, std::string> parseAnnotations(const std::string& data)
{
	int state = 0;
	std::string name;
	std::string content;
	std::map<std::string, std::string> ret;
	for (size_t i = 0; i < data.size(); ++i)
	{
		char ch = data[i];
		switch (state)
		{
		case 0:
		{
			if (ch == '@')
				state = 1;
		}	break;
		case 1:
		{
			if (ch=='\r' || ch == '\n' || ch==' ')
			{
				name = trim(name);
				state = 2;
				content += ch;
			}
			else
			{
				name += ch;
			}
		}break;
		case 3:
		case 2:
		{
			if (ch == '@')
			{
				state = 1;
				ret[name] = trim(cleanup_annotation(content));
				name.clear();
				content.clear();
			}
			else if (ch == '*' && state==2)
			{
				content += ch;
				state = 3;
			}
			else if (ch == '*' && state == 3)
			{
				content += ch;
			}
			else if (ch == '/' && state == 3)
			{
				state = 0;
				ret[name] = trim(cleanup_annotation(content));
				name.clear();
				content.clear();
			}
			else
			{
				content += ch;
				state = 2;
			}
		}break;
		}
	}

	return ret;
}

std::map<std::string, std::string> parseAnnotationSingle(const std::string& data)
{
	//Doesn't work with MSVC2015 (out of stack memory): "@([^ \\r\\n]*)[ ]*(((?!@)(?!\\*/)(\\S|\\s))*)"
	//std::regex find_annotations = std::regex("@([^ \\r\\n]*)[ ]*((\\S|\\s)*?)(?=(\\*/)|@)", std::regex::ECMAScript);
	std::regex find_annotations = std::regex("@([^ \\r\\n]*)[ ]*((\\S|\\s)*?)", std::regex::ECMAScript);
	std::map<std::string, std::string> ret;
	for (auto it = std::regex_iterator<std::string::const_iterator>(data.begin(), data.end(), find_annotations);
		it != std::regex_iterator<std::string::const_iterator>(); ++it)
	{
		auto m = *it;
		std::string annotation_text = m[2].str();
		ret[m[1].str()] = trim(cleanup_annotation(annotation_text));
	}

	return ret;
}

std::vector<AnnotatedCode> getAnnotatedCode(const std::vector<CPPToken>& tokens)
{
	std::vector<AnnotatedCode> ret;
	for(size_t i=0;i<tokens.size();++i)
	{
		if(tokens[i].type==CPPFileTokenType_Comment)
		{
			std::map<std::string, std::string> annotations = parseAnnotations(tokens[i].data);

			if (tokens[i].data.find("//") == 0)
			{
				annotations = parseAnnotationSingle(tokens[i].data);
			}

			ret.push_back(AnnotatedCode(tokens[i].data));

			if(!annotations.empty())
			{
				if(i+1<tokens.size() && tokens[i+1].type==CPPFileTokenType_Code)
				{
					std::string next_code=tokens[i+1].data;
					std::string first_function=extractFirstFunction(next_code);

					if(!first_function.empty())
					{
						ret.push_back(AnnotatedCode(annotations, first_function));
						ret.push_back(AnnotatedCode(next_code.substr(first_function.size())));
						++i;
					}
					else
					{
						ret.push_back(AnnotatedCode(annotations, ""));
					}
				}
			}
		}
		else
		{
			ret.push_back(AnnotatedCode(tokens[i].data));
		}
	}

	return ret;
}

struct ReturnType
{
	ReturnType(std::string type, std::string name)
		: type(type), name(name)
	{
	}

	std::string type;
	std::string name;
};

std::vector<ReturnType> parseReturnTypes(std::string return_str)
{
	std::vector<std::string> toks;
	Tokenize(return_str, toks, ",");
	std::vector<ReturnType> ret;
	for(size_t i=0;i<toks.size();++i)
	{
		toks[i]=trim(toks[i]);
		ret.push_back(ReturnType(getuntil(" ", toks[i]), getafter(" ", toks[i])));
	}
	return ret;
}

std::string parseSqlString(std::string sql, std::vector<ReturnType>& types)
{
	std::regex find_var(":([^ (]*)\\(([^)]*?)\\)",std::regex::ECMAScript);
	size_t lastPos=0;
	std::string retSql;
	for(auto it=std::regex_iterator<std::string::const_iterator>(sql.begin(), sql.end(), find_var);
				it!=std::regex_iterator<std::string::const_iterator>();++it)
	{
		auto m=*it;
		if(m.position()>lastPos)
		{
			retSql+=sql.substr(lastPos, m.position()-lastPos);
			lastPos=m.position()+m[0].length();
		}
		retSql+="?";
		types.push_back(ReturnType(m[2].str(), m[1].str()));
	}
	if(lastPos<sql.size())
	{
		retSql+=sql.substr(lastPos);
	}
	return retSql;
}

struct SStructure
{
	bool use_exist;
	std::string code;
};

struct GeneratedData
{
	std::string funcdecls;
	std::map<std::string, SStructure> structures;
	std::string variables;
};

void generateStructure(std::string name, std::vector<ReturnType> return_types, const GenConfig& config, GeneratedData& gen_data, bool use_exists)
{
	if(gen_data.structures.find(name)!=gen_data.structures.end() && (!use_exists || gen_data.structures[name].use_exist ) )
	{
		return;
	}

	std::string t = config.tab;
	std::string nl = config.newline;

	std::string code;
	code+=t + "struct "+name+config.newline;
	code+=t + "{" + nl;
	if(use_exists)
	{
		code+=t + t + "bool exists;" + nl;
	}
	for(size_t i=0;i<return_types.size();++i)
	{
		std::string type=return_types[i].type;
		if(type=="string")
			type="std::string";
		if(type=="blob")
			type="std::string";
		if(type=="int64")
			type="int64_t";

		code+=t + t + type+" "+return_types[i].name+";" + nl;
	}
	code+=t + "};" + nl;
	SStructure s = {use_exists, code};
	gen_data.structures[name] = s;
}

std::string generateConditional(ReturnType rtype, const GenConfig& config, GeneratedData& gen_data)
{
	std::string cond_name;
	if(!rtype.type.empty())
	{
		cond_name+=(char)toupper(rtype.type[0]);
		cond_name+=rtype.type.substr(1);
	}
	cond_name="Cond"+cond_name;

	if(gen_data.structures.find(cond_name)!=gen_data.structures.end())
	{
		return cond_name;
	}

	std::string t = config.tab;
	std::string nl = config.newline;

	std::string code;
	code+=t + "struct "+cond_name+config.newline;
	code+=t + "{" + nl;
	code+=t + t + "bool exists;" + nl;
	std::string type=rtype.type;
	if(type=="string")
		type="std::string";
	if(type=="blob")
		type="std::string";
	if(type=="int64")
		type="int64_t";
	code+=t + t + type+" value;" + nl;
	code+=t + "};" + nl;

	SStructure s = {true, code};
	gen_data.structures[cond_name] = s;

	return cond_name;
}

enum StatementType
{
	StatementType_Select,
	StatementType_Delete,
	StatementType_Insert,
	StatementType_Update,
	StatementType_Create,
	StatementType_Drop,
	StatementType_None
};

std::vector<std::string> tokenizeIgnoreParentheses(const std::string& data, char sep)
{
	int state = 0;
	std::string str;
	std::vector<std::string> ret;
	for (char ch : data)
	{
		if (state == 0)
		{
			if (ch == sep)
			{
				ret.push_back(str);
				str.clear();
			}
			else if (ch == '(')
			{
				++state;
			}
			else
			{
				str += ch;
			}
		}
		else
		{
			if (ch == '(')
				++state;
			else if (ch == ')')
				--state;
		}
	}

	if (state==0 && !str.empty())
		ret.push_back(str);

	return ret;
}


std::string return_blob(size_t tabs, std::string value_name, std::string sql_name, std::string res_idx, bool do_return, bool use_at)
{
	std::string ret;
	static int nb = 0;
	std::string tabss(tabs, '\t');
	++nb;
	auto astr = "res["+res_idx+"]";
	if(use_at)
		astr = "res.at("+res_idx+")";
	ret+=tabss+value_name+"="+astr+"[\""+sql_name+"\"];\r\n";
	if(do_return)
	{
		ret+=tabss+"return "+value_name+";\r\n";
	}
	return ret;
}

std::string getReturnCol(const std::string& return_name, const std::map<std::string, size_t>& return_cols)
{
	auto it = return_cols.find(return_name);
	if (it == return_cols.end())
		return "\"" + return_name + "\"";
	else
		return std::to_string(it->second);
}

AnnotatedCode generateSqlFunction(Database& db, AnnotatedCode input, const GenConfig& config, GeneratedData& gen_data, bool check)
{
	std::string nl = config.newline;
	std::string t = config.tab;
	std::string sql=input.annotations["sql"];
	std::string func=input.annotations["func"];
	std::string return_type=getuntil(" ", func);
	std::string funcsig=getafter(" ", func);

	std::cout << "Generating func " << funcsig << std::endl;

	std::string struct_name=return_type;

	std::string query_name=funcsig;
	std::string func_s_name=funcsig;
	std::string classname;
	if(query_name.find("::")!=std::string::npos)
	{
		classname=getuntil("::", query_name);
		query_name=getafter("::", query_name);
		func_s_name=query_name;
	}

	if(query_name.empty())
	{
		std::cout << "Empty query name" << std::endl;
		return AnnotatedCode(input.annotations, ""); 
	}

	query_name="_"+query_name;

	query_name[1] = strlower(query_name.substr(1,1))[0];


	bool return_optional = false;

	if (return_type.find("optional") == 0)
	{
		return_type = "std::" + return_type;
	}

	if (return_type.find("std::optional") == 0)
	{
		return_optional = true;

		size_t last_c = return_type.find_last_of('>');
		if (last_c == std::string::npos)
		{
			std::cout << "cannot find closing > for optional in func " << func << std::endl;
			return AnnotatedCode(input.annotations, "");
		}

		size_t first_c = return_type.find('<');
		if (first_c == std::string::npos)
		{
			std::cout << "cannot find opening < for optional in func " << func << std::endl;
			return AnnotatedCode(input.annotations, "");
		}

		return_type = return_type.substr(first_c+1, last_c - first_c - 1);
	}

	bool return_vector=false;

	if(return_type.find("vector")==0)
	{
		return_type="std::"+return_type;
	}

	if(return_type.find("std::vector")==0)
	{
		struct_name=getbetween("<", ">", return_type);
		if(struct_name=="string")
		{
			return_type="std::vector<std::string>";
		}
		if(struct_name=="blob")
		{
			return_type="std::vector<std::string>";
		}
		if(struct_name=="int64")
		{
			return_type="std::vector<int64_t>";
		}
		return_vector=true;
	}

	StatementType stmt_type=StatementType_None;
	size_t op_pos;
	if( (op_pos=strlower(sql).find("select"))!=std::string::npos)
	{
		stmt_type=StatementType_Select;
	}
	size_t new_pos;
	if( (new_pos = strlower(sql).find("delete"))!=std::string::npos
		&& new_pos<op_pos)
	{
		stmt_type=StatementType_Delete;
		op_pos=new_pos;
	}
	if( (new_pos = strlower(sql).find("insert"))!=std::string::npos
		&& new_pos<op_pos)
	{
		stmt_type=StatementType_Insert;
		op_pos=new_pos;
	}
	if( (new_pos = strlower(sql).find("update"))!=std::string::npos
		&& new_pos<op_pos)
	{
		stmt_type=StatementType_Update;
		op_pos=new_pos;
	}
	if( (new_pos = strlower(sql).find("create"))!=std::string::npos
		&& new_pos<op_pos)
	{
		stmt_type=StatementType_Create;
		op_pos=new_pos;
	}
	if( (new_pos = strlower(sql).find("drop"))!=std::string::npos
		&& new_pos<op_pos)
	{
		stmt_type=StatementType_Drop;
		op_pos=new_pos;
	}

	std::string return_vals=input.annotations["return"];

	std::vector<ReturnType> return_types=parseReturnTypes(return_vals);

	bool use_struct=false;
	bool use_cond=false;
	bool use_exists=false;
	bool use_raw=false;
	if(return_types.size()>1)
	{
		use_struct=true;
	}
	
	if(return_vector)
	{
		if(return_types.size()>1)
		{
			generateStructure(struct_name, return_types, config, gen_data, false);
		}
	}
	else if(strlower(return_type)!="void" && strlower(return_type)!="bool")
	{
		if(return_types.size()==1)
		{
			if(return_optional ||
				return_types[0].type.find("_raw")!=std::string::npos)
			{
				return_types[0].type=greplace("_raw", "", return_types[0].type);
				if(return_types[0].type=="int64")
					return_types[0].type = "int64_t";
				use_raw=true;
				struct_name=return_types[0].type;
			}
			else
			{
				struct_name=generateConditional(return_types[0], config, gen_data);
			}			
		}
		else
		{
			generateStructure(struct_name, return_types, config, gen_data, true);
			use_exists=true;
		}
	}

	std::vector<ReturnType> params;
	std::string parsedSql=parseSqlString(sql, params);

	if (check)
	{
		try
		{
			auto q = db.prepare("EXPLAIN "+parsedSql);
			q.read();
		}
		catch(sqlgen::PrepareError& e)
		{
			std::cout << "ERROR preparing statement: " << parsedSql << " Function: " << func << ": " << e.what() << std::endl;
			return AnnotatedCode(input.annotations, "");
		}		
	}

	std::map<std::string, size_t> return_cols;

	if (stmt_type == StatementType_Select ||
		stmt_type == StatementType_Delete ||
		stmt_type == StatementType_Insert ||
		stmt_type == StatementType_Update)
	{
		std::string select_vars;
		if(stmt_type == StatementType_Select)
		{
			size_t select_pos = strlower(parsedSql).find("select");
			size_t from_pos = strlower(parsedSql).find(" from ");
			select_vars = trim(parsedSql.substr(select_pos + 6, from_pos - select_pos - 6));
		}
		else
		{
			size_t returning_pos = strlower(parsedSql).find(" returning ");
			if(returning_pos!=std::string::npos)
				select_vars = trim(parsedSql.substr(returning_pos+11));
		}
		if (!select_vars.empty() && select_vars != "*")
		{
			std::vector<std::string> return_exp_vars = tokenizeIgnoreParentheses(select_vars, ',');
			for (size_t i = 0; i < return_exp_vars.size(); ++i)
			{
				return_exp_vars[i] = trim(return_exp_vars[i]);
				std::string rev_lower = strlower(return_exp_vars[i]);
				size_t as_pos = rev_lower.find(" as ");
				if (as_pos != std::string::npos)
				{
					return_exp_vars[i] = trim(return_exp_vars[i].substr(as_pos + 4));
				}
				else if (next(rev_lower, 0, "as "))
				{
					return_exp_vars[i] = trim(return_exp_vars[i].substr(3));
				}
				else if (return_exp_vars[i].find(".") != std::string::npos)
				{
					return_exp_vars[i] = getafter(".", return_exp_vars[i]);
				}

				return_cols[return_exp_vars[i]] = i;
			}

			if (check)
			{
				for (size_t i = 0; i < return_types.size(); ++i)
				{
					if (std::find(return_exp_vars.begin(), return_exp_vars.end(), return_types[i].name)
						== return_exp_vars.end())
					{
						std::cout << "ERROR Cannot find variable '" << return_types[i].name << "' in SQL: " << parsedSql << " Function: " << func << std::endl;
						return AnnotatedCode(input.annotations, "");
					}
				}
			}
		}
	}

	std::string return_outer=return_type;
	if(return_vector)
	{
		return_outer="std::vector<";
		if(use_struct)
		{
			return_outer+=(classname.empty()?"":classname+"::")+struct_name;
		}
		else
		{
			if(return_types.empty())
			{
				std::cout << "@return is missing!" << std::endl;
			}
			else
			{
				if(return_types[0].type=="string")
				{
					return_outer+="std::string";
				}
				else if(return_types[0].type=="blob")
				{
					return_outer+="std::string";
				}
				else if(return_types[0].type=="int64")
				{
					return_outer+="int64_t";
				}
				else
				{
					return_outer+=return_types[0].type;
				}
			}			
		}
		return_outer+=">";
	}
	else if(use_cond)
	{
		return_outer=(classname.empty()?"":classname+"::")+struct_name;
		return_type=struct_name;
	}
	else if(struct_name!="string" && struct_name!="void" && struct_name!="int"
		&& struct_name!="bool" && struct_name!="int64" && struct_name!="int64_t")
	{
		return_outer=(classname.empty()?"":classname+"::")+struct_name;
		return_type=struct_name;
	}
	

	if(return_outer=="string")
		return_outer="std::string";

	if(return_type=="string")
		return_type="std::string";

	if(return_type=="string")
		return_type="std::wstring";

	if(return_type=="int64")
		return_type = "int64_t";

	if (return_optional)
	{
		return_outer = "std::optional<" + return_type + ">";
		return_type = "std::optional<" + return_type + ">";
	}

	std::string funcdecl=return_type+" "+func_s_name+"(";
	std::string code=nl+return_outer+" "+funcsig+"(";
	for(size_t i=0;i<params.size();++i)
	{
		bool found=false;
		for(size_t j=0;j<i;++j)
		{
			if(params[j].name==params[i].name)
			{
				found=true;
				break;
			}
		}
		if(found)
		{
			continue;
		}

		if(i>0)
		{
			code+=", ";
			funcdecl+=", ";
		}
		std::string type=params[i].type;
		if(type=="string" || type=="std::string" )
		{
			type="const std::string&";
		}
		else if(type=="blob")
		{
			type="const std::string&";
		}
		else if(type=="int64")
		{
			type="int64_t";
		}
		code+=type+" "+params[i].name;
		funcdecl+=type+" "+params[i].name;
	}
	code+=")" + nl +"{" + nl;
	funcdecl+=");";

	gen_data.funcdecls+=t + funcdecl+ nl;
	gen_data.variables+="\tsqlgen::DatabaseQuery "+query_name+";\r\n";

	code+="\tif(!"+query_name+".prepared())\r\n\t{\r\n\t";
	code+="\t"+query_name+"=db.prepare(\""+parsedSql+"\");\r\n";
	code+=t + "}" + nl;

	for(size_t i=0;i<params.size();++i)
	{
		if(params[i].type=="blob")
		{
			code+="\t"+query_name+".bind("+params[i].name+".data(), "+params[i].name+".size());\r\n";
		}
		else
		{
			code+="\t"+query_name+".bind("+params[i].name+");\r\n";
		}
	}

	bool has_return=false;
	bool need_return = true;

	if(stmt_type==StatementType_Select || !return_types.empty())
	{
		code+=t + "auto& cursor="+query_name+".cursor();" + nl;
	}
	else if(stmt_type==StatementType_Delete
		|| stmt_type==StatementType_Insert
		|| stmt_type==StatementType_Update
		|| stmt_type==StatementType_Create
		|| stmt_type==StatementType_Drop )
	{
		if(return_type=="bool")
		{
			code+="\tbool ret = "+query_name+".write();\r\n";
			has_return=true;
		}
		else
		{
			code+="\t"+query_name+".write();\r\n";
			need_return=false;
		}
	}

	if(has_return)
	{
		code+=t + "return ret;" + nl;
		need_return=false;
	}

	if(return_vector)
	{
		code+=t + "std::vector<";
		std::string v_type;
		if(use_struct)
		{
			v_type = (classname.empty()?"":classname+"::")+struct_name;
		}
		else
		{
			if(!return_types.empty())
			{
				if(return_types[0].type=="string")
				{
					v_type = "std::string";
				}
				else if(return_types[0].type=="blob")
				{
					v_type = "std::string";
				}
				else if(return_types[0].type=="int64")
				{
					code+="int64_t";
				}
				else
				{
					v_type = return_types[0].type;
				}
			}
			else
			{
				std::cout << "@return is missing!" << std::endl;
				//TODO error handling
			}
		}
		code += v_type;
		code+="> ret;" + nl;
		code+=t + "while(cursor.next())" + nl;
		code+=t + "{" + nl;
		if(use_struct)
		{
			code+=t + t + "ret.emplace_back();" + nl;
			code += t + t + v_type + "& obj=ret.back();" + nl;
			if(gen_data.structures[struct_name].use_exist)
			{
				code+=t + t + "obj.exists=true;" + nl;
			}
			for(size_t i=0;i<return_types.size();++i)
			{

				code += t + t + "cursor.get("+ getReturnCol(return_types[i].name ,
					return_cols)+", obj." + return_types[i].name + ");" + nl;
			}
		}
		else
		{
			if(!return_types.empty())
			{
				code += t + t + "ret.emplace_back();" + nl;
				code += t + t + "cursor.get("+ getReturnCol(return_types[0].name,
					return_cols)+", ret.back());" + nl;
			}

		}
		code+=t + "}" + nl;		
	}
	else if(!return_types.empty() && !use_raw)
	{
		code+=t + struct_name+" ret = { ";
		if(!use_cond)
		{
			code+="false, ";
			for(size_t i=0;i<return_types.size();++i)
			{
				if(return_types[i].type=="int" || return_types[i].type=="int64" || return_types[i].type=="int64_t")
				{
					code+="0";
				}
				else if(return_types[i].type=="blob")
				{
					code+="\"\"";
				}
				else
				{
					code+="\"\"";
				}
				if(i+1<return_types.size())
				{
					code+=", ";
				}
			}
		}
		else
		{
			code+="false, ";
			if(return_types[0].type=="int" || return_types[0].type=="int64" || return_types[0].type=="int64_t")
			{
				code+="0";
			}
			else if(return_types[0].type=="blob")
			{
				code+="\"\"";
			}
			else
			{
				code+="\"\"";
			}
		}
		code+=" };" + nl;
		code+=t + "if(cursor.next())" + nl;
		code+=t + "{" + nl;
		if(use_exists)
		{
			code+=t + t + "ret.exists=true;" + nl;
		}
		if(!use_cond)
		{
			for(size_t i=0;i<return_types.size();++i)
			{
				code += t + t + "cursor.get("+getReturnCol(return_types[i].name ,
					return_cols)+", ret." + return_types[i].name + ");" + nl;
			}
		}
		else
		{
			code += t + t + "cursor.get("+getReturnCol(return_types[0].name,
				return_cols)+", ret.value);" + nl;
		}
		code+=t + "}" + nl;	
	}
	else if(return_types.size()==1)
	{
		if (!return_optional)
		{
			code += t + "assert(cursor.next());" + nl;
		}
		else
		{
			code += t + "if(!cursor.next())" + nl;
			code += t + "{" + nl;
			if (!params.empty())
			{
				code += t + t + query_name + ".reset();" + nl;
			}
			code += t + t + "return {};" + nl;
			code += t + "}" + nl;
		}

		if(return_types[0].type=="int")
		{
			code += t + "int ret;" + nl;
		}
		else if(return_types[0].type=="int64" || return_types[0].type=="int64_t")
		{
			code += t + "int64_t ret;" + nl;
		}
		else
		{
			code += t + "std::string ret;" + nl;
		}
		code += t + "cursor.get("+getReturnCol(return_types[0].name,
			return_cols) +", ret);" + nl;
	}
	if (!params.empty())
	{
		code += t + query_name + ".reset();" + nl;
	}
	/*if (return_optional)
	{
		code += t + "if(cursor->has_error())" + nl;
		code += t + t + "return {};" + nl;
	}*/
	if(need_return)
		code += t + "return ret;" + nl;
	code+="}";
	return AnnotatedCode(input.annotations, code);
}

void setup1(Database& db, std::vector<AnnotatedCode>& annotated_code, GenConfig& config, const std::string& cppfile)
{
	for(size_t i=0;i<annotated_code.size();++i)
	{
		AnnotatedCode& curr=annotated_code[i];
		if(!curr.annotations.empty())
		{
			if(curr.annotations.find("-SQLGenTempSetup")!=curr.annotations.end())
			{
				std::map<std::string, std::string>::iterator sql_it=curr.annotations.find("sql");
				if(sql_it!=curr.annotations.end())
				{
					db.write(sql_it->second);
				}
			}

			if (curr.annotations.find("-SQLGenConfig") != curr.annotations.end())
			{
				auto it = curr.annotations.find("tab");
				if (it != curr.annotations.end() && next(it->second, 0, "spaces:"))
				{
					int nspaces = atoi(it->second.substr(7).c_str());
					config.tab = "";
					for (int i = 0; i < nspaces; ++i)
						config.tab += " ";
				}
				else if (it == curr.annotations.end())
				{
					if (cppfile.find("\t") == std::string::npos)
						config.tab = "    ";
				}

				it = curr.annotations.find("newline");
				if (it != curr.annotations.end() && it->second == "unix")
					config.newline = "\n";
				else if (it == curr.annotations.end())
				{
					if (cppfile.find("\r\n") == std::string::npos)
						config.newline = "\n";
				}

				it = curr.annotations.find("query_type");
				if (it != curr.annotations.end())
					config.query_type = it->second;

				it = curr.annotations.find("cursor_type");
				if (it != curr.annotations.end())
					config.cursor_type = it->second;
			}
		}
	}
}

void generateCode1(Database& db, const GenConfig& config, std::vector<AnnotatedCode>& annotated_code, GeneratedData& generated_data)
{
	for(size_t i=0;i<annotated_code.size();++i)
	{
		AnnotatedCode& curr=annotated_code[i];
		if(!curr.annotations.empty())
		{
			if(curr.annotations.find("-SQLGenAccess")!=curr.annotations.end())
			{
				annotated_code[i]=generateSqlFunction(db, curr, config, generated_data, true);
			}
			else if(curr.annotations.find("-SQLGenAccessNoCheck")!=curr.annotations.end())
			{
				annotated_code[i]=generateSqlFunction(db, curr, config, generated_data, false);
			}
		}
	}
}

std::string replaceFunctionContent(std::string new_content, const std::string& data)
{
	int c=0;
	std::string ret;
	for(size_t i=0;i<data.size();++i)
	{
		if(data[i]=='{')
			++c;
		else if(data[i]=='}')
			--c;

		if(c>0)
		{
			if(!new_content.empty())
				ret+="{";
			ret+=new_content;
			new_content.clear();
		}
		else
		{
			ret+=data[i];
		}
	}
	return ret;
}

std::string getCode(const std::vector<AnnotatedCode>& annotated_code)
{
	std::string code;
	for(size_t i=0;i<annotated_code.size();++i)
	{
		const AnnotatedCode& curr=annotated_code[i];
		code+=curr.code;
	}
	return code;
}

std::string setbetween(const std::string& s1, const std::string& s2, const std::string& toset, const std::string& data, const GenConfig& config)
{
	size_t start_pos=data.find(s1);
	if(start_pos==std::string::npos)
		return std::string();

	size_t end_pos=data.find(s2, start_pos);
	if(end_pos==std::string::npos)
		return std::string();

	return data.substr(0, start_pos+s1.size())+config.newline+toset+data.substr(end_pos, data.size()-end_pos);
}

std::string getStructureCode(const GeneratedData& generated_data)
{
	std::string code;
	for(auto iter=generated_data.structures.begin();
		iter!=generated_data.structures.end();
		++iter)
	{
		code+=iter->second.code;
	}
	return code;
}

std::string placeData(const std::string& headerfile, const GenConfig& config, const GeneratedData generated_data)
{
	std::string t_headerfile=setbetween("//@-SQLGenFunctionsBegin", "//@-SQLGenFunctionsEnd", getStructureCode(generated_data)
		+config.newline+config.newline
		+generated_data.funcdecls+config.tab, headerfile, config);

	if(t_headerfile.empty())
	{
		std::cout << "ERROR: Cannot find \"//@-SQLGenFunctionsBegin\" or \"//@-SQLGenFunctionsEnd\" in Header-file" << std::endl;
		return headerfile;
	}

	t_headerfile=setbetween("//@-SQLGenVariablesBegin", "//@-SQLGenVariablesEnd", generated_data.variables+config.tab, t_headerfile, config);

	if(t_headerfile.empty())
	{
		std::cout << "ERROR: Cannot find \"//@-SQLGenVariablesBegin\" or \"//@-SQLGenVariablesEnd\" in Header-file" << std::endl;
		return headerfile;
	}

	return t_headerfile;
}

void sqlgen_main(Database& db, std::string &cppfile, std::string &headerfile)
{
	std::vector<CPPToken> tokens=tokenizeFile(cppfile);
	std::vector<AnnotatedCode> annotated_code=getAnnotatedCode(tokens);
	GeneratedData generated_data;
	GenConfig config;
	setup1(db, annotated_code, config, cppfile);
	generateCode1(db, config, annotated_code, generated_data);
	cppfile=getCode(annotated_code);
	headerfile=placeData(headerfile, config, generated_data);
}

} //namespace sqlgen
