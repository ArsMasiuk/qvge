/** \file
 * \brief Declaration of class UpSAT, which implements
 *        the upward-planarity testing formulations based on
 * 		  satisfiability (Chimani, Zeranski, 2012+)
 *
 * \author Robert Zeranski
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
#include <ogdf/basic/Graph_d.h>
#include <ogdf/basic/List.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/HashArray.h>
#include <ogdf/planarity/BoyerMyrvold.h>
#include <ogdf/external/Minisat.h>
#include <vector>

namespace ogdf {

class UpSAT {
public:
	//constructor
	explicit UpSAT(Graph &G);
	UpSAT(GraphCopy &G, bool feasibleOriginalEdges);
private:
	class Comp;
	//FLAGS
	bool feasibleOriginalEdges;
	//copy of the input graph
	Graph &m_G;
	//number of clauses and variables
	int numberOfVariables;
	long long numberOfClauses;
	//Indices of the nodes and edges of m_G
	NodeArray<int> N;
	EdgeArray<int> M;
	//Dominating edges-pairs
	EdgeArray< List<edge> > D;
	//Variables of the formulation(s)
	std::vector< std::vector<int> > tau;
	std::vector< std::vector<int> > sigma;
	std::vector< std::vector<int> > mu;
	//Formula
	Minisat::Formula m_F;
public:
	bool testUpwardPlanarity(NodeArray<int> *nodeOrder = nullptr);
	bool embedUpwardPlanar(adjEntry& externalToItsRight,NodeArray<int> *nodeOrder = nullptr);
	long long getNumberOfClauses();
	int getNumberOfVariables();
	void reset();
private:
	void computeDominatingEdges();
	void computeTauVariables();
	void computeSigmaVariables();
	void computeMuVariables();
	void ruleTauTransitive();
	void ruleSigmaTransitive();
	void ruleUpward();
	void rulePlanarity();
	void ruleTutte();
	void ruleFixed(const Minisat::Model &model);
	void sortBySigma(List<adjEntry> &adjList, const Minisat::Model &model);
	void embedFromModel(const Minisat::Model &model, adjEntry& externalToItsRight);
	bool OE(bool embed, adjEntry& externalToItsRight, NodeArray<int> *nodeOrder);
	bool FPSS(NodeArray<int> *nodeOrder); // cannot embed
	bool HL(bool embed, adjEntry& externalToItsRight, NodeArray<int> *nodeOrder);
	void writeNodeOrder(const Minisat::Model &model, NodeArray<int> *nodeOrder);
};

}
