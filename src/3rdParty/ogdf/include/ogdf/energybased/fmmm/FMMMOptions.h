/** \file
 * \brief Declaration of Fast Multipole Multilevel Method (FM^3) options.
 *
 * \author Stefan Hachul, Stephan Beyer
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

class FMMMOptions {
public:
	//! Possible page formats.
	enum class PageFormatType {
		Portrait,  //!< A4 portrait page.
		Landscape, //!< A4 landscape page.
		Square     //!< Square format.
	};

	//! Trade-off between run-time and quality.
	enum class QualityVsSpeed {
		GorgeousAndEfficient,  //!< Best quality.
		BeautifulAndFast,      //!< Medium quality and speed.
		NiceAndIncredibleSpeed //!< Best speed.
	};

	//! Specifies how the length of an edge is measured.
	enum class EdgeLengthMeasurement {
		Midpoint,      //!< Measure from center point of edge end points.
		BoundingCircle //!< Measure from border of circle s surrounding edge end points.
	};

	//! Specifies which positions for a node are allowed.
	enum class AllowedPositions {
		//! Every position is allowed
		All,

		//! Only integer positions are allowed that are in a range
		//! depending on the number of nodes and the average ideal edge length.
		Integer,

		//! Only integer positions in a range of -2^MaxIntPosExponent to
		//! 2^MaxIntPosExponent are alllowed
		Exponent
	};

	//! Specifies in which case it is allowed to tip over drawings of connected components.
	enum class TipOver {
		None,         //!< not allowed at all
		NoGrowingRow, //!< only if the height of the packing row does not grow
		Always        //!< always allowed
	};

	//! Specifies how connected components are sorted before the packing algorithm is applied.
	enum class PreSort {
		None,             //!< Do not presort.
		DecreasingHeight, //!< Presort by decreasing height of components.
		DecreasingWidth   //!< Presort by decreasing width of components.
	};

	//! Specifies how sun nodes of galaxies are selected.
	enum class GalaxyChoice {
		UniformProb,             //!< selecting by uniform random probability
		NonUniformProbLowerMass, //!< selecting by non-uniform probability depending on the star masses (prefering nodes with lower star mass)
		NonUniformProbHigherMass //!< as above but prefering nodes with higher star mass
	};

	//! Specifies how MaxIterations is changed in subsequent multilevels.
	enum class MaxIterChange {
		Constant,           //!< kept constant at the force calculation step at every level
		LinearlyDecreasing, //!< linearly decreasing from MaxIterFactor*FixedIterations to FixedIterations
		RapidlyDecreasing   //!< rapdily decreasing from MaxIterFactor*FixedIterations to FixedIterations
	};

	//! Specifies how the initial placement is generated.
	enum class InitialPlacementMult {
		Simple,  //!< only using information about placement of nodes on higher levels
		Advanced //!< using additional information about the placement of all inter solar system nodes
	};

	//! Specifies the force model.
	enum class ForceModel {
		FruchtermanReingold, //!< The force-model by Fruchterman, Reingold.
		Eades,               //!< The force-model by Eades.
		New                  //!< The new force-model.
	};

	//! Specifies how to calculate repulsive forces.
	enum class RepulsiveForcesMethod {
		Exact,             //!< Exact calculation (slow).
		GridApproximation, //!< Grid approximation (inaccurate).
		NMM                //!< Calculation as for new multipole method (fast and accurate).
	};

	//! Specifies the stop criterion.
	enum class StopCriterion {
		FixedIterations,           //!< Stop if fixedIterations() is reached.
		Threshold,                 //!< Stop if threshold() is reached.
		FixedIterationsOrThreshold //!< Stop if fixedIterations() or threshold() is reached.
	};

	//! Specifies how the initial placement is done.
	enum class InitialPlacementForces {
		UniformGrid,      //!< Uniform placement on a grid.
		RandomTime,       //!< Random placement (based on current time).
		RandomRandIterNr, //!< Random placement (based on randIterNr()).
		KeepPositions     //!< No change in placement.
	};

	//! Specifies how the reduced bucket quadtree is constructed.
	enum class ReducedTreeConstruction {
		PathByPath,      //!< Path-by-path construction.
		SubtreeBySubtree //!< Subtree-by-subtree construction.
	};

	//! Specifies how to calculate the smallest quadratic cell that surrounds
	//! the particles of a node in the reduced bucket quadtree.
	enum class SmallestCellFinding {
		Iteratively, //!< Iteratively (in constant time).
		Aluru        //!< According to formula by Aluru et al. (in constant time).
	};
};

}
