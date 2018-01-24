/** \file
 * \brief Includes declaration of dual graph class.
 *
 * \author Michael Schulz
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

#include <ogdf/basic/CombinatorialEmbedding.h>
#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/EdgeArray.h>
#include <ogdf/basic/FaceArray.h>

namespace ogdf {

/**
 * A dual graph including its combinatorial embedding of an embedded graph.
 *
 * Dual edges are rotated counter-clockwise compared to the primal ones.
 *
 * @ingroup graphs
 */
class OGDF_EXPORT DualGraph : public CombinatorialEmbedding
{
public:
	//! Constructor; creates dual graph and its combinatorial embedding
	explicit DualGraph(const ConstCombinatorialEmbedding &CE);
	//! Destructor
	~DualGraph();
	//! Returns a reference to the combinatorial embedding of the primal graph
	const ConstCombinatorialEmbedding &getPrimalEmbedding() const { return m_primalEmbedding; }
	//! Returns a reference to the primal graph
	const Graph &getPrimalGraph() const { return m_primalEmbedding.getGraph(); }

	//! Returns the node in the primal graph corresponding to \p f.
	/**
	* @param f is a face in the embedding of the dual graph
	* \return the corresponding node in the primal graph
	*/
	const node &primalNode(face f) const { return m_primalNode[f]; }

	//! Returns the edge in the primal graph corresponding to \p e.
	/**
	* @param e is an edge in the dual graph
	* \return the corresponding edge in the primal graph
	*/
	const edge &primalEdge(edge e) const { return m_primalEdge[e]; }

	//! Returns the face in the embedding of the primal graph corresponding to \p v.
	/**
	* @param v is a node in the dual graph
	* \return the corresponding face in the embedding of the primal graph
	*/
	const face &primalFace(node v) const { return m_primalFace[v]; }

	//! Returns the node in the dual graph corresponding to \p f.
	/**
	* @param f is a face in the embedding of the primal graph
	* \return the corresponding node in the dual graph
	*/
	const node &dualNode(face f) const { return m_dualNode[f]; }

	//! Returns the edge in the dual graph corresponding to \p e.
	/**
	* @param e is an edge in the primal graph
	* \return the corresponding edge in the dual graph
	*/
	const edge &dualEdge(edge e) const { return m_dualEdge[e]; }

	//! Returns the face in the embedding of the dual graph corresponding to \p v.
	/**
	* @param v is a node in the primal graph
	* \return the corresponding face in the embedding of the dual graph
	*/
	const face &dualFace(node v) const { return m_dualFace[v]; }

protected:
	const ConstCombinatorialEmbedding &m_primalEmbedding; //!< The embedding of the primal graph.
	FaceArray<node> m_primalNode; //!< The corresponding node in the primal graph.
	NodeArray<face> m_primalFace; //!< The corresponding facee in the embedding of the primal graph.
	EdgeArray<edge> m_primalEdge; //!< The corresponding edge in the primal graph.
	FaceArray<node> m_dualNode; //!< The corresponding node in the dual graph.
	NodeArray<face> m_dualFace; //!< The corresponding face in embedding of the dual graph.
	EdgeArray<edge> m_dualEdge; //!< The corresponding edge in the dual graph.
};

}
