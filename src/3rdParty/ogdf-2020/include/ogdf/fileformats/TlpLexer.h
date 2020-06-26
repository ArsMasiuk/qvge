/** \file
 * \brief Declares a TLP file format lexer class and related structures.
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

#pragma once

#include <iostream>
#include <string>
#include <vector>


namespace ogdf {

namespace tlp {


struct Token {

	enum class Type {
		leftParen,
		rightParen,
		identifier,
		string
	} type;

	std::string *value; // Optional token value (avaliable in id and string).
	size_t line, column; // Where given token occured for printing nice info.

	Token(const Type &type, size_t line, size_t column);
	friend std::ostream &operator <<(std::istream &os, const Token &token);

	bool inline leftParen() const {
		return type == Type::leftParen;
	}

	bool inline rightParen() const {
		return type == Type::rightParen;
	}

	bool inline identifier() const {
		return type == Type::identifier;
	}

	bool inline identifier(const char *str) const {
		return type == Type::identifier && *value == str;
	}

	bool inline string() const {
		return type == Type::string;
	}

	bool inline string(const char *str) const {
		return type == Type::string && *value == str;
	}
};

std::ostream &operator <<(std::ostream &os, const Token &token);


class Lexer {
private:
	std::istream &m_istream;
	std::string m_buffer;
	std::string::const_iterator m_begin, m_end;
	size_t m_line;

	std::vector<Token> m_tokens;

	bool fetchBuffer();
	void cleanValues();

	bool tokenizeLine();
	bool tokenizeString();
	bool tokenizeIdentifier();

	size_t line() const {
		return m_line;
	}

	size_t column() const {
		return std::distance(m_buffer.begin(), m_begin) + 1;
	}

	static bool isIdentifier(char c);

public:
	explicit Lexer(std::istream &is);
	~Lexer();

	bool tokenize();
	const std::vector<Token> &tokens() const {
		return m_tokens;
	}
};

}
}
