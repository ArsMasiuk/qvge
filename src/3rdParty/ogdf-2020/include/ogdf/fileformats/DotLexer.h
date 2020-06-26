/** \file
 * \brief Declares stuff related to DOT format lexical analysis.
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
#include <sstream>
#include <vector>


namespace ogdf {

namespace dot {


//! Just a simple token struct representing a DOT file fragment.
/**
 * Each token is represented by type: its either \a special string (a keyword
 * or punctuation character) or identifier (which is \a normal string).
 *
 * Additionaly, each token have its row and column fields indicating where
 * it appeared. This information could be useful for displaying useful debug
 * messages.
 *
 * \sa dot::Lexer
 */
struct Token {

	enum class Type {
		// Operators
		assignment, colon, semicolon, comma, edgeOpDirected, edgeOpUndirected,
		// Brackets
		leftBracket, rightBracket,
		leftBrace, rightBrace,
		// Keywords
		graph, digraph, subgraph, node, edge,
		strict,
		// Values
		identifier
	};

	//! The type of an field.
	Type type;
	//! Indicates a token row (line).
	size_t row;
	//! Indicated a token column.
	size_t column;
	//! Identifier content (nullptr for non-id tokens).
	std::string *value;

	Token(size_t tokenRow, size_t tokenColumn, std::string *identifierContent = nullptr);

	//! Returns string representation of given token type.
	static std::string toString(const Type &type);
};


//! Lexical analysis tool.
/**
 * This class reads the given input and generates a token list. Token list
 * representation of DOT file is much easier for further processing (like
 * parsing) as it automatically gets rid of comments and deals with various
 * identifier representations in DOT format (C-like identifier, double-quoted
 * strings, number literals).
 *
 * \sa dot::Parser
 */
class Lexer {
private:
	std::istream &m_input;

	std::string m_buffer; // Current line of given file.
	size_t m_row, m_col; // Current position in parsed file.

	std::vector<Token> m_tokens;

	bool tokenizeLine();

	//! Checks if \a head matches given token. Advances \a head on success.
	/**
	 * @param type A type of token being matched.
	 * @param word True if token is part of a word, false otherwise.
	 * @return True if matches, false otherwise.
	 */
	bool match(const Token::Type &type, bool word = false);

	//! Checks if \a head matches given string. Advances \a head on success.
	/**
	 * @param str A string being matched.
	 * @param word True if token is part of a word, false otherwise.
	 * @return True if matches, false otherwise.
	 */
	bool match(const std::string &str, bool word = false);

	//! Checks whether \a head is an identifier.
	/**
	 * @param token Function fills it with identifier value and col/row info.
	 * @return True if matches, false otherwise.
	 */
	bool identifier(Token &token);

	//! Checks if character is allowed in an identifier by DOT standard
	/**
	 * @param c A character
	 * @return True if c is one of alphabetic ([a-zA-Z\200-\377]) characters, underscores ('_') or digits ([0-9])
	 */
	bool isDotAlnum(signed char c);

public:
	//! Initializes lexer with given input (but does nothing to it).
	explicit Lexer(std::istream &input);
	~Lexer();

	//! Scans input and turns it into token list.
	/**
	 * @return True if success, false otherwise.
	 */
	bool tokenize();
	//! Returns list of tokens (first use Lexer#tokenize())
	const std::vector<Token> &tokens() const;
};

}
}
