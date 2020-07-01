/** \file
 * \brief Declaration of class GEMLayout.
 *
 * Fast force-directed layout algorithm (GEMLayout)
 * based on Frick et al.'s algorithm.
 *
 * \author Christoph Buchheim
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

#include <ogdf/basic/LayoutModule.h>
#include <ogdf/basic/Math.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/GraphCopy.h>
#include <random>

namespace ogdf {

//! The energy-based GEM layout algorithm.
/**
 * @ingroup gd-energy
 *
 * The implementation used in GEMLayout is based on the following publication:
 *
 * Arne Frick, Andreas Ludwig, Heiko Mehldau: <i>A Fast Adaptive %Layout
 * Algorithm for Undirected Graphs</i>. Proc. %Graph Drawing 1994,
 * LNCS 894, pp. 388-403, 1995.
 *
 * <H3>Optional parameters</H3>
 * GEM layout provides the following optional parameters.
 *
 * <table>
 *   <tr>
 *     <th><i>Option</i><th><i>Type</i><th><i>Default</i><th><i>Description</i>
 *   </tr><tr>
 *     <td><i>numberOfRounds</i><td>int<td>20000
 *     <td>The maximal number of rounds per node.
 *   </tr><tr>
 *     <td><i>minimalTemperature</i><td>double<td>0.005
 *     <td>The minimal temperature.
 *   </tr><tr>
 *     <td><i>initialTemperature</i><td>double<td>10.0
 *     <td>The initial temperature.
 *   </tr><tr>
 *     <td><i>gravitationalConstant</i><td>double<td>1/16
 *     <td>The gravitational constant.
 *   </tr><tr>
 *     <td><i>desiredLength</i><td>double<td>5.0
 *     <td>The desired edge length.
 *   </tr><tr>
 *     <td><i>maximalDisturbance</i><td>double<td>0
 *     <td>The maximal disturbance.
 *   </tr><tr>
 *     <td><i>rotationAngle</i><td>double<td>pi/3.0
 *     <td>The opening angle for rotations.
 *   </tr><tr>
 *     <td><i>oscillationAngle</i><td>double<td>pi/2.0
 *     <td>The opening angle for oscillations.
 *   </tr><tr>
 *     <td><i>rotationSensitivity</i><td>double<td>0.01
 *     <td>The rotation sensitivity.
 *   </tr><tr>
 *     <td><i>oscillationSensitivity</i><td>double<td>0.3
 *     <td>The oscillation sensitivity.
 *   </tr><tr>
 *     <td><i>attractionFormula</i><td>int<td>1
 *     <td>The used formula for attraction (1 = Fruchterman / Reingold, 2 = GEM).
 *   </tr><tr>
 *     <td><i>minDistCC</i><td>double<td>20
 *     <td>The minimal distance between connected components.
 *   </tr><tr>
 *     <td><i>pageRatio</i><td>double<td>1.0
 *     <td>The page ratio used for the layout of connected components.
 *   </tr>
 * </table>
*/
class OGDF_EXPORT GEMLayout : public LayoutModule
{

	// algorithm parameters (see below)

	int m_numberOfRounds;		    //!< The maximal number of rounds per node.
	double m_minimalTemperature;    //!< The minimal temperature.
	double m_initialTemperature;    //!< The initial temperature.
	double m_gravitationalConstant; //!< The gravitational constant.
	double m_desiredLength;         //!< The desired edge length.
	double m_maximalDisturbance;    //!< The maximal disturbance.
	double m_rotationAngle;         //!< The opening angle for rotations.
	double m_oscillationAngle;      //!< The opening angle for oscillations.
	double m_rotationSensitivity;   //!< The rotation sensitivity.
	double m_oscillationSensitivity;//!< The oscillation sensitivity.
	int m_attractionFormula;        //!< The used formula for attraction.
	double m_minDistCC;             //!< The minimal distance between connected components.
	double m_pageRatio;             //!< The page ratio used for the layout of connected components.

	// node data used by the algorithm

	NodeArray<double> m_impulseX; //!< x-coordinate of the last impulse of the node
	NodeArray<double> m_impulseY; //!< y-coordinate of the last impulse of the node
	NodeArray<double> m_localTemperature; //!< local temperature of the node
	NodeArray<double> m_skewGauge; //!< skew gauge of the node

	// other data used by the algorithm

	double m_barycenterX; //!< Weighted sum of x-coordinates of all nodes.
	double m_barycenterY; //!< Weighted sum of y-coordinates of all nodes.
	double m_newImpulseX; //!< x-coordinate of the new impulse of the current node.
	double m_newImpulseY; //!< y-coordinate of the new impulse of the current node.
	double m_globalTemperature; //!< Average of all node temperatures.
	double m_cos; //!< Cosine of m_oscillationAngle / 2.
	double m_sin; //!< Sine of (pi + m_rotationAngle) / 2.

	std::minstd_rand m_rng;

public:

	//! Creates an instance of GEM layout.
	GEMLayout();

	//! Copy constructor.
	GEMLayout(const GEMLayout &fl);

	// destructor
	~GEMLayout();

	//! Assignment operator.
	GEMLayout &operator=(const GEMLayout &fl);

	//! Calls the layout algorithm for graph attributes \p GA.
	virtual void call(GraphAttributes &GA) override;

	//! Returns the maximal number of rounds per node.
	int numberOfRounds() const { return m_numberOfRounds; }

	//! Sets the maximal number of round per node to \p n.
	void numberOfRounds(int n) {
		m_numberOfRounds = (n < 0) ? 0 : n;
	}

	//! Returns the minimal temperature.
	double minimalTemperature() const { return m_minimalTemperature; }

	//! Sets the minimal temperature to \p x.
	void minimalTemperature(double x) {
		m_minimalTemperature = (x < 0) ? 0 : x;
	}

	//! Returns the initial temperature.
	double initialTemperature() const { return m_initialTemperature; }

	//! Sets the initial temperature to \p x; must be >= minimalTemperature.
	void initialTemperature(double x) {
		m_initialTemperature = (x < m_minimalTemperature) ? m_minimalTemperature : x;
	}

	//! Returns the gravitational constant.
	double gravitationalConstant() const { return m_gravitationalConstant; }

	//! Sets the gravitational constant to \p x; must be >= 0.
	//! Attention! Only (very) small values give acceptable results.
	void gravitationalConstant(double x) {
		m_gravitationalConstant = (x < 0) ? 0 : x;
	}

	//! Returns the desired edge length.
	double desiredLength() const { return m_desiredLength; }

	//! Sets the desired edge length to \p x; must be >= 0.
	void desiredLength(double x) {
		m_desiredLength = (x < 0) ? 0 : x;
	}

	//! Returns the maximal disturbance.
	double maximalDisturbance() const { return m_maximalDisturbance; }

	//! Sets the maximal disturbance to \p x; must be >= 0.
	void maximalDisturbance(double x) {
		m_maximalDisturbance = (x < 0) ? 0 : x;
	}

	//! Returns the opening angle for rotations.
	double rotationAngle() const { return m_rotationAngle; }

	//! Sets the opening angle for rotations to \p x (0 <= \p x <= pi / 2).
	void rotationAngle(double x) {
		if(x < 0) x = 0;
		if(x > Math::pi / 2.0) x = Math::pi / 2.0;
		m_rotationAngle = x;
	}

	//! Returns the opening angle for oscillations.
	double oscillationAngle() const { return m_oscillationAngle; }

	//! Sets the opening angle for oscillations to \p x (0 <= \p x <= pi / 2).
	void oscillationAngle(double x) {
		if(x < 0) x = 0;
		if(x > Math::pi / 2.0) x = Math::pi / 2.0;
		m_oscillationAngle = x;
	}

	//! Returns the rotation sensitivity.
	double rotationSensitivity() const { return m_rotationSensitivity; }

	//! Sets the rotation sensitivity to \p x (0 <= \p x <= 1).
	void rotationSensitivity(double x) {
		if(x < 0) x = 0;
		if(x > 1) x = 1;
		m_rotationSensitivity = x;
	}

	//! Returns the oscillation sensitivity.
	double oscillationSensitivity() const { return m_oscillationSensitivity; }

	//! Sets the oscillation sensitivity to \p x (0 <= \p x <= 1).
	void oscillationSensitivity(double x) {
		if(x < 0) x = 0;
		if(x > 1) x = 1;
		m_oscillationSensitivity = x;
	}

	//! Returns the used formula for attraction (1 = Fruchterman / Reingold, 2 = GEM).
	int attractionFormula() const { return m_attractionFormula; }

	//! sets the formula for attraction to \p n (1 = Fruchterman / Reingold, 2 = GEM).
	void attractionFormula(int n) {
		if(n == 1 || n == 2) m_attractionFormula = n;
	}

	//! Returns the minimal distance between connected components.
	double minDistCC() const { return m_minDistCC; }

	//! Sets the minimal distance between connected components to \p x.
	void minDistCC(double x) { m_minDistCC = x; }

	//! Returns the page ratio used for the layout of connected components.
	double pageRatio() const { return m_pageRatio; }

	//! Sets the page ratio used for the layout of connected components to \p x.
	void pageRatio(double x) { m_pageRatio = x; }


private:
	//! Returns the length of the vector (\p x,\p y).
	double length(double x,double y = 0) const {
		return sqrt(x * x + y * y);
	}

	//! Returns the weight of node \p v according to its degree.
	double weight(node v) const {
		return (double)(v->degree()) / 2.5 + 1.0;
	}

	//! Computes the new impulse for node \p v.
	void computeImpulse(GraphCopy &GC, GraphAttributes &AGC,node v);

	//! Updates the node data for node \p v.
	void updateNode(GraphCopy &GC, GraphAttributes &AGC,node v);

	OGDF_NEW_DELETE
};

}
