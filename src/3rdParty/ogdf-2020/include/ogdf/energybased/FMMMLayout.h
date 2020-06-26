/** \file
 * \brief Declaration of Fast Multipole Multilevel Method (FM^3).
 *
 * \author Stefan Hachul
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

#include <ogdf/basic/Graph.h>
#include <ogdf/cluster/ClusterGraphAttributes.h>
#include <ogdf/basic/LayoutModule.h>
#include <ogdf/basic/geometry.h>
#include <ogdf/energybased/fmmm/NewMultipoleMethod.h>
#include <ogdf/energybased/fmmm/maar_packing/Rectangle.h>

namespace ogdf {

/**
 * \brief The fast multipole multilevel layout algorithm.
 *
 * @ingroup gd-energy
 *
 * The class FMMMLayout implements a force-directed graph drawing
 * method suited also for very large graphs. It is based on a
 * combination of an efficient multilevel scheme and a strategy for
 * approximating the repulsive forces in the system by rapidly
 * evaluating potential fields.
 *
 * The implementation is based on the following publication:
 *
 * Stefan Hachul, Michael Jünger: <i>Drawing Large Graphs with a
 * Potential-Field-Based Multilevel Algorithm</i>. 12th International
 * Symposium on %Graph Drawing 1998, New York (GD '04), LNCS 3383,
 * pp. 285-295, 2004.
 *
 * <H3>Optional parameters</H3>
 * The following options are the most important. You can set
 * useHighLevelOptions to true and just need to adjust a few parameters.
 * However, you can also adjust all parameters that the implementation
 * uses (see below), but this requires good knowledge of the algorithm.
 *
 * <table>
 *   <tr>
 *     <th><i>Option</i><th><i>Type</i><th><i>Default</i><th><i>Description</i>
 *   </tr><tr>
 *     <td><i>useHighLevelOptions</i><td>bool<td>false
 *     <td>Whether high-level options are used or not.
 *   </tr><tr>
 *     <td><i>pageFormat</i><td> FMMMOptions::PageFormatType <td> \c Square
 *     <td>The desired aspect ratio of the layout.
 *   </tr><tr>
 *     <td><i>unitEdgeLength</i><td>double<td>100.0
 *     <td>The unit edge length.
 *   </tr><tr>
 *     <td><i>newInitialPlacement</i><td>bool<td>false
 *     <td>Specifies if initial placement of nodes is varied.
 *   </tr><tr>
 *     <td><i>qualityVersusSpeed</i><td> FMMMOptions::QualityVsSpeed <td> \c BeautifulAndFast
 *     <td>Indicates if the algorithm is tuned either for best quality or best speed.
 *   </tr>
 * </table>
 *
 * If you want to do more detailed fine-tuning, you can adjust all parameters
 * used by the algorithm. Please refer to the paper cited above for better
 * understanding of the algorithm.
 *
 * <table>
 *   <tr>
 *     <th colspan="4" align="center"><b>General</b>
 *   </tr><tr>
 *     <td><i>randSeed</i><td>int<td>100
 *     <td>The seed of the random number generator.
 *   </tr><tr>
 *     <td><i>edgeLengthMeasurement</i><td> FMMMOptions::EdgeLengthMeasurement <td> \c BoundingCircle
 *     <td>Indicates how the length of an edge is measured.
 *   </tr><tr>
 *     <td><i>allowedPositions</i><td> FMMMOptions::AllowedPositions <td> \c Integer
 *     <td>Defines which positions for a node are allowed.
 *   </tr><tr>
 *     <td><i>maxIntPosExponent</i><td>int<td>40
 *     <td>Defines the exponent used if allowedPositions == Exponent.
 *   </tr><tr>
 *     <th colspan="4" align="center"><b>Divide et impera step</b>
 *   </tr><tr>
 *     <td><i>pageRatio</i><td>double<td>1.0
 *     <td>The desired page ratio.
 *   </tr><tr>
 *     <td><i>stepsForRotatingComponents</i><td>int<td>10
 *     <td>The number of rotations per connected component.
 *   </tr><tr>
 *     <td><i>tipOverCCs</i><td> FMMMOptions::TipOver <td> \c NoGrowingRow
 *     <td>Specifies when it is allowed to tip over drawings.
 *   </tr><tr>
 *     <td><i>minDistCC</i><td>double<td>100
 *     <td>The minimal distance between connected components.
 *   </tr><tr>
 *     <td><i>presortCCs</i><td> FMMMOptions::PreSort <td> \c DecreasingHeight
 *     <td>Defines if the connected components are sorted before
 *     the packing algorithm is applied.
 *   </tr><tr>
 *     <th colspan="4" align="center"><b>Multilevel step</b>
 *   </tr><tr>
 *     <td><i>minGraphSize</i><td>int<td>50
 *     <td>Determines the number of nodes of a graph for which
 *     no more collapsing of galaxies is performed.
 *   </tr><tr>
 *     <td><i>galaxyChoice</i><td> FMMMOptions::GalaxyChoice <td> \c NonUniformProbLowerMass
 *     <td>Defines how sun nodes of galaxies are selected.
 *   </tr><tr>
 *     <td><i>randomTries</i><td>int<td>20
 *     <td>Defines the number of tries to get a random node with
 *     minimal star mass.
 *   </tr><tr>
 *     <td><i>maxIterChange</i><td> FMMMOptions::MaxIterChange <td> \c LinearlyDecreasing
 *     <td>Defines how MaxIterations is changed in subsequent multilevels.
 *   </tr><tr>
 *     <td><i>maxIterFactor</i><td>int<td>10
 *     <td>Defines the factor used for decreasing MaxIterations.
 *   </tr><tr>
 *     <td><i>initialPlacementMult</i><td> FMMMOptions::InitialPlacementMult <td> \c Advanced
 *     <td>Defines how the initial placement is generated.
 *   </tr><tr>
 *     <th colspan="4" align="center"><b>Force calculation step</b>
 *   </tr><tr>
 *     <td><i>forceModel</i><td> FMMMOptions::ForceModel <td> \c New
 *     <td>The used force model.
 *   </tr><tr>
 *     <td><i>springStrength</i><td>double<td>1.0
 *     <td>The strength of the springs.
 *   </tr><tr>
 *     <td><i>repForcesStrength</i><td>double<td>1.0
 *     <td>The strength of the repulsive forces.
 *   </tr><tr>
 *     <td><i>repulsiveForcesCalculation</i><td> FMMMOptions::RepulsiveForcesMethod <td> \c NMM
 *     <td>Defines how to calculate repulsive forces.
 *   </tr><tr>
 *     <td><i>stopCriterion</i><td> FMMMOptions::StopCriterion <td> \c FixedIterationsOrThreshold
 *     <td>The stop criterion.
 *   </tr><tr>
 *     <td><i>threshold</i><td>double<td>0.01
 *     <td>The threshold for the stop criterion.
 *   </tr><tr>
 *     <td><i>fixedIterations</i><td>int<td>30
 *     <td>The fixed number of iterations for the stop criterion.
 *   </tr><tr>
 *     <td><i>forceScalingFactor</i><td>double<td>0.05
 *     <td>The scaling factor for the forces.
 *   </tr><tr>
 *     <td><i>coolTemperature</i><td>bool<td>false
 *     <td>Use coolValue for scaling forces.
 *   </tr><tr>
 *     <td><i>coolValue</i><td>double<td>0.99
 *     <td>The value by which forces are decreased.
 *   </tr><tr>
 *     <td><i>initialPlacementForces</i><td> FMMMOptions::InitialPlacementForces <td> \c RandomRandIterNr
 *     <td>Defines how the initial placement is done.
 *   </tr><tr>
 *     <th colspan="4" align="center"><b>Force calculation step</b>
 *   </tr><tr>
 *     <td><i>resizeDrawing</i><td>bool<td>true
 *     <td>Specifies if the resulting drawing is resized.
 *   </tr><tr>
 *     <td><i>resizingScalar</i><td>double<td>1
 *     <td>Defines a parameter to scale the drawing if resizeDrawing is true.
 *   </tr><tr>
 *     <td><i>fineTuningIterations</i><td>int<td>20
 *     <td>The number of iterations for fine tuning.
 *   </tr><tr>
 *     <td><i>fineTuneScalar</i><td>double<td>0.2
 *     <td>Defines a parameter for scaling the forces in the fine-tuning iterations.
 *   </tr><tr>
 *     <td><i>adjustPostRepStrengthDynamically</i><td>bool<td>true
 *     <td>If set to true, the strength of the repulsive force field is calculated.
 *   </tr><tr>
 *     <td><i>postSpringStrength</i><td>double<td>2.0
 *     <td>The strength of the springs in the postprocessing step.
 *   </tr><tr>
 *     <td><i>postStrengthOfRepForces</i><td>double<td>0.01
 *     <td>The strength of the repulsive forces in the postprocessing step.
 *   </tr><tr>
 *     <th colspan="4" align="center"><b>Repulsive force approximation methods</b>
 *   </tr><tr>
 *     <td><i>frGridQuotient</i><td>int<td>2
 *     <td>The grid quotient.
 *   </tr><tr>
 *     <td><i>nmTreeConstruction</i><td> FMMMOptions::ReducedTreeConstruction <td> \c SubtreeBySubtree
 *     <td>Defines how the reduced bucket quadtree is constructed.
 *   </tr><tr>
 *     <td><i>nmSmallCell</i><td> FMMMOptions::SmallestCellFinding <td> \c Iteratively
 *     <td>Defines how the smallest quadratic cell that surrounds
 *     the particles of a node in the reduced bucket quadtree is calculated.
 *   </tr><tr>
 *     <td><i>nmParticlesInLeaves</i><td>int<td>25
 *     <td>The maximal number of particles that are contained in
 *     a leaf of the reduced bucket quadtree.
 *   </tr><tr>
 *     <td><i>nmPrecision</i><td>int<td>4
 *     <td>The precision \a p for the <i>p</i>-term multipole expansions.
 *   </tr>
 * </table>
 *
 * <H3>Running time</H3>
 * The running time of the algorithm is
 * O(<i>n</i> log <i>n</i> + <i>m</i>) for graphs with \a n nodes
 * and \a m edges. The required space is linear in the input size.
 */
class OGDF_EXPORT FMMMLayout : public LayoutModule
{
	using Rectangle = energybased::fmmm::Rectangle;
	using NodeAttributes = energybased::fmmm::NodeAttributes;
	using EdgeAttributes = energybased::fmmm::EdgeAttributes;

public:
	//! Creates an instance of the layout algorithm.
	FMMMLayout();

	// destructor
	virtual ~FMMMLayout() { }


	/**
	 *  @name The algorithm call
	 *  @{
	 */

	//! Calls the algorithm for graph \p GA and returns the layout information in \p GA.
	virtual void call(GraphAttributes &GA) override;

	//! Calls the algorithm for clustered graph \p GA and returns the layout information in \p GA.
	//! Models cluster by simple edge length adaption based on least common ancestor
	//! cluster of end vertices.
	void call(ClusterGraphAttributes &GA);

	//! Extended algorithm call: Allows to pass desired lengths of the edges.
	/**
	 * @param GA represents the input graph and is assigned the computed layout.
	 * @param edgeLength is an edge array of the graph associated with \p GA
	 *        of positive edge length.
	 */
	void call(
		GraphAttributes &GA,   //graph and layout
		const EdgeArray<double> &edgeLength); //factor for desired edge lengths

	//! Extended algorithm call: Calls the algorithm for graph \p GA.
	/**
	 * Returns layout information in \p GA and a simple drawing is saved in file \p ps_file
	 * in postscript format (Nodes are drawn as uniformly sized circles).
	 */
	void call(GraphAttributes &GA, char* ps_file);

	//! Extend algorithm call: Allows to pass desired lengths of the edges.
	/**
	 * The EdgeArray \p edgeLength must be valid for GA.constGraph() and its values must
	 * be positive.
	 * A simple drawing is saved in file ps_file in postscript format (Nodes are drawn
	 * as uniformly sized circles).
	 */
	void call(
		GraphAttributes &GA,   //graph and layout
		const EdgeArray<double> &edgeLength, //factor for desired edge lengths
		char* ps_file);

	/** @}
	 *  @name Further information.
	 *  @{
	 */

	//! Returns the runtime (=CPU-time) of the layout algorithm in seconds.
	double getCpuTime() {
		return time_total;
	}


	/** @}
	 *  @name High-level options
	 *  Allow to specify the most relevant parameters.
	 *  @{
	 */

	//! Returns the current setting of option useHighLevelOptions.
	/**
	 * If set to true, the high-level options are used to set all low-level options.
	 * Usually, it is sufficient just to set high-level options; if you want to
	 * be more specific, set this parameter to false and set the low level options.
	 */
	bool useHighLevelOptions() const { return m_useHighLevelOptions; }

	//! Sets the option useHighLevelOptions to \p uho.
	void useHighLevelOptions(bool uho) { m_useHighLevelOptions = uho; }

	//! Sets single level option, no multilevel hierarchy is created if b == true
	void setSingleLevel(bool b) {m_singleLevel = b;}

	//! Returns the current setting of option pageFormat.
	FMMMOptions::PageFormatType pageFormat() const { return m_pageFormat; }

	//! Sets the option pageRatio to \p t.
	void pageFormat(FMMMOptions::PageFormatType t) { m_pageFormat = t; }

	//! Returns the current setting of option unitEdgeLength.
	double unitEdgeLength() const { return m_unitEdgeLength; }

	//! Sets the option unitEdgeLength to \p x.
	void unitEdgeLength(double x) {m_unitEdgeLength = (( x > 0.0) ? x : 1);}

	//! Returns the current setting of option newInitialPlacement.
	/**
	 * This option defines if the initial placement of the nodes at the
	 * coarsest multilevel is varied for each distinct call of FMMMLayout
	 * or keeps always the same.
	 */
	bool newInitialPlacement() const { return m_newInitialPlacement; }

	//! Sets the option newInitialPlacement to \p nip.
	void newInitialPlacement(bool nip) { m_newInitialPlacement = nip; }

	//! Returns the current setting of option qualityVersusSpeed.
	FMMMOptions::QualityVsSpeed qualityVersusSpeed() const { return m_qualityVersusSpeed; }

	//! Sets the option qualityVersusSpeed to \p qvs.
	void qualityVersusSpeed(FMMMOptions::QualityVsSpeed qvs) {m_qualityVersusSpeed = qvs; }


	/** @}
	 *  @name General low-level options
	 * The low-level options in this and the following sections are meant for
	 * experts or interested people only.
	 *  @{
	 */

	//! Sets the seed of the random number generator.
	void randSeed(int p) { m_randSeed = ((0<=p) ? p : 1);}

	//! Returns the seed of the random number generator.
	int randSeed() const {return m_randSeed;}

	//! Returns the current setting of option edgeLengthMeasurement.
	FMMMOptions::EdgeLengthMeasurement edgeLengthMeasurement() const {
		return m_edgeLengthMeasurement;
	}

	//! Sets the option edgeLengthMeasurement to \p elm.
	void edgeLengthMeasurement(FMMMOptions::EdgeLengthMeasurement elm) { m_edgeLengthMeasurement = elm; }

	//! Returns the current setting of option allowedPositions.
	FMMMOptions::AllowedPositions allowedPositions() const { return m_allowedPositions; }

	//! Sets the option allowedPositions to \p ap.
	void allowedPositions(FMMMOptions::AllowedPositions ap) { m_allowedPositions = ap; }

	//! Returns the current setting of option maxIntPosExponent.
	/**
	 * This option defines the exponent used if allowedPositions() == FMMMOptions::AllowedPositions::Exponent.
	 */
	int maxIntPosExponent() const { return m_maxIntPosExponent; }

	//! Sets the option maxIntPosExponent to \p e.
	void maxIntPosExponent(int e) {
		m_maxIntPosExponent = (((e >= 31)&&(e<=51))? e : 31);
	}


	/** @}
	 *  @name Options for the divide et impera step
	 *  @{
	 */

	//! Returns the current setting of option pageRatio.
	/**
	 * This option defines the desired aspect ratio of the rectangular drawing area.
	 */
	double pageRatio() const { return m_pageRatio; }

	//! Sets the option pageRatio to \p r.
	void pageRatio(double r) {m_pageRatio = (( r > 0) ? r : 1);}

	//! Returns the current setting of option stepsForRotatingComponents.
	/**
	 * This options determines the number of times each connected component is rotated with
	 * angles between 0 and 90 degree to obtain a bounding rectangle with small area.
	 */
	int stepsForRotatingComponents() const { return m_stepsForRotatingComponents; }

	//! Sets the option stepsForRotatingComponents to \p n.
	void stepsForRotatingComponents(int n) {
		m_stepsForRotatingComponents = ((0<=n) ? n : 0);
	}

	//! Returns the current setting of option tipOverCCs.
	FMMMOptions::TipOver tipOverCCs() const { return m_tipOverCCs; }

	//! Sets the option tipOverCCs to \p to.
	void tipOverCCs(FMMMOptions::TipOver to) { m_tipOverCCs = to; }

	//! Returns the  minimal distance between connected components.
	double minDistCC() const { return m_minDistCC; }

	//! Sets the  minimal distance between connected components to \p x.
	void minDistCC(double x) { m_minDistCC = (( x > 0) ? x : 1);}

	//! Returns the current setting of option presortCCs.
	FMMMOptions::PreSort presortCCs() const { return m_presortCCs; }

	//! Sets the option presortCCs to \p ps.
	void presortCCs(FMMMOptions::PreSort ps) { m_presortCCs = ps; }


	/** @}
	 *  @name Options for the multilevel step
	 *  @{
	 */

	//! Returns the current setting of option minGraphSize.
	/**
	 * This option determines the number of nodes of a graph in the
	 * multilevel representation for which no more collapsing of galaxies
	 * is performed (i.e. the graph at the highest level).
	 */
	int minGraphSize() const { return m_minGraphSize; }

	//! Sets the option minGraphSize to \p n.
	void minGraphSize(int n) { m_minGraphSize = ((n >= 2)? n : 2);}

	//! Returns the current setting of option galaxyChoice.
	FMMMOptions::GalaxyChoice galaxyChoice() const { return m_galaxyChoice; }

	//! Sets the option galaxyChoice to \p gc.
	void galaxyChoice(FMMMOptions::GalaxyChoice gc) { m_galaxyChoice = gc; }

	//! Returns the current setting of option randomTries.
	/**
	 * This option defines the number of tries to get a random node with
	 * minimal star mass (used in case of galaxyChoice() == NonUniformProbLowerMass
	 * and galaxyChoice() == NonUniformProbHigherMass).
	 */
	int randomTries() const { return m_randomTries; }

	//! Sets the option randomTries to \p n.
	void randomTries(int n) {m_randomTries = ((n>=1)? n: 1);}

	//! Returns the current setting of option maxIterChange.
	FMMMOptions::MaxIterChange maxIterChange() const { return m_maxIterChange; }

	//! Sets the option maxIterChange to \p mic.
	void maxIterChange(FMMMOptions::MaxIterChange mic) { m_maxIterChange = mic; }

	//! Returns the current setting of option maxIterFactor.
	/**
	 * This option defines the factor used for decrasing MaxIterations
	 * (in case of maxIterChange() == LinearlyDecreasing or maxIterChange()
	 * == RapidlyDecreasing).
	 */
	int maxIterFactor() const { return m_maxIterFactor; }

	//! Sets the option maxIterFactor to \p f.
	void maxIterFactor(int f) { m_maxIterFactor = ((f>=1) ? f : 1 ); }

	//! Returns the current setting of option initialPlacementMult.
	FMMMOptions::InitialPlacementMult initialPlacementMult() const {
		return m_initialPlacementMult;
	}

	//! Sets the option initialPlacementMult to \p ipm.
	void initialPlacementMult(FMMMOptions::InitialPlacementMult ipm) {
		m_initialPlacementMult = ipm;
	}


	/** @}
	 *  @name Options for the force calculation step
	 *  @{
	 */

	//! Returns the used force model.
	FMMMOptions::ForceModel forceModel() const { return m_forceModel; }

	//! Sets the used force model to \p fm.
	void forceModel(FMMMOptions::ForceModel fm) { m_forceModel = fm; }

	//! Returns the strength of the springs.
	double springStrength() const { return m_springStrength; }

	//! Sets the strength of the springs to \p x.
	void springStrength(double x) { m_springStrength  = ((x > 0)? x : 1);}

	//! Returns the strength of the repulsive forces.
	double repForcesStrength() const { return m_repForcesStrength; }

	//! Sets the strength of the repulsive forces to \p x.
	void repForcesStrength(double x) { m_repForcesStrength =((x > 0)? x : 1);}

	//! Returns the current setting of option repulsiveForcesCalculation.
	FMMMOptions::RepulsiveForcesMethod repulsiveForcesCalculation() const {
		return m_repulsiveForcesCalculation;
	}

	//! Sets the option repulsiveForcesCalculation to \p rfc.
	void repulsiveForcesCalculation(FMMMOptions::RepulsiveForcesMethod rfc) {
		m_repulsiveForcesCalculation = rfc;
	}

	//! Returns the stop criterion.
	FMMMOptions::StopCriterion stopCriterion() const { return m_stopCriterion; }

	//! Sets the stop criterion to \p rsc.
	void stopCriterion(FMMMOptions::StopCriterion rsc) { m_stopCriterion = rsc; }

	//! Returns the threshold for the stop criterion.
	/**
	 * (If the average absolute value of all forces in
	 * an iteration is less then threshold() then stop.)
	 */
	double threshold() const { return m_threshold; }

	//! Sets the threshold for the stop criterion to \p x.
	void threshold(double x) {m_threshold = ((x > 0) ? x : 0.1);}

	//! Returns the fixed number of iterations for the stop criterion.
	int fixedIterations() const { return m_fixedIterations; }

	//! Sets the fixed number of iterations for the stop criterion to \p n.
	void fixedIterations(int n) { m_fixedIterations = ((n >= 1) ? n : 1);}

	//! Returns the scaling factor for the forces.
	double forceScalingFactor() const { return m_forceScalingFactor; }

	//! Sets the scaling factor for the forces to \p f.
	void forceScalingFactor(double f) { m_forceScalingFactor = ((f > 0) ? f : 1);}

	//! Returns the current setting of option coolTemperature.
	/**
	 * If set to true, forces are scaled by coolValue()^(actual iteration) *
	 * forceScalingFactor(); otherwise forces are scaled by forceScalingFactor().
	 */
	bool coolTemperature() const { return m_coolTemperature; }

	//! Sets the option coolTemperature to \p b.
	void coolTemperature(bool b) { m_coolTemperature = b; }

	//! Returns the current setting of option coolValue.
	/**
	 * This option defines the value by which forces are decreased
	 * if coolTemperature is true.
	 */
	double coolValue() const { return m_coolValue; }

	//! Sets the option coolValue to \p x.
	void coolValue(double x) { m_coolValue = (((x >0 )&&(x<=1) )? x : 0.99);}


	//! Returns the current setting of option initialPlacementForces.
	FMMMOptions::InitialPlacementForces initialPlacementForces() const {
		return m_initialPlacementForces;
	}

	//! Sets the option initialPlacementForces to \p ipf.
	void initialPlacementForces(FMMMOptions::InitialPlacementForces ipf) {
		m_initialPlacementForces = ipf;
	}


	/** @}
	 *  @name Options for the postprocessing step
	 *  @{
	 */

	//! Returns the current setting of option resizeDrawing.
	/**
	 * If set to true, the resulting drawing is resized so that the average edge
	 * length is the desired edge length times resizingScalar().
	 */
	bool resizeDrawing() const { return m_resizeDrawing; }

	//! Sets the option resizeDrawing to \p b.
	void resizeDrawing(bool b) { m_resizeDrawing = b; }

	//! Returns the current setting of option resizingScalar.
	/**
	 * This option defines a parameter to scale the drawing if
	 * resizeDrawing() is true.
	 */
	double resizingScalar() const { return m_resizingScalar; }

	//! Sets the option resizingScalar to \p s.
	void resizingScalar(double s) { m_resizingScalar = ((s > 0) ? s : 1);}

	//! Returns the number of iterations for fine tuning.
	int fineTuningIterations() const { return m_fineTuningIterations; }

	//! Sets the number of iterations for fine tuning to \p n.
	void fineTuningIterations(int n) { m_fineTuningIterations =((n >= 0) ? n : 0);}

	//! Returns the curent setting of option fineTuneScalar.
	/**
	 * This option defines a parameter for scaling the forces in the
	 * fine-tuning iterations.
	 */
	double fineTuneScalar() const { return m_fineTuneScalar; }

	//! Sets the option fineTuneScalar to \p s
	void fineTuneScalar(double s) { m_fineTuneScalar = ((s >= 0) ? s : 1);}

	//! Returns the current setting of option adjustPostRepStrengthDynamically.
	/**
	 * If set to true, the strength of the repulsive force field is calculated
	 * dynamically by a formula depending on the number of nodes; otherwise the
	 * strength are scaled by PostSpringStrength and PostStrengthOfRepForces.
	 */
	bool adjustPostRepStrengthDynamically() const {
		return m_adjustPostRepStrengthDynamically;
	}

	//! Sets the option adjustPostRepStrengthDynamically to \p b.
	void adjustPostRepStrengthDynamically(bool b) {
		m_adjustPostRepStrengthDynamically = b;
	}

	//! Returns the strength of the springs in the postprocessing step.
	double postSpringStrength() const { return m_postSpringStrength; }

	//! Sets the strength of the springs in the postprocessing step to \p x.
	void postSpringStrength(double x) { m_postSpringStrength  = ((x > 0)? x : 1);}

	//! Returns the strength of the repulsive forces in the postprocessing step.
	double postStrengthOfRepForces() const { return m_postStrengthOfRepForces; }

	//! Sets the strength of the repulsive forces in the postprocessing step to \p x.
	void postStrengthOfRepForces(double x) {
		m_postStrengthOfRepForces = ((x > 0)? x : 1);
	}


	/** @}
	 *  @name Options for repulsive force approximation methods
	 *  @{
	 */

	//! Returns the current setting of option frGridQuotient.
	/**
	 * The number k of rows and columns of the grid is sqrt(|V|) / frGridQuotient().
	 * (Note that in [Fruchterman,Reingold] frGridQuotient is 2.)
	 */
	int  frGridQuotient() const {return m_frGridQuotient;}

	//! Sets the option frGridQuotient to \p p.
	void frGridQuotient(int p) { m_frGridQuotient = ((0<=p) ? p : 2);}

	//! Returns the current setting of option nmTreeConstruction.
	FMMMOptions::ReducedTreeConstruction nmTreeConstruction() const { return m_NMTreeConstruction; }

	//! Sets the option nmTreeConstruction to \p rtc.
	void nmTreeConstruction(FMMMOptions::ReducedTreeConstruction rtc) { m_NMTreeConstruction = rtc; }

	//! Returns the current setting of option nmSmallCell.
	FMMMOptions::SmallestCellFinding nmSmallCell() const { return m_NMSmallCell; }

	//! Sets the option nmSmallCell to \p scf.
	void nmSmallCell(FMMMOptions::SmallestCellFinding scf) { m_NMSmallCell = scf; }

	//! Returns the current setting of option nmParticlesInLeaves.
	/**
	 * Defines the maximal number of particles that are contained in
	 * a leaf of the reduced bucket quadtree.
	 */
	int nmParticlesInLeaves() const { return m_NMParticlesInLeaves; }

	//! Sets the option nmParticlesInLeaves to \p n.
	void nmParticlesInLeaves(int n) { m_NMParticlesInLeaves = ((n>= 1)? n : 1);}

	//! Returns the precision \a p for the <i>p</i>-term multipole expansions.
	int nmPrecision() const { return m_NMPrecision; }

	//! Sets the precision for the multipole expansions to \p p.
	void nmPrecision(int p) { m_NMPrecision  = ((p >= 1 ) ? p : 1);}

	//! @}

private:

	//high level options
	bool                  m_useHighLevelOptions; //!< The option for using high-level options.
	FMMMOptions::PageFormatType m_pageFormat; //!< The option for the page format.
	double                m_unitEdgeLength; //!< The unit edge length.
	bool                  m_newInitialPlacement; //!< The option for new initial placement.
	FMMMOptions::QualityVsSpeed m_qualityVersusSpeed; //!< The option for quality-vs-speed trade-off.

	//low level options
	//general options
	int                   m_randSeed; //!< The random seed.
	FMMMOptions::EdgeLengthMeasurement m_edgeLengthMeasurement; //!< The option for edge length measurement.
	FMMMOptions::AllowedPositions m_allowedPositions; //!< The option for allowed positions.
	int                   m_maxIntPosExponent; //!< The option for the used	exponent.

	//options for divide et impera step
	double                m_pageRatio; //!< The desired page ratio.
	int                   m_stepsForRotatingComponents; //!< The number of rotations.
	FMMMOptions::TipOver  m_tipOverCCs; //!< Option for tip-over of connected components.
	double                m_minDistCC; //!< The separation between connected components.
	FMMMOptions::PreSort  m_presortCCs; //!< The option for presorting connected components.

	//options for multilevel step
	bool                  m_singleLevel; //!< Option for pure single level.
	int                   m_minGraphSize; //!< The option for minimal graph size.
	FMMMOptions::GalaxyChoice m_galaxyChoice; //!< The selection of galaxy nodes.
	int                   m_randomTries; //!< The number of random tries.

	//! The option for how to change MaxIterations.
	//! If maxIterChange != micConstant, the iterations are decreased
	//! depending on the level, starting from
	//! ((maxIterFactor()-1) * fixedIterations())
	FMMMOptions::MaxIterChange m_maxIterChange;

	int                   m_maxIterFactor; //!< The factor used for decreasing MaxIterations.
	FMMMOptions::InitialPlacementMult m_initialPlacementMult; //!< The option for creating initial placement.

	//options for force calculation step
	FMMMOptions::ForceModel m_forceModel; //!< The used force model.
	double                m_springStrength; //!< The strengths of springs.
	double                m_repForcesStrength; //!< The strength of repulsive forces.
	FMMMOptions::RepulsiveForcesMethod m_repulsiveForcesCalculation; //!< Option for how to calculate repulsive forces.
	FMMMOptions::StopCriterion m_stopCriterion; //!< The stop criterion.
	double                m_threshold; //!< The threshold for the stop criterion.
	int                   m_fixedIterations; //!< The fixed number of iterations for the stop criterion.
	double                m_forceScalingFactor; //!< The scaling factor for the forces.
	bool                  m_coolTemperature; //!< The option for how to scale forces.
	double                m_coolValue; //!< The value by which forces are decreased.
	FMMMOptions::InitialPlacementForces m_initialPlacementForces; //!< The option for how the initial placement is done.

	//options for postprocessing step
	bool                  m_resizeDrawing; //!< The option for resizing the drawing.
	double                m_resizingScalar; //!< Parameter for resizing the drawing.
	int                   m_fineTuningIterations; //!< The number of iterations for fine tuning.
	double                m_fineTuneScalar; //!< Parameter for scaling forces during fine tuning.
	bool                  m_adjustPostRepStrengthDynamically; //!< The option adjustPostRepStrengthDynamically.
	double                m_postSpringStrength; //!< The strength of springs during postprocessing.
	double                m_postStrengthOfRepForces; //!< The strength of repulsive forces during postprocessing.

	//options for repulsive force approximation methods
	int                   m_frGridQuotient; //!< The grid quotient.
	FMMMOptions::ReducedTreeConstruction m_NMTreeConstruction; //!< The option for how to construct reduced bucket quadtree.
	FMMMOptions::SmallestCellFinding m_NMSmallCell; //!< The option for how to calculate smallest quadtratic cells.
	int                   m_NMParticlesInLeaves; //!< The maximal number of particles in a leaf.
	int                   m_NMPrecision; //!< The precision for multipole expansions.

	//other variables
	double max_integer_position; //!< The maximum value for an integer position.
	double cool_factor; //!< Needed for scaling the forces if coolTemperature is true.
	double average_ideal_edgelength; //!< Measured from center to center.
	double boxlength; //!< Holds the length of the quadratic comput. box.
	int number_of_components; //!< The number of components of the graph.
	DPoint down_left_corner; //!< Holds down left corner of the comput. box.
	NodeArray<double> radius; //!< Holds the radius of the surrounding circle for each node.
	double time_total; //!< The runtime (=CPU-time) of the algorithm in seconds.

	energybased::fmmm::FruchtermanReingold FR; //!< Class for repulsive force calculation (Fruchterman, Reingold).
	energybased::fmmm::NewMultipoleMethod NM; //!< Class for repulsive force calculation.

	//! \name Most important functions
	//! @{

	//! Calls the divide (decomposition into connected components) and impera (drawing and packing of the componenets) step.
	void call_DIVIDE_ET_IMPERA_step(
		Graph& G,
		NodeArray<NodeAttributes>& A,
		EdgeArray<EdgeAttributes>& E);

	//! Calls the multilevel step for subGraph \p G.
	void call_MULTILEVEL_step_for_subGraph(
		Graph& G,
		NodeArray<NodeAttributes>& A,
		EdgeArray<EdgeAttributes>& E);

	//! Returns true iff stopCriterion() is not met
	bool running(int iter, int max_mult_iter, double actforcevectorlength);

	//! Calls the force calculation step for \p G, \p A, \p E.
	/**
	 * If act_level is 0 and resizeDrawing is true the drawing is resized.
	 * Furthermore, the maximum number of force calc. steps is calculated
	 * depending on MaxIterChange, act_level, and max_level.
	 */
	void call_FORCE_CALCULATION_step (
		Graph& G,
		NodeArray<NodeAttributes>& A,
		EdgeArray<EdgeAttributes>& E,
		int act_level,
		int max_level);

	//! Calls the postprocessing step.
	void call_POSTPROCESSING_step(
		Graph& G,
		NodeArray<NodeAttributes>& A,
		EdgeArray<EdgeAttributes>& E,
		NodeArray<DPoint>& F,
		NodeArray<DPoint>& F_attr,
		NodeArray<DPoint>& F_rep,
		NodeArray<DPoint>& last_node_movement);

	//! @}
	//! \name Functions for pre- and post-processing
	//! @{

	//! All parameter options are set to the default values.
	void initialize_all_options();

	//! Updates several low level parameter options due to the settings of the high level parameter options.
	void update_low_level_options_due_to_high_level_options_settings();

	//! Imports for each node \a v of \p G its width, height and position (given from \p GA) in \p A.
	void import_NodeAttributes(
		const Graph& G,
		GraphAttributes& GA,
		NodeArray<NodeAttributes>& A);

	//! Imports for each edge e of G its desired length given via edgeLength.
	void import_EdgeAttributes (
		const Graph& G,
		const EdgeArray<double>& edgeLength,
		EdgeArray <EdgeAttributes>& E);

	//! Sets the individual ideal edge length for each edge \a e.
	void init_ind_ideal_edgelength(
		const Graph& G,
		NodeArray<NodeAttributes>&A,
		EdgeArray <EdgeAttributes>& E);

	//! The radii of the surrounding circles of the bounding boxes are computed.
	void set_radii(const Graph& G,NodeArray<NodeAttributes>& A);

	//! Exports for each node \a v in \p G_reduced the position of the original_node in \p GA.
	void export_NodeAttributes(
		Graph& G_reduced,
		NodeArray<NodeAttributes>& A_reduced,
		GraphAttributes& GA);

	//! Creates a simple and loopfree copy of \p G and stores the corresponding node / edge attributes.
	/**
	 * The corresponding node / edge attributes are stored in \p A_reduced and
	 * \p E_reduced; the links to the copy_node and original node are stored in \p A,
	 * \p A_reduced, too.
	 */
	void make_simple_loopfree(
		const Graph& G,
		NodeArray<NodeAttributes>& A,
		EdgeArray<EdgeAttributes>E,
		Graph& G_reduced,
		NodeArray<NodeAttributes>& A_reduced,
		EdgeArray<EdgeAttributes>& E_reduced);

	//! Deletes parallel edges of \p G_reduced.
	/**
	 * Saves for each set of parallel edges one representative edge in \p S and
	 * saves in \p new_edgelength the new edge length of this edge in \p G_reduced.
	 */
	void delete_parallel_edges(
		const Graph& G,
		EdgeArray<EdgeAttributes>& E,
		Graph& G_reduced,
		List<edge>& S,
		EdgeArray<double>& new_edgelength);

	//! Sets for each edge \a e of \a G_reduced in \p S its edgelength to \p new_edgelength[\a e].
	/**
	 * Also stores this information in \p E_reduced.
	 */
	void update_edgelength(
		List<edge>& S,
		EdgeArray <double>& new_edgelength,
		EdgeArray<EdgeAttributes>& E_reduced);

	//! Returns the value for the strength of the repulsive forces.
	/**
	 * Used in the postprocessing step; depending on \p n = G.numberOfNodes().
	 */
	double get_post_rep_force_strength(int n) {
		return min(0.2,400.0/double(n));
	}

	/**
	 * Adjust positions according to allowedPositions()
	 *
	 * \see FMMMOptions::AllowedPositions
	 */
	void adjust_positions(const Graph& G, NodeArray<NodeAttributes>& A);

	//! Creates a simple drawing of \p GA in postscript format and saves it in file \p ps_file.
	void create_postscript_drawing(GraphAttributes& GA, char* ps_file);

	//! @}
	//! \name Functions for divide et impera step
	//! @{

	//! Constructs the list of connected components of G.
	/**
	 * Also constructs the corresponding lists with the node / edge attributes
	 * (containing a pointer to the original node in \p G for each node in a subgraph).
	 */
	void create_maximum_connected_subGraphs(
		Graph& G,
		NodeArray<NodeAttributes>&A,
		EdgeArray<EdgeAttributes>&E,
		Graph G_sub[],
		NodeArray<NodeAttributes> A_sub[],
		EdgeArray<EdgeAttributes> E_sub[],
		NodeArray<int>& component);

	//! The drawings of the subgraphs are packed.
	/**
	 * This is done such that the subgraphs do not overlap and fit into a small
	 * box with the desired aspect ratio.
	 */
	void pack_subGraph_drawings(
		NodeArray<NodeAttributes>& A,
		Graph G_sub[],
		NodeArray<NodeAttributes> A_sub[]);

	//! The bounding rectangles of all connected componenents of \a G are calculated and stored in \p R.
	void  calculate_bounding_rectangles_of_components(
		List<Rectangle>& R,
		Graph  G_sub[],
		NodeArray<NodeAttributes> A_sub[]);

	//! The bounding rectangle of the componenet_index-th. component of G is returned.
	Rectangle calculate_bounding_rectangle(
		Graph& G,
		NodeArray<NodeAttributes>& A,
		int componenet_index);

	/**
	 * If number_of_components > 1, the subgraphs \p G_sub are rotated and skipped to
	 * find bounding rectangles with minimum area. The information is saved in \p R and
	 * the node positions in \p A_sub are updated. If number_of_components == 1 a rotation
	 * with minimal aspect ratio is found instead.
	 */
	void rotate_components_and_calculate_bounding_rectangles(
		List<Rectangle>&R,
		Graph G_sub[],
		NodeArray<NodeAttributes> A_sub[]);

	/**
	 * Returns the area (aspect ratio area) of a rectangle with width w and height h
	 * if comp_nr > 1 ( comp_nr == 1).
	 */
	double calculate_area(double width, double height, int comp_nr) {
		double scaling = 1.0;
		if (comp_nr == 1) {  //calculate aspect ratio area of the rectangle
			OGDF_ASSERT( height != 0.0 );
			double ratio = width / height;
			if (ratio < pageRatio()) { //scale width
				OGDF_ASSERT( ratio != 0.0 );
				scaling = pageRatio() / ratio;
			} else { //scale height
				OGDF_ASSERT( pageRatio() != 0.0 );
				scaling = ratio / pageRatio();
			}
		}
		return width * height * scaling;
	}

	/**
	 * The positions of the nodes in the subgraphs are calculated by using the
	 * information stored in R and are exported to A. (The coordinates of components
	 * which surrounding rectangles have been tipped over in the packing step are
	 * tipped over here,too)
	 */
	void export_node_positions(
		NodeArray<NodeAttributes>& A,
		List<Rectangle>& R,
		Graph G_sub[],
		NodeArray<NodeAttributes> A_sub[]);

	//! Frees dynamically allocated memory for the connected component subgraphs.
	void delete_all_subGraphs(
		Graph G_sub[],
		NodeArray<NodeAttributes> A_sub[],
		EdgeArray<EdgeAttributes> E_sub[])
	{
		delete[] G_sub;
		delete[] A_sub;
		delete[] E_sub;
	}

	//! @}
	//! \name Functions for multilevel step
	//! @{

	/**
	 * Returns the maximum number of iterations for the force calc. step depending
	 * on act_level, max_level, FixedIterations, MaxIterChange, MaxIterFactor,
	 * and the number of nodes of the Graph in the actual mutilevel.
	 */
	int get_max_mult_iter(int act_level, int max_level, int node_nr);

	//! @}
	//! \name Functions for force calculation
	//! @{

	//! The forces are calculated here.
	void calculate_forces(
		Graph& G,
		NodeArray<NodeAttributes>& A,
		EdgeArray<EdgeAttributes>& E,NodeArray<DPoint>& F,
		NodeArray<DPoint>& F_attr,
		NodeArray<DPoint>& F_rep,
		NodeArray<DPoint>& last_node_movement,
		int iter,
		int fine_tuning_step);

	//! The length of the computational box in the first iteration is set (down left corner is at (0,0).
	void init_boxlength_and_cornercoordinate(Graph& G,NodeArray<NodeAttributes>& A);

	//! The initial placements of the nodes are created by using initialPlacementForces().
	void create_initial_placement (Graph& G,NodeArray<NodeAttributes>& A);

	//! Places nodes uniformly in a grid.
	void create_initial_placement_uniform_grid(const Graph& G, NodeArray<NodeAttributes>& A);

	//! Places nodes randomly.
	void create_initial_placement_random(const Graph& G, NodeArray<NodeAttributes>& A);

	//! Sets all entries of \p F to (0,0).
	void  init_F (Graph& G, NodeArray<DPoint>& F);


	//! Make initializations for the data structures that are used in the choosen class for rep. force calculation.
	void make_initialisations_for_rep_calc_classes(
		Graph& G/*,
		NodeArray<NodeAttributes> &A,
		NodeArray<DPoint>& F_rep*/);

	//! Calculates repulsive forces for each node.
	void calculate_repulsive_forces(
		Graph &G,
		NodeArray<NodeAttributes>& A,
		NodeArray<DPoint>& F_rep)
	{
		switch (repulsiveForcesCalculation()) {
		case FMMMOptions::RepulsiveForcesMethod::Exact:
			FR.calculate_exact_repulsive_forces(G,A,F_rep);
			break;
		case FMMMOptions::RepulsiveForcesMethod::GridApproximation:
			FR.calculate_approx_repulsive_forces(G,A,F_rep);
			break;
		case FMMMOptions::RepulsiveForcesMethod::NMM:
			NM.calculate_repulsive_forces(G,A,F_rep);
			break;
		}
	}


	//! Deallocates dynamically allocated memory of the choosen rep. calculation class.
	void deallocate_memory_for_rep_calc_classes()
	{
		if(repulsiveForcesCalculation() == FMMMOptions::RepulsiveForcesMethod::NMM)
			NM.deallocate_memory();
	}

	//! Calculates attractive forces for each node.
	void calculate_attractive_forces(
		Graph& G,
		NodeArray<NodeAttributes> & A,
		EdgeArray<EdgeAttributes>& E,
		NodeArray<DPoint>& F_attr);

	//! Returns the attractive force scalar.
	double f_attr_scalar (double d,double ind_ideal_edge_length);

	//! Add attractive and repulsive forces for each node.
	void add_attr_rep_forces(
		Graph& G,
		NodeArray<DPoint>& F_attr,
		NodeArray<DPoint>& F_rep,
		NodeArray<DPoint>& F,
		int iter,
		int fine_tuning_step);

	//! Move the nodes.
	void move_nodes(Graph& G,NodeArray<NodeAttributes>& A,NodeArray<DPoint>& F);

	//! Computes a new tight computational square-box.
	/**
	 * (Guaranteeing, that all midpoints are inside the square.)
	 */
	void update_boxlength_and_cornercoordinate(Graph& G,NodeArray<NodeAttributes>& A);

	//! Describes the max. radius of a move in one time step, depending on the number of iterations.
	double max_radius(int iter) {
		return (iter == 1) ? boxlength/1000 : boxlength/5;
	}

	//! The average_ideal_edgelength for all edges is computed.
	void set_average_ideal_edgelength(Graph& G,EdgeArray<EdgeAttributes>& E);

	/**
	 * Calculates the average force on each node in the actual iteration, which is
	 * needed if StopCriterion is scThreshold() or scFixedIterationsOrThreshold().
	 */
	double get_average_forcevector_length (Graph& G, NodeArray<DPoint>& F);

	/**
	 * Depending on the direction of \p last_node_movement[\a v], the length of the next
	 * displacement of node \a v is restricted.
	 */
	void prevent_oscillations(
		Graph& G,
		NodeArray<DPoint>& F,
		NodeArray<DPoint>&
		last_node_movement,
		int iter);

	//! \p last_node_movement is initialized to \p F (used after first iteration).
	void init_last_node_movement(
		Graph& G,
		NodeArray<DPoint>& F,
		NodeArray<DPoint>& last_node_movement);

	/**
	 * If resizeDrawing is true, the drawing is adapted to the ideal average
	 * edge length by shrinking respectively expanding the drawing area.
	 */
	void adapt_drawing_to_ideal_average_edgelength(
		Graph& G,
		NodeArray<NodeAttributes>& A,
		EdgeArray<EdgeAttributes>& E);

	/**
	 * The force is restricted to have values within the comp. box (needed for
	 * exception handling, if the force is too large for further calculations).
	 */
	void restrict_force_to_comp_box(DPoint& force) {
		double x_min = down_left_corner.m_x;
		double x_max = down_left_corner.m_x+boxlength;
		double y_min = down_left_corner.m_y;
		double y_max = down_left_corner.m_y+boxlength;
		if (force.m_x < x_min )
			force.m_x = x_min;
		else if (force.m_x > x_max )
			force.m_x = x_max;
		if (force.m_y < y_min )
			force.m_y = y_min;
		else if (force.m_y > y_max )
			force.m_y = y_max;
	}

	//! @}
	//! \name Functions for analytic information
	//! @{

	//! Sets time_total to zero.
	void init_time() { time_total = 0; }

	//! @}
};

}
