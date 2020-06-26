/** \file
 * \brief Definition of RemoveReinsertType (used for postprocessing
 *        in edge insertion algorithms).
 *
 * \author Karsten Klein, Carsten Gutwenger
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

namespace ogdf {

//! The postprocessing method for edge insertion algorithms.
/**
 * @ingroup ga-insert
 */
enum class RemoveReinsertType {
	None,        //!< No postprocessing.
	Inserted,    //!< Postprocessing only with the edges that have to be inserted.
	MostCrossed, //!< Postprocessing with the edges involved in the most crossings.
	All,         //!< Postproceesing with all edges.
	Incremental, //!< Full postprocessing after each edge insertion.
	IncInserted  //!< Postprocessing for (so far) inserted edges after each edge insertion.
};

}
