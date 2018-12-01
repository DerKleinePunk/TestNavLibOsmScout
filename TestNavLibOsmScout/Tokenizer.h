#pragma once
#include <string>
//https://stackoverflow.com/questions/53849/how-do-i-tokenize-a-string-in-c

class Tokenizer {
public:
	static const std::string DELIMITERS;
	explicit Tokenizer(const std::string& str);
	Tokenizer(const std::string& str, const std::string& delimiters);
	bool NextToken();
	bool NextToken(const std::string& delimiters);
	std::string GetToken() const;
	void Reset();
protected:
	size_t m_offset;
	const std::string m_string;
	std::string m_token;
	std::string m_delimiters;
};
