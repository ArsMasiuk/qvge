/**************************************************************************************************
 Software License Agreement (BSD License)
 Copyright (c) 2011-2013, LAR toolkit developers - University of Aveiro - http://lars.mec.ua.pt
 All rights reserved.
 Redistribution and use in source and binary forms, with or without modification, are permitted
 provided that the following conditions are met:
  *Redistributions of source code must retain the above copyright notice, this list of
   conditions and the following disclaimer.
  *Redistributions in binary form must reproduce the above copyright notice, this list of
   conditions and the following disclaimer in the documentation and/or other materials provided
   with the distribution.
  *Neither the name of the University of Aveiro nor the names of its contributors may be used to
   endorse or promote products derived from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************************************/
/**
\file
\brief GVGraph class declaration
*/

#ifndef _GRAPH_WRAPPER_H_
#define _GRAPH_WRAPPER_H_

#include <iostream>
#include <graphviz/gvc/gvc.h>
#include <graphviz/cgraph/cgraph.h>
//#include <boost/lexical_cast.hpp>
#include <algorithm>
#include <vector>
#include <map>

using namespace std;
//using namespace boost;
		  
/**
\brief An object containing a libgraph graph and its associated nodes and edges.\n
This class is based around the Libgraph and STL containers, maps, vectors and pairs (their applications is a bit crazy and complicated but
works fine, try to use a double pair inside a map and check yourself).
For additional information obviously check the Libgraph documentation (i wish you luck, seriously you will need it!).
*/
class GVGraph
{
	public:
		/// Default DPI value used by dot (which uses points instead of pixels for coordinates)
		static const double DotDefaultDPI;

		/**
		\brief Construct a Graphviz graph object
		
		\param name The name of the graph, must be unique in the application
		\param node_size The size in pixels of each node
		*/
        GVGraph(string name, double node_size=50);
		
		/**
		\brief Graphviz graph destructor
		
		This function calls \c gvFreeLayout, \c agclose and \c gvFreeContext.
		*/
		~GVGraph();
		
		
		/**
		\brief Add a new node to the graph
		
		This function adds a new node to the graph, if a node with the same name already
		exits it will remove it first.
		\param name the name of the new node
		*/
		void addNode(const string& name);
		
		/**
		\brief Add several nodes
		
		This function adds a vector of nodes, it will call \c addNode on each vector element.
		\param names stl vector with the names to add
		*/
		void addNodes(vector<string>& names);
		
		/**
		\brief Remove a node
		
		This function will remove a node and all the edges attached to it using the \c removeEdge
		function.
		This function will change the _nodes map, so don't use it inside a loop using a _nodes iterator,
		it will change at each iteration.
		\param name name of the node to remove
		*/
		void removeNode(const string& name);
		
		/**
		\brief Clear the whole graph
		
		Clears the whole graph of nodes and edges using the \c removeNode function.
		*/
		void clearNodes();

		/**
		\brief Selects a node
		
		This function returns the \c Agnode_t pointer to a specific node. It is useful
		when functions require the node pointer for example attribute setting functions.
		\param name name of the node to select
		*/
        Agnode_t* selectNode(string &name);
		
        Agedge_t* selectEdge(string &source,string &target);
		
		/**
		\brief Sets node attributes
		
		Set the value of a node attribute using \c agsafeset.
		\param name name of the node
		\param attribute name of the attribute
		\param value value of the attribute
		*/
		int setNodeAttribute(string name,string attribute,string value);
		
		int setEdgeAttribute(string source,string target,string attribute,string value);
		
		/**
		\brief Add a new edge
		
		Add a new edge between source and target if it does not exist.
		\param source source node for the new edge
		\param target target node for the new edge
		*/
        void addEdge(const string &name, const string& source, const string& target);
		
		/**
		\brief Remove a edge
		
		Remove the edge between the source and target. This function calls
		the other \c removeEdge function.
		\param source source node of the edge
		\param target target node of the edge
		*/
		void removeEdge(const string& source, const string& target);
		
		/**
		\brief Remove a edge
		
		Remove the edge between the source and target. <b>This function should not
		be called directly</b>.
		\param key pair key for the edge map
		*/
		void removeEdge(const pair<string, string>& key);
		
		/**
		\brief Set a root node for the graph
		
		Set a root node to start the drawing.
		\param name name of the node
		*/
		void setRootNode(const string& name);
		
		/**
		\brief Apply a new layout
		
		This function applies a new layout, for now it will only
		use the \c dot layout algorithm.
		*/
		void applyLayout();
		
		/**
		\brief Free the layout
		This function should be called before any modifications to the current 
		graph, and the function applyLayout should be called after all modifications are done.
		*/
		void freeLayout();
		
		/**
		\brief Start render
		
		This function starts the render engine using the magnificent \c xgtk plug-in.
		It calls \c gvRender.
		*/
		void startRender();
		
		/**
		\brief Get the current GVC context
		
		Obtain the current \c GVC context pointer.
		*/
        GVC_t* getGVCcontext(void);
		
	private:
		/**
		\brief Open the graph
		
		Open the graph by calling \c agopen.
		\param name name of the graph.
		\param kind kind of graph, see the \c libgraph documentation for details.
		*/
        Agraph_t* _agopen(string name/*,int kind*/);
		
		/**
		\brief Set node attributes
		
		Set attributes for all nodes
		\param name name of the attribute
		\param value value of the attribute
		*/
		Agsym_t* _agnodeattr(string name,string value);
		
		/**
		\brief Set edge attributes
		
		Set attributes for all edge
		\param name name of the attribute
		\param value value of the attribute
		*/
		Agsym_t* _agedgeattr(string name,string value);
		
		/**
		\brief Get graph attribute
		
		Add an alternative value parameter to the method for getting an object's attribute
		\param object graph object, this should not be necessary but it is
		\param attr name of the attribute
		\param alt alternative value
		\return attribute value
		*/
        string _agget(void *object,string attr,string alt=string());

		/**
		\brief Set attribute
		
		Directly use agsafeset which always works, contrarily to agset.
		\param object graph, node or edge object
		\param attr name of the attribute
		\param value value of the attribute
		\return \c agsafeset return
		*/
        int _agset(void *object,string attr,string value);
		
		/**
		\brief Low level function to add new nodes
		
		Low level function to add new nodes, directly calls \c agnode.
		\param name name of the new node
		\return \c Agnode pointer object
		*/
		Agnode_t* _agnode(string name);
		
		/**
		\brief Layout wrapper function
		
		Wrapper for the layout function.
		\param gvc gv context pointer object
		\param graph graph pointer
		\param engine layout engine
		\return \c gvLayout return value
		*/
        int _gvLayout(GVC_t *gvc,graph_t *graph,string engine);
		
		///GV context main variable
        GVC_t *_context;
		
		///Agraph main object
        Agraph_t *_graph;
		
		///Node map, used for node tracking
		map<string, Agnode_t*> _nodes;
		
		///Edge map, used for edge tracking
		map<pair<string,string>, Agedge_t*> _edges;
};

#endif
