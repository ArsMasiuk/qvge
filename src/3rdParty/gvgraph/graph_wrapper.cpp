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
\brief GVGraph class implementation code
*/

#include "graph_wrapper.h"

Agnode_t* GVGraph::selectNode(string &name)
{
	map<string, Agnode_t*>::iterator it = _nodes.find(name);
	if(it!=_nodes.end())
		return it->second;
	else
        return nullptr;
}

Agedge_t* GVGraph::selectEdge(string &source,string &target)
{
	pair<string, string> key; //= std::make_pair<string,string>(source,target);
	key.first = source;
    key.second = target;
	
	if(_edges.find(key)!=_edges.end())
    	return _edges[key];
    
    return nullptr;
}
		
int GVGraph::setNodeAttribute(string name,string attribute,string value)
{
	return agsafeset(selectNode(name),const_cast<char*>(attribute.c_str()),const_cast<char*>(value.c_str()),const_cast<char*>(""));

// 	return _agset(selectNode(name),attribute,value);
}

int GVGraph::setEdgeAttribute(string source,string target,string attribute,string value)
{
	return agsafeset(selectEdge(source,target),const_cast<char*>(attribute.c_str()),const_cast<char*>(value.c_str()),const_cast<char*>(""));

// 	return _agset(selectEdge(source,target),attribute,value);
}

void GVGraph::startRender()
{
    gvRender(_context,_graph,(char*)"xgtk",nullptr);
}
		
GVC_t* GVGraph::getGVCcontext(void)
{
	return _context;
}

Agraph_t* GVGraph::_agopen(string name/*,int kind*/)
{
    Agdesc_s desc;
    desc.directed = 1;
    desc.strict = 1;
    desc.no_loop = 0;
    desc.no_write = 0;
    desc.flatlock = 0;
    desc.has_attrs = 1;
    desc.has_cmpnd = 0;
    desc.maingraph = 1;

    return agopen(const_cast<char*>(name.c_str()),/*kind*/desc, nullptr);
}

string GVGraph::_agget(void *object,string attr,string alt)
{
    string str=agget(object, const_cast<char*>(attr.c_str()));

    if(str==string())
        return alt;
    else
        return str;
}

int GVGraph::_agset(void *object,string attr,string value)
{
	return agsafeset(object, const_cast<char*>(attr.c_str()),const_cast<char*>(value.c_str()),const_cast<char*>(value.c_str()));
}

Agsym_t* GVGraph::_agnodeattr(string name,string value)
{
    return agattr(_graph,AGNODE,const_cast<char*>(name.c_str()),const_cast<char*>(value.c_str()));
}

Agsym_t* GVGraph::_agedgeattr(string name,string value)
{
    return agattr(_graph,AGEDGE,const_cast<char*>(name.c_str()),const_cast<char*>(value.c_str()));
}
Agnode_t* GVGraph::_agnode(string name)
{
    return agnode(_graph,const_cast<char*>(name.c_str()),true);
}

int GVGraph::_gvLayout(GVC_t *gvc,graph_t *graph,string engine)
{
	return gvLayout(gvc,graph,engine.c_str());
}

GVGraph::GVGraph(string name,double node_size):
        _context(gvContext()),
        _graph(_agopen(name/*, AGDIGRAPHSTRICT*/)) // Strict directed graph, see libgraph doc
{
    //Set graph attributes
//     _agset(_graph, "overlap", "prism");
//     _agset(_graph, "splines", "true");
//     _agset(_graph, "pad", "0,2");
//     _agset(_graph, "dpi", "96,0");
    _agset(_graph, "nodesep", "0.1");
// 	_agset(_graph, "ranksep", "2.0");
// 	_agset(_graph, "landscape","true");
// 	_agset(_graph, "center","true");
// 	_agset(_graph, "aspect","1");
// 	_agset(_graph, "forcelabels","true");
	_agset(_graph, "rankdir","LR");
// 	_agset(_graph, "style","filled");

	
    //Set default attributes for the future nodes
    _agnodeattr("shape", "box");
	_agnodeattr("height", "0.02");
	
//     _agnodeattr(_graph, "label", "");
//     _agnodeattr(_graph, "regular", "true");

    //Divide the wanted width by the DPI to get the value in points
//	double dpi=lexical_cast<double>(_agget(_graph, "dpi", "96,0"));
//     string nodePtsWidth= lexical_cast<string>(node_size/dpi);
	
	//GV uses , instead of . for the separator in floats
// 	replace(nodePtsWidth.begin(), nodePtsWidth.end(), '.', ',');
	
// 	cout<<"nodePtsWidth:"<<nodePtsWidth<<endl;
//     _agnodeattr(_graph, "width", nodePtsWidth);
}

GVGraph::~GVGraph()
{
    gvFreeLayout(_context, _graph);
    agclose(_graph);
    gvFreeContext(_context);
}

void GVGraph::addNode(const string& name)
{
    if(_nodes.find(name)!=_nodes.end())
        removeNode(name);

    _nodes.insert(pair<string,Agnode_t*>(name, _agnode(name)));
}

void GVGraph::addNodes(vector<string>& names)
{
    for(unsigned i=0; i<names.size(); ++i)
        addNode(names[i]);
}

void GVGraph::removeNode(const string& name)
{
	string nn=name;
	
    if(_nodes.find(name)!=_nodes.end())
    {
        agdelete(_graph, _nodes[name]);
        _nodes.erase(name);

		map< pair<string,string>, Agedge_t*>::iterator it;
		
		for(it=_edges.begin();it!=_edges.end();it++)
		{
			if(it->first.first==nn || it->first.second==nn)
				removeEdge(it->first.first,it->first.second);
		}
    }
}

void GVGraph::clearNodes()
{
	map<string, Agnode_t*>::iterator it;

	vector<string>names;
	
	for(it=_nodes.begin();it!=_nodes.end();it++)
		names.push_back(it->first);
	
    for(unsigned i=0;i<names.size();i++)
		removeNode(names[i]);
}

void GVGraph::setRootNode(const string& name)
{
    if(_nodes.find(name)!=_nodes.end())
        _agset(_graph, "root", name);
}

void GVGraph::addEdge(const string& name, const string &source, const string &target)
{
    if(_nodes.find(source)!=_nodes.end() && _nodes.find(target)!=_nodes.end())
    {
		pair<string,string> key(source,target);
		
        if(_edges.find(key)==_edges.end())
            _edges.insert( pair< pair<string,string >, Agedge_t* >(key,
                   agedge(_graph, _nodes[source], _nodes[target], const_cast<char*>(name.data()), true)));
    }
}

void GVGraph::removeEdge(const string &source, const string &target)
{
    removeEdge(pair<string,string>(source, target));
}

void GVGraph::removeEdge(const pair<string, string>& key)
{
    if(_edges.find(key)!=_edges.end())
    {
        agdelete(_graph, _edges[key]);
        _edges.erase(key);
    }
}

void GVGraph::freeLayout()
{
    gvFreeLayout(_context, _graph);
}

void GVGraph::applyLayout()
{
    _gvLayout(_context, _graph, "dot");
}
