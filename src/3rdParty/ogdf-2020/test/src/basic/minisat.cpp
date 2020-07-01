/** \file
 * \brief Tests for Minisat wrapper
 *
 * \author Stephan Beyer (satisfiableTest by Robert Zeranski)
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

#include <ogdf/basic/Graph.h>
#include <ogdf/external/Minisat.h>
#include <resources.h>

static void satisfiableTest()
{
	Minisat::Formula F;
	Minisat::Model model;

	F.addClause(std::vector<int>{-1, -2, -3, 4});

	F.newVars(11);

	for (int i = 1; i < 10; i++) {
		Minisat::clause c = F.newClause();
		if (i % 2 == 0) {
			c->add(i);
		}
		else {
			c->add(-i);
		}
		c->add(i+1);
		F.finalizeClause(c);
	}

	bool satisfiable = F.solve(model);

	AssertThat(satisfiable, IsTrue());
#if 0
	std::cout << "#vars = " << F.getVariableCount() << std::endl;
	std::cout << "#clauses = " << F.getClauseCount() << std::endl;
	if (val) {
		model.printModel();
	}
	F.reset();
#endif
}

static void nonsatisfiableTest()
{
	Minisat::Formula F;
	Minisat::Model model;
	F.addClause(std::vector<int>{1, 2});
	F.addClause(std::vector<int>{1, -2, 3});
	F.addClause(std::vector<int>{-1, 2});
	F.addClause(std::vector<int>{-1, -2});
	F.addClause(std::vector<int>{-3});

	bool satisfiable = F.solve(model);

	AssertThat(satisfiable, IsFalse());
}

static void readDIMACSTest()
{
	Minisat::Formula formula;
	std::stringstream ss{ResourceFile::data("minisat/satisfiable.txt")};
	AssertThat(formula.readDimacs(ss), IsTrue());
	Minisat::Model model;
	bool satisfiable = formula.solve(model);
	AssertThat(satisfiable, IsTrue());

	formula.addClause(std::vector<int>{3});
	satisfiable = formula.solve(model);
	AssertThat(satisfiable, IsFalse());
}

go_bandit([]() {
	describe("Minisat wrapper", []() {
		it("solves a satisfiable formula", []() {
			satisfiableTest();
		});
		it("solves a non-satisfiable formula", []() {
			nonsatisfiableTest();
		});
		it("reads a DIMACS file and is able to solve the formula and change it", []() {
			readDIMACSTest();
		});
	});
});
