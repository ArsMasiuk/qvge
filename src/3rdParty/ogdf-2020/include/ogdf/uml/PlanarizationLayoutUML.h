/** \file
 * \brief Declaration of class PlanarizationLayoutUML.
 *
 * \author Carsten Gutwenger
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

#include <ogdf/uml/UMLLayoutModule.h>
#include <ogdf/uml/UMLCrossingMinimizationModule.h>
#include <ogdf/uml/LayoutPlanRepUMLModule.h>
#include <ogdf/packing/CCLayoutPackModule.h>
#include <memory>
#include <ogdf/planarity/EmbedderModule.h>
#include <ogdf/basic/HashArray.h>
#include <ogdf/orthogonal/OrthoRep.h>

namespace ogdf {

/**
 * \brief The planarization layout algorithm.
 *
 * The class PlanarizationLayout represents a customizable implementation
 * of the planarization approach for drawing graphs. The class provides
 * three different algorithm calls:
 *   - Calling the algorithm for a usual graph (call with GraphAttributes).
 *   - Calling the algorithm for a mixed-upward graph (e.g., a UML class
 *     diagram; call with UMLGraph); a simplified version is provided by
 *     simpleCall.
 *   - Calling the algorithm for simultaneous drawing.
 *
 * If the planarization layout algorithm shall be used for simultaneous drawing,
 * you need to define the different subgraphs by setting the <i>subgraphs</i>
 * option.
 *
 * The implementation used in PlanarizationLayout is based on the following
 * publication:
 *
 * C. Gutwenger, P. Mutzel: <i>An Experimental Study of Crossing
 * Minimization Heuristics</i>. 11th International Symposium on %Graph
 * Drawing 2003, Perugia (GD '03), LNCS 2912, pp. 13-24, 2004.
 *
 * <H3>Optional parameters</H3>
 *
 * <table>
 *   <tr>
 *     <th><i>Option</i><th><i>Type</i><th><i>Default</i><th><i>Description</i>
 *   </tr><tr>
 *     <td><i>pageRatio</i><td>double<td>1.0
 *     <td>Specifies the desired ration of width / height of the computed
 *     layout. It is currently only used when packing connected components.
 *   </tr><tr>
 *     <td><i>preprocessCliques</i><td>bool<td>false
 *     <td>If set to true, a preprocessing for cliques (complete subgraphs)
 *     is performed and cliques will be laid out in a special form (straight-line,
 *     not orthogonal). The preprocessing may reduce running time and improve
 *     layout quality if the input graphs contains dense subgraphs.
 *   </tr><tr>
 *     <td><i>minCliqueSize</i><td>int<td>10<td>If preprocessing of cliques is
 *     enabled, this option determines the minimal size of cliques to search for.
 *   </tr>
 * </table>
 *
 * <H3>%Module options</H3>
 * The various phases of the algorithm can be exchanged by setting
 * module options allowing flexible customization. The algorithm provides
 * the following module options:
 *
 * <table>
 *   <tr>
 *     <th><i>Option</i><th><i>Type</i><th><i>Default</i><th><i>Description</i>
 *   </tr><tr>
 *     <td><i>crossMin</i><td>UMLCrossingMinimizationModule<td>SubgraphPlanarizerUML
 *     <td>The module used for the crossing minimization step.
 *   </tr><tr>
 *     <td><i>embedder</i><td>EmbedderModule<td>SimpleEmbedder
 *     <td>The graph embedding algorithm applied after the crossing minimization
 *     step.
 *   </tr><tr>
 *     <td><i>planarLayouter</i><td>LayoutPlanRepUMLModule<td>OrthoLayoutUML
 *     <td>The planar layout algorithm used to compute a planar layout
 *     of the planarized representation resulting from the crossing minimization step.
 *   </tr><tr>
 *     <td><i>packer</i><td>CCLayoutPackModule<td>TileToRowsCCPacker
 *     <td>The packer module used for arranging connected components.
 *   </tr>
 * </table>
 */
class OGDF_EXPORT PlanarizationLayoutUML : public UMLLayoutModule
{
public:
	//! Creates an instance of planarization layout and sets options to default values.
	PlanarizationLayoutUML();

	// destructor
	virtual ~PlanarizationLayoutUML() { }

	/**
	 *  @name Algorithm call
	 *  @{
	 */

	/**
	 * \brief Calls planarization layout for GraphAttributes \p GA and computes a layout.
	 * \pre The graph has no self-loops.
	 * @param GA is the input graph and will also be assigned the layout information.
	 */
	void call(GraphAttributes &GA) {
		doSimpleCall(GA);
	}

	/**
	 * \brief Calls planarization layout for UML-graph \p umlGraph and computes a mixed-upward layout.
	 * \pre The graph has no self-loops.
	 * @param umlGraph is the input graph and will also be assigned the layout information.
	 */
	virtual void call(UMLGraph &umlGraph) override;

	//! Simple call function that does not care about cliques etc.
	void simpleCall(UMLGraph &umlGraph) {
		//this simple call method does not care about any special treatments
		//of subgraphs, layout informations etc., therefore we save the
		//option status and set them back later on
#if 0
		//cliques are only handled for UMLGraphs, so it is save to
		//only set this value here and not in the GraphAttributes interface method.
		bool l_saveCliqueHandling = m_processCliques;
		m_processCliques = false;
#endif

		// preprocessing: insert a merger for generalizations

		preProcess(umlGraph);
		umlGraph.insertGenMergers();

		doSimpleCall(umlGraph);

		umlGraph.undoGenMergers();

		umlGraph.removeUnnecessaryBendsHV();

		postProcess(umlGraph);

#if 0
		m_processCliques = l_saveCliqueHandling;
#endif
	}

	//! Simple call function.
	void simpleCall(GraphAttributes &GA)
	{
		doSimpleCall(GA);
		GA.removeUnnecessaryBendsHV();
	}

#if 0
	//! Call for simultaneous drawing with graph \p umlGraph.
	virtual void callSimDraw(UMLGraph &umlGraph);

	**
	 * \brief Calls planarization layout with fixed embedding given by \p umlGraph.
	 * \pre The graph has no self-loops.
	 * @param umlGraph is the input graph and will also be assigned the layout information.
	 *        The fixed embedding is obtained from the layout information (node
	 *        coordinates, bend points) in \p umlGraph.
	 */
	virtual void callFixEmbed(UMLGraph &umlGraph);
#endif

	//! Incremental call function.
	/**
	 * Call with information about objects that should be fixed as much as possible
	 * in the old/new drawing for incremental drawing: takes a fixed part of the input
	 * graph (indicated by fixedNodes(Edges)==true), embeds it using the input layout,
	 * then inserts the remaining part into this embedding.
	 */
	virtual void callIncremental(UMLGraph &umlgraph,
		NodeArray<bool> &fixedNodes, const EdgeArray<bool> &fixedEdges);


	/** @}
	 *  @name Optional parameters
	 *  @{
	 */

	/**
	 * \brief Returns the current setting of option pageRatio.
	 *
	 * This option specifies the desired ration width / height of the computed
	 * layout. It is currently only used for packing connected components.
	 */
	double pageRatio() const {
		return m_pageRatio;
	}

	//! Sets the option pageRatio to \p ratio.
	void pageRatio(double ratio) {
		m_pageRatio = ratio;
	}


	//set the option field for the planar layouter
	void setLayouterOptions(int ops) { m_planarLayouter->setOptions(ops); }

	//draw hierarchy nodes corresponding to their level
	void alignSons(bool b)
	{
		int opts = m_planarLayouter->getOptions();

		if (b) m_planarLayouter->setOptions(opts | UMLOpt::OpAlign);
		else  m_planarLayouter->setOptions(opts & ~UMLOpt::OpAlign);
	}


	/** @}
	 *  @name Module options
	 *  @{
	 */

	//! Sets the module option for UML crossing minimization.
	void setCrossMin(UMLCrossingMinimizationModule *pCrossMin) {
		m_crossMin.reset(pCrossMin);
	}

	/**
	 * \brief Sets the module option for the graph embedding algorithm.
	 *
	 * The result of the crossing minimization step is a planar graph,
	 * in which crossings are replaced by dummy nodes. The embedding
	 * module then computes a planar embedding of this planar graph.
	 */
	void setEmbedder(EmbedderModule *pEmbedder) {
		m_embedder.reset(pEmbedder);
	}

	/**
	 * \brief Sets the module option for the planar layout algorithm.
	 *
	 * The planar layout algorithm is used to compute a planar layout
	 * of the planarized representation resulting from the crossing
	 * minimization step. Planarized representation means that edge crossings
	 * are replaced by dummy nodes of degree four, so the actual layout
	 * algorithm obtains a planar graph as input. By default, the planar
	 * layout algorithm produces an orthogonal drawing.
	 */
	void setPlanarLayouter(LayoutPlanRepUMLModule *pPlanarLayouter) {
		m_planarLayouter.reset(pPlanarLayouter);
	}

	/**
	 * \brief Sets the module option for the arrangement of connected components.
	 *
	 * The planarization layout algorithm draws each connected component of
	 * the input graph seperately, and then arranges the resulting drawings
	 * using a packing algorithm.
	 */
	void setPacker(CCLayoutPackModule *pPacker) {
		m_packer.reset(pPacker);
	}

	/** @}
	 *  @name Further information
	 *  @{
	 */

	//! Returns the number of crossings in computed layout.
	int numberOfCrossings() const {
		return m_nCrossings;
	}

	//! \pre \p umlGraph may not violate a precondition of planarization layout.
	void assureDrawability(UMLGraph& umlGraph);

	//! @}

protected:
	void doSimpleCall(GraphAttributes &GA);

	//sorts the additional nodes for piecewise insertion
	void sortIncrementalNodes(List<node> &addNodes, const NodeArray<bool> &fixedNodes);
	void getFixationDistance(node startNode, HashArray<int, int> &distance,
		const NodeArray<bool> &fixedNodes);
	//reembeds already planarized PG in case of errors
	void reembed(PlanRepUML &PG, int ccNumber, bool l_align = false,
		bool l_gensExist = false);

	virtual void preProcess(UMLGraph &UG);
	virtual void postProcess(UMLGraph& UG); //redo changes at original

	void arrangeCCs(PlanRep &PG, GraphAttributes &GA, Array<DPoint> &boundingBox);

private:
	face findBestExternalFace(
		const PlanRep &PG,
		const CombinatorialEmbedding &E);

	//! The moule for UML crossing minimization
	std::unique_ptr<UMLCrossingMinimizationModule> m_crossMin;

	//! The module for planar embedding.
	std::unique_ptr<EmbedderModule>       m_embedder;

	//! The module for computing a planar layout.
	std::unique_ptr<LayoutPlanRepUMLModule>  m_planarLayouter;

	//! The module for arranging connected components.
	std::unique_ptr<CCLayoutPackModule>   m_packer;

	double m_pageRatio;    //!< The desired page ratio.
	int m_nCrossings;      //!< The number of crossings in the computed layout.
	bool m_arrangeLabels;  //!< Option for re-arranging labels.

	// temporary changes to avoid errors
	List<edge> m_fakedGens; // made to associations
	bool m_fakeTree;
};

}
