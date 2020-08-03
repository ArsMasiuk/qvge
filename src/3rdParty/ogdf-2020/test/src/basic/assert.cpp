/** \file
 * \brief Tests for the OGDF_ASSERT macro
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
#include <testing.h>

static void assert_positive(int);

go_bandit([] {
	describe("OGDF_ASSERT", [] {
		it("does not fail if the condition holds", [] {
			assert_positive(1);
		});

#ifndef OGDF_DEBUG
		it("does not fail if OGDF_DEBUG is not set", [] {
			assert_positive(-1);
		});
#endif

#ifdef OGDF_USE_ASSERT_EXCEPTIONS
		it("throws an AssertionFailed exception if the condition does not hold", [] {
			AssertThrows(AssertionFailed, assert_positive(-1));
		});

		it("throws an exception with an explanatory what()", [] {
			int fails = false;
			try {
				assert_positive(-1);
			} catch (AssertionFailed &e) {
				fails = true;
				std::string what(e.what());
				AssertThat(what, Contains("a > 0"));
				AssertThat(what, Contains("fail"));
				AssertThat(what, Contains(__FILE__ ":1000"));
				AssertThat(what, Contains("assert_positive"));
			}
			AssertThat(fails, IsTrue());
		});
#endif

#ifdef OGDF_USE_ASSERT_EXCEPTIONS_WITH_STACKTRACE
		it("throws an exception with a stack trace in what()", [] {
			int fails = false;
			try {
				assert_positive(-1);
			} catch (AssertionFailed &e) {
				fails = true;
				AssertThat(std::string(e.what()), Contains("Stack trace"));
			}
			AssertThat(fails, IsTrue());
		});
#endif
	});
});

static void assert_positive(int a)
{
#line 1000
	OGDF_ASSERT(a > 0);
}
