/** \file
 * \brief Implementation of get_stacktrace()
 *
 * This code uses the backward header library.
 *
 * \author Stephan Beyer
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
#ifdef OGDF_USE_ASSERT_EXCEPTIONS_WITH_STACKTRACE
#include <ogdf/lib/backward/backward.hpp>
#endif

namespace ogdf {

void get_stacktrace(std::ostream &stream)
{
#ifdef OGDF_USE_ASSERT_EXCEPTIONS_WITH_STACKTRACE
	stream << "\n";
	backward::StackTrace st;
	st.load_here(24);
	st.skip_n_firsts(3); // skip this function and backward
	backward::Printer p;
	p.color = false;
	p.inliner_context_size = p.trace_context_size = 3;
	p.print(st, stream);
#endif
}

}
