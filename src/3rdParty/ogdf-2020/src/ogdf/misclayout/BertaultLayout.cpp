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


#include <ogdf/misclayout/BertaultLayout.h>

namespace ogdf {

BertaultLayout::BertaultLayout(double length, int number)
	: userReqLength(length)
	, userIterNo(number)
	, req_length(0)
	, iter_no(0)
	, impred(false)
	{}

BertaultLayout::BertaultLayout(int number) : BertaultLayout(0, number) {}

BertaultLayout::BertaultLayout() : BertaultLayout(0) {}

BertaultLayout::~BertaultLayout()
{
}

void BertaultLayout::call(GraphAttributes &AG)
{
	const Graph &G = AG.constGraph();
	if(G.numberOfNodes() == 0)
		return;
	if(!AG.has(GraphAttributes::nodeGraphics))
		return;
	if(AG.has(GraphAttributes::edgeGraphics))
		AG.clearAllBends();

	iter_no = userIterNo <= 0 ? G.numberOfNodes()*10 : userIterNo;

	if(userReqLength <= 0) {
		req_length = 0;
		for(edge e : G.edges)
		{
			node a=e->source();
			node b=e->target();
			req_length+=sqrt((AG.x(a)-AG.x(b))*(AG.x(a)-AG.x(b))+(AG.y(a)-AG.y(b))*(AG.y(a)-AG.y(b)));
		}
		req_length = req_length/(G.numberOfEdges() == 0 ? 1 : G.numberOfEdges());
	} else {
		req_length = userReqLength;
	}
	F_x.init(G);
	F_y.init(G);
	sect.init(G);

#if 0
	impred=true;
#endif

	if(impred)
		preprocess(AG);

	for(int k=0;k<iter_no;k++)
	{
		for(node v : G.nodes)
		{
			//initialise everything
			F_x[v]=0;
			F_y[v]=0;
			sect[v].initialize();
#if 0
			int i;
			for(i=1;i<9;i++)
			{
				std::cout << sect[v].R[i];
				std::cout << " ";
			}
			std::cout << "\n";
#endif
		}

		for(node v : G.nodes)
		{
			//calculate total node-node repulsive force
			for(node j : G.nodes)
			{
				if(j!=v)
				f_Node_Repulsive(&v,&j,AG);
			}

			//calculate total node-node attractive force
			for(adjEntry adj : v->adjEntries)
			{
				node ad=adj->twinNode();
				f_Node_Attractive(&v,&ad,AG);
			}
			//calculate total node-edge repulsive force
			for(edge e : G.edges)
			{
				if(e->target()!=v&&e->source()!=v)
				{

					compute_I(&v,&e,AG); //computes the projection

					if(i_On_Edge(&e,AG)) //computes if projection is on the edge
					{
						if((!impred)||surr(v->index(),e->index())==1)
						{
							f_Edge(&v,&e,AG);
						}
						r_Calc_On_Edge(&v,&e,AG); // updates values of section radii
					}
					else
					{
						r_Calc_Outside_Edge(&v,&e,AG); // updates values of section radii
					}

				}
			}
		}

		for(node v : G.nodes)
		{
#if 0
			std::cout << "F_x : ";
			std::cout << F_x[v];

			std::cout << "   F_y : ";
			std::cout << F_y[v];
			std::cout << "\n";
#endif
			//moves the nodes according to forces
			move(&v,AG);
#if 0
			std::cout << "move_x : ";
			std::cout << F_x[v];

			std::cout << "   move_y : ";
			std::cout << F_y[v];
			std::cout << "\n";

			int i;
			for(i=1;i<9;i++)
			{
				std::cout << sect[v].R[i];
				std::cout << " ";
			}
			std::cout << "\n\n";
#endif
		}
	}

}


void BertaultLayout::f_Node_Repulsive(node *v,node *j, GraphAttributes &AG)
{
	double dist=sqrt((AG.x(*v)-AG.x(*j))*(AG.x(*v)-AG.x(*j))+(AG.y(*v)-AG.y(*j))*(AG.y(*v)-AG.y(*j)));
	(F_x)[*v]+=((req_length)/dist)*((req_length)/dist)*(AG.x(*v)-AG.x(*j));
	(F_y)[*v]+=((req_length)/dist)*((req_length)/dist)*(AG.y(*v)-AG.y(*j));
}

void BertaultLayout::f_Node_Attractive(node *v,node *j, GraphAttributes &AG)
{
	double dist=sqrt((AG.x(*v)-AG.x(*j))*(AG.x(*v)-AG.x(*j))+(AG.y(*v)-AG.y(*j))*(AG.y(*v)-AG.y(*j)));
	(F_x)[*v]+=(-(dist/req_length)*(AG.x(*v)-AG.x(*j)));
	(F_y)[*v]+=(-(dist/req_length)*(AG.y(*v)-AG.y(*j)));
}

void BertaultLayout::compute_I(node *v,edge *e, GraphAttributes &AG)
{
	node a=(*e)->source();
	node b=(*e)->target();
	double m=(AG.y(a)-AG.y(b))/(AG.x(a)-AG.x(b)); //slope of edge
	double n=-1/m; //slope of a perpendicular
	double c=AG.y(a)-m*(AG.x(a)); //y=mx+c for edge
	double d=AG.y(*v)-n*(AG.x(*v)); //y=nx+d for the perpendicular
	proj.x=(d-c)/(m-n); //solve for x
	proj.y=m*proj.x+c; //solve for y
}

bool BertaultLayout::i_On_Edge(edge *e, GraphAttributes &AG)
{
	const node a = (*e)->source();
	const node b = (*e)->target();
	// x and y coordinates of i must be in between that of a and b
	const bool xGood = (proj.x <= AG.x(a) && proj.x>=AG.x(b))
	                || (proj.x >= AG.x(a) && proj.x<=AG.x(b));
	const bool yGood = (proj.y <= AG.y(a) && proj.y>=AG.y(b))
	                || (proj.y >= AG.y(a) && proj.y<=AG.y(b));
	return xGood && yGood;
}

void BertaultLayout::f_Edge(node *v,edge *e, GraphAttributes &AG)
{
	double dist=sqrt((AG.x(*v)-proj.x)*(AG.x(*v)-proj.x)+(AG.y(*v)-proj.y)*(AG.y(*v)-proj.y));

	// limit is the max distance (between a node and its projection) at which
	// the edge force on the node is considered. The value is taken from the
	// research paper but can be changed.
	double limit = 4*req_length;
	if(dist<=limit&&dist>0)
	{
		double fx=(limit-dist)*(limit-dist)*(AG.x(*v)-proj.x)/dist;
		double fy=(limit-dist)*(limit-dist)*(AG.y(*v)-proj.y)/dist;
		(F_x)[*v]+=fx;
		(F_y)[*v]+=fy;
		node a=(*e)->source();
		node b=(*e)->target();
		(F_x)[a]-=fx;
		(F_y)[a]-=fy;
		(F_x)[b]-=fx;
		(F_y)[b]-=fy;
	}
}

void BertaultLayout::r_Calc_On_Edge(node *v, edge *e, GraphAttributes &AG)
{
	node a=(*e)->source();
	node b=(*e)->target();
	int s=0;
	double x_diff=proj.x-AG.x(*v);
	double y_diff=proj.y-AG.y(*v);

	//determines the section in which the line-segment (v,i) lies
	if(x_diff>=0)
	{
		if(y_diff>=0)
		{
			if(x_diff>=y_diff)
				s=1;
			else
				s=2;
		}
		else
		{
			if(x_diff>=-(y_diff))
				s=8;
			else
				s=7;
		}
	}
	else
	{
		if(y_diff>=0)
		{
			if(-(x_diff)>=y_diff)
				s=4;
			else
				s=3;
		}
		else
		{
			if(-(x_diff)>=-(y_diff))
				s=5;
			else
				s=6;
		}
	}

	OGDF_ASSERT(s!=0); //section>=1
	double max_radius=(sqrt(x_diff*x_diff+y_diff*y_diff))/3;

#if 0
	std::cout << "node:" << (*v)->index() << "edge:" << (*e)->index()
	     << "between" << a->index() << "and" << b->index()
	     << "INSIDE\nsection:" << s
	     << "x_a:" << AG.x(a)
	     << "x_b:" << AG.x(b)
	     << "y_a:" << AG.y(a)
	     << "y_b:" << AG.y(b)
	     << "i_x:" << i.x
	     << "i_y:" << i.y
	     << "dist_v-i:" << max_radius*3 << "\n\n" ;
#endif

#if 0
	if(max_radius>=0) {
#endif
		int r,num;
		//determines which sections should have their values changed
		for(r=s-2;r<=(s+2);r++)
		{
			num=1+((r-1)%8);
			if(num<=0)
				num+=8;
			Math::updateMin(sect[*v].R[num], max_radius);
		}
		for(r=s+2;r<=(s+6);r++)
		{
			num=1+((r-1)%8);
			if(num<=0)
				num+=8;
			Math::updateMin(sect[a].R[num], max_radius);
			Math::updateMin(sect[b].R[num], max_radius);
		}
#if 0
	}
#endif
#if 0
	int i;
	for(i=1;i<9;i++)
	{
		std::cout << sect[*v].R[i];
		std::cout << " ";
	}
	std::cout << "\n";
#endif
}


void BertaultLayout::r_Calc_Outside_Edge(node *v, edge *e, GraphAttributes &AG)
{
	node a=(*e)->source();
	node b=(*e)->target();
	double dav=sqrt((AG.x(*v)-AG.x(a))*(AG.x(*v)-AG.x(a))+(AG.y(*v)-AG.y(a))*(AG.y(*v)-AG.y(a)));
	double dbv=sqrt((AG.x(*v)-AG.x(b))*(AG.x(*v)-AG.x(b))+(AG.y(*v)-AG.y(b))*(AG.y(*v)-AG.y(b)));
#if 0
	std::cout << "node:" << (*v)->index() << "edge:" << (*e)->index() << "between" << a->index() << "and" << b->index() << "OUTSIDE\ndist_v_a:" << dav << "dist_v_b:" << dbv << "\n\n" ;
#endif

	int r;
	for(r=1;r<=8;r++)
	{
		Math::updateMin(sect[*v].R[r], min(dav, dbv) / 3);
		Math::updateMin(sect[a].R[r], dav / 3);
		Math::updateMin(sect[b].R[r], dbv / 3);
	}

#if 0
	int i;
	for(i=1;i<9;i++)
	{
		std::cout << sect[*v].R[i];
		std::cout << " ";
	}
	std::cout << "\n";
#endif
}

void BertaultLayout::move(node *v, GraphAttributes &AG)
{
	int s=0;
	double x_diff=(F_x)[*v];
	double y_diff=(F_y)[*v];

	//determines the section in which the node has to move
	if(x_diff>=0)
	{
		if(y_diff>=0)
		{
			if(x_diff>=y_diff)
				s=1;
			else
				s=2;
		}
		else
		{
			if(x_diff>=-(y_diff))
				s=8;
			else
				s=7;
		}
	}
	else
	{
		if(y_diff>=0)
		{
			if(-(x_diff)>=y_diff)
				s=4;
			else
				s=3;
		}
		else
		{
			if(-(x_diff)>=-(y_diff))
				s=5;
			else
				s=6;
		}
	}

	OGDF_ASSERT(s!=0);

	double mov_mag=sqrt((x_diff*x_diff)+(y_diff*y_diff)); // the length of the move
	if ((sect)[*v].R[s] < mov_mag) // if move is greater than zone(section) radius
	{
		// magnitudes of forces are normalised so that the length becomes equal to the radius
		// and the move will now take the node on the arc of that section
		(F_x)[*v]=((F_x)[*v]/mov_mag)*(sect)[*v].R[s];
		(F_y)[*v]=((F_y)[*v]/mov_mag)*(sect)[*v].R[s];
	}
	//moves the node
	AG.x(*v)+=(F_x)[*v];
	AG.y(*v)+=(F_y)[*v];
}

void BertaultLayout::initPositions(GraphAttributes &AG, char c)
{
	if( !AG.has(GraphAttributes::nodeGraphics) && (c=='c'||c=='m'||c=='r'))
	{
		if(req_length==0)
			req_length=50;
		AG.addAttributes((AG.attributes()|GraphAttributes::nodeGraphics|GraphAttributes::edgeGraphics|GraphAttributes::nodeStyle|GraphAttributes::edgeStyle));
		const Graph &G = AG.constGraph();
		int m = (int) sqrt((double)G.numberOfNodes());
		int cnth=0,cntc=0;
		int dim = (int)(req_length*G.numberOfNodes()/2);
		srand ( (unsigned int) time(nullptr) );
		for(node v : G.nodes)
		{
			if(c=='r')
			{
				int flag=1;
				while(flag==1)
				{
					AG.x(v)=(double)(rand()%dim)-dim/2;
					AG.y(v)=(double)(rand()%dim)-dim/2;
					flag=0;
					for(node x : G.nodes)
					{
						if(x==v)
							break;
						if(AG.x(v)==AG.x(x)&&AG.y(v)==AG.y(x))
						{
							flag=1;
							break;
						}
					}
				}
			}
			else
			{
				int flag=1;
				while(flag==1)
				{
					if(c=='c')
					{
						double r=req_length*(cntc+1)/2;
						double ang=(2*Math::pi/m)*cnth;
						double cs=cos(ang);
						double sn=sin(ang);
						if((cs<1.0e-8&&cs>0)||(cs>-1.0e-8&&cs<0))
						{
							if(sn<0)
								sn=-1;
							else
								sn=1;

							cs=0;
						}
						if((sn<1.0e-8&&sn>0)||(sn>-1.0e-8&&sn<0))
						{
							if(cs<0)
								cs=-1;
							else
								cs=1;

							sn=0;
						}

						AG.x(v)=r*cs;
						AG.y(v)=r*sn;
#if 0
						if((AG.x(v)*AG.x(v)+AG.y(v)*AG.y(v))!=r*r)
						{
							std::cout << (AG.x(v)*AG.x(v)+AG.y(v)*AG.y(v))-(r*r);
							std::cout << "\n";
						}
#endif
					}
					else if(c=='m')
					{
						AG.x(v)=req_length*cnth/2;
						AG.y(v)=req_length*cntc/2;
					}

					flag=0;
					for(node x : G.nodes)
					{
						if(x==v)
							break;
						if(AG.x(v)==AG.x(x)&&AG.y(v)==AG.y(x))
						{
							flag=1;
							cnth--;
							break;
						}
					}
				}

				cnth++;
				if(cnth==m)
				{
					cnth=0;
					cntc++;
				}
			}

			AG.width(v)=req_length/10;
			AG.height(v)=req_length/10;
		}
	}
}

void BertaultLayout::preprocess(GraphAttributes &AG)
{
	const Graph &G = AG.constGraph();
	surr.init(0,G.numberOfNodes()-1,0,G.numberOfEdges()-1,0);
	GraphCopy G1(G);
	GraphAttributes AG1(G1);
	AG1.directed() = AG.directed();
	for(node v : G1.nodes)
	{
		AG1.x(v)=AG.x(G1.original(v));
		AG1.y(v)=AG.y(G1.original(v));
		AG1.width(v)=AG.width(G1.original(v));
		AG1.height(v)=AG.height(G1.original(v));
	}

	labelling(AG1);
	crossingPlanarize(AG1);

#if 0
	surr.init(G);
	for(node v : G.nodes)
	{
		surr[v].init(G);
		for(edge e : G.edges)
		{
			surr[v][e]=0;
		}
	}
	getOriginal(AG);
#endif

	PlanRep PG(AG1);
	int numCC = PG.numberOfCCs();
#if 0
	GraphAttributes PAG(PG, GraphAttributes::nodeGraphics|GraphAttributes::edgeGraphics);
	List< node > list;

	std::cout << numCC <<"\n";
#endif

	List<CCElement*> forest;
	Array<CCElement> Carr(numCC);

	for(int i = 0; i < numCC; i++)
	{
		Carr[i].init(i);
		Carr[i].faceNum=-1;
	}

	for(int i = 0; i < numCC; i++)
	{
		CCElement* new1=&Carr[i];
		int rootnum=0,flag=0;
			while(rootnum<forest.size())
			{
#if 0
				CCElement *Croot=&(**(forest.get(rootnum)));
				std::cout << "Children of " << (**(forest.get(rootnum))).num <<"bfor calling :\n";
				int k;
				for(k=0;k<(**(forest.get(rootnum))).child.size();k++)
				{
					std::cout << (*((**(forest.get(rootnum))).child.get(k)))->num << "\n";
				}
				std::cout << "Inserting " << (*new1).num << " into " << (**(forest.get(rootnum))).num << "\n";
#endif

				int retv=insert(new1,&(**(forest.get(rootnum))),AG1,PG);
				if(retv==2)
				{
					flag=1;
					break;
				}
				else if(retv==1)
				{
					(**(forest.get(rootnum))).root=false;
					forest.del(forest.get(rootnum));
					rootnum--;
				}
				rootnum++;
			}

		if(flag==0)
		{
			(*new1).faceNum=-1;
			(*new1).root=true;

			forest.pushBack(&(*new1));
#if 0
			std::cout << (*new1).num << " is a root now " << "\nChildren of " << (*new1).num << ":\n";
			int j;
			for(j=0;j<(*new1).child.size();j++)
			{
				std::cout << (*((*new1).child.get(j)))->num << "\n";
			}
#endif
		}
	}

	// Uncomment below statements to see output... for debugging use
#if 0
	std::cout << "forest size : " << forest.size();
	for(int i=0;i<forest.size();i++)
	{
		std::cout << "\nRoot : " << i <<"\n";
		compute(*forest.get(i),PG,AG1,G1);
	}

	node n=G.chooseNode();
	AG.fillColor(n)="RED";
	for(edge e : G.edges)
	{
		if(surr(n->index(), e->index()))
			AG.strokeColor(e)="RED";
	}

	GraphIO::writeGML(AG, "planarized.gml");
#endif
}

void BertaultLayout::labelling(GraphAttributes &AG)
{
	AG.addAttributes(GraphAttributes::edgeIntWeight);
	for(edge e : AG.constGraph().edges)
	{
		AG.intWeight(e)=e->index();
	}
}

void BertaultLayout::crossingPlanarize(GraphAttributes &AG)
{
	Graph &G= const_cast<Graph&> (AG.constGraph());

	for(edge e : G.edges)
	{
		for(edge i = G.lastEdge(); i != e; i = i->pred())
		{
			node a=e->source();
			node b=e->target();
			double m=(AG.y(a)-AG.y(b))/(AG.x(a)-AG.x(b));
			double c=AG.y(a)-m*AG.x(a);

			node x=i->source();
			node y=i->target();

			if(a!=x&&a!=y&&b!=x&&b!=y)
			{
				double m2=(AG.y(x)-AG.y(y))/(AG.x(x)-AG.x(y));
				double c2=AG.y(x)-m2*AG.x(x);

				double ainc=(AG.y(a)-m2*AG.x(a)-c2),binc=(AG.y(b)-m2*AG.x(b)-c2),xinc=AG.y(x)-m*AG.x(x)-c,yinc=AG.y(y)-m*AG.x(y)-c;
#if 0
				int temp=AG.intWeight(e);
#endif

				if(xinc*yinc<0&&ainc*binc<0)
				{
#if 0
					std::cout << "edge " << e->index()
					     << " and edge " << i->index()
					     << " between nodes " << a->index()
					     << " and " << b->index()
					     << " and nodes " << x->index()
					     << " and " << y->index() << "\n";
#endif
					int temp=AG.intWeight(e);
					edge enew=G.split(e);
					node nnew=enew->source();
					AG.width(nnew)=AG.width(a);
					AG.height(nnew)=AG.height(a);
					AG.x(nnew)=(c2-c)/(m-m2);
					AG.y(nnew)=m*AG.x(nnew)+c;
					AG.intWeight(enew)=temp;
					edge xn=G.newEdge(x,nnew);
					AG.intWeight(xn)=AG.intWeight(i);
					AG.intWeight(G.newEdge(nnew,y))=AG.intWeight(i);
					G.delEdge(i);
				}
			}
			//*/
		}
	}
}

int BertaultLayout::insert(CCElement *new1,CCElement *element,GraphAttributes &PAG,PlanRep &PG)
{
	int contface=contained(new1,element,PAG,PG);
	if(contface!=-1)
	{
		int flag=0;
#if 0
		std::cout << "Children of " << (*element).num <<"bfor :\n";
		for(int i=0;i<(*element).child.size();i++)
		{
			std::cout << (*((*element).child.get(i)))->num << "\n";
		}
#endif
		if((*element).child.size()!=0)
		{

			for (int i = 0; i < (*element).child.size(); i++)
			{
				CCElement *child = &(**((*element).child.get(i)));
				if (child->faceNum == contface)
				{
#if 0
					std::cout << "Gonna insert " << (*new1).num << " in " << child->num << "\n";
#endif

					int retv = insert(new1, child, PAG, PG);
					if (retv == 2)
					{
						flag = 1;
						break;
					}
					else if (retv == 1)
					{
						i--;
					}
				}
			}
		}

		if(flag==0)
		{
			(*new1).parent=&(*element);
			(*new1).faceNum=contface;
			(*element).child.pushBack(&(*new1));
#if 0
			std::cout << (*new1).num << " is child of " << new1->parent->num << "\nChildren of " << (*element).num << ":\n";
			for(i=0;i<(*element).child.size();i++)
			{
				std::cout << (*((*element).child.get(i)))->num << "\n";
			}

			std::cout << "Children of " << (*new1).num <<" ("<< (*new1).child.size() <<"):\n";
			for(i=0;i<(*new1).child.size();i++)
			{
				std::cout << (*((*new1).child.get(i)))->num << "\n";
			}
#endif

		}
		return 2;
	}
	else
	{
		contface=contained(element,new1,PAG,PG);
		if (contface!=-1) {
			if (!(*element).root) { // delete from children list
				(*element).parent->child.removeFirst(element);
			}
			(*element).faceNum = contface;
			(*element).parent = new1;
			(*new1).child.pushBack(element);

#if 0
			std::cout << "Pushed " << (*element).num << " into " << (*new1).num << "\n";

			std::cout << (*new1).num << " is parent of " << (*element).num << "\nChildren of " << (*new1).num << ":\n";

			int i;
			for(i=0;i<(*new1).child.size();i++)
			{
				std::cout << (**((*new1).child.get(i))).num << "\n";
			}
#endif
			return 1;
		}
		else
		{
			return 0;
		}
	}
}


int BertaultLayout::contained(CCElement* new1,CCElement* element,GraphAttributes &PAG,PlanRep &PG)
{
#if 0
	PlanRep &PG= (PlanRep&)(const_cast<Graph&> (PAG.constGraph()));
#endif

	PG.initCC(new1->num);
	node v;

	v=PG.chooseNode();
	double yc=PAG.y(PG.original(v));
	double xc=PAG.x(PG.original(v));

#if 0
	std::cout << "Number of edges in " << new1->num << " is " << PG.numberOfEdges() << "\n";
#endif

	PG.initCC(element->num);
#if 0
	std::cout << "Number of edges in " << element->num << " is " << PG.numberOfEdges() << "\n";
	std::cout << "Chosen " << xc;
#endif
	ConstCombinatorialEmbedding E(PG);
	E.computeFaces();
	for (face f : E.faces)
	{
		int crossings = 0;
		List< int > edges;
		for (adjEntry adj : f->entries)
		{

			if (!edges.search(adj->theEdge()->index()).valid())
			{
				edges.pushBack(adj->theEdge()->index());
				node x = adj->theEdge()->source();
				node y = adj->theEdge()->target();
				double m = (PAG.y(PG.original(x)) - PAG.y(PG.original(y))) / (PAG.x(PG.original(x)) - PAG.x(PG.original(y)));

				double c = PAG.y(PG.original(x)) - m*PAG.x(PG.original(x));
				if ((PAG.y(PG.original(x)) - yc)*(PAG.y(PG.original(y)) - yc) <= 0 && ((yc - c) / m) >= xc)
				{
					crossings++;
				}

			}

		}

		if (crossings % 2 != 0)
		{
#if 0
			std::cout << "yo";
#endif
			return f->index();
		}
	}
	return -1;
}


void BertaultLayout::compute(CCElement* element, PlanRep &PG, GraphAttributes &AG1, GraphCopy &G1)
{
	int num = element->num;
	PG.initCC(num);
	ConstCombinatorialEmbedding E(PG);
#if 0
	E.computeFaces();
	FaceArray< List<edge> > farray(E);
#endif
	for (face f : E.faces)
	{
#if 0
		std::cout<<"yeah";
#endif
		for (adjEntry adj : f->entries)
		{
			node ver = adj->theNode();
			node ver2 = adj->twinNode();
			bool dum = false, dum2 = false;
			if (G1.isDummy(PG.original(ver)))
				dum = true;
			if (G1.isDummy(PG.original(ver2)))
				dum2 = true;

			node v = G1.original(PG.original(ver));
			node v2 = G1.original(PG.original(ver2));

#if 0
			std::cout << v->index() << "\n";
#endif
			adjEntry adj1 = f->firstAdj(), adj3 = adj1;
			do {
#if 0
				std::cout<<"lala\n";
#endif
				if (!dum)
					surr(v->index(), AG1.intWeight(PG.original(adj3->theEdge()))) = true;
				if (!dum2)
					surr(v2->index(), AG1.intWeight(PG.original(adj3->theEdge()))) = true;
				adj3 = adj3->faceCycleSucc();
			} while (adj3->index() != adj1->index());

			int j;
			for (j = 0; j < element->child.size(); j++)
			{
				if ((*(element->child.get(j)))->faceNum == f->index())
				{
					PG.initCC((*(element->child.get(j)))->num);
					ConstCombinatorialEmbedding E2(PG);
					E2.computeFaces();
					for (face f2 : E2.faces)
					{
						for(adjEntry adj2 : f2->entries)
						{
							if (!dum)
								surr(v->index(), AG1.intWeight(PG.original(adj2->theEdge()))) = true;
							if (!dum2)
								surr(v2->index(), AG1.intWeight(PG.original(adj2->theEdge()))) = true;
						}
					}
				}
			}

			if (element->faceNum != -1)
			{
				PG.initCC(element->parent->num);
				ConstCombinatorialEmbedding E3(PG);
				E3.computeFaces();
#if 0
				int flag=0;
#endif
				face f2 = nullptr;
				for (face fi : E3.faces)
				{
					if (fi->index() == element->faceNum)
					{
#if 0
						flag=1;
#endif
						f2 = fi;
						break;
					}
				}

				for(adjEntry adj2 : f2->entries)
				{
					if (!dum)
						surr(v->index(), AG1.intWeight(PG.original(adj2->theEdge()))) = true;
					if (!dum2)
						surr(v2->index(), AG1.intWeight(PG.original(adj2->theEdge()))) = true;
				}
			}
			PG.initCC(num);
		}
	}
#if 0
	std::cout << num <<" Done\n";
#endif

	int i;
	for (i = 0; i < element->child.size(); i++)
	{
		compute(*(element->child.get(i)), PG, AG1, G1);
	}
}


int BertaultLayout::edgeCrossings(GraphAttributes &AG)
{
	const Graph &G = AG.constGraph();
	int crossings = 0;
	for (edge e : G.edges)
	{
		node a = e->source();
		node b = e->target();
		double m = (AG.y(a) - AG.y(b)) / (AG.x(a) - AG.x(b));
		double c = AG.y(a) - m*AG.x(a);
		for(edge i = G.lastEdge(); i != e; i = i->pred())
		{
			node x = i->source();
			node y = i->target();
			double m2 = (AG.y(x) - AG.y(y)) / (AG.x(x) - AG.x(y));
			double c2 = AG.y(x) - m2*AG.x(x);
			double distab = sqrt((AG.x(a) - AG.x(b))*(AG.x(a) - AG.x(b)) + (AG.y(a) - AG.y(b))*(AG.y(a) - AG.y(b)));
			double distxy = sqrt((AG.x(x) - AG.x(y))*(AG.x(x) - AG.x(y)) + (AG.y(x) - AG.y(y))*(AG.y(x) - AG.y(y)));
			double distax = sqrt((AG.x(a) - AG.x(x))*(AG.x(a) - AG.x(x)) + (AG.y(a) - AG.y(x))*(AG.y(a) - AG.y(x)));
			double distay = sqrt((AG.x(a) - AG.x(y))*(AG.x(a) - AG.x(y)) + (AG.y(a) - AG.y(y))*(AG.y(a) - AG.y(y)));
			double distbx = sqrt((AG.x(x) - AG.x(b))*(AG.x(x) - AG.x(b)) + (AG.y(x) - AG.y(b))*(AG.y(x) - AG.y(b)));
			double distby = sqrt((AG.x(y) - AG.x(b))*(AG.x(y) - AG.x(b)) + (AG.y(y) - AG.y(b))*(AG.y(y) - AG.y(b)));
			double d = distab + distxy;

			if (a != x&&a != y&&b != x&&b != y)
			{
				double ainc = (AG.y(a) - m2*AG.x(a) - c2), binc = (AG.y(b) - m2*AG.x(b) - c2), xinc = AG.y(x) - m*AG.x(x) - c, yinc = AG.y(y) - m*AG.x(y) - c;

				if ((xinc * yinc < 0 && ainc * binc < 0)
				 || (xinc * yinc == 0 && ainc * binc < 0)
				 || (xinc * yinc < 0 && ainc * binc == 0))
				{
#if 0
					std::cout << "edge " << e->index()
					     << " and edge " << i->index()
					     << " between nodes " << a->index()
					     << " and " << b->index()
					     << " and nodes " << x->index()
					     << " and " << y->index() << "\n";
#endif
					crossings++;
				}
				else
				{
					if (m == m2&&c == c2&&distax < d&&distay < d&&distbx < d&&distby < d)
					{
#if 0
						std::cout << "edge " << e->index()
						     << " and edge " << i->index()
						     << " between nodes " << a->index()
						     << " and " << b->index()
						     << " and nodes " << x->index()
						     << " and " << y->index() << "OVERLAP\n";
#endif
						crossings += 2;
					}
				}
			}
			else if (m == m2 && c == c2
			      && distax < d &&distay < d &&distbx < d &&distby < d
			      && ((a != y && b != x && b != y)
			       || (a != x && b != x && b != y)
			       || (a != y && a != x && b != y)
			       || (a != y && a != x && b != x)))
			{
#if 0
				std::cout << "edge " << e->index()
				     << " and edge " << i->index()
				     << " between nodes " << a->index()
				     << " and " << b->index()
				     << " and nodes " << x->index()
				     << " and " << y->index() << "OVERLAP\n";
#endif
				crossings += 1;
			}

		}
	}
	return crossings;
}


double BertaultLayout::edgelength(GraphAttributes &AG)
{
	EdgeArray<double> el;
	const Graph &G = AG.constGraph();
	el.init(G);
	double mean = 0, stdev = 0;
	for (edge e : G.edges)
	{
		node a = e->source();
		node b = e->target();
		el[e] = sqrt((AG.x(a) - AG.x(b))*(AG.x(a) - AG.x(b)) + (AG.y(a) - AG.y(b))*(AG.y(a) - AG.y(b)));
		mean += el[e];
	}
	mean = mean / (G.numberOfEdges());
	for (edge e : G.edges)
	{
		stdev += (el[e] - mean)*(el[e] - mean);
	}
	stdev = sqrt(stdev / (G.numberOfEdges())) / mean;
#if 0
	std::cout << "\n";
	std::cout << "mean : ";
	std::cout << mean;
	std::cout << "\n";
#endif
	return stdev;
}


double BertaultLayout::nodeDistribution(GraphAttributes &AG)
{
	const Graph &G = AG.constGraph();
	if (G.numberOfNodes()<2)
		return -1;
	double
		minx = AG.x(G.firstNode()),
		maxx = minx,
		miny = AG.y(G.firstNode()),
		maxy = miny;

	for (node v: G.nodes) {
		if (AG.x(v) > maxx)
			maxx = AG.x(v);
		if (AG.x(v) < minx)
			minx = AG.x(v);
		if (AG.y(v) > maxy)
			maxy = AG.y(v);
		if (AG.y(v) < miny)
			miny = AG.y(v);
	}

	int rows = 8, columns = 8, i, j;
	double sizex = (maxx - minx) / (double) (columns - 1), sizey = (maxy - miny) / (double) (columns - 1);
	double startx = minx - sizex / 2, /* endx = maxx+sizex/2, */ starty = miny - sizey / 2 /* , endy = maxy+sizey/2 */;
#if 0
	int box[rows][columns];
#endif
	Array2D<int> box(0, rows - 1, 0, columns - 1);

	for (i = 0; i < rows; i++) {
		for (j = 0; j < columns; j++)
			box(i, j) = 0;
	}
	if (maxy != miny && maxx != minx)
	{
		for (node v : G.nodes)
		{
			box((int) ((AG.y(v) - starty) / sizey), (int) ((AG.x(v) - startx) / sizex))++;
		}

		double mean = (double) G.numberOfNodes() / (double) (rows*columns);
		double stdev = 0;
		for (i = 0; i < rows; i++)
		for (j = 0; j < columns; j++)
		{
#if 0
			std::cout << box(i,j);
#endif
			stdev += ((double) (box(i, j)) - mean)*((double) (box(i, j)) - mean);
		}

#if 0
		std::cout << "\n";
#endif
		stdev = sqrt(stdev / (rows*columns)) / mean;
		return stdev;
	}
	return -1;
}

}
