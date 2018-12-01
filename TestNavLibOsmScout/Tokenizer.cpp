#include "Tokenizer.h"

const std::string Tokenizer::DELIMITERS(" \t\n\r");

Tokenizer::Tokenizer(const std::string& str) :
	m_offset(0),
	m_string(str),
	m_delimiters(DELIMITERS) {
	//no Code
}

Tokenizer::Tokenizer(const std::string& str, const std::string& delimiters):
	m_offset(0),
	m_string(str),
	m_delimiters(delimiters)
{

}

bool Tokenizer::NextToken() {
	return NextToken(m_delimiters);
}

bool Tokenizer::NextToken(const std::string& delimiters) {
	if (m_offset >= m_string.length()) return false;

	auto i = m_string.find_first_of(delimiters, m_offset);
	if (std::string::npos == i)
	{
		m_token = m_string.substr(m_offset);
		m_offset = m_string.length();
		return true;
	}
	
	m_token = m_string.substr(m_offset, i - m_offset);
	m_offset = i+1;
	return true;
}

std::string Tokenizer::GetToken() const {
	return m_token;

}

void Tokenizer::Reset() {
}
