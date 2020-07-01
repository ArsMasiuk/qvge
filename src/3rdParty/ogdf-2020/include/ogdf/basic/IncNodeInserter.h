/** \file
 * \brief Declaration of class IncNodeInserter.
 *
 * This class represents the base class for strategies
 * for the incremental drawing approach to insert nodes
 * (having no layout fixation) into the fixed part of
 * a PlanRep.
 *
 * \author Karsten Klein
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

#include <ogdf/planarity/PlanRepInc.h>
#include <ogdf/uml/UMLGraph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/GraphObserver.h>

namespace ogdf {

class OGDF_EXPORT IncNodeInserter
{
public:
	//! Creates inserter on \p PG
	explicit IncNodeInserter(PlanRepInc &PG) : m_planRep(&PG ) { }

	//! Inserts copy in #m_planRep for original node \p v
	virtual void insertCopyNode(node v, CombinatorialEmbedding &E,
		Graph::NodeType vTyp) = 0;

protected:
	//! Returns a face to insert a copy of \p v and a list of
	//! adjacency entries corresponding to the insertion adjEntries
	//! for the adjacent edges
	virtual face getInsertionFace(node v, CombinatorialEmbedding &E) = 0;

	//! pointer to a PlanRepInc that is to be changed
	PlanRepInc* m_planRep;
};

}
