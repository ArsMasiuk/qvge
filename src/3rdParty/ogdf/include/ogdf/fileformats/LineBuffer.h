/** \file
 * \brief Declaration of the clssses LineBuffer and
 * LineBufferPosition
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

#include <ogdf/basic/basic.h>
#include <ogdf/basic/ArrayBuffer.h>


namespace ogdf {

/** This class characterizes uniquely a position in the line
 *  buffer.
 *
 * Note that the element m_lineUpdateCount allows to check
 * if a position is obsolete, i.e. its content has already
 * been overwritten.
 */
class OGDF_EXPORT LineBufferPosition {

private:

	/** Contains the lineNumber */
	int m_lineNumber;

	/** Contains the number of times line m_lineNumber has been
	 * overwritten by new data; Range [0 .. ]
	 */
	int m_lineUpdateCount;

	/** Contains the position in line m_lineNumber */
	int m_linePosition;

public:

	/** Default Constructor */
	LineBufferPosition() :
		m_lineNumber(0),
		m_lineUpdateCount(0),
		m_linePosition(0)
	{ }

	/** Constructor */
	LineBufferPosition(
		int lineNumber,
		int lineUpdateCount,
		int linePosition);

	/** Copy Constructor */
	LineBufferPosition(const LineBufferPosition &position);

	/** Get the line number */
	inline int getLineNumber() const {
		return m_lineNumber;
	}

	/** Get the update count of the line */
	inline int getLineUpdateCount() const {
		return m_lineUpdateCount;
	}

	/** Get the position in the line */
	inline int getLinePosition() const {
		return m_linePosition;
	}

	/** Set all values */
	void set(int lineNumber, int lineUpdateCount, int linePosition);

	/** Increments the position by 1 */
	void incrementPosition();

	/** Test if inequal */
	bool operator!=(const LineBufferPosition &position) const;

	/** Assignment */
	LineBufferPosition &operator=(const LineBufferPosition &position);
};

/** This class maintains the input file and provides a
 *  convenient interface to handle it.
 */
class OGDF_EXPORT LineBuffer {

private:

	//! Handle to the input file
	std::istream *m_pIs;

	//! Contains for each line of the line buffer its update count
	ArrayBuffer<int> m_lineUpdateCountArray;

	//! Pointer to the line buffer
	ArrayBuffer<string> m_linBuf;

	//! The current position in m_linBuf
	LineBufferPosition m_currentPosition;

	/**
	 * The line which has been read from the file most recently;
	 * this does not have to be equal to m_currentPosition.m_lineNumber
	 * because of the lookahead facilities.
	 */
	int m_numberOfMostRecentlyReadLine;

	//! Contains the current line number of the input file;
	int m_inputFileLineCounter;

public:

	//! Construction
	explicit LineBuffer(std::istream &is);

	//! Destruction
	~LineBuffer();

	//! Returns the current position (as a copy)
	LineBufferPosition getCurrentPosition() const{
		return m_currentPosition;
	}

	//! Returns the character which is currently pointed to
	inline char getCurrentCharacter() const {
		if (m_currentPosition.getLineNumber() >= m_linBuf.size()
		 || m_currentPosition.getLinePosition() > (int)m_linBuf[m_currentPosition.getLineNumber()].size()) {
			return EOF;
		}
		return m_linBuf[m_currentPosition.getLineNumber()][m_currentPosition.getLinePosition()];
	}

	//! Returns line number of the most recently read line of the input file
	inline int getInputFileLineCounter() const {
		return m_inputFileLineCounter;
	}

	/**
	 * Moves to the next position;
	 * reading of new lines and handling of eof are done internally.
	 * If end of file is reached the position will stuck to EOF character.
	 * The current character after moving is returned
	 */
	char moveToNextCharacter();

	/**
	 * Sets the current position to new positon.
	 * Takes care if the given newPosition is valid.
	 * @param newPosition new position
	 * @return false if given position is invalid
	 */
	bool setCurrentPosition(const LineBufferPosition &newPosition);

	/**
	 * Moves to the next character until the currentCharacter is no whitespace.
	 */
	void skipWhitespace();

	/**
	 * Copies the characters which have been extracted from the
	 * line buffer starting from position \p startPosition (including it)
	 * to \p endPosition (excluding it) to targetString.
	 * @param startPostion start position
	 * @param endPosition end position
	 * @param targetString string [\p startPostion .. \p endPosition] of the LineBuffer
	 * @return false if the startPosition is not valid, i.e. the string is too long; targetString will contain the message "String too long!"
	 */
	bool extractString(
		const LineBufferPosition &startPostion,
		const LineBufferPosition &endPosition,
		string &targetString);

private:

	//! Checks wether the given \p position is valid
	bool isValidPosition(const LineBufferPosition &position) const;
};

}
