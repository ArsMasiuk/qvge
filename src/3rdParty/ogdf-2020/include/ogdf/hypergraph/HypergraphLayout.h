/** \file
 * \brief Layout algorithms for hypergraph based on edge standard
 *        representations (clique / star / tree) - HypergraphLayoutES.
 *
 * ... edge standard is based partly on Section 7.2 of PhD Thesis
 * by Dr. Chimani, subset standard is based on the following paper:
 *
 * Bertault, François and Eades, Peter.:Drawing Hypergraphs in the
 * Subset Standard (Short Demo Paper) Graph Drawing Springer Berlin /
 * Heidelberg 2001. pp.45-76. ISBN 978-3-540-41554-1
 *
 * \author Ondrej Moris
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

#include <ogdf/hypergraph/Hypergraph.h>
#include <ogdf/hypergraph/EdgeStandardRep.h>
#include <ogdf/hypergraph/HypergraphAttributes.h>
#include <ogdf/hypergraph/HypergraphLayoutModule.h>

#include <ogdf/basic/exceptions.h>
#include <memory>
#include <ogdf/planarity/EmbedderModule.h>
#include <ogdf/planarity/CrossingMinimizationModule.h>
#include <ogdf/planarity/LayoutPlanRepModule.h>

#include <ogdf/planarity/PlanRep.h>

namespace ogdf {

class OGDF_EXPORT HypergraphLayoutES : public HypergraphLayoutModule {

public:

	//! Final appearance is driven by given profile.
	enum class Profile {
		Normal          = 0x000001,
		ElectricCircuit = 0x000002
	};

private:

	//! The ration between width and height of a drawing.
	double m_ratio;

	//! The number of crossings in the layout.
	int m_crossings;

	//! Defines whether a drawing IO constraint is desired or not.
	bool m_constraintIO;

	//! Defines whether inputs and outputs are placed on different "sides".
	// TODO: This might require some tweaks in Hypergraph class.
	bool m_constraintPorts;

	//! Defines the profile of the layout (eg. Electric Circuit).
	Profile m_profile;

	//! The module for computing the final layout.
	std::unique_ptr<LayoutPlanRepModule>  m_planarLayoutModule;

	//! The module for crossing minimization.
	std::unique_ptr<CrossingMinimizationModule> m_crossingMinimizationModule;

	//! The module for embedding planarization.
	std::unique_ptr<EmbedderModule>  m_embeddingModule;

public:

	// constructor
	HypergraphLayoutES();

	// destructor
	virtual ~HypergraphLayoutES() { }

	// Dynamic casting is currently not working as desired and hence we left
	// the following call inherited from superclass empty.
	virtual void call(HypergraphAttributes &HA) override;

#if 0
	void call(HypergraphAttributesES &HA);
#endif

	//! Assignment operator.
	HypergraphLayoutES &operator=(const HypergraphLayoutES &hl);

	//! Returns the number of crossings in computed layout.
	int crossings() const
	{
		return m_crossings;
	}

	//! Returns the ratio  between width and height of a drawing.
	double ratio() const
	{
		return m_ratio;
	}

	//! Sets the Input / Output drawing requirement.
	void setConstraintIO(bool pConstraintIO)
	{
		m_constraintIO = pConstraintIO;
	}

	//! Sets the layout profile.
	void setProfile(Profile pProfile)
	{
		m_profile = pProfile;
	}

	/** @}
	 *  @name Modules
	 *  @{
	 */

	/**
	 * \brief Sets the module option for the planar layout.
	 *
	 * Crossing minimization produces a planar representation of a hypergraph
	 * such that all crossings are replaced by additional dummy nodes.
	 * This is in fact a planar graph and hence it can be drawn quite
	 * easily by any planar layout algorithm.
	 */
	void setPlanarLayoutModule
		(LayoutPlanRepModule *pPlanarLayoutModule)
	{
		m_planarLayoutModule.reset(pPlanarLayoutModule);
	}


	/**
	 * \brief Sets the module option for crossing minimization.
	 *
	 * The crossing minimization module minimizes the crossings of a hypergraph
	 * in an edge standard  representation.
	 */
	void setCrossingMinimizationModule
		(CrossingMinimizationModule *pCrossingMinimizationModule)
	{
		m_crossingMinimizationModule.reset(pCrossingMinimizationModule);
	}

	/**
	 * \brief Sets the module option for embedding.
	 *
	 * When a planarized edge representation of a hypergraph in computed,
	 * we have to found a way how to embed it (ie. find faces).
	 */
	void setEmbeddingModule
		(EmbedderModule *pEmbeddingModule)
	{
		m_embeddingModule.reset(pEmbeddingModule);
	}

private:

	void layout(HypergraphAttributesES &pHA);

#if 0
	void planarizeCC(PlanRep &ccPlanarRep, List<edge> &fixedShell);
#endif

	void packAllCC(const PlanRep &planarRep,
	               const GraphCopySimple &gc,
	               HypergraphAttributesES &pHA,
	               Array<DPoint> &bounding);

	void insertShell(GraphCopySimple &planarRep, List<node> &src, List<node> &tgt, List<edge> &fixedShell);

	void removeShell(PlanRep &planarRep, NodePair &st);

	void applyProfile(HypergraphAttributesES &HA);
};

}
