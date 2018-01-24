/** \file
 * \brief Declaration of class Edge.
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

namespace ogdf {
namespace energybased {
namespace fmmm {

//! helping data structure for deleting parallel edges in class FMMMLayout and
//! Multilevel (needed for the bucket sort algorithm)
class Edge
{

	//! outputstream for Edge
	friend std::ostream &operator<< (std::ostream & output, const Edge & E)
	{
		output <<"edge_index " << E.e->index() << " Graph_ptr " << E.Graph_ptr << " angle"
			<< E.angle << " cut vertex " << E.cut_vertex->index();
		return output;
	}

#if 0
	//! inputstream for Edge
	friend std::istream &operator>> (std::istream & input,  Edge & E)
	{
		input >> E;//.e>>E.Graph_ptr;
		return input;
	}
#endif

public:
	//! constructor
	Edge() {
		e = nullptr;
		Graph_ptr = nullptr;
		angle = 0;
		cut_vertex = nullptr;
	}

	void set_Edge (edge f,Graph* g_ptr) {
		Graph_ptr = g_ptr;
		e = f;
	}

	void set_Edge(edge f,double i,node c) {
		angle = i;
		e = f;
		cut_vertex = c;
	}

	Graph* get_Graph_ptr() const { return Graph_ptr; }
	edge get_edge() const { return e; }
	double get_angle() const { return angle; }
	node get_cut_vertex() const { return cut_vertex; }

private:
	edge e;
	Graph* Graph_ptr;
	double angle;
	node cut_vertex;
};


class EdgeMaxBucketFunc : public BucketFunc<Edge>
{
public:
	EdgeMaxBucketFunc() {};

	int getBucket(const Edge& E) override { return get_max_index(E); }

private:
	//! returns the maximum index of e
	int get_max_index(const Edge& E) {
		int source_index = E.get_edge()->source()->index();
		int target_index = E.get_edge()->target()->index();
		OGDF_ASSERT(source_index != target_index); // no self-loop
		if(source_index < target_index) {
			return target_index;
		} else {
			return source_index;
		}
	}
};


class EdgeMinBucketFunc : public BucketFunc<Edge>
{
public:
	EdgeMinBucketFunc() { }

	int getBucket(const Edge& E) override { return get_min_index(E); }

private:

	//! returns the minimum index of e
	int get_min_index(const Edge& E)
	{
		int source_index = E.get_edge()->source()->index();
		int target_index = E.get_edge()->target()->index();
		OGDF_ASSERT(source_index != target_index); // no self-loop
		if (source_index < target_index) {
			return source_index;
		} else {
			return target_index;
		}
	}
};

}
}
}
