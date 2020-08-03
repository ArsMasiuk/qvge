/** \file
 * \brief Declares ClusterGraphAttributes, an extension of class
 * GraphAttributes,  to store clustergraph layout informations
 * like cluster cage positions and sizes that can be accessed
 * over the cluster/cluster ID
 *
 * \author Karsten Klein, Carsten Gutwenger, Max Ilsen
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
#include <ogdf/cluster/ClusterArray.h>


namespace ogdf {

//! Stores additional attributes of a clustered graph (like layout information).
/**
 * @ingroup graph-containers graph-drawing
 */
class OGDF_EXPORT ClusterGraphAttributes : public GraphAttributes
{
protected:
	const ClusterGraph* m_pClusterGraph;//!< Only points to existing graphs.

	ClusterArray<double> m_x; //!< X-position of lower left corner
	ClusterArray<double> m_y; //!< Y-position of lower left corner
	ClusterArray<double> m_width; //!< Cluster width
	ClusterArray<double> m_height; //!< Cluster height
	ClusterArray<string> m_label; //!< Cluster label
	ClusterArray<Stroke> m_stroke; //!< Stroke (style of boundary)
	ClusterArray<Fill> m_fill; //!< Fill (style of interior)
	ClusterArray<string> m_clusterTemplate; //!< Name of cluster template

public:
	/**
	 * @name Flags for enabling attributes
	 * @{
	 */

	//! Corresponds to cluster attributes #x(cluster), #y(cluster),
	//! #width(cluster), #height(cluster).
	static const long clusterGraphics;

	//! Corresponds to cluster attributes #strokeColor(cluster),
	//! #strokeType(cluster), #strokeWidth(cluster), #fillPattern(cluster),
	//! #fillColor(cluster), and #fillBgColor(cluster).
	static const long clusterStyle;

	//! Corresponds to cluster attribute #label(cluster).
	static const long clusterLabel;

	//! Corresponds to cluster attribute #templateCluster(cluster).
	static const long clusterTemplate;

	//! Enables all available flags.
	static const long all;

	//!@}

	// Don't hide these inherited methods by overloading.
	using GraphAttributes::x;
	using GraphAttributes::y;
	using GraphAttributes::width;
	using GraphAttributes::height;

	using GraphAttributes::label;

	using GraphAttributes::strokeType;
	using GraphAttributes::strokeColor;
	using GraphAttributes::strokeWidth;

	using GraphAttributes::fillPattern;
	using GraphAttributes::fillColor;
	using GraphAttributes::fillBgColor;

	/**
	 * @name Construction and management of attributes
	 * @{
	 */

	//! Constructs cluster graph attributes for no associated graph.
	ClusterGraphAttributes() : GraphAttributes(), m_pClusterGraph(nullptr) { }

	//! Constructs cluster graph attributes for cluster graph \p cg with attributes \p initAttributes.
	explicit ClusterGraphAttributes(const ClusterGraph& cg,
			long initAttributes = nodeGraphics | edgeGraphics | clusterGraphics
	);

	virtual ~ClusterGraphAttributes() { }

private:
	//! Forbidden initialization, use init(ClusterGraph &cg, long initAttributes) instead!
	using GraphAttributes::init;

	//! Add all cluster-related attributes in \p attr.
	/**
	 * @pre #m_attributes already contains \p attr.
	 */
	void addClusterAttributes(long attr);

	//! Destroy all cluster-related attributes in \p attr.
	/**
	 * @pre #m_attributes already contains \p attr.
	 */
	void destroyClusterAttributes(long attr);

public:
	//! Initializes the ClusterGraphAttributes for ClusterGraph \p cg.
	/**
	 * @param cg is the new associated ClusterGraph.
	 * @param attr specifies the set of attributes that can be accessed.
	 *
	 * \warning All attributes that were allocated before are destroyed by this function!
	 *  If you wish to extend the set of allocated attributes, use #addAttributes.
	 */
	void init(ClusterGraph &cg, long attr = 0);

	//! Re-initializes the ClusterGraphAttributes while maintaining the associated CluterGraph.
	//! @see init(const ClusterGraph&, long)
	void init(long attr = 0);

	//! @copydoc GraphAttributes::addAttributes()
	void addAttributes(long attr);

	//! @copydoc GraphAttributes::destroyAttributes()
	void destroyAttributes(long attr);

	//! Returns the associated cluster graph.
	const ClusterGraph& constClusterGraph() const { return *m_pClusterGraph; }


	/**
	 * @}
	 * @name Cluster attributes
	 * @{
	 */

	//! Returns the x-position of cluster \p c's cage (lower left corner).
	/**
	 * \pre #clusterGraphics is enabled
	 */
	double x(cluster c) const {
		OGDF_ASSERT(has(clusterGraphics));
		return m_x[c];
	}

	//! Returns the x-position of cluster \p c's cage (lower left corner).
	/**
	 * \pre #clusterGraphics is enabled
	 */
	double& x(cluster c) {
		OGDF_ASSERT(has(clusterGraphics));
		return m_x[c];
	}

	//! Returns the y-position of cluster \p c's cage (lower left corner).
	/**
	 * \pre #clusterGraphics is enabled
	 */
	double y(cluster c) const {
		OGDF_ASSERT(has(clusterGraphics));
		return m_y[c];
	}

	//! Returns the y-position of cluster \p c's cage (lower left corner).
	/**
	 * \pre #clusterGraphics is enabled
	 */
	double& y(cluster c) {
		OGDF_ASSERT(has(clusterGraphics));
		return m_y[c];
	}

	//! Returns the width of cluster \p c.
	/**
	 * \pre #clusterGraphics is enabled
	 */
	double width(cluster c) const {
		OGDF_ASSERT(has(clusterGraphics));
		return m_width[c];
	}

	//! Returns the width of cluster \p c.
	/**
	 * \pre #clusterGraphics is enabled
	 */
	double& width(cluster c) {
		OGDF_ASSERT(has(clusterGraphics));
		return m_width[c];
	}

	//! Returns the height of cluster \p c.
	/**
	 * \pre #clusterGraphics is enabled
	 */
	double height(cluster c) const {
		OGDF_ASSERT(has(clusterGraphics));
		return m_height[c];
	}

	//! Returns the height of cluster \p c.
	/**
	 * \pre #clusterGraphics is enabled
	 */
	double& height(cluster c) {
		OGDF_ASSERT(has(clusterGraphics));
		return m_height[c];
	}

	//! Returns the stroke type of cluster \p c.
	/**
	 * \pre #clusterStyle is enabled
	 */
	const StrokeType &strokeType(cluster c) const {
		OGDF_ASSERT(has(clusterStyle));
		return m_stroke[c].m_type;
	}

	//! Returns the stroke type of cluster \p c.
	/**
	 * \pre #clusterStyle is enabled
	 */
	StrokeType &strokeType(cluster c) {
		OGDF_ASSERT(has(clusterStyle));
		return m_stroke[c].m_type;
	}

	//! Returns the stroke color of cluster \p c.
	/**
	 * \pre #clusterStyle is enabled
	 */
	const Color &strokeColor(cluster c) const {
		OGDF_ASSERT(has(clusterStyle));
		return m_stroke[c].m_color;
	}

	//! Returns the stroke color of cluster \p c.
	/**
	 * \pre #clusterStyle is enabled
	 */
	Color &strokeColor(cluster c) {
		OGDF_ASSERT(has(clusterStyle));
		return m_stroke[c].m_color;
	}

	//! Returns the stroke width of cluster \p c.
	/**
	 * \pre #clusterStyle is enabled
	 */
	const float &strokeWidth(cluster c) const {
		OGDF_ASSERT(has(clusterStyle));
		return m_stroke[c].m_width;
	}

	//! Returns the stroke width of cluster \p c.
	/**
	 * \pre #clusterStyle is enabled
	 */
	float &strokeWidth(cluster c) {
		OGDF_ASSERT(has(clusterStyle));
		return m_stroke[c].m_width;
	}

	//! Returns the fill pattern of cluster \p c.
	/**
	 * \pre #clusterStyle is enabled
	 */
	const FillPattern &fillPattern(cluster c) const {
		OGDF_ASSERT(has(clusterStyle));
		return m_fill[c].m_pattern;
	}

	//! Returns the fill pattern of cluster \p c.
	/**
	 * \pre #clusterStyle is enabled
	 */
	FillPattern &fillPattern(cluster c) {
		OGDF_ASSERT(has(clusterStyle));
		return m_fill[c].m_pattern;
	}

	//! Returns the fill color of cluster \p c.
	/**
	 * \pre #clusterStyle is enabled
	 */
	const Color &fillColor(cluster c) const {
		OGDF_ASSERT(has(clusterStyle));
		return m_fill[c].m_color;
	}

	//! Returns the fill color of cluster \p c.
	/**
	 * \pre #clusterStyle is enabled
	 */
	Color &fillColor(cluster c) {
		OGDF_ASSERT(has(clusterStyle));
		return m_fill[c].m_color;
	}

	//! Returns the background color of fill patterns for cluster \p c.
	/**
	 * \pre #clusterStyle is enabled
	 */
	const Color &fillBgColor(cluster c) const {
		OGDF_ASSERT(has(clusterStyle));
		return m_fill[c].m_bgColor;
	}

	//! Returns the background color of fill patterns for cluster \p c.
	/**
	 * \pre #clusterStyle is enabled
	 */
	Color &fillBgColor(cluster c) {
		OGDF_ASSERT(has(clusterStyle));
		return m_fill[c].m_bgColor;
	}

	//! Returns the label of cluster \p c.
	/**
	 * \pre #clusterLabel is enabled
	 */
	const string &label(cluster c) const {
		OGDF_ASSERT(has(clusterLabel));
		return m_label[c];
	}

	//! Returns the label of cluster \p c.
	/**
	 * \pre #clusterLabel is enabled
	 */
	string &label(cluster c) {
		OGDF_ASSERT(has(clusterLabel));
		return m_label[c];
	}

	//! Returns the template of cluster \p c.
	/**
	 * \pre #clusterTemplate is enabled
	 */
	const string &templateCluster(cluster c) const {
		OGDF_ASSERT(has(clusterTemplate));
		return m_clusterTemplate[c];
	}

	//! Returns the template of cluster \p c.
	/**
	 * \pre #clusterTemplate is enabled
	 */
	string &templateCluster(cluster c) {
		OGDF_ASSERT(has(clusterTemplate));
		return m_clusterTemplate[c];
	}

	/**
	 * @}
	 * @name Layout transformations
	 * @}
	 */

	using GraphAttributes::scale;
	using GraphAttributes::flipVertical;
	using GraphAttributes::flipHorizontal;

	//! Scales the layout by (\p sx,\p sy).
	/**
	* If \p scaleNodes is true, node sizes are scaled as well.
	*
	* \param sx         is the scaling factor for x-coordinates.
	* \param sy         is the scaling factor for y-coordinates.
	* \param scaleNodes determines if nodes size are scaled as well (true) or not.
	*/
	virtual void scale(double sx, double sy, bool scaleNodes = true) override;

	//! Translates the layout by (\p dx,\p dy).
	/**
	* \param dx is the translation in x-direction.
	* \param dy is the translation in y-direction.
	*/
	virtual void translate(double dx, double dy) override;

	//! Flips the (whole) layout vertically such that the part in \p box remains in this area.
	/**
	* The whole layout is flipped and then moved such that the part that was in \p box before
	* flipping is moved to this area.
	*/
	virtual void flipVertical(const DRect &box) override;

	//! Flips the (whole) layout horizontally such that the part in \p box remains in this area.
	/**
	* The whole layout is flipped and then moved such that the part that was in \p box before
	* flipping is moved to this area.
	*/
	virtual void flipHorizontal(const DRect &box) override;

	//@}
	/**
	 * @name Utility functions
	 */
	//@{

	//! Returns the bounding box of the layout.
	virtual DRect boundingBox() const override;

	//! Updates positions of cluster boundaries wrt to children and child clusters
	void updateClusterPositions(double boundaryDist = 1.0);

	//! Returns the parent cluster of node \p v.
	cluster clusterOf(node v) { return m_pClusterGraph->clusterOf(v); }

	//@}
};

}
