/** \file
 * \brief Implements DOT format Lexer class.
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
#include <ogdf/fileformats/DotLexer.h>
#include <ogdf/fileformats/GraphIO.h>

namespace ogdf {

namespace dot {


Token::Token(
	size_t tokenRow, size_t tokenColumn,
	std::string *identifierContent)
: row(tokenRow), column(tokenColumn), value(identifierContent)
{
}


std::string Token::toString(const Type &type)
{
	switch(type) {
		case Type::assignment:       return "=";
		case Type::colon:            return ":";
		case Type::semicolon:        return ";";
		case Type::comma:            return ",";
		case Type::edgeOpDirected:   return "->";
		case Type::edgeOpUndirected: return "--";
		case Type::leftBracket:      return "[";
		case Type::rightBracket:     return "]";
		case Type::leftBrace:        return "{";
		case Type::rightBrace:       return "}";
		case Type::graph:            return "graph";
		case Type::digraph:          return "digraph";
		case Type::subgraph:         return "subgraph";
		case Type::node:             return "node";
		case Type::edge:             return "edge";
		case Type::strict:           return "strict";
		case Type::identifier:       return "identifier";
	}
	OGDF_ASSERT(false);
	return "UNKNOWN";
}


Lexer::Lexer(std::istream &input) : m_input(input)
{
}


Lexer::~Lexer()
{
	for(const Token &t : m_tokens)
	{
		delete t.value;
	}
}


const std::vector<Token> &Lexer::tokens() const
{
	return m_tokens;
}


bool Lexer::tokenize()
{
	m_row = 0;
	while(m_input.good()) {
		if(!tokenizeLine()) {
			return false;
		}
	}

	return true;
}


bool Lexer::tokenizeLine()
{
	std::getline(m_input, m_buffer);
	m_row++;

	// Handle line output from a C preprocessor (#blabla).
	if(m_buffer[0] == '#') {
		return true;
	}

	for(m_col = 0; m_col < m_buffer.size(); m_col++) {
		// Ignore whitespaces.
		if(isspace(m_buffer[m_col])) {
			continue;
		}

		// Handle single-line comments.
		if(match("//")) {
			break;
		}

		// Handle multi-line comments.
		if(match("/*")) {
			const size_t column = m_col;
			const size_t row = m_row;

			do {
				m_col++;

				// Get a new line if a current one has ended.
				if(m_col >= m_buffer.size()) {
					if(!m_input.good()) {
						GraphIO::logger.lout() << "Unclosed comment at" << column << ", " << row << std::endl;
						return false;
					}
					std::getline(m_input, m_buffer);
					m_row++;
					m_col = 0;
				}
			} while(!(m_buffer[m_col - 1] == '*' && m_buffer[m_col] == '/'));

			m_col += 2;
			continue;
		}

		Token token(m_row + 1, m_col + 1);

		if(match(Token::Type::assignment)) {
			token.type = Token::Type::assignment;
		} else if(match(Token::Type::colon)) {
			token.type = Token::Type::colon;
		} else if(match(Token::Type::semicolon)) {
			token.type = Token::Type::semicolon;
		} else if(match(Token::Type::comma)) {
			token.type = Token::Type::comma;
		} else if(match(Token::Type::edgeOpDirected)) {
			token.type = Token::Type::edgeOpDirected;
		} else if(match(Token::Type::edgeOpUndirected)) {
			token.type = Token::Type::edgeOpUndirected;
		} else if(match(Token::Type::leftBracket)) {
			token.type = Token::Type::leftBracket;
		} else if(match(Token::Type::rightBracket)) {
			token.type = Token::Type::rightBracket;
		} else if(match(Token::Type::leftBrace)) {
			token.type = Token::Type::leftBrace;
		} else if(match(Token::Type::rightBrace)) {
			token.type = Token::Type::rightBrace;
		} else if(match(Token::Type::graph, true)) {
			token.type = Token::Type::graph;
		} else if(match(Token::Type::digraph, true)) {
			token.type = Token::Type::digraph;
		} else if(match(Token::Type::subgraph, true)) {
			token.type = Token::Type::subgraph;
		} else if(match(Token::Type::node, true)) {
			token.type = Token::Type::node;
		} else if(match(Token::Type::edge, true)) {
			token.type = Token::Type::edge;
		} else if(match(Token::Type::strict, true)) {
			token.type = Token::Type::strict;
		} else if(identifier(token)) {
			token.type = Token::Type::identifier;
		} else {
			GraphIO::logger.lout() << "Unknown token at: " << m_row << "; " << m_col << std::endl;
			return false;
		}

		m_tokens.push_back(token);
	}

	return true;
}


bool Lexer::match(const Token::Type &type, bool word)
{
	return match(Token::toString(type), word);
}


bool Lexer::match(const std::string &str, bool word)
{
	// Check whether buffer is too short to match.
	if(m_buffer.length() < m_col + str.length()) {
		return false;
	}

	for(size_t i = 0; i < str.length(); i++) {
		if(m_buffer[m_col + i] != str[i]) {
			return false;
		}
	}

	// we've matched a part of a word instead of a whole word
	if (word && m_buffer.length() >= m_col + str.length() + 1 &&
		isDotAlnum(m_buffer[m_col + str.length()])) {
		return false;
	}

	// After successful match we move the "head".
	m_col += str.length() - 1;

	return true;
}


bool Lexer::identifier(Token &token)
{
	// Check whether identifier is double-quoted string.
	if(m_buffer[m_col] == '"') {
		m_col++;
		std::stringstream ss;

		while(m_buffer[m_col] != '"' || m_buffer[m_col - 1] == '\\') {
			ss << m_buffer[m_col++];

			// Get a new line if a current one has ended.
			if(m_col >= m_buffer.size()) {
				if(!m_input.good()) {
					GraphIO::logger.lout() << "Unclosed string at " << token.row << ", " << token.column << std::endl;
					return false;
				}
				std::getline(m_input, m_buffer);
				m_row++;
				m_col = 0;
			}
		}

		token.value = new std::string(ss.str());
		return true;
	}

	// Check whether identifier is a normal C-like identifier.
	// according to DOT standard, an ID may not begin with a digit
	if (isDotAlnum(m_buffer[m_col]) && !isdigit(m_buffer[m_col])) {
		std::ostringstream ss;

		while (isDotAlnum(m_buffer[m_col])) {
			ss << m_buffer[m_col++];
		}

		m_col--;
		token.value = new std::string(ss.str());
		return true;
	}

	// Check whether identifier is a numeric literal. Quite ugly and slow but works.
	std::istringstream ss(m_buffer.c_str() + m_col);
	double temp;
	if (ss >> temp) {
		std::istringstream::pos_type length = ss.tellg();
		if (length < 0) { // end of line
			token.value = new std::string(ss.str());
			m_col = m_buffer.size();
		} else {
			token.value = new std::string(m_buffer.substr(m_col, length));
			m_col += length;
		}
		return true;
	}

	// TODO: HTML string identifiers.

	return false;
}

bool Lexer::isDotAlnum(signed char c) {
	return isalnum(c) || c < 0 || c == '_';
}

}

}
