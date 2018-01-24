/** \file
 * \brief Declaration of CrossingMinimization Module, an interface for crossing minimization algorithms
 *
 * \author Markus Chimani
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

#include <ogdf/uml/PlanRepUML.h>
#include <ogdf/basic/Module.h>
#include <ogdf/basic/Timeouter.h>


namespace ogdf {

//! Base class for UML crossing minimization algorithms.
class OGDF_EXPORT UMLCrossingMinimizationModule : public Module, public Timeouter
{
public:
	//! Initializes a UML crossing minimization module (default constructor).
	UMLCrossingMinimizationModule() { }

	//! Initializes a UML crossing minimization module (copy constructor).
	UMLCrossingMinimizationModule(const UMLCrossingMinimizationModule &cmm) : Timeouter(cmm) { }

	//! Destructor.
	virtual ~UMLCrossingMinimizationModule() { }

	//! Returns a new instance of the UML crossing minimization module with the same option settings.
	virtual UMLCrossingMinimizationModule *clone() const = 0;


	//! Computes a planarized representation of the input graph.
	/**
	 * @param prUML          represents the input graph as well as the computed planarized representation
	 *                       after the call. \p prUML has to be initialzed as a PlanRep of the input graph and
	 *                       is modified to obatain the planarized representation (crossings are replaced
	 *                       by dummy vertices with degree four).
	 * @param cc             is the index of the connected component in \p prUML that is considered.
	 * @param crossingNumber is assigned the number of crossings.
	 * @param pCostOrig      points to an edge array (of the original graph) that gives the cost of each edge.
	 *                       May be a 0-pointer, in which case all edges have cost 1.
	 * @return the status of the result.
	 */
	ReturnType call(PlanRepUML &prUML,
		int cc,
		int&  crossingNumber,
		const EdgeArray<int>  *pCostOrig = nullptr)
	{
		return doCall(prUML, cc, pCostOrig, crossingNumber);
	}

	//! Computes a planarized representation of the input graph.
	/**
	 * @param prUML          represents the input graph as well as the computed planarized representation
	 *                       after the call. \p prUML has to be initialzed as a PlanRep of the input graph and
	 *                       is modified to obatain the planarized representation (crossings are replaced
	 *                       by dummy vertices with degree four).
	 * @param cc             is the index of the connected component in \p prUML that is considered.
	 * @param crossingNumber is assigned the number of crossings.
	 * @param pCostOrig      points to an edge array (of the original graph) that gives the cost of each edge.
	 *                       May be a 0-pointer, in which case all edges have cost 1.
	 * @return the status of the result.
	 */
	ReturnType operator()(PlanRepUML &prUML,
		int cc,
		int & crossingNumber,
		const EdgeArray<int>  *pCostOrig = nullptr)
	{
		return doCall(prUML, cc, pCostOrig, crossingNumber);
	}

	//! Checks if the planarized represenation contains crossing generalizations.
	static bool checkCrossingGens(const PlanRepUML &prUML);

protected:
	//! Actual algorithm call that needs to be implemented by derived classes.
	/**
	 * @param prUML          represents the input graph as well as the computed planarized representation
	 *                       after the call. \p prUML has to be initialzed as a PlanRep of the input graph and
	 *                       is modified to obatain the planarized representation (crossings are replaced
	 *                       by dummy vertices with degree four).
	 * @param cc             is the index of the connected component in \p prUML that is considered.
	 * @param crossingNumber is assigned the number of crossings.
	 * @param pCostOrig      points to an edge array (of the original graph) that gives the cost of each edge.
	 *                       May be a 0-pointer, in which case all edges have cost 1.
	 * @return the status of the result.
	 */
	virtual ReturnType doCall(PlanRepUML &prUML,
		int cc,
		const EdgeArray<int>  *pCostOrig,
		int &crossingNumber) = 0;

	OGDF_MALLOC_NEW_DELETE
};

}
