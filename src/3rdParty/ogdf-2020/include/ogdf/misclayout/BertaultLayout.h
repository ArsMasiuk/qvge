/** \file
 * \brief Declaration of class BertaultLayout.
 * Computes a force directed layout (Bertault Layout) for preserving the planar embedding in the graph.
 * The algorithm is based on the paper
 * "A force-directed algorithm that preserves
 * edge-crossing properties" by Francois Bertault
 *
 * \author Smit Sanghavi
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
#include <ogdf/basic/List.h>
#include <ogdf/basic/Array.h>
#include <ogdf/basic/Array2D.h>
#include <ogdf/basic/CombinatorialEmbedding.h>
#include <ogdf/planarity/PlanRep.h>

namespace ogdf {

class OGDF_EXPORT BertaultLayout : public LayoutModule
{
public:
	//! Constructor, sets options to default values.
	BertaultLayout();
	~BertaultLayout();

	//! Constructor, with user defined values for required length and number of iterations.
	BertaultLayout(double length, int number); // length= desired edge length... number= number of iterations

	//! Constructor, with user defined values for number of iterations.
	explicit BertaultLayout(int number); // number= number of iterations


	//! The main call to the algorithm. AG should have nodeGraphics and EdgeGraphics attributes enabled.
	virtual void call(GraphAttributes &AG) override;


	//! Sets impred option true or false.
	void setImpred(bool option) { impred=option;};

	//! Sets the number of iterations. If \p no <= 0, 10*n will be used.
	void iterno(int no) { userIterNo=no; }

	//! Returns the number of iterations
	int iterno() { return iter_no; }

	//! Sets the required length. If \p length <= 0, the average edge length
	//! will be used.
	void reqlength(double length) { userReqLength=length; }

	//! Returns the required length
	double reqlength() { return req_length; }

	/** Set the initPositions of nodes. Must for graphs without node attributes
	* c accepts character arguments:
	* 'm' for Grid-like Layout of nodes
	* 'c' for arranging nodes in concentric circles
	* 'r' for random arrangement of nodes
	*/
	void initPositions(GraphAttributes &AG,char c);

	//! Calculates the edge crossings in the graph corresponding to AG. Node attributes required.
	int edgeCrossings(GraphAttributes &AG);

	//! Calculates the normalised standard deviation of edge lengths in the graph corresponding to AG. Node attributes required.
	double edgelength(GraphAttributes &GA);

	//! Gives a measure of the node distribution in the graph corresponding to AG. The lesser the value, the more uniform the distribution. Node attributes required.
	double nodeDistribution(GraphAttributes &GA);

protected:

	//! Calculates the repulsive force on node v due to node j and adds it to total force on v
	void f_Node_Repulsive(node *v,node *j, GraphAttributes &AG);

	//! Calculates the attractive force on node v due to node j and adds it to total force on v
	void f_Node_Attractive(node *v,node *j, GraphAttributes &AG);

	//! Computes the projection of node v on the edge (a,b)
	void compute_I(node *v, edge *e, GraphAttributes &AG);

	//! Returns true if node i lies on the edge (a,b)
	bool i_On_Edge(edge *e, GraphAttributes &AG);

	//! Calculates the repulsive force on node v due to the edge on which node i lies and adds it to total force on v
	void f_Edge(node *v, edge *e, GraphAttributes &AG);

	//! Calculates the radii of the zones of node v if node i lies on edge (a,b)
	void r_Calc_On_Edge(node *v, edge *e, GraphAttributes &AG);

	//! Calculates the radii of the zones of node v if node i does not lie on edge (a,b)
	void r_Calc_Outside_Edge(node *v,edge *e, GraphAttributes &AG);

	//! Moves the node v according to the forces Fx and Fy on it. Also ensures that movement is within the respective zones
	void move(node *v, GraphAttributes &AG);

	//! Objects of this class are members of the containment heirarchy made in preprocessing stage of ImPrEd
	class CCElement
	{
	public:
		bool root; // denotes if a element is root
		int num; // The number of the connected component represented by	the object
		CCElement* parent;  // refers to parent of this object in the heirarchy
		int faceNum; // the index of the face of parent in which it is contained
		List <CCElement*> child; //list of CCElements refering to the CCs which are contained inside this CC

		// Initialises the CCElement to the ith CC
		void init(int i) {
			root = false;
			num = i;
			child.clear();
			parent = this;
		}
	};


private:
	//! The sections associated with each node
	class BertaultSections
	{
	public:
		double R[9]; //! Ri is radius of ith section

		//! Radii are initialised to std::numeric_limits<double>::max() at the start
		void initialize()
		{
			int i;
			for(i=0;i<9;i++)
				R[i] = std::numeric_limits<double>::max();
		}
	};


	//! preprocessing for ImPrEd
	void preprocess(GraphAttributes &AG);

	//! labels the edges with weights which aids in surrounding edge computation
	void labelling(GraphAttributes &AG);

	//! Inserts a node at each edge crossing in a GraphCopy and assigns weights to the new edges formed
	void crossingPlanarize(GraphAttributes &AG);

	//! Insert method for the data structure which stores the heirarchy of containment of Connected Components
	int insert(CCElement *new1,CCElement *node,GraphAttributes &PAG,PlanRep &PG);

	//! Checks if the first connected component is within the second one.
	//! Returns -1 if not contained.
	//! If contained, returns the index of the face of the second connected component which contains it
	int contained(CCElement *ele1,CCElement *ele2,GraphAttributes &PAG,PlanRep &PG);

	//! Computes the surrounding edges from the data calculated so far
	void compute(CCElement* element,PlanRep &PG,GraphAttributes &AG1,GraphCopy &G1);

	//! a structure which stores the projection of a node on an edge
	struct proj
	{
		double x;
		double y;
	} proj;
	NodeArray<BertaultSections> sect; //! Sections associated with all nodes
	NodeArray<double> F_x; //! Force in x direction
	NodeArray<double> F_y; //! Force in y direction
	double userReqLength; //! required edge length set by the user
	double userIterNo; //! number of iterations set by the user
	double req_length; //! req_length is the required edge length
	int iter_no; //! number of iterations to be performed
	bool impred; //! sets the algorithm to ImPrEd when true
	Array2D<bool> surr; //! stores the indices of the surrounding edges for each node

	OGDF_NEW_DELETE
};

}
