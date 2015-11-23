#include "ezpwd/rs"
#include "ezpwd/corrector"
#include "ezpwd/definitions"
#include <string>
#include <iostream>

bool success = false;
std::string encode(const std::string &input)
{
	ezpwd::RS<255, 223> rs;
	int size = input.size();
	std::string modifiedInput = input;
	modifiedInput+="||||";
	modifiedInput+=std::to_string(size);
	rs.encode(modifiedInput);
	return modifiedInput;
}
std::string decode(const std::string &input)
{
	ezpwd::RS<255, 223> rs;
	//std::string data = input.substr(0, input.find("||||"));
	//std::string size_str = input.substr(input.find("||||") + 4);
	//int size = std::stoi(size_str);
	std::string in(input);
	int fixed = rs.decode(in);
	std::string data = in.substr(0, in.find("||||"));
	std::string remain = in.substr(in.find("||||") + 4);
	std::string size_str = remain.substr(0, remain.find("||||"));
	int size = std::stoi(size_str);
	if(fixed >= 0)
	{
		success = true;
	}
	else
	{
		success = false;
	}
	data.resize(size);
	return data;
}

bool decodeSuccessful()
{
	if(success)
	{
		success = false;
		return true;
	}
	else
	{
		return false;
	}
}
int main(int argc, char* argv[])
{
	std::string in = "The quick brown fox jumps over the lazy dog";
	std::string out = encode(in);
	out[7]='x';
	std::cout<<out<<std::endl;
	std::string result = decode(out);
	std::cout<<result<<std::endl;
}