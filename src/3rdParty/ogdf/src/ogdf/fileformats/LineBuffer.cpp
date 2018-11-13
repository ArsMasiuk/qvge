/** \file
 * \brief Implementation of a line buffer serving the class XmlScanner
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

#include <ogdf/fileformats/LineBuffer.h>

namespace ogdf {

LineBufferPosition::LineBufferPosition(
	int lineNumber,
	int lineUpdateCount,
	int linePosition)
{
	set(lineNumber, lineUpdateCount, linePosition);
}

LineBufferPosition::LineBufferPosition(const LineBufferPosition &position)
{
	m_lineNumber = position.m_lineNumber;
	m_lineUpdateCount = position.m_lineUpdateCount;
	m_linePosition = position.m_linePosition;
}

void LineBufferPosition::set(int lineNumber, int lineUpdateCount, int linePosition)
{
	OGDF_ASSERT(lineNumber >= 0);
	OGDF_ASSERT(lineUpdateCount >= 0);
	OGDF_ASSERT(linePosition >= 0);

	m_lineNumber = lineNumber;
	m_lineUpdateCount = lineUpdateCount;
	m_linePosition = linePosition;

}

void LineBufferPosition::incrementPosition()
{
	++m_linePosition;

	OGDF_ASSERT(m_linePosition >= 0);

}

bool LineBufferPosition::operator!=(const LineBufferPosition &position) const
{
	return m_lineNumber != position.m_lineNumber
	    || m_lineUpdateCount != position.m_lineUpdateCount
	    || m_linePosition != position.m_linePosition;

}

LineBufferPosition &
LineBufferPosition::operator=(const LineBufferPosition &position)
{
	if (&position != this){

		m_lineNumber = position.getLineNumber();
		m_lineUpdateCount = position.getLineUpdateCount();
		m_linePosition = position.getLinePosition();

	}

	return *this;

}

LineBuffer::LineBuffer(std::istream &is) :
	m_pIs(&is),
	m_numberOfMostRecentlyReadLine(0),
	m_inputFileLineCounter(0)
{
	if (!(*m_pIs)) {
		Logger::slout() << "LineBuffer::LineBuffer: Error opening file!\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, AlgorithmFailureCode::Unknown);
	}

	// Read first line
	if (m_pIs->good()){

		m_lineUpdateCountArray.push(0);
		m_linBuf.push("");

		// Read first line
		std::getline(*m_pIs, m_linBuf[0]);

		// Increase inputFileLineCounter
		++m_inputFileLineCounter;

		// Increase updateCount
		++(m_lineUpdateCountArray[0]);
	}

	// Set position
	m_currentPosition.set(0, m_lineUpdateCountArray[0], 0);

}

LineBuffer::~LineBuffer()
{
}

char LineBuffer::moveToNextCharacter(){
	// Return if end of file is reached
	if (getCurrentCharacter() == EOF){
		return EOF;
	}

	// Increment position
	m_currentPosition.incrementPosition();

	// End of line is reached, there can be some consecutive lines
	// with only \0 in it; hence we use a while loop
	while (getCurrentCharacter() == '\0'){

		// Current line is equal to most recently read line,
		// i.e. we have to read a new line from the file
		if (m_currentPosition.getLineNumber() == m_numberOfMostRecentlyReadLine){

			++m_numberOfMostRecentlyReadLine;
			m_lineUpdateCountArray.push(0);

			// Increment update count
			++(m_lineUpdateCountArray[m_numberOfMostRecentlyReadLine]);

			// Increment inputFileLineCounter
			++m_inputFileLineCounter;

			// Set current position
			m_currentPosition.set(
				m_numberOfMostRecentlyReadLine,
				m_lineUpdateCountArray[m_numberOfMostRecentlyReadLine],
				0);

			// End of file is reached
			if (!m_pIs->good()){

				// Set eof marker
				return EOF;

			}
			// Read next line and put it to the new position
			else{
				m_linBuf.push("");
				std::getline(*m_pIs, m_linBuf[m_numberOfMostRecentlyReadLine]);
			}
		}

		// Current line is NOT equal to most recently read line, i.e.
		// it is not necessary to read a new line from the file but to
		// set the currentPosition to the next line which is already in
		// the line buffer.
		else{
			// Set current position
			const int newLine = m_currentPosition.getLineNumber() + 1;
			m_currentPosition.set(newLine, m_lineUpdateCountArray[newLine], 0);
		}
	}

	return getCurrentCharacter();
}

bool LineBuffer::setCurrentPosition(const LineBufferPosition &newPosition){
	// Given positon is not valid
	if (!isValidPosition(newPosition))
	{
		return false;
	}

	m_currentPosition = newPosition;

	return true;
}

void LineBuffer::skipWhitespace()
{
	if (getCurrentCharacter() == EOF) {
		return;
	}

	while (isspace(getCurrentCharacter()) && getCurrentCharacter() != EOF)
	{
		moveToNextCharacter();
	}
}

bool LineBuffer::extractString(
	const LineBufferPosition &startPosition,
	const LineBufferPosition &endPosition,
	string &targetString)
{
	targetString.clear();

	// StartPosition invalid, probably because the line of the startPosition
	// has already been overwritten, i.e. the string is too long
	if (!isValidPosition(startPosition))
	{
		return false;
	}

	// EndPosition must be valid
	OGDF_ASSERT(isValidPosition(endPosition));

	// Remember original currentPosition
	LineBufferPosition originalCurrentPosition = getCurrentPosition();

	// Begin at startPosition
	setCurrentPosition(startPosition);

	// Copy characters to tempString
	while (getCurrentPosition() != endPosition)
	{

		// Check if eof
		OGDF_ASSERT(getCurrentCharacter() != EOF);

		// Put character into targetString
		targetString.push_back(getCurrentCharacter());

		// Move to next character
		moveToNextCharacter();
	}

	// Set back the original current position
	setCurrentPosition(originalCurrentPosition);

	return true;
}

bool LineBuffer::isValidPosition(const LineBufferPosition &position) const
{
	// We can assume that the position is valid according to
	// array ranges since these things are checked in constructor and set of
	// class LineBufferPosition

	return position.getLineUpdateCount() == m_lineUpdateCountArray[position.getLineNumber()];
}

}
