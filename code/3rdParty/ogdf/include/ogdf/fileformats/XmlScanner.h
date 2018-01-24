/** \file
 * \brief Contains the enum XmlToken and the class XmlScanner.
 *
 * \author Dino Ahr
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

#include <ogdf/fileformats/LineBuffer.h>

namespace ogdf {

/** This enum type represents the values, which are returned by
 *  the function XmlScanner::getNextToken().
 *  @see XmlScanner::getNextToken()
 */
enum class XmlToken {
	openingBracket,		///< <
	closingBracket,		///< >
	questionMark,		///< ?
	exclamationMark,	///< !
	minus,				///< &minus;
	slash,				///< /
	equalSign,			///< =
	identifier,			///< (a..z|A..Z){(a..z|A..Z|0..9|.|_|:)}
	attributeValue,		///< a sequence of characters, digits, minus - and dot .
	quotedValue,		///< all quoted content " ... " or ' ... '
	endOfFile,			///< End of file detected
	invalidToken,		///< No token identified
	noToken				///< Used for the m_lookAheadToken to indicate that there
						///< is no lookahead token
};

/** This class scans the characters of the input file and
 *  provides the detected token.
 */
class OGDF_EXPORT XmlScanner {

private:

	// Pointer to the line buffer
	LineBuffer *m_pLineBuffer;

	// String which contains the characters of the current token
	string m_currentToken;

public:
	// construction
	explicit XmlScanner(std::istream &is);

	//! Destruction: destroys the parse tree
	~XmlScanner();

	// This function represents the core of the scanner. It scans the input
	// and returns the identified token. After performing getNextToken() the
	// token is "consumed", i.e. the line buffer pointer already points to the
	// next token.
	// The scanned string is deposited in m_currentToken, hence it is
	// available via getCurrentToken()
	XmlToken getNextToken();

	// Returns the current token string (a copy)
	inline const string getCurrentToken()
	{
		return m_currentToken;
	}

	// This function provides a lookahead to the next token;
	// the token is NOT consumed like it is the case for getNextToken()
	XmlToken testNextToken();

	// This function provides a lookahead to the nextnext token;
	// the tokens are NOT consumed like it is the case for getNextToken()
	XmlToken testNextNextToken();

	// Skips until the searchCharacter is found;
	//
	// If skipOverSearchCharacter is set true the currentPosition will be set
	// BEHIND the search character
	// otherwise the pointer currentPosition points TO the searchCharacter
	//
	// Returns true if the searchCharacter is found
	// Returns false if file ends before the searchCharacter is found
	bool skipUntil(char searchCharacter, bool skipOverSearchCharacter = true);

	// Skips until '>' is found (> is consumed)
	// Nested brackets are taken into account
	// Returns true if matching bracket has been found; false otherwise
	bool skipUntilMatchingClosingBracket();

	// Reads until the searchCharacter is found; the string starting at the current
	// position and ending at the position where the search character is found
	// is deposited in m_currentToken.
	// If includeSearchCharacter is false (default) the search character is
	// not contained; otherwise it is contained
	//
	// Returns true if the searchCharacter is found
	// Returns false if file ends before the searchCharacter is found
	bool readStringUntil(char searchCharacter, bool includeSearchCharacter = false);

	// Returns line number of the most recently read line of the input file
	inline int getInputFileLineCounter() const {
		return m_pLineBuffer->getInputFileLineCounter();
	}

	// This function tests the scanner by reading the complete
	// input file and printing the identified token to stdout
	void test();
};

}
