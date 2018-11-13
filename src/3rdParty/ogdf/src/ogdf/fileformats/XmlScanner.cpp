/** \file
 * \brief Implementation of the class XmlScanner serving the
 *        class XmlParser
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

#include <ogdf/fileformats/XmlScanner.h>

namespace ogdf {

XmlScanner::XmlScanner(std::istream &is) {
	// Create line buffer
	m_pLineBuffer = new LineBuffer(is);
}


XmlScanner::~XmlScanner() {
	// Destroy line buffer
	delete m_pLineBuffer;
}

// Take a look at the state machine of getNextToken() to understand
// what is going on here.
//
// TODO: It seems to be useful that this function throws an exception
//       if something goes wrong.
XmlToken XmlScanner::getNextToken(){

	// First skip whitespaces
	m_pLineBuffer->skipWhitespace();

	// Let's have a look at the current character
	char currentCharacter = m_pLineBuffer->getCurrentCharacter();

	// End of file reached
	if (currentCharacter == EOF){
		return XmlToken::endOfFile;
	}

	// First we handle single characters with a switch statement
	switch (currentCharacter){

	// Opening Bracket
	case '<':
		{
			m_pLineBuffer->moveToNextCharacter();
			return XmlToken::openingBracket;
		}
		break;

	// Closing Bracket
	case '>':
		{
			m_pLineBuffer->moveToNextCharacter();
			return XmlToken::closingBracket;
		}
		break;

	// Question Mark
	case '?':
		{
			m_pLineBuffer->moveToNextCharacter();
			return XmlToken::questionMark;
		}
		break;

	// Exclamation Mark
	case '!':
		{
			m_pLineBuffer->moveToNextCharacter();
			return XmlToken::exclamationMark;
		}
		break;

	// Minus
	case '-':
		{
			m_pLineBuffer->moveToNextCharacter();
			return XmlToken::minus;
		}
		break;

	// Slash
	case '/':
		{
			m_pLineBuffer->moveToNextCharacter();
			return XmlToken::slash;
		}
		break;

	// Equal Sign
	case '=':
		{
			m_pLineBuffer->moveToNextCharacter();
			return XmlToken::equalSign;
		}
		break;
	}

	// Now we handle more complex token

	// Identifier
	if (isalpha(currentCharacter)){
		// Put a pointer to the beginning of the identifier
		LineBufferPosition startPosition = m_pLineBuffer->getCurrentPosition();

		currentCharacter = m_pLineBuffer->moveToNextCharacter();

		// Read valid identifier characters
		while ((isalnum(currentCharacter)) ||  // a..z|A..Z|0..9
			(currentCharacter == '.') ||
			(currentCharacter == ':') ||
			(currentCharacter == '_'))
		{
			currentCharacter = m_pLineBuffer->moveToNextCharacter();
		}

		// Copy identifier to currentTokenString
		m_pLineBuffer->extractString(startPosition, m_pLineBuffer->getCurrentPosition(), m_currentToken);

		// Return identifier token
		return XmlToken::identifier;
	}

	// Quoted characters " ... " or ' ... '
	if ((currentCharacter == '\"') ||
		(currentCharacter == '\''))
	{
		// Distinguish what kind of quote sign we have
		bool doubleQuote;
		if (currentCharacter == '\"')
			doubleQuote = true;
		else
			doubleQuote = false;

		// Skip quote sign
		currentCharacter = m_pLineBuffer->moveToNextCharacter();

		// Read until the closing quotation sign is found
		// String is copied to m_currentToken by readStringUntil()
		if (doubleQuote){
			readStringUntil('\"', false);
		}
		else{
			readStringUntil('\'', false);
		}

		// Skip over the end quote character
		m_pLineBuffer->moveToNextCharacter();

		// Return token for quoted value
		return XmlToken::quotedValue;
	}

	// An atributeValue, i.e. a sequence of characters, digits, minus - or dot .
	if ((isalnum(currentCharacter)) ||
		(currentCharacter == '-') ||
		(currentCharacter == '.'))
	{
		// Put a pointer to the beginning of the quoted text
		LineBufferPosition startPosition = m_pLineBuffer->getCurrentPosition();

		// Read until until an invalid character occurs
		currentCharacter = m_pLineBuffer->moveToNextCharacter();
		while ((isalnum(currentCharacter)) ||
			(currentCharacter == '-') ||
			(currentCharacter == '.'))
		{
			currentCharacter = m_pLineBuffer->moveToNextCharacter();
		}

		// Copy attributeValue to currentTokenString
		m_pLineBuffer->extractString(startPosition, m_pLineBuffer->getCurrentPosition(), m_currentToken);

		// Return token for attribute value
		return XmlToken::attributeValue;
	}

	// No valid token
	m_pLineBuffer->moveToNextCharacter();
	return XmlToken::invalidToken;
}

XmlToken XmlScanner::testNextToken(){
	// Save pointer to the current position
	LineBufferPosition originalPosition = m_pLineBuffer->getCurrentPosition();

	// Call getNextToken()
	XmlToken returnToken = getNextToken();

	// Set pointer back to the original position
	m_pLineBuffer->setCurrentPosition(originalPosition);

	// Return token
	return returnToken;
}

XmlToken XmlScanner::testNextNextToken(){

	// Save pointer to the current position
	LineBufferPosition originalPosition = m_pLineBuffer->getCurrentPosition();

	// Call getNextToken()
	getNextToken();

	// Again Call getNextToken()
	XmlToken returnToken = getNextToken();

	// Set pointer back to the original position
	m_pLineBuffer->setCurrentPosition(originalPosition);

	// Return token
	return returnToken;
}

bool XmlScanner::skipUntil(char searchCharacter, bool skipOverSearchCharacter){
	while (m_pLineBuffer->getCurrentCharacter() != EOF){
		// Search character has been found!
		if (m_pLineBuffer->getCurrentCharacter() == searchCharacter){

			// Move to the position behind the search character if desired
			if (skipOverSearchCharacter){
				m_pLineBuffer->moveToNextCharacter();
			}

			return true;
		}

		// Move to next character and proceed
		m_pLineBuffer->moveToNextCharacter();
	}

	return false;
}

bool XmlScanner::skipUntilMatchingClosingBracket(){
	// We assume that the opening bracket has already been read
	int bracketParity = 1;

	while ((m_pLineBuffer->getCurrentCharacter() != EOF) &&
		(bracketParity != 0))
	{
		// Opening bracket has been found!
		if (m_pLineBuffer->getCurrentCharacter() == '<'){

			++bracketParity;
		}

		// Closing bracket has been found!
		if (m_pLineBuffer->getCurrentCharacter() == '>'){

			--bracketParity;
		}

		// Move to next character and proceed
		m_pLineBuffer->moveToNextCharacter();
	}

	if (bracketParity != 0 )
		return false;
	else
		return true;
}

bool XmlScanner::readStringUntil(char searchCharacter,
                                 bool includeSearchCharacter) {
	// Remember start position
	LineBufferPosition startPosition = m_pLineBuffer->getCurrentPosition();

	// Use skipUntil()
	if (skipUntil(searchCharacter, includeSearchCharacter)){

		// Copy found string to m_currentToken
		m_pLineBuffer->extractString(startPosition, m_pLineBuffer->getCurrentPosition(), m_currentToken);

		return true;

	}
	// An error occurred
	else{
		return false;
	}
}

void XmlScanner::test(){
	bool terminate = false;
	XmlToken currentToken;

	while (!terminate){

		std::cout << "Line " << getInputFileLineCounter() << ": ";
		currentToken = getNextToken();

		switch (currentToken){
		case XmlToken::openingBracket:
			std::cout << "<" << std::endl;
			break;
		case XmlToken::closingBracket:
			std::cout << ">" << std::endl;
			break;
		case XmlToken::questionMark:
			std::cout << "?" << std::endl;
			break;
		case XmlToken::exclamationMark:
			std::cout << "!" << std::endl;
			break;
		case XmlToken::minus:
			std::cout << "-" << std::endl;
			break;
		case XmlToken::slash:
			std::cout << "/" << std::endl;
			break;
		case XmlToken::equalSign:
			std::cout << "<" << std::endl;
			break;
		case XmlToken::identifier:
			std::cout << "Identifier: " << m_currentToken << std::endl;
			break;
		case XmlToken::attributeValue:
			std::cout << "Attribute value: " << m_currentToken << std::endl;
			break;
		case XmlToken::quotedValue:
			std::cout << "Quoted value: \"" << m_currentToken << "\"" << std::endl;
			break;
		case XmlToken::endOfFile:
			std::cout << "EOF" << std::endl;
			terminate = true;
			break;
		default:
			std::cout << "Invalid token!" << std::endl;
		}
	}
}

}
