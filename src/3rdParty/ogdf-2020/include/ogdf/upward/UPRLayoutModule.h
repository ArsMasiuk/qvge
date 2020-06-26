/** \file
 * \brief Declaration of interface for layout algorithms for a UpwardPlanRep
 *
 * \author Hoi-Ming Wong
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

#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/upward/UpwardPlanRep.h>


namespace ogdf {


/**
 * \brief Interface of hierarchy layout algorithms.
 *
 * \see SugiyamaLayout
 */
class OGDF_EXPORT UPRLayoutModule {
public:
	//! Initializes a upward planarized representation layout module.
	UPRLayoutModule() { }

	virtual ~UPRLayoutModule() { }

	/**
	 * \brief Computes a upward layout of \p UPR in \p AG.
	 * @param UPR is the upward planarized representation of the input graph. The original graph of UPR muss be the input graph.
	 * @param AG is assigned the hierarchy layout.
	 */
	void call(const UpwardPlanRep &UPR, GraphAttributes &AG) {
		doCall(UPR, AG);
	}

	int numberOfLevels;

protected:
	/**
	 * \brief Implements the actual algorithm call.
	 *
	 * Must be implemented by derived classes.
	 *
	 * @param UPR is the upward planarized representation of the input graph. The original graph of UPR muss be the input graph.
	 * @param AG has to be assigned the hierarchy layout.
	 */
	virtual void doCall(const UpwardPlanRep &UPR, GraphAttributes &AG) = 0;

	OGDF_MALLOC_NEW_DELETE
};

}
