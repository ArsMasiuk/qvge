/** \file
 * \brief Implements class CircularLayout
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


//#define OGDF_CIRCULAR_LAYOUT_LOGGING

#include <ogdf/misclayout/CircularLayout.h>
#include <ogdf/basic/GraphCopy.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/Queue.h>
#include <ogdf/basic/tuples.h>
#include <ogdf/packing/TileToRowsCCPacker.h>

#ifdef OGDF_CIRCULAR_LAYOUT_LOGGING
#include <ogdf/fileformats/GraphIO.h>
#endif

namespace ogdf {

double angleNormalize(double alpha)
{
	while(alpha < 0)
		alpha += 2*Math::pi;

	while(alpha >= 2*Math::pi)
		alpha -= 2*Math::pi;

	return alpha;
}

bool angleSmaller(double alpha, double beta)
{
	double alphaNorm = angleNormalize(alpha);
	double betaNorm  = angleNormalize(beta);

	double start = betaNorm-Math::pi;

	if(start >= 0) {
		return start < alphaNorm && alphaNorm < betaNorm;
	} else {
		return alphaNorm < betaNorm || alphaNorm >= start+2*Math::pi;
	}
}

double angleDistance(double alpha, double beta)
{
	double alphaNorm = angleNormalize(alpha);
	double betaNorm  = angleNormalize(beta);

	double dist = alphaNorm - betaNorm;
	if (dist < 0) dist += 2*Math::pi;

	return (dist <= Math::pi) ? dist : 2*Math::pi-dist;
}


void angleRangeAdapt(double sectorStart, double sectorEnd, double &start, double &length)
{
	double start1 = angleNormalize(sectorStart);
	double end1   = angleNormalize(sectorEnd);
	double start2 = angleNormalize(start);
	double end2   = angleNormalize(start+length);

	if(end1   < start1) end1   += 2*Math::pi;
	if(start2 < start1) start2 += 2*Math::pi;
	if(end2   < start1) end2   += 2*Math::pi;

	if(start2 > end1) start = start1;
	if(end2   > end1) start = angleNormalize(sectorEnd - length);
}

struct ClusterStructure
{
	explicit ClusterStructure(const Graph &G) : m_G(G), m_clusterOf(G) { }

	operator const Graph &() const { return m_G; }

	void initCluster(int nCluster, const Array<int> &parent);

	void sortChildren(int i,
		List<node> &nodes,
		Array<List<int> > &posList,
		Array<double> &parentWeight,
		Array<double> &dirFromParent,
		List<Tuple2<int,double> > &mainSiteWeights);

	int numberOfCluster() const { return m_nodesIn.size(); }

	void resetNodes(int clusterIdx, const List<node> &nodes);

	const Graph &m_G;
	Array<SList<node> > m_nodesIn;
	NodeArray<int>      m_clusterOf;
	List<int>           m_mainSiteCluster;

	Array<int>        m_parentCluster;
	Array<List<int> > m_childCluster;

	ClusterStructure(const ClusterStructure &) = delete;
	ClusterStructure &operator=(const ClusterStructure &) = delete;
};


void ClusterStructure::resetNodes(int clusterIdx, const List<node> &nodes)
{
	OGDF_ASSERT(m_nodesIn[clusterIdx].size() == nodes.size());

	SList<node> &list = m_nodesIn[clusterIdx];

	list.clear();

	ListConstIterator<node> it;
	for(it = nodes.begin(); it.valid(); ++it)
		list.pushBack(*it);
}


void ClusterStructure::initCluster(int nCluster, const Array<int> &parent)
{
	m_nodesIn      .init(nCluster);
	m_parentCluster.init(nCluster);
	m_childCluster .init(nCluster);

	for(node v : m_G.nodes)
		m_nodesIn[m_clusterOf[v]].pushBack(v);

	int i;
	for(i = 0; i < nCluster; ++i) {
		m_parentCluster[i] = parent[i];
		if(parent[i] != -1)
			m_childCluster[parent[i]].pushBack(i);
	}
}

void ClusterStructure::sortChildren(
	int i,
	List<node> &nodes,
	Array<List<int> > &posList,
	Array<double> &parentWeight,
	Array<double> &dirFromParent,
	List<Tuple2<int,double> > &mainSiteWeights)
{
	const int n = nodes.size();
	const int parent = m_parentCluster[i];

	if (parent != -1)
		posList[parent].clear();

	int pos = 0;
	ListConstIterator<node> it;
	for(it = nodes.begin(); it.valid(); ++it)
	{
		for(adjEntry adj : (*it)->adjEntries) {
			edge e = adj->theEdge();
			node w = e->opposite(*it);
			if (m_clusterOf[w] != i)
				posList[m_clusterOf[w]].pushBack(pos);
		}
		pos++;
	}

	List<Tuple2<int,double> > weights;

	// build list of all adjacent clusters (children + parent)
	List<int> adjClusters = m_childCluster[i];
	if (parent != -1)
		adjClusters.pushBack(parent);

	ListConstIterator<int> itC;
	for(itC = adjClusters.begin(); itC.valid(); ++itC)
	{
		int child = *itC;

		int size = posList[child].size();
		OGDF_ASSERT(size >= 1);
		if(size == 1) {
			weights.pushBack(Tuple2<int,double>(child,posList[child].front()));

		} else {
			// Achtung: Dieser Teil ist noch nicht richtig ausgetestet,da
			// bei Bic.comp. immer nur ein Knoten benachbart ist
			const List<int> &list = posList[child];
			int gapEnd    = list.front();
			int gapLength = list.front() - list.back() + n;

			int posPred = list.front();
			for(int j: list) {
				if (j - posPred > gapLength) {
					gapEnd    = j;
					gapLength = j - posPred;
				}
				posPred = j;
			}

			int x = (n - gapEnd) % n;

			int sum = 0;
			for(int j: list)
				sum += ((j + x) % n);

			double w = double(sum)/double(size);

			w -= x;
			if(w < 0) w += n;

			weights.pushBack(Tuple2<int,double>(child,w));
		}
	}

	using TheWeight = Tuple2<int, double>;
	weights.quicksort(GenericComparer<TheWeight, double>([](const TheWeight& w) { return w.x2(); }));
#ifdef OGDF_CIRCULAR_LAYOUT_LOGGING
	std::cout << "weights after: " << weights << std::endl;
#endif

	m_childCluster[i].clear();
	ListConstIterator<Tuple2<int,double> > itWeights;

	if(parent != -1) {
		// find list element containing parent cluster
		for(itWeights = weights.begin();
			(*itWeights).x1() != parent;
			itWeights = weights.cyclicSucc(itWeights)) { }

		parentWeight[i] = (*itWeights).x2();
		for(itWeights = weights.cyclicSucc(itWeights);
			(*itWeights).x1() != parent;
			itWeights = weights.cyclicSucc(itWeights))
		{
			m_childCluster[i].pushBack((*itWeights).x1());

			if(m_nodesIn[i].size() == 1)
				dirFromParent[(*itWeights).x1()] = Math::pi;
			else {
				double x = (*itWeights).x2() - parentWeight[i];
				if(x < 0) x += n;
				dirFromParent[(*itWeights).x1()] = x/n*2*Math::pi;
			}
		}

	} else {
		parentWeight[i] = 0;
		for(itWeights = weights.begin(); itWeights.valid(); ++itWeights)
		{
			m_childCluster[i].pushBack((*itWeights).x1());

			// not yet determined!
			dirFromParent[(*itWeights).x1()] = -1;
		}
		mainSiteWeights = weights;
	}
}


struct InfoAC
{
	node m_vBC, m_predCutBC, m_predCut;
	int m_parentCluster;

	InfoAC(node vBC, node predCutBC, node predCut, int parentCluster)
		: m_vBC(vBC), m_predCutBC(predCutBC), m_predCut(predCut), m_parentCluster(parentCluster) { }
};


class CircleGraph : public Graph
{
public:
	CircleGraph(const ClusterStructure &C, NodeArray<node> &toCircle, int c);

	void order(List<node> &nodeList);
	void swapping(List<node> &nodeList, int maxIterations);

	node fromCircle(node vCircle) const { return m_fromCircle[vCircle]; }

private:
	NodeArray<node> m_fromCircle;

	void dfs(
		NodeArray<int>  &depth,
		NodeArray<node> &father,
		node v,
		node f,
		int d);
};


CircleGraph::CircleGraph(
	const ClusterStructure &C,
	NodeArray<node> &toCircle,
	int c)
{
	m_fromCircle.init(*this);

	SListConstIterator<node> it;
	for(it = C.m_nodesIn[c].begin(); it.valid(); ++it)
	{
		node vCircle = newNode();
		toCircle    [*it]     = vCircle;
		m_fromCircle[vCircle] = *it;
	}

	for(it = C.m_nodesIn[c].begin(); it.valid(); ++it)
	{
		for(adjEntry adj : (*it)->adjEntries) {
			edge e = adj->theEdge();
			node w = e->target();
			if (w == *it) continue;

			if(C.m_clusterOf[w] == c)
				newEdge(toCircle[*it],toCircle[w]);
		}
	}
}


class DepthBucket : public BucketFunc<node>
{
public:
	explicit DepthBucket(const NodeArray<int> &depth) : m_depth(depth) { }

	int getBucket(const node &v) override
	{
		return -m_depth[v];
	}

	// undefined methods to avoid automatic creation
	DepthBucket(const DepthBucket &);
	DepthBucket &operator=(const DepthBucket &);

private:
	const NodeArray<int> &m_depth;
};


// Idee: Benutzung von outerplanarity (nachschlagen!)
void CircleGraph::order(List<node> &nodeList)
{
	NodeArray<int>  depth  (*this,0);
	NodeArray<node> father (*this);

	dfs(depth, father, firstNode(), nullptr, 1);

	SListPure<node> circleNodes;
	allNodes(circleNodes);

	DepthBucket bucket(depth);
	circleNodes.bucketSort(-numberOfNodes(),0,bucket);

	NodeArray<bool> visited(*this,false);

	ListIterator<node> itCombined;
	bool combinedAtRoot = false;

	SListConstIterator<node> it;
	for(it = circleNodes.begin(); it.valid(); ++it)
	{
		node v = *it;
		List<node> currentPath;

		ListIterator<node> itInserted;
		while(v != nullptr && !visited[v])
		{
			visited[v] = true;
			itInserted = currentPath.pushBack(v);
			v = father[v];
		}

		if(v && father[v] == nullptr && !combinedAtRoot) {
			combinedAtRoot = true;

			while(!currentPath.empty())
				currentPath.moveToSucc(currentPath.begin(),nodeList,itCombined);

		} else {
			if (v == nullptr)
				itCombined = itInserted;

			nodeList.conc(currentPath);
		}
	}
}

void CircleGraph::dfs(
	NodeArray<int>  &depth,
	NodeArray<node> &father,
	node v,
	node f,
	int d)
{
	if (depth[v] != 0)
		return;

	depth [v] = d;
	father[v] = f;

	for(adjEntry adj : v->adjEntries) {
		edge e = adj->theEdge();
		node w = e->opposite(v);
		if(w == f) continue;

		dfs(depth,father,w,v,d+1);
	}
}


void CircleGraph::swapping(List<node> &nodeList, int maxIterations)
{
	ListIterator<node> it;

	if (nodeList.size() >= 3)
	{
		NodeArray<int> pos(*this);
		const int n = numberOfNodes();

		int currentPos = 0;
		for(it = nodeList.begin(); it.valid(); ++it)
			pos[*it] = currentPos++;

		int iterations = 0;
		bool improvement;
		do {
			improvement = false;

			for(it = nodeList.begin(); it.valid(); ++it)
			{
				ListIterator<node> itNext = nodeList.cyclicSucc(it);

				node u = *it, v = *itNext;
				// we fake a numbering around the circle starting with u at pos. 0
				// using the formula: (pos[t]-offset) % n
				// and pos[u] + offset = n
				int offset = n - pos[u];

				// we count how many crossings we save when swapping u and v
				int improvementCrosings = 0;

				for(adjEntry adj : u->adjEntries) {
					edge ux = adj->theEdge();
					node x = ux->opposite(u);
					if(x == v) continue;

					int posX = (pos[x] + offset) % n;

					for(adjEntry adjV : v->adjEntries) {
						edge vy = adjV->theEdge();
						node y = vy->opposite(v);
						if (y == u || y == x) continue;

						int posY = (pos[y] + offset) % n;

						if(posX > posY)
							--improvementCrosings;
						else
							++improvementCrosings;
					}
				}

				if(improvementCrosings > 0) {
					improvement = true;
					std::swap(*it, *itNext);
					std::swap(pos[u], pos[v]);
				}
			}
		} while(improvement && ++iterations <= maxIterations);
	}

	for(it = nodeList.begin(); it.valid(); ++it)
		*it = m_fromCircle[*it];
}



CircularLayout::CircularLayout()
{
	// set options to defaults
	m_minDistCircle  = 20.0;
	m_minDistLevel   = 20.0;
	m_minDistSibling = 10.0;
	m_minDistCC      = 20.0;
	m_pageRatio      = 1.0;
}


// uses biconnected components as clusters
void CircularLayout::call(GraphAttributes &AG)
{
	const Graph &G = AG.constGraph();
	if(G.empty())
		return;

	// all edges straight-line
	AG.clearAllBends();

	GraphCopy GC;
	GC.createEmpty(G);

	// compute connected component of G
	NodeArray<int> component(G);
	int numCC = connectedComponents(G,component);

	// intialize the array of lists of nodes contained in a CC
	Array<List<node> > nodesInCC(numCC);

	for(node v : G.nodes)
		nodesInCC[component[v]].pushBack(v);

	EdgeArray<edge> auxCopy(G);
	Array<DPoint> boundingBox(numCC);

	int i;
	for(i = 0; i < numCC; ++i)
	{
		GC.initByNodes(nodesInCC[i],auxCopy);

		GraphAttributes AGC(GC);

		if(GC.numberOfNodes() == 1)
		{
			node v1 = GC.firstNode();
			AGC.x(v1) = AGC.y(v1) = 0;

		} else {
			// partition nodes into clusters
			// default uses biconnected components as cluster
			ClusterStructure C(GC);
			assignClustersByBiconnectedComponents(C);

			// call the actual layout algorithm with predefined clusters
			doCall(AGC,C);
		}

		node vFirst = GC.firstNode();
		double minX = AGC.x(vFirst), maxX = AGC.x(vFirst),
			minY = AGC.y(vFirst), maxY = AGC.y(vFirst);

		for(node vCopy : GC.nodes) {
			node v = GC.original(vCopy);
			AG.x(v) = AGC.x(vCopy);
			AG.y(v) = AGC.y(vCopy);

			Math::updateMin(minX, AG.x(v) - AG.width(v)/2);
			Math::updateMax(maxX, AG.x(v) + AG.width(v)/2);
			Math::updateMin(minY, AG.y(v) - AG.height(v)/2);
			Math::updateMax(maxY, AG.y(v) + AG.height(v)/2);
		}

		minX -= m_minDistCC;
		minY -= m_minDistCC;

		for(node vCopy : GC.nodes) {
			node v = GC.original(vCopy);
			AG.x(v) -= minX;
			AG.y(v) -= minY;
		}

		boundingBox[i] = DPoint(maxX - minX, maxY - minY);
	}

	Array<DPoint> offset(numCC);
	TileToRowsCCPacker packer;
	packer.call(boundingBox,offset,m_pageRatio);

	// The arrangement is given by offset to the origin of the coordinate
	// system. We still have to shift each node and edge by the offset
	// of its connected component.

	for(i = 0; i < numCC; ++i)
	{
		const double dx = offset[i].m_x;
		const double dy = offset[i].m_y;

		for(node v : nodesInCC[i])
		{
			AG.x(v) += dx;
			AG.y(v) += dy;
		}
	}
}


struct QueuedCirclePosition
{
	int m_cluster;
	double m_minDist;
	double m_sectorStart, m_sectorEnd;

	QueuedCirclePosition(int clusterIdx,
		double minDist,
		double sectorStart,
		double sectorEnd)
	{
		m_cluster = clusterIdx;
		m_minDist = minDist;
		m_sectorStart = sectorStart;
		m_sectorEnd = sectorEnd;
	}
};

struct ClusterRegion
{
	ClusterRegion(int c, double start, double length, double scaleFactor = 1.0)
	{
		m_start       = start;
		m_length      = length;
		m_scaleFactor = scaleFactor;
		m_clusters.pushBack(c);
	}

	double m_start, m_length, m_scaleFactor;
	SList<int> m_clusters;
};


struct SuperCluster
{
	SuperCluster(SList<int> &clusters,
		double direction,
		double length,
		double scaleFactor = 1.0)
	{
		m_direction   = direction;
		m_length      = length;
		m_scaleFactor = scaleFactor;
		m_cluster.conc(clusters);  // cluster is emtpy afterwards!
	}

	double m_direction, m_length, m_scaleFactor;
	SList<int> m_cluster;
};


using PtrSuperCluster = SuperCluster*;
std::ostream &operator<<(std::ostream &os, const PtrSuperCluster &sc)
{
	os << "{" << Math::radiansToDegrees(sc->m_direction) << ","
	          << Math::radiansToDegrees(sc->m_length) << ","
	          << sc->m_scaleFactor << ":" << sc->m_cluster << "}";
	return os;
}


struct SCRegion
{
	SCRegion(SuperCluster &sc) {
		m_length = sc.m_scaleFactor*sc.m_length;
		m_start  = angleNormalize(sc.m_direction - m_length/2);
		m_superClusters.pushBack(&sc);
	}

	double m_start, m_length;
	SList<SuperCluster*> m_superClusters;
};

void outputRegions(List<SCRegion> &regions)
{
	std::cout << "regions:\n";
	ListIterator<SCRegion> it;
	for(it = regions.begin(); it.valid(); ++it)
	{
		std::cout << "[" << (*it).m_superClusters << ", " <<
			Math::radiansToDegrees((*it).m_start) << ", " <<
			Math::radiansToDegrees((*it).m_length) << "]" << std::endl;
	}
}


// call for predefined clusters
// performs the actual layout algorithm
void CircularLayout::doCall(GraphAttributes &AG, ClusterStructure &C)
{
	// we consider currently only the case that we have a single main-site cluster
	OGDF_ASSERT(C.m_mainSiteCluster.size() == 1);

	// compute radii of clusters
	const int nCluster = C.numberOfCluster();
	Array<double> radius     (nCluster);
	Array<double> outerRadius(nCluster);

	int i;
	for(i = 0; i < nCluster; ++i)
	{
		const int n = C.m_nodesIn[i].size();

		double sumDiameters = 0, maxR = 0;
		SListConstIterator<node> it;
		for(it = C.m_nodesIn[i].begin(); it.valid(); ++it) {
			double d = sqrt(AG.width(*it) * AG.width(*it) + AG.height(*it) * AG.height(*it));
			sumDiameters += d;
			if (d/2 > maxR) maxR = d/2;
		}

		if(n == 1) {
			radius     [i] = 0;
			outerRadius[i] = maxR;

		} else if (n == 2) {
			radius     [i] = 0.5*m_minDistCircle + sumDiameters / 4;
			outerRadius[i] = 0.5*m_minDistCircle + sumDiameters / 2;

		} else {
			radius     [i] = (n*m_minDistCircle + sumDiameters) / (2*Math::pi);
			outerRadius[i] = radius[i] + maxR;
		}

#ifdef OGDF_CIRCULAR_LAYOUT_LOGGING
		std::cout << "radius of       " << i << " = " << radius[i] << std::endl;
		std::cout << "outer radius of " << i << " = " << outerRadius[i] << std::endl;
#endif
	}


	int mainSite = C.m_mainSiteCluster.front();

	NodeArray<node> toCircle(C);

	Queue<int> queue;
	queue.append(mainSite);

	Array<List<int> > posList      (nCluster);
	Array<double>     parentWeight (nCluster);
	Array<double>     dirFromParent(nCluster);
	List<Tuple2<int,double> > mainSiteWeights;

	while(!queue.empty())
	{
		int clusterIdx = queue.pop();

		CircleGraph GC(C, toCircle, clusterIdx);

		// order nodes on circle
		List<node> nodes;
		GC.order(nodes);
		GC.swapping(nodes,50);
		C.resetNodes(clusterIdx, nodes);
#ifdef OGDF_CIRCULAR_LAYOUT_LOGGING
		std::cout << "after swapping of " << clusterIdx << ": " << nodes << std::endl;
#endif

		C.sortChildren(clusterIdx, nodes, posList, parentWeight, dirFromParent, mainSiteWeights);
#ifdef OGDF_CIRCULAR_LAYOUT_LOGGING
		std::cout << "child cluster of " << clusterIdx << ": " << C.m_childCluster[clusterIdx] << std::endl;
#endif

		// append all children of cluster to queue
		ListConstIterator<int> itC;
		for(itC = C.m_childCluster[clusterIdx].begin(); itC.valid(); ++itC)
			queue.append(*itC);
	}

	// compute positions of circles
	Array<double> preferedAngle(nCluster);
	Array<double> preferedDirection(nCluster);
	computePreferedAngles(C,outerRadius,preferedAngle);

	Array<double> circleDistance(nCluster);
	Array<double> circleAngle(nCluster);

	circleDistance[mainSite] = 0;
	circleAngle[mainSite] = 0;

	Queue<QueuedCirclePosition> circleQueue;
	// sectors are assigned to children weighted by the prefered angles
	double sumPrefAngles = 0;
	double sumChildrenLength = 0;
	ListConstIterator<int> itC;
	for(itC = C.m_childCluster[mainSite].begin(); itC.valid(); ++itC) {
		sumPrefAngles += preferedAngle[*itC];
		sumChildrenLength += 2*outerRadius[*itC]+m_minDistSibling;
	}

	// estimation for distance of child cluster
	double rFromMainSite = max(m_minDistLevel+outerRadius[mainSite],
		sumChildrenLength / (2 * Math::pi));
	// estiamtion for maximal allowed angle (which is 2*maxHalfAngle)
	double maxHalfAngle = acos(outerRadius[mainSite] / rFromMainSite);

	// assignment of angles around main-site with pendulum method

	// initialisation
	double minDist   = outerRadius[mainSite] + m_minDistLevel;
	List<SuperCluster>  superClusters;
	List<SCRegion>      regions;
#if 0
	Array<double> scaleFactor(nCluster);
#endif
	ListConstIterator<Tuple2<int,double> > it;
	for(it = mainSiteWeights.begin(); it.valid(); )
	{
		double currentWeight    = (*it).x2();
		double currentDirection = currentWeight*2*Math::pi/C.m_nodesIn[mainSite].size();
		double sumLength = 0;
		SList<int> currentClusters;

		do {
			int    child  = (*it).x1();
#if 0
			double weight = (*it).x2();
#endif

			preferedDirection[child] = currentDirection;
			currentClusters.pushBack(child);
			sumLength += preferedAngle[child];

			++it;
		} while (it.valid() && (*it).x2() == currentWeight);

		ListIterator<SuperCluster> itSC = superClusters.pushBack(
			SuperCluster(currentClusters,currentDirection,sumLength,
			(sumLength <= 2*maxHalfAngle) ? 1.0 : 2*maxHalfAngle/sumLength));

		regions.pushBack(SCRegion(*itSC));
	}

#ifdef OGDF_CIRCULAR_LAYOUT_LOGGING
	outputRegions(regions);
#endif

	// merging of regions
	bool changed;
	do {
		changed = false;

		ListIterator<SCRegion> itR1, itR2, itR3, itRNext;
		for(itR1 = regions.begin(); itR1.valid() && regions.size() >= 2; itR1 = itRNext)
		{
			itRNext = itR1.succ();

			itR2 = itR1.succ();
			bool finish = !itR2.valid();
			bool doMerge = false;

			if(!itR2.valid()) {
				itR2 = regions.begin();
				double alpha = angleNormalize((*itR1).m_start + 2*Math::pi);
				double beta = angleNormalize((*itR2).m_start);
				double dist = beta - alpha;
				if (dist < 0) dist += 2*Math::pi;
				double dx = (*itR1).m_length - dist;
#if 1
				doMerge = dx > DBL_EPSILON;
#else
				doMerge = dist < (*itR1).m_length;
#endif
			} else {
				double alpha = angleNormalize((*itR1).m_start);
				double beta = angleNormalize((*itR2).m_start);
				double dist = beta - alpha;
				if (dist < 0) dist += 2*Math::pi;
				double dx = (*itR1).m_length - dist;
#if 1
				doMerge = dx > DBL_EPSILON;
#else
				doMerge = dist < (*itR1).m_length;
#endif
			}

			if(!doMerge) continue;

			do {
				(*itR1).m_superClusters.conc((*itR2).m_superClusters);

				if(finish) {
					regions.del(itR2);
					break;
				}

				itR3 = itR2.succ();
				finish = !itR3.valid();
				doMerge = false;

				if(!itR3.valid()) {
					itR3 = regions.begin();
					double beta = angleNormalize((*itR3).m_start + 2*Math::pi);
					double alpha = angleNormalize((*itR2).m_start);
					double dist = beta - alpha;
					if (dist < 0) dist += 2*Math::pi;
					double dx = (*itR2).m_length - dist;
					doMerge = dx > DBL_EPSILON;
					//doMerge = dist < (*itR2).m_length;

				} else {
					double beta = angleNormalize((*itR3).m_start);
					double alpha = angleNormalize((*itR2).m_start);
					double dist = beta - alpha;
					if (dist < 0) dist += 2*Math::pi;
					double dx = (*itR2).m_length - dist;
					doMerge = dx > DBL_EPSILON;
					//doMerge = dist < (*itR2).m_length;
				}

				itRNext = itR2.succ();
				regions.del(itR2);

				itR2 = itR3;

			} while(regions.size() >= 2 && doMerge);

			double sectorStart = 0;
			double sectorEnd = 2*Math::pi;
			bool singleRegion = true;

			if (regions.size() != 1) {
				singleRegion = false;
				sectorEnd = angleNormalize((*regions.cyclicSucc(itR1)).m_start);
				ListIterator<SCRegion> itPred = regions.cyclicPred(itR1);
				sectorStart = angleNormalize((*itPred).m_start + (*itPred).m_length);
			}
			double sectorLength = sectorEnd - sectorStart;
			if (sectorLength < 0) {
				sectorLength += 2*Math::pi;
			}

			changed = true;
			//compute deflection of R1
			double sumLength = 0, maxGap = -1;
			SListConstIterator<SuperCluster*> iter, itStartRegion;
			const SList<SuperCluster*> &superClustersR1 = (*itR1).m_superClusters;
			for(iter = superClustersR1.begin(); iter.valid(); ++iter)
			{
				sumLength += (*iter)->m_length;

				SListConstIterator<SuperCluster*> itSucc = superClustersR1.cyclicSucc(iter);
				double gap = (*itSucc)->m_direction - (*iter)->m_direction;
				if (gap < 0) gap += 2*Math::pi;
				if(gap > maxGap) {
					maxGap = gap; itStartRegion = itSucc;
				}
			}

			// compute scaling
			double scaleFactor = (sumLength <= sectorLength) ? 1 : sectorLength/sumLength;

			double sumWAngles = 0;
			double sumDef    = 0;
			(*itR1).m_start = (*itStartRegion)->m_direction - scaleFactor * (*itStartRegion)->m_length/2;
			double posStart  = (*itR1).m_start;
			iter = itStartRegion;
			do
			{
				double currentLength = scaleFactor * (*iter)->m_length;
				sumDef    += angleDistance((*iter)->m_direction, posStart + currentLength/2);
				posStart  += currentLength;

				double currentPos = (*iter)->m_direction;
				if (currentPos < (*itR1).m_start)
					currentPos += 2*Math::pi;
				sumWAngles += (*iter)->m_length * currentPos;

				iter = superClustersR1.cyclicSucc(iter);
			} while(iter != itStartRegion);

			double deflection = sumDef / (*itR1).m_superClusters.size();
			while(deflection < -Math::pi) deflection += 2*Math::pi;
			while(deflection >  Math::pi) deflection -= 2*Math::pi;

			(*itR1).m_start += deflection;

			double center = sumWAngles / sumLength;
			while(center < 0   ) center += 2*Math::pi;
			while(center > 2*Math::pi) center -= 2*Math::pi;

			double tmpScaleFactor = scaleFactor;
			double left = center - tmpScaleFactor*sumLength/2;
			for(iter = (*itR1).m_superClusters.begin(); iter.valid(); ++iter)
			{
				if(left < center) {
					double minLeft = (*iter)->m_direction-maxHalfAngle;
					if(angleSmaller(left, minLeft)) {
						Math::updateMin(scaleFactor,
							tmpScaleFactor * angleDistance(minLeft,center) / angleDistance(left,center));
					}
					OGDF_ASSERT(scaleFactor > 0);
				}

				double right = left + tmpScaleFactor*(*iter)->m_length;

				if(right > center) {
					double maxRight = (*iter)->m_direction+maxHalfAngle;
					if(angleSmaller(maxRight, right)) {
						Math::updateMin(scaleFactor,
							tmpScaleFactor * angleDistance(maxRight,center) / angleDistance(right,center));
					}
					OGDF_ASSERT(scaleFactor > 0);
				}

				double currentLength = right-left;
				if(currentLength < 0) currentLength += 2*Math::pi;
				if(currentLength > 2*maxHalfAngle)
					Math::updateMin(scaleFactor, 2*maxHalfAngle/currentLength);

				left = right;
			}

			OGDF_ASSERT(scaleFactor > 0);

			// set scale factor for all super clusters in region
			if(!singleRegion) itStartRegion = superClustersR1.begin();
			ListIterator<SCRegion> itFirst;
			iter = itStartRegion;
			do
			{
				(*iter)->m_scaleFactor = scaleFactor;

				// build new region for each super-cluster in R1
				ListIterator<SCRegion> itInserted =
					regions.insertBefore(SCRegion(*(*iter)),itR1);

				if(!singleRegion) {
					angleRangeAdapt(sectorStart, sectorEnd,
						(*itInserted).m_start, (*itInserted).m_length);
				}

				(*itInserted).m_start = angleNormalize((*itInserted).m_start);

				if(!itFirst.valid())
					itFirst = itInserted;

				iter = superClustersR1.cyclicSucc(iter);
			} while(iter != itStartRegion);

			// merge regions
			bool changedInternal;
			do {
				changedInternal = false;

				ListIterator<SCRegion> itA = (singleRegion) ? regions.begin() : itFirst,itB;
				bool finished = false;
				itB = itA.succ();
				for( ; ; )
				{
					if(itB == itR1) {
						if(singleRegion) {
							itB = regions.begin();
							if (itA == itB) break;
							finished = true;
						} else break;
					}

					if(angleSmaller((*itB).m_start, (*itA).m_start + (*itA).m_length))
					{
						(*itA).m_superClusters.conc((*itB).m_superClusters);
						(*itA).m_length += (*itB).m_length;

						//compute deflection of RA
						sumDef    = 0;
						posStart  = (*itA).m_start;
						SListConstIterator<SuperCluster*> itSuperCluster;
						for(itSuperCluster = (*itA).m_superClusters.begin(); itSuperCluster.valid(); ++itSuperCluster) {
							double currentDef = (*itSuperCluster)->m_direction - (posStart + (*itSuperCluster)->m_scaleFactor * (*itSuperCluster)->m_length/2);
							if(currentDef > Math::pi) currentDef -= 2*Math::pi;
							if(currentDef < -Math::pi) currentDef += 2*Math::pi;
							sumDef    += currentDef; //(*it)->m_direction - (posStart + (*it)->m_length/2);
							posStart  += (*itSuperCluster)->m_length * (*itSuperCluster)->m_scaleFactor;
						}
						deflection = sumDef / (*itA).m_superClusters.size();
						(*itA).m_start += deflection;
						(*itA).m_start = angleNormalize((*itA).m_start);

						if(!singleRegion) {
							angleRangeAdapt(sectorStart, sectorEnd,
								(*itA).m_start, (*itA).m_length);
						}

						(*itA).m_start = angleNormalize((*itA).m_start);

						regions.del(itB);
						changedInternal = true;

					} else {
						itA = itB;
					}

					if(finished) break;

					itB = itA.succ();
				}
			} while(changedInternal);

			regions.del(itR1);

#ifdef OGDF_CIRCULAR_LAYOUT_LOGGING
			outputRegions(regions);
#endif
		}
	} while(changed);

#if 0
		ListIterator<ClusterRegion> itR1 = regions.begin(),itR2;
		for(itR2 = itR1.succ(); true; itR2 = itR1.succ())
		{
			if(regions.size() == 1)
				break;

			bool finish = !itR2.valid();
			bool doMerge = false;

			if(!itR2.valid()) {
				itR2 = regions.begin();
				doMerge = (*itR2).m_start + 2*Math::pi < (*itR1).m_start + (*itR1).m_length;
			} else
				doMerge = (*itR2).m_start < (*itR1).m_start + (*itR1).m_length;

			if (doMerge)
			{
				(*itR1).m_clusters.conc((*itR2).m_clusters);
				//(*itR1).m_length += (*itR2).m_length; // sp?ter bestimmen

				//compute deflection of R1
				double sumPrefAngles = 0;
				double sumDef = 0;
				double posStart = (*itR1).m_start;
				SListConstIterator<int> it;
				for(it = (*itR1).m_clusters.begin(); it.valid(); ++it)
				{
					sumPrefAngles += preferedAngle[*it];
					sumDef += preferedDirection[*it] - (posStart + preferedAngle[*it]/2);
					posStart += preferedAngle[*it];
				}
				double deflection = sumDef / (*itR1).m_clusters.size();
				(*itR1).m_start += deflection;

				regions.del(itR2);
				changed = true;

				// compute scaling
				double scaleFactor = (sumPrefAngles <= 2*Math::pi) ? 1 : 2*Math::pi/sumPrefAngles;
				double left = (*itR1).m_start;
				double center = left + sumPrefAngles/2;
				for(it = (*itR1).m_clusters.begin(); it.valid(); ++it)
				{
					double minLeft = preferedDirection[*it]-maxHalfAngle;

					if(left < minLeft) {
						scaleFactor =
							(preferedDirection[*it]-maxHalfAngle-center)/(left-center);
					}

					left += preferedAngle[*it]; // "left" is now "right" for this cluster

					double maxRight = preferedDirection[*it]+maxHalfAngle;
					if(maxRight < left) {
						scaleFactor =
							(preferedDirection[*it]+maxHalfAngle-center)/(left-center);
					}
				}

				(*itR1).m_length = scaleFactor * sumPrefAngles;
				(*itR1).m_start  = center - (*itR1).m_length / 2;
				(*itR1).m_scaleFactor = scaleFactor;
				if((*itR1).m_start > 2*Math::pi)
					(*itR1).m_start -= 2*Math::pi;

#ifdef OGDF_CIRCULAR_LAYOUT_LOGGING
				outputRegions(regions);
#endif


			} else {
				itR1 = itR2;
			}

			if(finish) break;
		}
	} while(changed);
#endif

	ListIterator<SCRegion> itR;
#if 0
	double minDist   = outerRadius[mainSite] + m_minDistLevel;
	double sectorEnd = posStart+2*Math::pi;
#endif
	for(itR = regions.begin(); itR.valid(); ++itR)
	{
		double posStart  = (*itR).m_start;

		SListConstIterator<SuperCluster*> itSC;
		for(itSC = (*itR).m_superClusters.begin(); itSC.valid(); ++itSC)
		{
			double scaleFactor = (*itSC)->m_scaleFactor;

			SListConstIterator<int> iter;
			for(iter = (*itSC)->m_cluster.begin(); iter.valid(); ++iter)
			{
				double length = scaleFactor * preferedAngle[*iter];

				circleAngle[*iter] = posStart + length/2;
				circleQueue.append(QueuedCirclePosition(
					*iter,minDist,posStart,posStart+length));

				posStart += length;
			}
		}
	}

#if 0
		double posRegionEnd = R1.m_start;

		SListConstIterator<int> it;
		for(it = R1.m_clusters.begin(); it.valid(); ++it)
		{
			posRegionEnd += R1.m_scaleFactor*preferedAngle[*it];
			if(iter.valid() && iter.succ().valid())
			{
				circleQueue.append(QueuedCirclePosition(
					*it,minDist,posStart,posRegionEnd));
				circleAngle[*it] = posRegionEnd - R1.m_scaleFactor*preferedAngle[*it]/2;

				posStart = posRegionEnd;

			} else {
				itR2 = itR1.succ();
				circleAngle[*it] = posRegionEnd - R1.m_scaleFactor*preferedAngle[*it]/2;

				if(itR2.valid()) {
					double gap = (*itR2).m_start - posRegionEnd;
					posRegionEnd += gap * R1.m_scaleFactor*preferedAngle[*it] /
						(R1.m_scaleFactor*preferedAngle[*it] + (*itR2).m_scaleFactor*preferedAngle[(*itR2).m_clusters.front()]);
					circleQueue.append(QueuedCirclePosition(
						*it,minDist,posStart,posRegionEnd));
					posStart = posRegionEnd;
				} else {
					circleQueue.append(QueuedCirclePosition(
						*it,minDist,posStart,sectorEnd));
				}
			}
		}
	}
#endif
	// end of pendulum method


#if 0
	double completeAngle = 2 * Math::pi;
	double angle = 0;
	//double minDist = outerRadius[mainSite] + m_minDistLevel;//
	//ListConstIterator<Tuple2<int,double> > it;//
	double sum = 0;
	for(it = mainSiteWeights.begin(); it.valid(); ++it)
	{
		int    child  = (*it).x1();
		double weight = (*it).x2();

		double delta = completeAngle * preferedAngle[child] / sumPrefAngles;

		//double gammaC = angle+delta/2 - weight*2*Math::pi/C.m_nodesIn[mainSite].size();
		double gammaC = circleAngle[child] - weight*2*Math::pi/C.m_nodesIn[mainSite].size();

		sum += gammaC;

#if 0
		circleAngle[child] = angle + delta/2;
		circleQueue.append(QueuedCirclePosition(child,minDist,angle,angle+delta));
#if 0
		circleQueue.append(QueuedCirclePosition(child,minDist,
			circleAngle[child]-realDelta/2,circleAngle[child]+realDelta/2));
#endif
#endif
		angle += delta;
	}

	double gammaMainSite = (mainSiteWeights.size() == 0) ? 0 : sum / mainSiteWeights.size();
	if(gammaMainSite < 0) gammaMainSite += 2*Math::pi;
#endif
	double gammaMainSite = 0;

	while(!circleQueue.empty())
	{
		QueuedCirclePosition qcp = circleQueue.pop();
		int clusterIdx = qcp.m_cluster;

#ifdef OGDF_CIRCULAR_LAYOUT_LOGGING
		std::cout << "cluster = " << clusterIdx << ", start = " << Math::radiansToDegrees(qcp.m_sectorStart) <<
			", end = " << Math::radiansToDegrees(qcp.m_sectorEnd) << std::endl;
		std::cout << "  minDist = " << qcp.m_minDist <<
			", angle = " << Math::radiansToDegrees(circleAngle[clusterIdx]) << std::endl;
#endif

		double delta = qcp.m_sectorEnd - qcp.m_sectorStart;
#if 1
		if (delta >= Math::pi)
#else
		if (delta <= Math::pi)
#endif
		{
			circleDistance[clusterIdx] = qcp.m_minDist + outerRadius[clusterIdx];

		} else {
			double rMin = (outerRadius[clusterIdx] + m_minDistSibling/2) /
				(sin(delta/2));

			circleDistance[clusterIdx] = max(rMin,qcp.m_minDist+outerRadius[clusterIdx]);
		}

		if(C.m_childCluster[clusterIdx].empty())
			continue;

		minDist = circleDistance[clusterIdx] + outerRadius[clusterIdx] + m_minDistLevel;
#if 0
		double alpha = acos((circleDistance[clusterIdx]-outerRadius[clusterIdx])/minDist);

		if(circleAngle[clusterIdx]-alpha > qcp.m_sectorStart)
			qcp.m_sectorStart = circleAngle[clusterIdx]-alpha;
		if(circleAngle[clusterIdx]+alpha < qcp.m_sectorEnd)
			qcp.m_sectorEnd = circleAngle[clusterIdx]+alpha;
#endif
		delta = qcp.m_sectorEnd - qcp.m_sectorStart;

		sumPrefAngles = 0;
		for(itC = C.m_childCluster[clusterIdx].begin(); itC.valid(); ++itC)
		{
			sumPrefAngles += preferedAngle[*itC];

			// computing prefered directions
			double r = circleDistance[clusterIdx];
			double a = minDist + outerRadius[*itC];
			double gamma = dirFromParent[*itC];

#ifdef OGDF_CIRCULAR_LAYOUT_LOGGING
			std::cout << "    gamma of " << *itC << " = " << Math::radiansToDegrees(gamma) << std::endl;
#endif
			if(gamma <= Math::pi/2)
				preferedDirection[*itC] = qcp.m_sectorStart;
			else if(gamma >= 3*Math::pi/2)
				preferedDirection[*itC] = qcp.m_sectorEnd;
#if 0
			else if(gamma == Math::pi) // Achtung! nicht Gleichheit ohne Toleranz testen!
#endif
			else if(OGDF_GEOM_ET.equal(gamma,Math::pi))
				preferedDirection[*itC] = circleAngle[clusterIdx];
			else {
				double gamma2 = (gamma < Math::pi) ? Math::pi - gamma : gamma - Math::pi;
				double K = 1 + 1 /(tan(gamma2)*tan(gamma2));
				double newC = r/(a*tan(gamma2))/K;
				double C2 = sqrt((1-(r/a)*(r/a))/K + newC*newC);

				double beta = asin(C2-newC);
				if (gamma < Math::pi)
					preferedDirection[*itC] = circleAngle[clusterIdx]-beta;
				else
					preferedDirection[*itC] = circleAngle[clusterIdx]+beta;
			}
#ifdef OGDF_CIRCULAR_LAYOUT_LOGGING
			std::cout << "    dir. of  " << *itC << ": " << Math::radiansToDegrees(preferedDirection[*itC]) << std::endl;
#endif
		}

		if(sumPrefAngles >= delta)
		{
			double angle = qcp.m_sectorStart;
			for(itC = C.m_childCluster[clusterIdx].begin(); itC.valid(); ++itC)
			{
				double deltaChild = delta * preferedAngle[*itC] / sumPrefAngles;

				circleAngle[*itC] = angle + deltaChild/2;
				circleQueue.append(QueuedCirclePosition(*itC,minDist,angle,angle+deltaChild));
				angle += deltaChild;
			}

		} else {
			List<ClusterRegion> clusterRegions;
			for(itC = C.m_childCluster[clusterIdx].begin(); itC.valid(); ++itC)
			{
				double start  = preferedDirection[*itC]-preferedAngle[*itC]/2;
				double length = preferedAngle[*itC];

				if(start < qcp.m_sectorStart)
					start = qcp.m_sectorStart;
				if(start + length >  qcp.m_sectorEnd)
					start = qcp.m_sectorEnd - length;

				clusterRegions.pushBack(ClusterRegion(*itC,start,length));
			}

			bool somethingChanged;
			do {
				somethingChanged = false;

				ListIterator<ClusterRegion> itR1 = clusterRegions.begin(),itR2;
				for(itR2 = itR1.succ(); itR2.valid(); itR2 = itR1.succ())
				{
					if((*itR2).m_start < (*itR1).m_start + (*itR1).m_length)
					{
						(*itR1).m_clusters.conc((*itR2).m_clusters);
						(*itR1).m_length += (*itR2).m_length;

						//compute deflection of R1
						double sumDef = 0;
						double posStart = (*itR1).m_start;
						SListConstIterator<int> iter;
						for(iter = (*itR1).m_clusters.begin(); iter.valid(); ++iter) {
							sumDef += preferedDirection[*iter] - (posStart + preferedAngle[*iter]/2);
							posStart += preferedAngle[*iter];
						}
						double deflection = sumDef / (*itR1).m_clusters.size();
						(*itR1).m_start += deflection;

						if((*itR1).m_start < qcp.m_sectorStart)
							(*itR1).m_start = qcp.m_sectorStart;
						if((*itR1).m_start + (*itR1).m_length >  qcp.m_sectorEnd)
							(*itR1).m_start = qcp.m_sectorEnd - (*itR1).m_length;

						clusterRegions.del(itR2);
						somethingChanged = true;

					} else {
						itR1 = itR2;
					}
				}
			} while(somethingChanged);

			double posStart = qcp.m_sectorStart;
			ListIterator<ClusterRegion> itR1, itR2;
			for(itR1 = clusterRegions.begin(); itR1.valid(); itR1 = itR2)
			{
				const ClusterRegion &R1 = *itR1;

				double posRegionEnd = R1.m_start;
				SListConstIterator<int> iter;
				for(iter = R1.m_clusters.begin(); iter.valid(); ++iter)
				{
					posRegionEnd += preferedAngle[*iter];
					if(iter.valid() && iter.succ().valid())
					{
						circleQueue.append(QueuedCirclePosition(
							*iter,minDist,posStart,posRegionEnd));
						circleAngle[*iter] = posRegionEnd - preferedAngle[*iter]/2;

						posStart = posRegionEnd;

					} else {
						itR2 = itR1.succ();
						circleAngle[*iter] = posRegionEnd - preferedAngle[*iter]/2;
						if(itR2.valid()) {
							double gap = (*itR2).m_start - posRegionEnd;
							posRegionEnd += gap * preferedAngle[*iter] /
								(preferedAngle[*iter] + preferedAngle[(*itR2).m_clusters.front()]);
							circleQueue.append(QueuedCirclePosition(
								*iter,minDist,posStart,posRegionEnd));
							posStart = posRegionEnd;
						} else {
							circleQueue.append(QueuedCirclePosition(
								*iter,minDist,posStart,qcp.m_sectorEnd));
						}
					}
				}
			}
		}
	}

#ifdef OGDF_CIRCULAR_LAYOUT_LOGGING
	std::cout << "\ncircle positions:\n";
#endif
	for(i = 0; i < nCluster; ++i) {
#ifdef OGDF_CIRCULAR_LAYOUT_LOGGING
		std::cout << i << ": dist  \t" << circleDistance[i] << std::endl;
		std::cout << "    angle \t" << circleAngle[i] << std::endl;
#endif

		// determine gamma and M
		double mX, mY, gamma;

		if(i == mainSite)
		{
			mX = 0;
			mY = 0;
			gamma = gammaMainSite;

		} else {
			double alpha = circleAngle[i];
			if(alpha <= Math::pi/2) {
				// upper left
				double beta = Math::pi/2 - alpha;
				mX = -circleDistance[i] * cos(beta);
				mY =  circleDistance[i] * sin(beta);
				gamma = 1.5*Math::pi - beta;

			} else if(alpha <= Math::pi) {
				// lower left
				double beta = alpha - Math::pi/2;
				mX = -circleDistance[i] * cos(beta);
				mY = -circleDistance[i] * sin(beta);
				gamma = 1.5*Math::pi + beta;

			} else if(alpha <= 1.5*Math::pi) {
				// lower right
				double beta = 1.5*Math::pi - alpha;
				mX =  circleDistance[i] * cos(beta);
				mY = -circleDistance[i] * sin(beta);
				gamma = Math::pi/2 - beta;

			} else {
				// upper right
				double beta = alpha - 1.5*Math::pi;
				mX =  circleDistance[i] * cos(beta);
				mY =  circleDistance[i] * sin(beta);
				gamma = Math::pi/2 + beta;
			}
		}

		const int n = C.m_nodesIn[i].size();
		int pos = 0;
		SListConstIterator<node> itV;
		for(itV = C.m_nodesIn[i].begin(); itV.valid(); ++itV, ++pos)
		{
			node v = *itV;

			double phi = pos - parentWeight[i];
			if(phi < 0) phi += n;

			phi = phi * 2*Math::pi/n + gamma;
			if(phi >= 2*Math::pi) phi -= 2*Math::pi;

			double x, y;
			if(phi <= Math::pi/2) {
				// upper left
				double beta = Math::pi/2 - phi;
				x = -radius[i] * cos(beta);
				y =  radius[i] * sin(beta);

			} else if(phi <= Math::pi) {
				// lower left
				double beta = phi - Math::pi/2;
				x = -radius[i] * cos(beta);
				y = -radius[i] * sin(beta);

			} else if(phi <= 1.5*Math::pi) {
				// lower right
				double beta = 1.5*Math::pi - phi;
				x =  radius[i] * cos(beta);
				y = -radius[i] * sin(beta);

			} else {
				// upper right
				double beta = phi - 1.5*Math::pi;
				x =  radius[i] * cos(beta);
				y =  radius[i] * sin(beta);
			}

			AG.x(v) = x + mX;
			// minus sign only for debugging!
			AG.y(v) = -(y + mY);
		}
	}

}


void CircularLayout::computePreferedAngles(
	ClusterStructure &C,
	const Array<double> &outerRadius,
	Array<double> &preferedAngle)
{
	const int nCluster = C.numberOfCluster();
	const int mainSite = C.m_mainSiteCluster.front();

	Array<int> level(nCluster);
	Queue<int> Q;

	level[mainSite] = 0;
	Q.append(mainSite);

	while(!Q.empty())
	{
		int c = Q.pop();

		int nLevel = level[c]+1;
		ListConstIterator<int> it;
		for(it = C.m_childCluster[c].begin(); it.valid(); ++it) {
			level[*it] = nLevel;
			Q.append(*it);
		}
	}

	ListConstIterator<int> it;
	for(it = C.m_childCluster[mainSite].begin(); it.valid(); ++it)
		assignPrefAngle(C, outerRadius, preferedAngle,
			*it, outerRadius[mainSite]+m_minDistLevel);

#ifdef OGDF_CIRCULAR_LAYOUT_LOGGING
	std::cout << "\nprefered angles:" << std::endl;
	for(int i = 0; i < nCluster; ++i)
		std::cout << i << ": " << Math::radiansToDegrees(preferedAngle[i]) << std::endl;
#endif
}

void CircularLayout::assignPrefAngle(ClusterStructure &C,
	const Array<double> &outerRadius,
	Array<double> &preferedAngle,
	int c,
	double r1)
{
	double maxPrefChild = 0;

	ListConstIterator<int> it;
	for(it = C.m_childCluster[c].begin(); it.valid(); ++it) {
		assignPrefAngle(C, outerRadius, preferedAngle,
			*it, r1 + m_minDistLevel + 2*outerRadius[c]);
#if 0
		if(preferedAngle[*it] > maxPrefChild)
			maxPrefChild = preferedAngle[*it];
#endif
		maxPrefChild += preferedAngle[*it];
	}

	double rc = r1 + outerRadius[c];
#if 0
	preferedAngle[c] = max((2*outerRadius[c] + m_minDistSibling) / rc, maxPrefChild);
#else
	preferedAngle[c] = max(2*asin((outerRadius[c] + m_minDistSibling/2)/rc), maxPrefChild);
#endif
}


// assigns the biconnected components of the graph as clusters
void CircularLayout::assignClustersByBiconnectedComponents(ClusterStructure &C)
{
	const Graph &G = C;

	// compute biconnected components
	EdgeArray<int> compnum(G);
	int k = biconnectedComponents(G,compnum);

	// compute BC-tree
	//
	// better: proved a general class BCTree with the functionality
	//
	NodeArray<SList<int> > compV(G);
	Array<SList<node> >    nodeB(k);

	// edgeB[i] = list of edges in component i
	Array<SList<edge> > edgeB(k);
	for(edge e : G.edges)
		if(!e->isSelfLoop())
			edgeB[compnum[e]].pushBack(e);

	// construct arrays compV and nodeB such that
	// compV[v] = list of components containing v
	// nodeB[i] = list of vertices in component i
	NodeArray<bool> mark(G,false);

	int i;
	for(i = 0; i < k; ++i) {
		SListConstIterator<edge> itEdge;
		for(itEdge = edgeB[i].begin(); itEdge.valid(); ++itEdge)
		{
			edge e = *itEdge;

			if (!mark[e->source()]) {
				mark[e->source()] = true;
				nodeB[i].pushBack(e->source());
			}
			if (!mark[e->target()]) {
				mark[e->target()] = true;
				nodeB[i].pushBack(e->target());
			}
		}

		SListConstIterator<node> itNode;
		for(itNode = nodeB[i].begin(); itNode.valid(); ++itNode)
		{
			node v = *itNode;
			compV[v].pushBack(i);
			mark[v] = false;
		}
	}
	mark.init();

	Graph BCTree;
	NodeArray<int>  componentOf(BCTree,-1);
	NodeArray<node> cutVertexOf(BCTree,nullptr);
	Array<node>     nodeOf(k);

	for(i = 0; i < k; ++i) {
		node vBC = BCTree.newNode();
		componentOf[vBC] = i;
		nodeOf[i] = vBC;
	}

	for(node v : G.nodes)
	{
		if (compV[v].size() > 1) {
			node vBC = BCTree.newNode();
			cutVertexOf[vBC] = v;
			SListConstIterator<int> it;
			for(it = compV[v].begin(); it.valid(); ++it)
				BCTree.newEdge(vBC,nodeOf[*it]);
		}
	}

	// find center of BC-tree
	//
	// we currently use the center of the tree as main-site cluster
	// alternatives are: "weighted" center (concerning size of BC's,
	//                   largest component
	//
	node centerBC = nullptr;

	if(BCTree.numberOfNodes() == 1)
	{
		centerBC = BCTree.firstNode();

	} else {
		NodeArray<int> deg(BCTree);
		Queue<node> leaves;

		for(node vBC : BCTree.nodes) {
			deg[vBC] = vBC->degree();
			if(deg[vBC] == 1)
				leaves.append(vBC);
		}

		node current = nullptr;
		while(!leaves.empty())
		{
			current = leaves.pop();

			for(adjEntry adj : current->adjEntries) {
				edge e = adj->theEdge();
				node w = e->opposite(current);
				if (--deg[w] == 1)
					leaves.append(w);
			}
		}

		OGDF_ASSERT(current != nullptr);
		centerBC = current;

		// if center node current of BC-Tree is a cut-vertex, we choose the
		// maximal bic. comp. containing current as centerBC
		if (componentOf[centerBC] == -1) {
			int sizeCenter = 0;
			node vCand = nullptr;

			for(adjEntry adj : current->adjEntries) {
				edge e = adj->theEdge();
				node w = e->opposite(current);
				int sizeW = sizeBC(w);
				if(sizeW > sizeCenter) {
					vCand = w;
					sizeCenter = sizeW;
				}
			}

			// take maximal bic. comp only if not a a bridge
			if(vCand && nodeB[componentOf[vCand]].size() > 2)
				centerBC = vCand;

		// if a bridge is chosen as center, we take the closest non-bridge
		} else if(nodeB[componentOf[centerBC]].size() == 2 && centerBC->degree() == 2)
		{
			//Queue<adjEntry> Q;
			SListPure<adjEntry> currentCand, nextCand;
			nextCand.pushBack(centerBC->firstAdj());
			nextCand.pushBack(centerBC->lastAdj());

			bool found = false;
			int bestSize = -1;
			while(!nextCand.empty() && !found)
			{
				currentCand.conc(nextCand);

				while(!currentCand.empty())
				{
					adjEntry adjParent = currentCand.popFrontRet()->twin();

					for(adjEntry adj = adjParent->cyclicSucc(); adj != adjParent; adj = adj->cyclicSucc())
					{
						adjEntry adjB = adj->twin();
						node vB = adjB->theNode();
						if(nodeB[componentOf[vB]].size() > 2) {
							int candSize = sizeBC(vB);
							if(!found || candSize > bestSize) {
								centerBC = vB;
								bestSize = candSize;
								found = true;
							}
						}
						adjEntry adjB2 = adjB->cyclicSucc();
						if(adjB2 != adjB)
							nextCand.pushBack(adjB2);
					}
				}
			}
		}
	}

#ifdef OGDF_CIRCULAR_LAYOUT_LOGGING
	std::cout << "bic. comp.\n";
	for(i = 0; i < k; ++i)
		std::cout << i << ": " << nodeB[i] << std::endl;

	std::cout << "\nBC-Tree:\n";
	for(node v : BCTree.nodes) {
		std::cout << v << " [" << componentOf[v] << "," << cutVertexOf[v] << "]: ";
		for(adjEntry adj : v->adjEntries) {
			edge e = adj->theEdge();
			std::cout << e->opposite(v) << " ";
		}
		std::cout << std::endl;
	}

	std::ofstream out("BC-Tree.gml");
	GraphIO::writeGML(BCTree, out);
#endif


	// assign cluster
	//
	// we traverse the tree from the center to the outside
	// cut-vertices are assigned to the inner cluster which contains them
	// exception: bridges are no cluster at all if outer cut-vertex is only
	//   connected to one non-bridge [bridge -> c -> non bridge]
	int currentCluster = 0;
	Queue<InfoAC> Q;
	Array<int> parentCluster(k+1);

	if(componentOf[centerBC] == -1)
	{ // case cut vertex as center
		parentCluster[currentCluster] = -1;
		C.m_clusterOf[cutVertexOf[centerBC]] = currentCluster;

		for(adjEntry adj : centerBC->adjEntries) {
			edge e = adj->theEdge();
			node bBC = e->opposite(centerBC);
			Q.append(InfoAC(bBC,centerBC,cutVertexOf[centerBC],currentCluster));
		}

		++currentCluster;

	} else { // case bic. comp. as center
		Q.append(InfoAC(centerBC,nullptr,nullptr,-1));
	}

	while(!Q.empty())
	{
		InfoAC info = Q.pop();

		// bridge?
		if(nodeB[componentOf[info.m_vBC]].size() == 2 &&
			info.m_predCut != nullptr &&
			info.m_vBC->degree() == 2)
		{
			node wBC = info.m_vBC->firstAdj()->twinNode();
			if(wBC == info.m_predCutBC)
				wBC = info.m_vBC->lastAdj()->twinNode();

			if(wBC->degree() == 2)
			{
				node bBC = wBC->firstAdj()->twinNode();
				if(bBC == info.m_vBC)
					bBC = wBC->lastAdj()->twinNode();

				if(nodeB[componentOf[bBC]].size() != 2)
				{
					Q.append(InfoAC(bBC,wBC,nullptr,info.m_parentCluster));
					continue; // case already handled
				}
			}
		}

		SListConstIterator<node> itV;
		for(itV = nodeB[componentOf[info.m_vBC]].begin(); itV.valid(); ++itV)
			if (*itV != info.m_predCut)
				C.m_clusterOf[*itV] = currentCluster;

		parentCluster[currentCluster] = info.m_parentCluster;

		for(adjEntry adj : info.m_vBC->adjEntries) {
			edge e1 = adj->theEdge();
			node wBC = e1->opposite(info.m_vBC);
			if(wBC == info.m_predCutBC) continue;

			for(adjEntry adjWBC : wBC->adjEntries) {
				edge e2 = adjWBC->theEdge();
				node bBC = e2->opposite(wBC);
				if (bBC == info.m_vBC) continue;

				Q.append(InfoAC(bBC,wBC,cutVertexOf[wBC],currentCluster));
			}
		}

		++currentCluster;
	}

	C.initCluster(currentCluster,parentCluster);
	// in this case, the main-site cluster is always the first created
	C.m_mainSiteCluster.pushBack(0);

#ifdef OGDF_CIRCULAR_LAYOUT_LOGGING
	std::cout << "\ncluster:\n";
	for(i = 0; i < currentCluster; ++i) {
		std::cout << i << ": " << C.m_nodesIn[i] << std::endl;
		std::cout << "   parent = " << C.m_parentCluster[i] << ", children " << C.m_childCluster[i] << std::endl;
	}
	std::cout << "main-site cluster: " << C.m_mainSiteCluster << std::endl;
#endif
}


int CircularLayout::sizeBC(node vB)
{
	int sum = 0;
	for(adjEntry adj : vB->adjEntries)
		sum += adj->twinNode()->degree() - 1;
	return sum;
}

}
