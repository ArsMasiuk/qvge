/** \file
 * \brief Implementation of TLP file format lexer class.
 *
 * \author Łukasz Hanuszczak
 *
 * \par License:
 * This file is part of the Open Graph Drawing Framework (OGDF).
 *
 * \par
 * Copyright (C)<br>
 * See README.md in the OGDF root directory for details.
 *
 * \par
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 or 3 as published by the Free Software Foundation;
 * see the file LICENSE.txt included in the packaging of this file
 * for details.
 *
 * \par
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, see
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <ogdf/basic/basic.h>
#include <ogdf/fileformats/TlpLexer.h>
#include <ogdf/fileformats/GraphIO.h>


namespace ogdf {

namespace tlp {


std::ostream &operator <<(std::ostream &os, const Token &token)
{
	switch(token.type) {
	case Token::Type::leftParen:
		os << "tok_(";
		break;
	case Token::Type::rightParen:
		os << "tok_)";
		break;
	case Token::Type::identifier:
		os << "tok_id(" << *(token.value) << ")";
		break;
	case Token::Type::string:
		os << "tok_str(\"" << *(token.value) << "\")";
		break;
	}

	return os;
}


Token::Token(const Type &tokenType, size_t tokenLine, size_t tokenColumn)
: type(tokenType), line(tokenLine), column(tokenColumn)
{
	if(type == Type::identifier || type == Type::string) {
		value = new std::string;
	} else {
		value = nullptr;
	}
}


bool Lexer::isIdentifier(char c)
{
	return isalnum(c) || c == '_' || c == '.' || c == '-';
}


bool Lexer::fetchBuffer()
{
	if(!std::getline(m_istream, m_buffer)) {
		return false;
	}

	m_begin = m_buffer.begin();
	m_end = m_buffer.end();

	m_line++;
	return true;
}


void Lexer::cleanValues()
{
	for(const Token &t : m_tokens)
	{
		delete t.value;
	}
}


Lexer::Lexer(std::istream &is) : m_istream(is)
{
}


Lexer::~Lexer()
{
	cleanValues();
}


bool Lexer::tokenize()
{
	cleanValues();
	m_tokens.clear();

	m_line = 0;
	while(fetchBuffer()) {
		if(!tokenizeLine()) {
			return false;
		}
	}

	return true;
}


bool Lexer::tokenizeLine()
{
	while(m_begin != m_end && isspace(*m_begin)) {
		++m_begin;
	}

	// We got an end of a line or a comment.
	if(m_begin == m_end || *m_begin == ';') {
		return true;
	}

	if(*m_begin == '(') {
		m_tokens.push_back(Token(Token::Type::leftParen, line(), column()));
		++m_begin;
		return tokenizeLine();
	}

	if(*m_begin == ')') {
		m_tokens.push_back(Token(Token::Type::rightParen, line(), column()));
		++m_begin;
		return tokenizeLine();
	}

	if(*m_begin == '"') {
		return tokenizeString() && tokenizeLine();
	}

	if(isIdentifier(*m_begin)) {
		return tokenizeIdentifier() && tokenizeLine();
	}

	GraphIO::logger.lout() << "Unexpected character \"" << *m_begin << "\" at (" << line() << ", " << column() << ")." << std::endl;
	return false;
}


bool Lexer::tokenizeString()
{
	++m_begin;

	Token token(Token::Type::string, line(), column());

	for(;;) {
		// Check whether we need to refill the buffer.
		if(m_begin == m_end && !fetchBuffer()) {
			GraphIO::logger.lout() << "End of input while parsing a string at (" << token.line << ", " << token.column << ")." << std::endl;
			return false;
		}

		if(m_begin == m_end) {
			continue;
		}

		// Check whether we got a end of a string.
		if(*m_begin == '"') {
			m_tokens.push_back(token);
			++m_begin;
			return true;
		}

		// If the to above failed, we just put new character.
		*(token.value) += *m_begin;
		++m_begin;
	}

	return true;
}


bool Lexer::tokenizeIdentifier()
{
	Token token(Token::Type::identifier, line(), column());

	while(m_begin != m_end && isIdentifier(*m_begin)) {
		*(token.value) += *m_begin;
		++m_begin;
	}

	m_tokens.push_back(token);
	return true;
}

}
}
