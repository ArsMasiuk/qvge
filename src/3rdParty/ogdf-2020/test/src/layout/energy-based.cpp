/** \file
 * \brief Tests for several energy-based layout classes
 *
 * \author Carsten Gutwenger, Tilo Wiedera
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

#include <ogdf/energybased/DavidsonHarelLayout.h>
#include <ogdf/energybased/DTreeMultilevelEmbedder.h>
#include <ogdf/energybased/FastMultipoleEmbedder.h>
#include <ogdf/energybased/FMMMLayout.h>
#include <ogdf/energybased/GEMLayout.h>
#include <ogdf/energybased/MultilevelLayout.h>
#include <ogdf/energybased/NodeRespecterLayout.h>
#include <ogdf/energybased/PivotMDS.h>
#include <ogdf/energybased/SpringEmbedderFRExact.h>
#include <ogdf/energybased/SpringEmbedderGridVariant.h>
#include <ogdf/energybased/SpringEmbedderKK.h>
#include <ogdf/energybased/StressMinimization.h>
#include <ogdf/energybased/TutteLayout.h>

#include "layout_helpers.h"

#define TEST_ENERGY_BASED_LAYOUT(NAME, EXTRA_ATTR, ...) describeEnergyBasedLayout<NAME>(#NAME, EXTRA_ATTR, {__VA_ARGS__})

template<class T>
void init(T& layout) {
	// just keep the default
}

template<>
void init(DavidsonHarelLayout& layout) {
	layout.setNumberOfIterations(50);
}

template<>
void init(FastMultipoleEmbedder& layout) {
	layout.setNumIterations(50);
	layout.setNumberOfThreads(System::numberOfProcessors());
}

template<>
void init(FastMultipoleMultilevelEmbedder& layout) {
	layout.maxNumThreads(System::numberOfProcessors());
}

template<>
void init(FMMMLayout& layout) {
	layout.fixedIterations(50);
}

template<>
void init(GEMLayout& layout) {
	layout.numberOfRounds(50);
}

template<>
void init(NodeRespecterLayout& layout) {
	layout.setNumberOfIterations(50);
}

template<>
void init(SpringEmbedderFRExact& layout) {
	layout.iterations(50);
}

template<>
void init(SpringEmbedderGridVariant& layout) {
	layout.iterations(25);
	layout.iterationsImprove(25);
}

template<>
void init(StressMinimization& layout) {
	layout.setIterations(50);
}

template<class T>
void describeEnergyBasedLayout(const string &name, int extraAttr, std::initializer_list<GraphProperty> requirements) {
	T layout;
	init(layout);
	describeLayout(name, layout, extraAttr, requirements);
}

void describeFMMM() {
	TEST_ENERGY_BASED_LAYOUT(FMMMLayout, 0);

	FMMMLayout fmmm;

	fmmm.fixedIterations(50);
	fmmm.useHighLevelOptions(true);
	fmmm.qualityVersusSpeed(FMMMOptions::QualityVsSpeed::GorgeousAndEfficient);
	describeLayout("FMMMLayout with high quality settings", fmmm);

	fmmm.qualityVersusSpeed(FMMMOptions::QualityVsSpeed::NiceAndIncredibleSpeed);
	describeLayout("FMMMLayout with nice quality and incredible speed", fmmm);

	fmmm.allowedPositions(FMMMOptions::AllowedPositions::Exponent);
	fmmm.edgeLengthMeasurement(FMMMOptions::EdgeLengthMeasurement::Midpoint);
	fmmm.forceModel(FMMMOptions::ForceModel::Eades);
	fmmm.galaxyChoice(FMMMOptions::GalaxyChoice::UniformProb);
	fmmm.initialPlacementForces(FMMMOptions::InitialPlacementForces::UniformGrid);
	fmmm.maxIterChange(FMMMOptions::MaxIterChange::RapidlyDecreasing);
	fmmm.minGraphSize(10);
	fmmm.nmParticlesInLeaves(70);
	fmmm.nmSmallCell(FMMMOptions::SmallestCellFinding::Aluru);
	fmmm.nmTreeConstruction(FMMMOptions::ReducedTreeConstruction::PathByPath);
	fmmm.pageFormat(FMMMOptions::PageFormatType::Landscape);
	fmmm.presortCCs(FMMMOptions::PreSort::None);
	fmmm.stopCriterion(FMMMOptions::StopCriterion::FixedIterations);
	fmmm.tipOverCCs(FMMMOptions::TipOver::None);
	fmmm.useHighLevelOptions(false);
	describeLayout("FMMMLayout with very specific configuration (using NewMultipoleMethod))", fmmm);

	fmmm.allowedPositions(FMMMOptions::AllowedPositions::All);
	fmmm.forceModel(FMMMOptions::ForceModel::FruchtermanReingold);
	fmmm.galaxyChoice(FMMMOptions::GalaxyChoice::NonUniformProbHigherMass);
	fmmm.initialPlacementForces(FMMMOptions::InitialPlacementForces::RandomTime);
	fmmm.initialPlacementMult(FMMMOptions::InitialPlacementMult::Simple);
	fmmm.maxIterChange(FMMMOptions::MaxIterChange::Constant);
	fmmm.pageFormat(FMMMOptions::PageFormatType::Portrait);
	fmmm.presortCCs(FMMMOptions::PreSort::DecreasingWidth);
	fmmm.repulsiveForcesCalculation(FMMMOptions::RepulsiveForcesMethod::GridApproximation);
	fmmm.stopCriterion(FMMMOptions::StopCriterion::Threshold);
	fmmm.tipOverCCs(FMMMOptions::TipOver::Always);
	describeLayout("FMMMLayout with very specific configuration (using GridApproximation)", fmmm);
}

go_bandit([] { describe("Energy-based layouts", [] {
	TEST_ENERGY_BASED_LAYOUT(DavidsonHarelLayout, 0);

	TEST_ENERGY_BASED_LAYOUT(DTreeMultilevelEmbedder2D, 0, GraphProperty::connected);
	TEST_ENERGY_BASED_LAYOUT(DTreeMultilevelEmbedder3D, GraphAttributes::threeD, GraphProperty::connected);

	TEST_ENERGY_BASED_LAYOUT(FastMultipoleEmbedder, 0, GraphProperty::connected);
	TEST_ENERGY_BASED_LAYOUT(FastMultipoleMultilevelEmbedder, 0, GraphProperty::connected);

	describeFMMM();

	TEST_ENERGY_BASED_LAYOUT(GEMLayout, 0);

	TEST_ENERGY_BASED_LAYOUT(MultilevelLayout, 0);

	TEST_ENERGY_BASED_LAYOUT(NodeRespecterLayout, 0);

	TEST_ENERGY_BASED_LAYOUT(PivotMDS, 0, GraphProperty::connected);

	TEST_ENERGY_BASED_LAYOUT(SpringEmbedderFRExact, 0);

	TEST_ENERGY_BASED_LAYOUT(SpringEmbedderGridVariant, 0);

	TEST_ENERGY_BASED_LAYOUT(SpringEmbedderKK, 0, GraphProperty::connected);

	TEST_ENERGY_BASED_LAYOUT(StressMinimization, 0);

	TEST_ENERGY_BASED_LAYOUT(TutteLayout, 0, GraphProperty::triconnected, GraphProperty::planar, GraphProperty::simple);
}); });
