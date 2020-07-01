/** \file
 * \brief BalloonLayout for trees that can also be applied to
 * general graphs.
 *
 * Partially based on the papers by Lin/Yen and Carriere/Kazman
 *
 * \author Karsten Klein
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

//There are two radii at each node: The outer radius of the circle
//surrounding its subtree, and the inner radius of the circle
//on which the children of the node are placed
//For each angle assignment at a node p (parent), its own angle is
//used as offset, so that the children are correctly oriented

#include <ogdf/misclayout/BalloonLayout.h>


#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/Queue.h>


namespace ogdf {


BalloonLayout::BalloonLayout() :
	m_rootSelection(BalloonLayout::RootSelection::Center),  //how to select the root
	m_estimateFactor(1.2),        //weight of radius addition value
	m_childOrder(ChildOrder::Fixed),    //how to arrange the children
	m_treeComputation(BalloonLayout::TreeComputation::Bfs),  //spanning tree by...
	m_evenAngles(false)
{ }

BalloonLayout &BalloonLayout::operator=(const BalloonLayout &bl)
{
	m_treeComputation = bl.m_treeComputation;
	m_childOrder      = bl.m_childOrder;
	m_rootSelection   = bl.m_rootSelection;
	m_estimateFactor  = bl.m_estimateFactor;
	m_evenAngles      = bl.m_evenAngles;

	return *this;
}


void BalloonLayout::call(GraphAttributes &AG)
{
	const Graph &G = AG.constGraph();
	if(G.numberOfNodes() == 0) return;

	OGDF_ASSERT(isConnected(G));
#ifdef OGDF_DEBUG
	m_treeEdge.init(G, false);
#endif

	m_rootSelection = RootSelection::Center;
	m_treeComputation = TreeComputation::Bfs;
	m_childOrder = ChildOrder::Fixed;

	computeTree(G);
#ifdef OGDF_DEBUG
	AG.fillColor(m_treeRoot) = Color::Name::Mediumblue;
	Logger::slout() << "Treeroot is blue\n";
#endif

	// determine root of tree (m_root)
	m_root = m_treeRoot;
	selectRoot(G);
#ifdef OGDF_DEBUG
	AG.fillColor(m_root) = Color(0xaa, 0x00, 0xaa);
	for(edge e : G.edges)
	{
		if (m_treeEdge[e])
			AG.strokeColor(e) = Color(0xaa, 0x00, 0xaa);
	}
#endif

	computeRadii(AG);

	// computes m_angle[v]
	computeAngles(G);

	// computes final coordinates of nodes
	computeCoordinates(AG);
}


void BalloonLayout::selectRoot(const Graph &G)
//Todo: Vorgabewert durch user erlauben, firstNode?
{
#ifdef OGDF_DEBUG
	checkTree(G, true);
#endif

	switch(m_rootSelection)
	{

	case BalloonLayout::RootSelection::HighestDegree:
		{
			int maxDeg = -1;
			for(node v : G.nodes)
				if(v->degree() > maxDeg)
				{
					m_root = v;
					maxDeg = v->degree();
				}
		}
		break;

	case BalloonLayout::RootSelection::Center:
		{
			NodeArray<int> degree(G);
			Queue<node> leaves;

			if (G.numberOfNodes() == 1)
				leaves.append(G.firstNode());
			else
				for(node v : G.nodes) {
					degree[v] = m_childCount[v];
					if (m_parent[v] != nullptr) degree[v]++;
					if(degree[v] == 1)
						leaves.append(v);
			}

			node v = nullptr;
			while(!leaves.empty()) {
				v = leaves.pop();

				node p = m_parent[v];
				if (p != nullptr && --degree[p] == 1) {
					leaves.append(p);
				}
				ListConstIterator<node> it = m_childList[v].begin();
				while (it.valid())
				{
					if (--degree[*it] == 1)
						leaves.append(*it);
					++it;
				}
			}

			m_root = v;
			//we swap the parent relationship on the path
			//from m_treeRoot to m_root

			node u = m_root;
			v = nullptr;
			while (u != nullptr)
			{
				node w = m_parent[u];
				m_parent[u] = v;
				if (v != nullptr)
				{
					//may change the child order
					m_childCount[v]++;
					m_childList[v].pushBack(u);
				}
				if (w != nullptr)
				{
					m_childCount[w]--;
					ListIterator<node> it = m_childList[w].begin();
					while (it.valid())
					{
						if ((*it)==u)
						{
							m_childList[w].del(it);
							break;
						}
						++it;
					}
				}
				v = u;
				u = w;
			}
		}
		break;
	default: {Logger::slout() << m_rootSelection << "\n";
		throw AlgorithmFailureException(AlgorithmFailureCode::Unknown);} break;
	}

#ifdef OGDF_DEBUG
	checkTree(G, false);
#endif
}

#ifdef OGDF_DEBUG
void BalloonLayout::computeRadii(GraphAttributes &AG)
#else
void BalloonLayout::computeRadii(const GraphAttributes &AG)
#endif
{
	const Graph &G = AG.constGraph();
	m_radius.init(G, 0.0);
	m_oRadius.init(G, 0.0);
	m_estimate.init(G, 0.0);
	m_maxChildRadius.init(G, 0.0);
	m_size.init(G, 0.0);

	for(node u : G.nodes)
	{
		double w = AG.width(u);
		double h = AG.height(u);
		double t = 0.5*sqrt(w*w+h*h);
		m_size[u] = max(0.007, t); //assure we  don't have zero values, some default
	}

	Queue<node> level;

	switch (m_childOrder)
	{
		case ChildOrder::Optimized: //todo
		case ChildOrder::Fixed:
		{
			//compute radii in SNS model bottom up
			//for leaves we use the smallest enclosing circle radius
			//Using sqrt is quite slow, maybe an approximation will do
			//r = 0.5*sqrt(w*w + h*h)
			NodeArray<int> children(G);
			Queue<node> leaves;

			if (G.numberOfNodes() > 1)
			{
				for (node v : G.nodes) {
					if((children[v] = m_childCount[v]) == 0)
					{
						leaves.append(v);
						m_oRadius[v] = m_size[v];
					}
				}
				//kann man wieder zusammenfassen mit unterer Schleife,
				//da Berechnung geaendert
				while(!leaves.empty()) {
					node v = leaves.pop();

					node p = m_parent[v];

					if (p != nullptr)
					{
						double t = m_oRadius[v];
						//we sum up the outer radius values here at
						//the parent node to compute the estimate for
						//the inner radius later
						m_estimate[p] += t;
						//set the value for the largest child radius
						if (m_maxChildRadius[p] < t)
							m_maxChildRadius[p] = t;
						//if all children are processed, push p into queue
						if (--children[p] == 0)
						{
							//we processed all children of p
							//and can derive the radius value
							level.append(p);
						}
					}
					//inner radius estimate
					m_radius[v] =  m_oRadius[v];

					//Logger::slout() << "Child-Radius: " << m_radius[v] << std::endl;
				}
				while(!level.empty())
				{
#ifdef OGDF_DEBUG
					Logger::slout() << "Non-leaf node processed\n";
#endif
					node v = level.pop();

					//compute radii
					//inner radius estimate, outer currently holds sum of children
					//we add the node object size to the radii

					//even angles: just sum up size of largest child
					if (m_evenAngles)
					{
						m_radius[v] = max((m_maxChildRadius[v]/max(m_childCount[v], 1) +
									m_estimateFactor*2.0*(m_childCount[v]*m_maxChildRadius[v]))/
									(2*Math::pi), 2.0*m_size[v]);
#ifdef OGDF_DEBUG
						Logger::slout() <<"Even angles \n";
#endif
					}
					else
					{
						if (m_childCount[v] == 1)
						{
							m_radius[v] = max(2.0*m_size[v], 1.1*m_maxChildRadius[v]);
#ifdef OGDF_DEBUG
							AG.fillColor(v) = Color(0x33, 0x00, 0x00);

							Logger::slout() << "Radii with single child: "<< v->degree() <<" : "<<m_radius[v] << " "<< (m_maxChildRadius[v]/max(m_childCount[v], 4) +
									m_estimateFactor*2.0*m_estimate[v])/
									(2.0*Math::pi)<<" "<< 1.1*m_maxChildRadius[v]<<"\n";

#endif
						}
						else
						{
						//better version:
						m_radius[v] = max(max((m_maxChildRadius[v]/max(m_childCount[v], 4) +
									m_estimateFactor*2.0*m_estimate[v])/
									(2.0*Math::pi), 2*m_size[v]), 1.1*m_maxChildRadius[v]);
#ifdef OGDF_DEBUG
							Logger::slout() << "Radii: "<< v->degree() <<" : "<<m_radius[v] << " "<< (m_maxChildRadius[v]/max(m_childCount[v], 4) +
									m_estimateFactor*2.0*m_estimate[v])/
									(2.0*Math::pi)<<" "<< 1.1*m_maxChildRadius[v]<<"\n";
#endif
#if 0
						else
						m_radius[v] = max((m_maxChildRadius[v]/max(m_childCount[v], 2) +
									m_estimateFactor*2.0*m_estimate[v])/
									(2.0*Math::pi), 2*m_size[v]);
#endif
						}
					}

					//outer radius is inner radius + radius of largest child
					double t;
					//if there is only a single child, it will be placed
					//on the same ray, therefore we do not need to reserve
					//space for a circle (we only estimate the space needed
					//here by taking the maximum instead of computing the
					//real value, which would take the distance into account
					if (m_childCount[v] == 1)
					//parent radius should never be smaller!
					{
						t = max(m_radius[v], m_maxChildRadius[v]);
#ifdef OGDF_DEBUG
						if (m_radius[v]< m_maxChildRadius[v]) {
							Logger::slout() << "\n\nParent radius is smaller!" << std::endl;
						}
#endif
					}
					else
						t = m_radius[v] + m_maxChildRadius[v];

					//adjust parent values
					node p = m_parent[v];
					//Invariant: outer radius of children of p is already computed
					if (p != nullptr)
					{
						//we sum up the outer radius values here at
						//the parent node to compute the estimate for
						//the inner radius later
						m_estimate[p] += t;
						//set the value for the largest child radius
						if (m_maxChildRadius[p] < t)
							m_maxChildRadius[p] = t;
						//if all children are processed, push p into queue
						if (--children[p] == 0)
						{
							//we processed all children of p
							//and can derive the radius value
							level.append(p);
						}
					}
					m_oRadius[v] = t;
				}
			}
		}
		break;
	}
#ifdef OGDF_DEBUG
	checkTree(G, false);
#endif
}

void BalloonLayout::computeTree(const Graph &G)
{
	node v = G.firstNode();
	m_parent.init(G, nullptr);
	m_childCount.init(G, 0);
	m_childList.init(G);

	//if the graph is not a tree, compute some spanning tree
	//and store the corresponding pointers in m_parent
	switch (m_treeComputation)
	{
		case (TreeComputation::Dfs): //todo
		case (TreeComputation::BfsRandom): //todo
		case (TreeComputation::Bfs):
		{
			computeBFSTree(G, v);
		}
		break;
	}
}

void BalloonLayout::computeBFSTree(const Graph &G, node v)
{
	SListPure<node> bfsqueue;
	NodeArray<bool> marked(G, false);

	bfsqueue.pushBack(v);

	marked[v] =  true;

	m_treeRoot = v;

	node w;

	while (!bfsqueue.empty())
	{
		w = bfsqueue.popFrontRet();

		for(adjEntry adj : w->adjEntries) {

			edge e = adj->theEdge();
			node u = e->opposite(w);
			if (!marked[u])
			{
				m_parent[u] = w;
				m_childCount[w]++;
				bfsqueue.pushBack(u);
				m_childList[w].pushBack(u);

				marked[u] = true;
#ifdef OGDF_DEBUG
				m_treeEdge[e] = true;
#endif
			}
		}
	}
#ifdef OGDF_DEBUG
	checkTree(G, true);
#endif
}

#ifdef OGDF_DEBUG
void BalloonLayout::checkTree(const Graph &G, bool treeRoot)
{
	//check the tree
	//can each node be reached?
	int testchecker = 0;
	int listchecker = 0;
	SListPure<node> testqueue;
	testqueue.pushBack((treeRoot ? m_treeRoot : m_root));
	while (!testqueue.empty())
	{
		node z = testqueue.popFrontRet();
		testchecker++;
		ListConstIterator<node> it = m_childList[z].begin();
		while (it.valid() && (listchecker <= 2*G.numberOfNodes()))
		{
			listchecker++;
			testqueue.pushBack((*it));
			++it;
		}
	}
	OGDF_ASSERT(G.numberOfNodes() == testchecker);
}
#endif

void BalloonLayout::computeAngles(const Graph &G)
{
#ifdef OGDF_DEBUG
	checkTree(G, false);
	int checker = 0;
#endif

	m_angle.init(G, 0.0);
	node v;

	v = m_root;
	SListPure<node> queue;
	queue.pushBack(v);

	while (!queue.empty())
	{
		node p = queue.popFrontRet();

		//process children

		if (m_childCount[p] > 0)
		{
			double pestimate = m_estimate[p]; //the circumference estimate of the parent
			double fullAngle = 2.0*Math::pi;  //angle that has to be shared by children
			ListConstIterator<node> it = m_childList[p].begin();
			if (m_childCount[p] == 1)
			{
				m_angle[*it] = Math::pi;//not used currently, fixed to parent angle
				queue.pushBack(*it);
#ifdef OGDF_DEBUG
				checker++;
#endif
			}
			else
			{
				//The outer radius of ONE of the children may be larger
				//than half of the estimate, but we never assign more than
				//pi, therefore we have to make two runs to check and correct
				//this. If the outer radius is more than half of the parent's
				//estimate, we only assign pi
				if (!m_evenAngles)
				{
#ifdef OGDF_DEBUG
					bool checkMulti = false;
#endif
					ListConstIterator<node> it2 = it;
					while (it2.valid())
					{
						if (m_oRadius[*it2]/m_estimate[p]>0.501)
						{
#ifdef OGDF_DEBUG
							if (checkMulti) {
								Logger::slout() << "More than one large child vertex!\n";
							}
							checkMulti = true;
#endif
							pestimate = pestimate - m_oRadius[*it2];
							fullAngle = Math::pi;
#ifndef OGDF_DEBUG
							break;
#endif
						}
						++it2;
					}
				}
				while (it.valid())
				{
					v = (*it);
					++it;

#ifdef OGDF_DEBUG
					checker++;
#endif

					if (m_evenAngles)
					{
						m_angle[v] = Math::pi*2.0/m_childCount[p];
						queue.pushBack(v);
					}
					else
					{
						double anglesum = 0.0;
#if 0
						//erst alle Winkel aufaddieren und dann anteilig
						//auf 2pi bzw. 100%
						m_angle[v] = anglesum;
#endif

						queue.pushBack(v);

						//Anteil an Plazierungsradius des Parent
						//we use the diameter fraction of the estimate value
						double ratio = m_oRadius[v]/m_estimate[p];
						//restrict vertices to at most half of the space, otherwise.
						//there will be an overlap
						if (ratio > 0.501) anglesum = Math::pi;
						else anglesum = fullAngle*m_oRadius[v]/pestimate;
#if 0
						Logger::slout() << std::endl << "Amount: " << m_oRadius[v]/m_estimate[p] << " at children: " << m_childCount[p] << std::endl;
#endif

						m_angle[v] = anglesum;
#ifdef OGDF_DEBUG
						if (anglesum >Math::pi) {
							Logger::slout() << "Angle larger than pi!" << anglesum << "children" << m_childCount[p] << " full: "<< fullAngle << std::endl;
						}
#endif
#if 0
						Logger::slout() << "Set angle at " << v->index() << " " << m_angle[v] << std::endl;
#endif
					}

#if 0
					it++;
#endif
				}
			}
		}
	}
#ifdef OGDF_DEBUG
	Logger::slout() << "Checker computeAngles: " << checker << " Non: " << G.numberOfNodes() << std::endl;
#endif

	return;
}

void BalloonLayout::computeCoordinates(GraphAttributes &AG)
{
#if 0
	const Graph &G = AG.constGraph();
#endif
	node v;
	//place the nodes top down
	//first root
	v = m_root;
	AG.x(v) = 0.0;
	AG.y(v) = 0.0;

	SListPure<node> queue;
	queue.pushBack(v);
#ifdef OGDF_DEBUG
	Logger::slout() << "Processing queue" << std::endl;
#if 0
	for(node v : G.nodes)
	{
		Logger::slout() << "Angle " << v << " " << m_angle[v] << std::endl;
	}
#endif
#endif
	while (!queue.empty())
	{
		node p = queue.popFrontRet();
		double x = AG.x(p);
		double y = AG.y(p);
		//process children

#if 0
		Logger::slout() << "Pop Queue" << std::endl;
#endif
		if (m_childCount[p] > 0)
		{
			//we start at the parent's angle and skip half of the angle
			//of the last element
			double anglesum = fmod(m_angle[p]-Math::pi+
				m_angle[*m_childList[p].begin()]/2.0, 2.0*Math::pi);//0.0;
#if 0
			double sumchecker = 0.0;// debug value
			Logger::slout() << "Angle start offset: " << anglesum << std::endl;
#endif
			ListConstIterator<node> it = m_childList[p].begin();
			#ifdef OGDF_DEBUG
			AG.fillColor(*it) = Color(0xab, 0x00, 0x07);
			#endif
			//special case if only a single child: Same direction as parent
			if (m_childCount[p] == 1)
			{
				node w = (*it);
				queue.pushBack(w);
				anglesum = m_angle[p];
				m_angle[w] = anglesum;
				AG.x(w) = x+cos(anglesum)*m_radius[p];
				AG.y(w) = y+sin(anglesum)*m_radius[p];
#ifdef OGDF_DEBUG
				AG.fillColor(w) = Color(0x00, 0xab, 0x00);
#endif
			}
			else
			while (it.valid())
			{
#if 0
				Logger::slout() << "Next child" << std::endl;
#endif
				node w = (*it);
				queue.pushBack(w);

				++it;

				node z;
				if (it.valid()) z = (*it);
				else z =  (*m_childList[p].begin());

#if 0
				Logger::slout() << w << " " << w->degree() << " Angle: " << m_angle[w] << " anglesum " << anglesum << std::endl;
				Logger::slout() << "angles..." << anglesum << std::endl;
#endif

				AG.x(w) = x+cos(anglesum)*m_radius[p];

				AG.y(w) = y+sin(anglesum)*m_radius[p];

#if 0
				sumchecker += m_angle[w];
#endif
				double s = m_angle[w];
				//assign the direction to w to allow its children to use it
				m_angle[w] = anglesum;

				//z's value is the required angle, not the direction
				anglesum = fmod((anglesum + (s+m_angle[z])/2.0), 2.0*Math::pi);
#if 0
				Logger::slout() << "Finished..." << std::endl;
#endif
			}
#if 0
			Logger::slout() << std::endl << "Angle total sum: " << sumchecker << " Number of children: " << m_childCount[p] << std::endl << std::endl;
#endif
		}
	}
#ifdef OGDF_DEBUG
	AG.fillColor(m_treeRoot) = Color::Name::Mediumblue;
	//AG.fillColor(m_root) = "#BBCC00";
#endif
	AG.clearAllBends();
}

#ifdef OGDF_DEBUG
void BalloonLayout::check(Graph &G)
{
	checkTree(G, true);
	//TODO: check angle
}
#endif

std::ostream &operator<<(std::ostream &os, const BalloonLayout::RootSelection &rs) {
	switch (rs) {
		case BalloonLayout::RootSelection::Center:        os << "Center";        break;
		case BalloonLayout::RootSelection::HighestDegree: os << "HighestDegree"; break;
	}
	return os;
}

}
