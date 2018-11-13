/*!\file
 * \author Matthias Elf
 *
 * \par License:
 * This file is part of ABACUS - A Branch And CUt System
 * Copyright (C) 1995 - 2003
 * University of Cologne, Germany
 *
 * \par
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * \par
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * \see http://www.gnu.org/copyleft/gpl.html
 */

#pragma once

#include <ogdf/basic/Logger.h>
#include <ogdf/basic/Array.h>
#include <ogdf/basic/ArrayBuffer.h>

#include <iomanip>
#include <sstream>


// TODO: just a temporary fix
#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif


#define ABACUS_VERSION 301
#define ABACUS_VERSION_STRING "3.0.1/OGDF"

namespace abacus {

using ogdf::AlgorithmFailureException;
using ogdf::ArrayBuffer;
using ogdf::Array;
using ogdf::Logger;

using std::ostream;
using std::ostringstream;
using std::setw;
using std::string;
using std::to_string;
using std::ws;

//! Base class of all other classes of ABACUS.
/**
 * This class is the base class of all other classes of ABACUS.
 * By embedding an enumeration and some useful functions in this class
 * we can avoid a global scope of these names.
 */
class OGDF_EXPORT AbacusRoot {
public:
	//! The destructor
	/**
	 * Is only implemented since it should be virtual in derived classes.
	 */
	virtual ~AbacusRoot() { }


	//! Converts a boolean variable to the strings \a "on" and \a "off".
	/**
	 * \param value The boolean variable being converted.
	 * \return \a "on" if \a value is \a true
	 * \return \a "off" otherwise
	 */
	static const char *onOff(bool value);

	//! Returns the absolute value of the fractional part of \a x.
	/**
	 * E.g., it holds \a fracPart(2.33) == 0.33 and \a fracPart(-1.77) == 0.77.
	 *
	 * \param x The value of which the fractional part is computed.
	 */
	static double fracPart(double x) {
		return (x >= 0.0) ? x-floor(x) : ceil(x)-x;
	}

	//! Converts the string \a str to a boolean value.
	/**
	 * This is only possible for the strings <tt>"true"</tt> and <tt>"false"</tt>.
	 */
	static bool ascii2bool(const string &str);

	//! Returns true if \a str ends with \a end, false otherwise.
	static bool endsWith(const string &str, const string &end);
};

}
