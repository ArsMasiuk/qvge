/** \file
 * \brief Provide an interface for edge label information
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

#pragma once

#include <ogdf/basic/GridLayout.h>
#include <ogdf/basic/GridLayoutMapped.h>
#include <ogdf/uml/PlanRepUML.h>


namespace ogdf {

// the available labels
// the five basic labels are not allowed to be changed,
// cause they have a special meaning/position, insert
// other labels between mult1/End2

enum class LabelType {
	End1 = 0,
	Mult1,
	Name,
	End2,
	Mult2,
	NumLabels  //!< the number of available labels at an edge
};

enum class UsedLabels {
	End1  = (1 << static_cast<int>(LabelType::End1)),         //  1
	Mult1 = (1 << static_cast<int>(LabelType::Mult1)),        //  2
	Name  = (1 << static_cast<int>(LabelType::Name)),         //  4
	End2  = (1 << static_cast<int>(LabelType::End2)),         //  8
	Mult2 = (1 << static_cast<int>(LabelType::Mult2)),        // 16
	lAll   = (1 << static_cast<int>(LabelType::NumLabels)) -1, // 31
};

// the basic single label defining class
// holds info about all labels for one edge
template <class coordType>
class OGDF_EXPORT EdgeLabel
{
public:
	static const int numberUsedLabels = static_cast<int>(UsedLabels::lAll);

	//construction and destruction
	EdgeLabel() { m_edge = 0; m_usedLabels = 0; }

	//bit pattern 2^labelenumpos bitwise
	explicit EdgeLabel(edge e, int usedLabels = numberUsedLabels) : m_usedLabels(usedLabels), m_edge(e)
	{
		for(int i = 0; i < m_numberLabelTypes; i++)
		{
			//zu testzwecken randoms
			m_xSize[i] = double(randomNumber(5,13))/50.0; //1
			m_ySize[i] = double(randomNumber(3,7))/50.0;  //1

			m_xPos[i] = 0;
			m_yPos[i] = 0;
		}
	}

	// Construction with specification of label sizes in arrays of length labelnum
	EdgeLabel(edge e, coordType w[], coordType h[], int usedLabels = numberUsedLabels) : m_usedLabels(usedLabels), m_edge(e)
	{
		for(int i = 0; i < m_numberLabelTypes; i++)
		{
			m_xSize[i] = w[i];
			m_ySize[i] = h[i];
			m_xPos[i] = 0;
			m_yPos[i] = 0;
		}
	}

	EdgeLabel(edge e, coordType w, coordType h, int usedLabels) : m_usedLabels(usedLabels), m_edge(e)
	{
		for (int i = 0; i < m_numberLabelTypes; i++)
			if (m_usedLabels & (1 << i)) {
				m_xPos[i] = 0.0;
				m_yPos[i] = 0.0;
				m_xSize[i] = w;
				m_ySize[i] = h;
			}
	}

	//copy constructor
	EdgeLabel(const EdgeLabel& rhs) : m_usedLabels(rhs.m_usedLabels), m_edge(rhs.m_edge)
	{
		for(int i = 0; i < m_numberLabelTypes; i++)
		{
			m_xPos[i] = rhs.m_xPos[i];
			m_yPos[i] = rhs.m_yPos[i];
			m_xSize[i] = rhs.m_xSize[i];
			m_ySize[i] = rhs.m_ySize[i];
		}
	}

	~EdgeLabel() { }

	//assignment
	EdgeLabel& operator=(const EdgeLabel& rhs)
	{
		if (this != &rhs)
		{
			m_usedLabels = rhs.m_usedLabels;
			m_edge = rhs.m_edge;
			int i;
			for(i = 0; i < m_numberLabelTypes; i++)
			{
				m_xPos[i] = rhs.m_xPos[i];
				m_yPos[i] = rhs.m_yPos[i];
				m_xSize[i] = rhs.m_xSize[i];
				m_ySize[i] = rhs.m_ySize[i];
			}
		}
		return *this;
	}

	EdgeLabel& operator|=(const EdgeLabel& rhs)
	{
		if (m_edge) {
			OGDF_ASSERT(m_edge == rhs.m_edge);
		}
		else
			m_edge = rhs.m_edge;
		if (this != &rhs)
		{
			m_usedLabels |= rhs.m_usedLabels;
			for (int i = 0; i < m_numberLabelTypes; i++)
				if (rhs.m_usedLabels & (1 << i)) {
					m_xPos[i] = rhs.m_xPos[i];
					m_yPos[i] = rhs.m_yPos[i];
					m_xSize[i] = rhs.m_xSize[i];
					m_ySize[i] = rhs.m_ySize[i];
				}
		}
		return *this;
	}


	//set
	void setX(LabelType elt, coordType x) { m_xPos[static_cast<int>(elt)] = x; }
	void setY(LabelType elt, coordType y) { m_yPos[static_cast<int>(elt)] = y; }
	void setHeight(LabelType elt, coordType h) { m_ySize[static_cast<int>(elt)] = h; }
	void setWidth(LabelType elt, coordType w) { m_xSize[static_cast<int>(elt)] = w; }
	void setEdge(edge e) { m_edge = e; }
	void addType(LabelType elt) { m_usedLabels |= (1<< static_cast<int>(elt)); }

	//get
	coordType getX(LabelType elt) const { return m_xPos[static_cast<int>(elt)]; }
	coordType getY(LabelType elt) const { return m_yPos[static_cast<int>(elt)]; }
	coordType getWidth(LabelType elt) const { return m_xSize[static_cast<int>(elt)]; }
	coordType getHeight(LabelType elt) const { return m_ySize[static_cast<int>(elt)]; }
	edge theEdge() const { return m_edge; }

	bool usedLabel(LabelType elt) const {
		return ( m_usedLabels & (1 << static_cast<int>(elt)) ) > 0;
	}

	int &usedLabel() { return m_usedLabels; }


private:

	static const int m_numberLabelTypes = static_cast<int>(LabelType::NumLabels);

	//the positions of the labels
	coordType m_xPos[m_numberLabelTypes];
	coordType m_yPos[m_numberLabelTypes];

	//the input label sizes
	coordType m_xSize[m_numberLabelTypes];
	coordType m_ySize[m_numberLabelTypes];

	//which labels have to be placed bit pattern 2^labelenumpos bitwise
	int m_usedLabels; //1 = only name, 5 = name and end2, ...

	//the edge of heaven
	edge m_edge;

	//the label text
#if 0
	string m_string;
#endif
};

//Interface to algorithm
template <class coordType>
class ELabelInterface
{
public:
	//constructor
	explicit ELabelInterface(PlanRepUML& pru)
	{
		//the PRU should not work with real world data but with
		//normalized integer values
		m_distDefault = 2;
		m_minFeatDist = 1;
		m_labels.init(pru.original());
		m_ug = 0;

		for(edge e : pru.original().edges)
			setLabel(e, EdgeLabel<coordType>(e, 0));
	}

	//constructor on GraphAttributes
	explicit ELabelInterface(GraphAttributes& uml) : m_ug(&uml)
	{
		//the GraphAttributes should work on real world data,
		//which can be floats or ints
		m_distDefault = 0.002;
		m_minFeatDist = 0.003;
		m_labels.init(uml.constGraph());

		for(edge e : uml.constGraph().edges)
			setLabel(e, EdgeLabel<coordType>(e, 0));
	}

	GraphAttributes& graph() { return *m_ug; }

	//set new EdgeLabel
	void setLabel(const edge &e, const EdgeLabel<coordType>& el) {
		m_labels[e] = el;
	}

	void addLabel(const edge &e, const EdgeLabel<coordType>& el) {
		m_labels[e] |= el;
	}

	//get info about current EdgeLabel
	EdgeLabel<coordType>& getLabel(edge e) { return m_labels[e]; }

	coordType getWidth(edge e, LabelType elt) {
		return m_labels[e].getWidth(elt);
	}
	coordType getHeight(edge e, LabelType elt) {
		return m_labels[e].getHeight(elt);
	}

	//get general information
	coordType& minFeatDist() { return m_minFeatDist; }
	coordType& distDefault() { return m_distDefault; }

private:

	EdgeArray<EdgeLabel<coordType> > m_labels; //holds all labels for original edges
	//the base graph
	GraphAttributes* m_ug;

	coordType m_distDefault; //default distance label/edge for positioner
	coordType m_minFeatDist; //min Distance label/feature in candidate posit.
};

}
