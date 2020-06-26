/** \file
 * \brief Implements class HananiTutteCPlanarity, which represents a
 *        c-planarity test based on the Hanani-Tutte theorem.
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

#include <ogdf/cluster/HananiTutteCPlanarity.h>
#include <ogdf/basic/GF2Solver.h>

#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/cluster/CconnectClusterPlanar.h>
#include <ogdf/cluster/ClusterPlanarity.h>
#include <unordered_map>
#include <map>

using std::unordered_map;
using std::map;

using std::chrono::time_point;
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;

namespace ogdf {

class HananiTutteCPlanarity::CLinearSystem {
public:
	struct Object {
		Type    m_t;
		SubType m_st;

		node    m_v;
		edge    m_e;
		cluster m_c, m_c2;

		// default constructor
		Object() :
			m_t(Type::tVertex), m_st(SubType::stVertex), m_v(nullptr), m_e(nullptr), m_c(nullptr), m_c2(nullptr) { }

		// type v/v
		explicit Object(node v) :
			m_t(Type::tVertex), m_st(SubType::stVertex), m_v(v), m_e(nullptr), m_c(nullptr), m_c2(nullptr) { }

		// type e/e
		explicit Object(edge e) :
			m_t(Type::tEdge), m_st(SubType::stEdge), m_v(nullptr), m_e(e), m_c(nullptr), m_c2(nullptr) { }

		// type v/c
		explicit Object(cluster c) :
			m_t(Type::tVertex), m_st(SubType::stCluster), m_v(nullptr), m_e(nullptr), m_c(c), m_c2(nullptr) { }

		// types v/ic, v/oc, e/oc, e/ic, e/cr
		Object(Type t, SubType st, cluster c, edge e) :
			m_t(t), m_st(st), m_v(nullptr), m_e(e), m_c(c), m_c2(nullptr) { }

		// type e/vc
		Object(node v, cluster c, edge e) :
			m_t(Type::tEdge), m_st(SubType::stVertexCluster), m_v(v), m_e(e), m_c(c), m_c2(nullptr) { }

		// type e/cc
		Object(cluster c, cluster c2, edge e) :
			m_t(Type::tEdge), m_st(SubType::stClusterCluster), m_v(nullptr), m_e(e)
		{
			if(c->index() < c2->index()) {
				m_c = c;  m_c2 = c2;
			} else {
				m_c = c2; m_c2 = c;
			}
		}

		bool operator==(const Object &other) const {
			return
				(m_t  == other.m_t ) &&
				(m_st == other.m_st) &&
				(m_v  == other.m_v ) &&
				(m_e  == other.m_e ) &&
				(m_c  == other.m_c ) &&
				(m_c2 == other.m_c2);
		}

		bool operator<(const Object &other) const {
			if(m_t != other.m_t)
				return m_t < other.m_t;
			if(m_st != other.m_st)
				return m_st < other.m_st;

			if(m_v != other.m_v) {
				if(other.m_v == nullptr)
					return true;
				if(m_v == nullptr)
					return false;
				return m_v->index() < other.m_v->index();
			}

			if(m_e != other.m_e) {
				if(other.m_e == nullptr)
					return true;
				if(m_e == nullptr)
					return false;
				return m_e->index() < other.m_e->index();
			}

			if(m_c != other.m_c) {
				if(other.m_c == nullptr)
					return true;
				if(m_c == nullptr)
					return false;
				return m_c->index() < other.m_c->index();
			}

			if(m_c2 != other.m_c2) {
				if(other.m_c2 == nullptr)
					return true;
				if(m_c2 == nullptr)
					return false;
				return m_c2->index() < other.m_c2->index();
			}

			return false;
		}
	};

	struct ObjectHash {
		size_t operator()(const Object &obj) const {
			size_t hv  = std::hash<int>()( (obj.m_v  == nullptr) ? -1 : obj.m_v ->index() );
			size_t he  = std::hash<int>()( (obj.m_e  == nullptr) ? -1 : obj.m_e ->index() );
			size_t hc  = std::hash<int>()( (obj.m_c  == nullptr) ? -1 : obj.m_c ->index() );
			size_t hc2 = std::hash<int>()( (obj.m_c2 == nullptr) ? -1 : obj.m_c2->index() );

#if 0
			size_t hv = std::hash<void*>()(obj.m_v);
			size_t he = std::hash<void*>()(obj.m_e);
			size_t hc = std::hash<void*>()(obj.m_c);
			size_t hc2 = std::hash<void*>()(obj.m_c2);
#endif
			size_t ht = std::hash<uint32_t>()(static_cast<uint32_t>(obj.m_t) + (static_cast<uint32_t>(obj.m_st) << 4));
			return hv ^ he ^ hc ^ hc2 ^ ht;
		}
	};

#if 0
	using ObjectTable = unordered_map<Object,int,ObjectHash>;
#endif
	using ObjectTable = map<Object,int>;

	CLinearSystem() : m_ox() {
		m_objectCounter = 0;
	}

	void clear();

	int numOx(const Object &obj);
	int numCond(const Object *eo1, const Object *eo2);
	int numeomove(const Object *eo, const Object &obj);

	int addTrivialEquation();

#if 0
	const unordered_map<Object,int,ObjectHash> &objects() const {
#endif
	const map<Object,int> &objects() const {
		return m_ox;
	}

	const map<int,std::pair<const Object*,const Object*>> &pairs() const { return m_pairs; }

	GF2Solver::Equation &equation(int numc) { return m_matrix[numc]; }

	int numberOfRows() const { return m_matrix.numRows(); }
	int numberOfColumns() const { return m_matrix.numColumns(); }
	int numberOfConditions() const { return (int)m_cx.size(); }
	int numberOfMoves() const { return (int)m_mx.size(); }

	bool solve();

private:
	ObjectTable m_ox;
	map<std::pair<int,int>,int> m_cx;
	map<int,std::pair<const Object*,const Object*>> m_pairs;
	map<std::pair<int,int>,int> m_mx;

	GF2Solver::Matrix m_matrix;

	int m_objectCounter;
};

void HananiTutteCPlanarity::CLinearSystem::clear()
{
	m_ox.clear();
	m_cx.clear();
	m_pairs.clear();
	m_mx.clear();
	m_matrix.clear();
	m_objectCounter = 0;
}

int HananiTutteCPlanarity::CLinearSystem::numOx(const Object &obj)
{
	auto it = m_ox.find(obj);
	if(it != m_ox.end())
		return it->second;
	else {
		int id = m_objectCounter++;
		m_ox.emplace(obj, id);
		return id;
	}
}

int HananiTutteCPlanarity::CLinearSystem::numCond(const Object *eo1, const Object *eo2)
{
	int o1num = numOx(*eo1);
	int o2num = numOx(*eo2);
	if(o2num < o1num)
		std::swap(o1num, o2num);

	std::pair<int,int> pnum(o1num,o2num);
	auto it = m_cx.find(pnum);

	if(it != m_cx.end())
		return it->second;

	int r = m_matrix.addRow();
	m_cx[pnum] = r;
	m_pairs[r] = std::make_pair(eo1,eo2);
	return r;
}

int HananiTutteCPlanarity::CLinearSystem::numeomove(const Object *eo, const Object &obj)
{
	OGDF_ASSERT(eo->m_t == Type::tEdge);
	OGDF_ASSERT(obj.m_t == Type::tVertex);

	std::pair<int,int> p(numOx(*eo), numOx(obj));

	auto it = m_mx.find(p);
	if(it != m_mx.end())
		return it->second;

	int c = m_matrix.addColumn();
	m_mx[p] = c;
#if 0
	std::cout << "(" << p.first << "," << p.second << ") : " << c << std::endl;
#endif

	return c;
}

int HananiTutteCPlanarity::CLinearSystem::addTrivialEquation()
{
	int c1 = m_matrix.addColumn();
	int c2 = m_matrix.addColumn();

	int r = m_matrix.addRow();

	m_matrix[r] |= c1;
	m_matrix[r] |= c2;

	return c2;
}

bool HananiTutteCPlanarity::CLinearSystem::solve()
{
	GF2Solver solver(m_matrix);

	return solver.solve2();
}

class HananiTutteCPlanarity::CGraph {

	const ClusterGraph &m_cg;

	ClusterArray<ArrayBuffer<edge>> m_cbe; // edges crossing the cluster boundary
	ClusterArray<ArrayBuffer<edge>> m_cbeRot; // order in which edges in m_cbe cross cluster boundary
	ClusterArray<SList<const CLinearSystem::Object*>> m_ce2;  // object edges lying in a region (or crossing into it)
	map<const CLinearSystem::Object*,SList<std::pair<const CLinearSystem::Object*,CLinearSystem::Object>>> m_aff;

	CLinearSystem m_ls;

	int64_t m_tPrepare;
	int64_t m_tCreateSparse;
	int64_t m_tSolve;


public:
	explicit CGraph(const ClusterGraph &C);

	bool cplanar(int &nRows, int &nCols);
	Verification cpcheck(int &nRows, int &nCols);

	int64_t timePrepare() const { return m_tPrepare; }
	int64_t timeCreateSparse() const { return m_tCreateSparse; }
	int64_t timesolve() const { return m_tSolve; }

private:

	cluster clusterOfEdge(const CLinearSystem::Object &obj, cluster &cl2) const;
	cluster cp(node u, node v, List<cluster> &path) const;

	void ends(const CLinearSystem::Object *eo, CLinearSystem::Object &obj1, CLinearSystem::Object &obj2) const;
	bool incident(const CLinearSystem::Object &vo, const CLinearSystem::Object *eo) const;
	bool adjacent(const CLinearSystem::Object *eo1, const CLinearSystem::Object *eo2) const;
	bool cAdjacent(const CLinearSystem::Object *eo1, const CLinearSystem::Object *eo2) const;
	bool fixed(const CLinearSystem::Object *eo);
	void affect(const CLinearSystem::Object *eo1, const CLinearSystem::Object &obj, const CLinearSystem::Object *eo2);

	bool iD(const CLinearSystem::Object *eo1, const CLinearSystem::Object *eo2) const;
	bool before(const CLinearSystem::Object &vo1, const CLinearSystem::Object &vo2) const;
	bool bdbefore(edge e1, edge e2, cluster c) const;

	void prepareLinearSystem();
	void createSparse();

	void resetLinearSystem();
};

void HananiTutteCPlanarity::CGraph::resetLinearSystem()
{
	m_ce2.fill(SList<const CLinearSystem::Object*>());
	m_aff.clear();
	m_ls.clear();
}

HananiTutteCPlanarity::CGraph::CGraph(const ClusterGraph &C)
	: m_cg(C), m_cbe(C), m_ce2(C)
{
	const Graph &G = m_cg.constGraph();

	for(edge e : G.edges) {
		node u = e->source(), v = e->target();

		cluster lca = C.commonCluster(u,v);

		for(cluster c = C.clusterOf(u); c != lca; c = c->parent())
			m_cbe[c].push(e);
		for(cluster c = C.clusterOf(v); c != lca; c = c->parent())
			m_cbe[c].push(e);
	}
}

bool HananiTutteCPlanarity::CGraph::cplanar(int &nRows, int &nCols)
{
	m_tCreateSparse = m_tSolve = 0;
	time_point<high_resolution_clock> tStart = high_resolution_clock::now();

	nRows = nCols = 0;
	prepareLinearSystem();

	time_point<high_resolution_clock> tAfterPrepare = high_resolution_clock::now();
	m_tPrepare = duration_cast<std::chrono::milliseconds>(tAfterPrepare-tStart).count();

	// trivial?
	if(m_ls.objects().size() == 0)
		return true;

	createSparse();
	nRows = m_ls.numberOfConditions();
	nCols = m_ls.numberOfMoves();

	time_point<high_resolution_clock> tAfterCreateSparse = high_resolution_clock::now();
	m_tCreateSparse = duration_cast<std::chrono::milliseconds>(tAfterCreateSparse-tAfterPrepare).count();

	// return success
	bool solvable = m_ls.solve();
	time_point<high_resolution_clock> tAfterSolve = high_resolution_clock::now();
	m_tSolve = duration_cast<std::chrono::milliseconds>(tAfterSolve-tAfterCreateSparse).count();

	return solvable;
}

HananiTutteCPlanarity::Verification HananiTutteCPlanarity::CGraph::cpcheck(int &nRows, int &nCols)
{
	if(!cplanar(nRows,nCols)) {
		return Verification::nonCPlanarVerified;
	}

	int nR, nC;
	m_cbeRot.init(m_cg);

	for(cluster c : m_cg.clusters) {
		if(!m_cbe[c].empty())
			m_cbeRot[c].push(m_cbe[c][0]);
	}

	for(cluster c : m_cg.clusters) {
		List<edge> remainingEdges;
		for(edge e : m_cbe[c]) {
			if(e != m_cbeRot[c][0])
				remainingEdges.pushBack(e);
		}

		while(m_cbeRot[c].size() < m_cbe[c].size()) {
			bool findNext = false;

			for(edge e : remainingEdges) {
				m_cbeRot[c].push(e);
				if(m_cbeRot[c].size() == m_cbe[c].size()) {
					findNext = true;
					break;
				}

				resetLinearSystem();
				if(cplanar(nR,nC)) {
					findNext = true;
					remainingEdges.removeFirst(e);
					break;

				} else {
					m_cbeRot[c].pop();
				}
			}

			if(!findNext)
				return Verification::verificationFailed;
		}
	}

	return Verification::cPlanarVerified;
}

cluster HananiTutteCPlanarity::CGraph::cp(node u, node v, List<cluster> &path) const
{
	cluster lca = m_cg.commonClusterPath(u, v, path);

	ListIterator<cluster> it = path.begin();
	while(*it != lca)
		++it;

	path.del(it);
	return lca;
}

cluster HananiTutteCPlanarity::CGraph::clusterOfEdge(const CLinearSystem::Object &obj, cluster &cl2) const
{
	OGDF_ASSERT(obj.m_t == Type::tEdge);

	cl2 = nullptr;

	switch (obj.m_st) {
	case SubType::stEdge:
		return m_cg.clusterOf(obj.m_e->source());

	case SubType::stVertexCluster:
		return m_cg.clusterOf(obj.m_v);

	case SubType::stClusterCluster:
		{
			cluster c1 = obj.m_c;
			cluster c2 = obj.m_c2;

			if(c1->parent() == c2)
				return c2;
			else if(c2->parent() == c1)
				return c1;
			else {
				// my assumptions....
				OGDF_ASSERT(c1->parent() == c2->parent());
				OGDF_ASSERT(c1->parent() == m_cg.commonCluster(obj.m_e->source(),obj.m_e->target()));
				return c1->parent();
			}
		}

	case SubType::stInnerCluster:
		return obj.m_c;

	case SubType::stOuterCluster:
		{
			cluster c = obj.m_c->parent();
			OGDF_ASSERT(c != nullptr);
			return c;
		}

	case SubType::stCrossCluster:
		cl2 = obj.m_c->parent();
		return obj.m_c;

	case SubType::stVertex:
	case SubType::stCluster:
		return nullptr;
	}

	return nullptr;
}

void HananiTutteCPlanarity::CGraph::ends(const CLinearSystem::Object *eo, CLinearSystem::Object &obj1, CLinearSystem::Object &obj2) const
{
	OGDF_ASSERT(eo->m_t == Type::tEdge);

	switch (eo->m_st) {
	case SubType::stEdge:
		obj1 = CLinearSystem::Object(eo->m_e->source());
		obj2 = CLinearSystem::Object(eo->m_e->target());
		break;

	case SubType::stVertexCluster:
		{
			node u = eo->m_v;
			cluster c = eo->m_c;

			SubType st = (m_cg.clusterOf(u) == c) ? SubType::stInnerCluster : SubType::stOuterCluster;
			obj1 = CLinearSystem::Object(u);
			obj2 = CLinearSystem::Object(Type::tVertex, st, c, eo->m_e);
		}
		break;

	case SubType::stClusterCluster:
		{
			cluster c1 = eo->m_c;
			cluster c2 = eo->m_c2;
			SubType st = SubType::stInnerCluster;

			if(c2->parent() == c1)
				std::swap(c1, c2);
			else if(c1->parent() != c1)
				st = SubType::stOuterCluster;

			obj1 = CLinearSystem::Object(Type::tVertex, SubType::stOuterCluster, c1, eo->m_e);
			obj2 = CLinearSystem::Object(Type::tVertex, st, c2, eo->m_e);
		}
		break;

	case SubType::stInnerCluster:
		obj1 = CLinearSystem::Object(eo->m_c);
		obj2 = CLinearSystem::Object(Type::tVertex, SubType::stInnerCluster, eo->m_c, eo->m_e);
		break;

	case SubType::stOuterCluster:
		obj1 = CLinearSystem::Object(eo->m_c);
		obj2 = CLinearSystem::Object(Type::tVertex, SubType::stOuterCluster, eo->m_c, eo->m_e);
		break;

	case SubType::stCrossCluster:
		obj1 = CLinearSystem::Object(Type::tVertex, SubType::stInnerCluster, eo->m_c, eo->m_e);
		obj2 = CLinearSystem::Object(Type::tVertex, SubType::stOuterCluster, eo->m_c, eo->m_e);
		break;

	case SubType::stVertex:
	case SubType::stCluster:
		break;
	}
}

bool HananiTutteCPlanarity::CGraph::adjacent(const CLinearSystem::Object *eo1, const CLinearSystem::Object *eo2) const
{
	CLinearSystem::Object obj11, obj12, obj21, obj22;
	ends(eo1, obj11, obj12);
	ends(eo2, obj21, obj22);
	return obj11 == obj21 || obj11 == obj22 || obj12 == obj21 || obj12 == obj22;
}

bool HananiTutteCPlanarity::CGraph::cAdjacent(const CLinearSystem::Object *eo1, const CLinearSystem::Object *eo2) const
{
	CLinearSystem::Object obj11, obj12, obj21, obj22;
	ends(eo1, obj11, obj12);
	ends(eo2, obj21, obj22);

	bool isClusterVertex = obj11.m_t == Type::tVertex
			    && obj11.m_st == SubType::stCluster;

	if (isClusterVertex && (obj11 == obj21 || obj11 == obj22)) {
		return true;
	}
	if (obj12 == obj21 || obj12 == obj22) {
		return isClusterVertex;
	}

	return false;
}

bool HananiTutteCPlanarity::CGraph::fixed(const CLinearSystem::Object *eo)
{
	if(!m_cbeRot.valid())
		return false;  // no rotation system given

	if(!(eo->m_t == Type::tEdge && (eo->m_st == SubType::stInnerCluster || eo->m_st == SubType::stOuterCluster) ))
		return false;

	return m_cbeRot[eo->m_c].linearSearch(eo->m_e) != -1;
}

bool HananiTutteCPlanarity::CGraph::incident(const CLinearSystem::Object &vo, const CLinearSystem::Object *eo) const
{
	CLinearSystem::Object obj1, obj2;
	ends(eo, obj1, obj2);
	return vo == obj1 || vo == obj2;
}

bool HananiTutteCPlanarity::CGraph::bdbefore(edge e1, edge e2, cluster c) const
{
	// TODO for given Rot: add code for case c in CBErot! [DONE]

	if(!m_cbeRot.valid()) {  // no rotation system given?
		const auto &cbe_c = m_cbe[c];
		int p1 = searchPos(cbe_c, e1);
		int p2 = searchPos(cbe_c, e2);
		OGDF_ASSERT(p1 != -1);
		OGDF_ASSERT(p2 != -1);

		return p1 > p2;
	}

	const auto &rot_c = m_cbeRot[c];
	int p1 = searchPos(rot_c, e1);
	int p2 = searchPos(rot_c, e2);

	if(p1 != -1) {
		if(p2 != -1)
			return p1 > p2;
		else
			return true;

	} else if(p2 != -1)
		return false;

	else {
		const auto &cbe_c = m_cbe[c];
		p1 = searchPos(cbe_c, e1);
		p2 = searchPos(cbe_c, e2);
		OGDF_ASSERT(p1 != -1);
		OGDF_ASSERT(p2 != -1);

		return p1 > p2;
	}
}

bool HananiTutteCPlanarity::CGraph::before(const CLinearSystem::Object &vo1, const CLinearSystem::Object &vo2) const
{
	OGDF_ASSERT(vo1.m_t == Type::tVertex);
	OGDF_ASSERT(vo2.m_t == Type::tVertex);

	SubType st1 = vo1.m_st, st2 = vo2.m_st;
	OGDF_ASSERT(st1 == SubType::stVertex || st1 == SubType::stInnerCluster || st1 == SubType::stOuterCluster);
	OGDF_ASSERT(st2 == SubType::stVertex || st2 == SubType::stInnerCluster || st2 == SubType::stOuterCluster);

	switch (st1) {
	case SubType::stVertex:
		if (st2 == SubType::stVertex) {
			return vo1.m_v->index() < vo2.m_v->index();
		} else {
			return true;
		}

	case SubType::stInnerCluster:
		if (st2 == SubType::stVertex) {
			return false;
		} else if (st2 == SubType::stInnerCluster) {
			OGDF_ASSERT(vo1.m_c == vo2.m_c);
			return !bdbefore(vo1.m_e, vo2.m_e, vo1.m_c);
		} else {
			return true;
		}

	case SubType::stOuterCluster:
		if (st2 == SubType::stVertex
		 || st2 == SubType::stInnerCluster) {
			return false;
		} else {
			if (vo1.m_c == vo2.m_c) {
				return bdbefore(vo1.m_e, vo2.m_e, vo1.m_c);
			} else {
				return vo1.m_c->index() < vo2.m_c->index();
			}
		}

	case SubType::stCluster:
	case SubType::stEdge:
	case SubType::stVertexCluster:
	case SubType::stClusterCluster:
	case SubType::stCrossCluster:
		OGDF_ASSERT(false);
	}
	return false; // should not arrive here!
}

// do eo1 and eo2 cross oddly in the initial drawing?
bool HananiTutteCPlanarity::CGraph::iD(const CLinearSystem::Object *eo1, const CLinearSystem::Object *eo2) const
{
	OGDF_ASSERT(eo1->m_t == Type::tEdge);
	OGDF_ASSERT(eo2->m_t == Type::tEdge);

	cluster c1a, c1b, c2a, c2b;
	c1a = clusterOfEdge(*eo1, c1b);
	c2a = clusterOfEdge(*eo2, c2b);

	if (c1a != c2a
	 && c1a != c2b
	 && ((c1b == nullptr)
	  || (c1b != c2a
	   && c1b != c2b))) {
		return false;
	}

	SubType st1 = eo1->m_st, st2 = eo2->m_st;
	bool canCross = false;
	switch (st1) {
	case SubType::stEdge:
		canCross = ( st2 == SubType::stEdge || st2 == SubType::stVertexCluster );
		break;
	case SubType::stVertexCluster:
		canCross = ( st2 == SubType::stEdge || st2 == SubType::stVertexCluster || st2 == SubType::stClusterCluster );
		break;
	case SubType::stClusterCluster:
		canCross = ( st2 == SubType::stVertexCluster || st2 == SubType::stClusterCluster );
		break;
	default:
		break;
	}

	if (!canCross) {
		return false;
	}

	CLinearSystem::Object uo1, vo1, uo2, vo2;
	ends(eo1, uo1, vo1);
	ends(eo2, uo2, vo2);

	if(before(vo1,uo1))
		std::swap(vo1, uo1);
	if(before(vo2,uo2))
		std::swap(vo2, uo2);

	return (before(uo1,uo2) && before(uo2,vo1) && before(vo1,vo2))
	    || (before(uo2,uo1) && before(uo1,vo2) && before(vo2,vo1));
}

void HananiTutteCPlanarity::CGraph::affect(const CLinearSystem::Object *eo1, const CLinearSystem::Object &obj, const CLinearSystem::Object *eo2)
{
	// first check if there is something to do
	if (obj.m_t == Type::tVertex) {
		switch (obj.m_st) {
		case SubType::stCluster:
			if (m_ce2[obj.m_c].search(eo1).valid()) {
				if (eo1->m_t != Type::tEdge || eo1->m_st != SubType::stCrossCluster) return;
				if (m_ce2[obj.m_c].search(eo2).valid()) return;
			}
			break;
		case SubType::stInnerCluster:
			if (!m_ce2[obj.m_c].search(eo1).valid()) return;
			if (eo1->m_t == Type::tEdge && eo1->m_st == SubType::stEdge) return;
			break;
		case SubType::stOuterCluster:
			if (obj.m_c->parent() == nullptr || !m_ce2[obj.m_c->parent()].search(eo1).valid()) return;
			if (eo1->m_t == Type::tEdge && eo1->m_st == SubType::stEdge) return;
			break;
		case SubType::stVertex:
			if (!m_ce2[m_cg.clusterOf(obj.m_v)].search(eo1).valid()) return;
			if (eo1->m_t != Type::tEdge) return;
			if (eo1->m_st != SubType::stEdge
			 && eo1->m_st != SubType::stVertexCluster
			 && eo1->m_st != SubType::stClusterCluster) {
				return;
			}
			break;
		default:
			break;
		}
	}

	if (eo1->m_t == Type::tEdge) {
		// do not allow eo1, o moves if o is v vertex and eo1 is
		// inner cluster or outer cluster edge or if o belongs
		// to a different cluster
		switch (eo1->m_st) {
		case SubType::stInnerCluster:
		case SubType::stOuterCluster:
			if (obj.m_t == Type::tVertex) {
				if (obj.m_st == SubType::stVertex || obj.m_st == SubType::stCluster) return;
				if (obj.m_st == SubType::stInnerCluster || obj.m_st == SubType::stOuterCluster) {
					if (eo1->m_c != obj.m_c) return;
					if (eo1->m_st != obj.m_st) return;
				}
			}
			break;
		case SubType::stCrossCluster:
			if (obj.m_t != Type::tVertex) return;
			if (obj.m_st != SubType::stInnerCluster && obj.m_st != SubType::stOuterCluster) return;
			if (eo1->m_c != obj.m_c) return;
			if (eo2->m_t != Type::tEdge) return;
			if (eo2->m_st != SubType::stCrossCluster
			 && eo2->m_st != SubType::stInnerCluster
			 && eo2->m_st != SubType::stOuterCluster) {
				return;
			}
			break;
		default:
			break;
		}
	}

	if (eo2->m_t == Type::tEdge && eo2->m_st == SubType::stCrossCluster) {
		SubType st = eo1->m_st;
		if (eo1->m_t == Type::tEdge
		 && st != SubType::stInnerCluster
		 && st != SubType::stOuterCluster
		 && st != SubType::stCrossCluster) {
			return;
		}
		if (eo2->m_c != eo1->m_c) return;
	}

	if (incident(obj, eo1)) return;

	// now affect
	auto it = m_aff.find(eo2);
	if (it == m_aff.end()) {
		m_aff[eo2].pushBack(std::make_pair(eo1, obj));
	} else {
		std::pair<const CLinearSystem::Object*, CLinearSystem::Object> p(eo1, obj);
		if (!it->second.search(p).valid()) {
			it->second.pushBack(p);
		}
	}
}

void HananiTutteCPlanarity::CGraph::prepareLinearSystem()
{
	const Graph &G = m_cg.constGraph();

	// create vertices + inner/outer/cross cluster edges
	for(node v : G.nodes)
		m_ls.numOx(CLinearSystem::Object(v));

	for(cluster c : m_cg.clusters) {
		m_ls.numOx(CLinearSystem::Object(c));

		for(edge e : m_cbe[c]) {
			m_ls.numOx(CLinearSystem::Object(Type::tVertex, SubType::stInnerCluster, c, e));
			m_ls.numOx(CLinearSystem::Object(Type::tVertex, SubType::stOuterCluster, c, e));
			m_ls.numOx(CLinearSystem::Object(Type::tEdge,   SubType::stOuterCluster, c, e));
			m_ls.numOx(CLinearSystem::Object(Type::tEdge,   SubType::stInnerCluster, c, e));
			m_ls.numOx(CLinearSystem::Object(Type::tEdge,   SubType::stCrossCluster, c, e));
		}
	}

	// create edges
	for(edge e : G.edges) {
		node u = e->source(), v = e->target();
		cluster cu = m_cg.clusterOf(u), cv = m_cg.clusterOf(v);

		if(cu == cv)
			m_ls.numOx(CLinearSystem::Object(e));

		else {
			List<cluster> path;
			cp(u,v,path);

			cluster c = path.front();
			m_ls.numOx(CLinearSystem::Object(u, c, e));

			for(ListConstIterator<cluster> it = path.begin(); it.valid(); ++it) {
				c = *it;
				if(!it.succ().valid())
					m_ls.numOx(CLinearSystem::Object(v, c, e));
				else {
					cluster c2 = *it.succ();
					m_ls.numOx(CLinearSystem::Object(c,c2,e));
				}
			}
		}
	}

	// object vertices and edges lying in a cluster
	for(CLinearSystem::ObjectTable::const_reference value : m_ls.objects()) {
		const CLinearSystem::Object &obj = value.first;

		if(obj.m_t == Type::tEdge) {
			cluster c2;
			m_ce2[clusterOfEdge(obj, c2)].pushBack(&obj);
			if(c2 != nullptr)
				m_ce2[c2].pushBack(&obj);
		}
	}

	CLinearSystem::Object uo1, uo2, vo1, vo2;
	for(cluster c : m_cg.clusters) {
		for(const CLinearSystem::Object *eo1 : m_ce2[c]) {
			for(const CLinearSystem::Object *eo2 : m_ce2[c]) {
				ends(eo1, uo1, uo2);
				ends(eo2, vo1, vo2);

				affect(eo1, vo1, eo2);
				affect(eo1, vo2, eo2);
				affect(eo2, uo1, eo1);
				affect(eo2, uo2, eo1);
			}
		}
	}
}


void HananiTutteCPlanarity::CGraph::createSparse()
{
	for(auto &elem : m_aff) {
		const CLinearSystem::Object *eo2 = elem.first;

		for(const auto &p : elem.second) {
			const CLinearSystem::Object *eo1 = p.first;

			// TODO for given Rot: add condition wrt fixed and cAdjacent here! [DONE]
			if( !adjacent(eo1,eo2) || ( (fixed(eo1) || fixed(eo2) ) && cAdjacent(eo1,eo2) ) ) {
				int numc = m_ls.numCond(eo1,eo2);

				// TODO for given Rot: add check for not fixed(eo1) [DONE]
				if(!fixed(eo1)) {
					int numeo = m_ls.numeomove(eo1,p.second);
					m_ls.equation(numc) |= numeo;
				}
			}
		}
	}

	int lastCol = m_ls.addTrivialEquation();

	for(auto it = m_ls.pairs().begin(); it != m_ls.pairs().end(); ++it) {
		int numc = it->first;
		const CLinearSystem::Object *eo1 = it->second.first;
		const CLinearSystem::Object *eo2 = it->second.second;

		if(iD(eo1,eo2))
			m_ls.equation(numc) |= lastCol;
	}
}

HananiTutteCPlanarity::Verification HananiTutteCPlanarity::isCPlanar(const ClusterGraph &C, bool doPreproc, bool forceSolver, Solver solver)
{
	m_nRows = m_nCols = 0;
	m_tPrepare = m_tCreateSparse = m_tSolve = 0;

	Graph G;
	ClusterGraph H(C,G);
	makeLoopFree(G);

	if(doPreproc)
		preprocessing(H,G);
	else {
		m_numNodesPreproc = G.numberOfNodes();
		m_numEdgesPreproc = G.numberOfEdges();
		m_numClustersPreproc = C.numberOfClusters();
	}

	for(edge e : G.edges) {
		if(e->source()->index() > e->target()->index())
			G.reverseEdge(e);
	}
	makeParallelFree(G);

	if(forceSolver) {
		switch (solver) {
		case Solver::HananiTutte:
			{
				m_status = Status::applyHananiTutte;
				CGraph cgraph(H);
				bool icp = cgraph.cplanar(m_nRows, m_nCols);

				m_tPrepare = cgraph.timePrepare();
				m_tCreateSparse = cgraph.timeCreateSparse();
				m_tSolve = cgraph.timesolve();

				return icp ? Verification::cPlanar : Verification::nonCPlanarVerified;
			}

		case Solver::HananiTutteVerify:
			{
				m_status = Status::applyHananiTutte;
				CGraph cgraph(H);
				return cgraph.cpcheck(m_nRows, m_nCols);
			}

		case Solver::ILP:
			{
				ClusterPlanarity cPlanarity;
				cPlanarity.setTimeLimit("00:10:00");
				bool icp = cPlanarity.isClusterPlanar(H);

				switch (cPlanarity.getOptStatus())
				{
				case abacus::Master::Optimal:
					m_status = Status::applyILP; break;
				case abacus::Master::MaxCpuTime:
				case abacus::Master::MaxCowTime:
					m_status = Status::timeoutILP; break;
				default:
					m_status = Status::errorILP;
				}

				if (m_status != Status::applyILP)
					return Verification::timeout;
				return icp ? Verification::cPlanarVerified : Verification::nonCPlanarVerified;
			}
		}

	} else {
		if(G.empty()) {
			m_status = Status::emptyAfterPreproc;
			return Verification::cPlanarVerified;

		} else if(isCConnected(H)) {
			m_status = Status::cConnectedAfterPreproc;
			CconnectClusterPlanar ccPlanarityTest;
			return ccPlanarityTest.call(H) ? Verification::cPlanarVerified : Verification::nonCPlanarVerified;

		} else if(!isPlanar(G)) {
			m_status = Status::nonPlanarAfterPreproc;
			return Verification::nonCPlanarVerified;

		} else {
			switch (solver) {
			case Solver::HananiTutte:
				{
					m_status = Status::applyHananiTutte;
					CGraph cgraph(H);
					bool icp = cgraph.cplanar(m_nRows, m_nCols);

					m_tPrepare = cgraph.timePrepare();
					m_tCreateSparse = cgraph.timeCreateSparse();
					m_tSolve = cgraph.timesolve();

					return icp ? Verification::cPlanar : Verification::nonCPlanarVerified;
				}

			case Solver::HananiTutteVerify:
				{
					m_status = Status::applyHananiTutte;
					CGraph cgraph(H);
					return cgraph.cpcheck(m_nRows, m_nCols);
				}

			case Solver::ILP:
				{
					ClusterPlanarity cPlanarity;
					cPlanarity.setTimeLimit("00:10:00");
					bool icp = cPlanarity.isClusterPlanar(H);

					switch (cPlanarity.getOptStatus())
					{
					case abacus::Master::Optimal:
						m_status = Status::applyILP; break;
					case abacus::Master::MaxCpuTime:
					case abacus::Master::MaxCowTime:
						m_status = Status::timeoutILP; break;
					default:
						m_status = Status::errorILP;
					}

					if (m_status != Status::applyILP)
						return Verification::timeout;
					return icp ? Verification::cPlanarVerified : Verification::nonCPlanarVerified;
				}
			}
		}
	}

	return Verification::verificationFailed;  // never reached (silence compiler warning)
}

//#define PRINT_INFO

static bool areAdjacent(node v, node w)
{
	if(v->degree() > w->degree())
		std::swap(v, w);

	for(adjEntry adj : v->adjEntries) {
		if(w == adj->twinNode())
			return true;
	}
	return false;
}

static bool preprocessStep(ClusterGraph &C, Graph &G)
{
	bool modified = false;
	SList<node> toRemove;

	//
	// Case: Degree-0/1 vertices
	//

	for(node v : G.nodes)
	{
		bool removeV = false;

		if(v->degree() == 0)
			removeV = true;

		else if(v->degree() == 1) {
			node u = v->firstAdj()->twinNode();
			cluster cv = C.clusterOf(v);
			cluster cu = C.clusterOf(u);

			if(cv == cu)
				removeV = true;

			else {
				for(adjEntry adj : u->adjEntries) {
					node w = adj->twinNode();
					cluster cw = C.clusterOf(w);
					if(w == v || cw == cu) continue;

					List<cluster> path;
					C.commonClusterPath(u,w,path);

					for(cluster c : path) {
						if(c == cv) {
							removeV = true; break;
						}
					}
				}
			}
		}

		if(removeV)
			toRemove.pushBack(v);
	}

	if(!toRemove.empty()) {
#ifdef PRINT_INFO
		std::cout << "Remove " << toRemove.size() << " deg-0/1 vertices" << std::endl;
#endif
		modified = true;
		for(node vDel : toRemove)
			G.delNode(vDel);
		toRemove.clear();
	}
	OGDF_ASSERT(isLoopFree(G));

	//
	// Case: Degree-2 vertices
	//

	NodeArray<bool> marked(G,false); // mark nodes that shall be unsplit
	for(node v : G.nodes)
	{
		if(v->degree() != 2)
			continue;
		cluster cv = C.clusterOf(v);

		// the two neighbors
		node u = v->firstAdj()->twinNode();
		node w = v->lastAdj ()->twinNode();

		if(marked[u] || marked[w])
			continue;

		List<cluster> path;
		C.commonClusterPath(u,w,path);

		for(cluster c : path) {
			if(c == cv) {
				marked[v] = true;
				toRemove.pushBack(v);
				break;
			}
		}
	}


	if(!toRemove.empty()) {
#ifdef PRINT_INFO
		std::cout << "Unsplit " << toRemove.size() << " deg-2 vertices" << std::endl;
#endif
		modified = true;
		for(node vDel : toRemove) {
			node u = vDel->firstAdj()->twinNode();
			node w = vDel->lastAdj ()->twinNode();
			G.delNode(vDel);
			if(u != w && !areAdjacent(u,w))
				G.newEdge(u,w);
		}
		toRemove.clear();
	}
	OGDF_ASSERT(isLoopFree(G));

	SList<cluster> toRemoveC;

	//
	// Case: Clusters with no sub-clusters and only external edges, and at most 1 node with deg > 1
	//   -> Replace by star
	//

	for(cluster c : C.clusters) {
		if(c->cCount() > 0 || c->nCount() < 3)
			continue;

		bool replaceByStar = true;
		node w = nullptr; // node with deg > 1
		for(node v : c->nodes) {
			for(adjEntry adj : v->adjEntries) {
				if(C.clusterOf(adj->twinNode()) == c)
					replaceByStar = false;
			}
			if(v->degree() > 1) {
				if(w == nullptr)
					w = v;
				else
					replaceByStar = false;
			}
		}

		if(replaceByStar) {
#ifdef PRINT_INFO
			std::cout << "Replace cluster by star" << std::endl;
#endif
			modified = true;
			if(w == nullptr)
				w = *c->nBegin();
			for(node v : c->nodes) {
				if(v != w) {
					OGDF_ASSERT(v->degree() == 1);
					G.newEdge(v,w);
				}
			}
			C.delCluster(c);
			break;
		}
	}
	OGDF_ASSERT(isLoopFree(G));

	//
	// Case: Clusters with two nodes (and no sub-clusters)
	//

	for(cluster c : C.clusters) {
		if(c->nCount() == 2 && c->cCount() == 0 && c != C.rootCluster()) {
			toRemoveC.pushBack(c);
			node v = *c->nBegin();
			node w = *c->nBegin().succ();

			if(!areAdjacent(v,w))
				G.newEdge(v,w);
		}
	}

	if(!toRemoveC.empty()) {
#ifdef PRINT_INFO
		std::cout << "Remove " << toRemoveC.size() << " 2-node clusters" << std::endl;
#endif
		modified = true;
		for(cluster cDel : toRemoveC)
			C.delCluster(cDel);
		toRemoveC.clear();
	}
	OGDF_ASSERT(isLoopFree(G));

	//
	// Case: Singleton clusters
	//

	for(cluster c : C.clusters) {
		if(c->cCount() + c->nCount() == 1 && c != C.rootCluster())
			toRemoveC.pushBack(c);
	}

	if(!toRemoveC.empty()) {
#ifdef PRINT_INFO
		std::cout << "Remove " << toRemoveC.size() << " singleton clusters" << std::endl;
#endif
		modified = true;
		for(cluster cDel : toRemoveC)
			C.delCluster(cDel);
		toRemoveC.clear();
	}
	OGDF_ASSERT(isLoopFree(G));

	//
	// Case: Empty clusters
	//

	C.emptyClusters(toRemoveC);

	if(!toRemoveC.empty()) {
#ifdef PRINT_INFO
		std::cout << "Remove " << toRemoveC.size() << " empty clusters" << std::endl;
#endif
		modified = true;
		for(cluster cDel : toRemoveC)
			C.delCluster(cDel);
		toRemoveC.clear();
	}
	OGDF_ASSERT(isLoopFree(G));

	return modified;
}

void HananiTutteCPlanarity::preprocessing(ClusterGraph &C, Graph &G)
{
	while(preprocessStep(C,G));

	m_numNodesPreproc = G.numberOfNodes();
	m_numEdgesPreproc = G.numberOfEdges();
	m_numClustersPreproc = C.numberOfClusters();
}

}
