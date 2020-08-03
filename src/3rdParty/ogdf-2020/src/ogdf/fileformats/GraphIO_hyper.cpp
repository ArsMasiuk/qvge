/** \file
 * \brief Implements read and write functionality for hypergraphs.
 *
 * \author Carsten Gutwenger, Markus Chimani
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

#include <ogdf/basic/Logger.h>
#include <ogdf/fileformats/GraphIO.h>
#include <ogdf/basic/HashArray.h>


namespace ogdf {

static string::size_type extractIdentifierLength(const string &from, string::size_type start, int line)
{
	string::size_type p = start+1;
	while(from[p] != ',' && from[p] != ')' && from[p] != ' ' && from[p] != '(') {
		++p;
		if(p >= from.size()) {
			Logger::slout() << "GraphIO::readBENCH: Error in line " << line <<
				". Expected comma, bracket or whitespace before EOL; Ignoring.\n";
			break;
		}
	}
	return p-start;
}

static string::size_type newStartPos(const string &from, string::size_type p, int line)
{
	while(from[p] == '\t' || from[p] == ' ' || from[p] == ',') {
		++p;
		if(p >= from.size()) {
			Logger::slout() << "GraphIO::readBENCH: Error in line " << line <<
				". Expected whitespace or delimiter before EOL; Ignoring.\n";
			break;
		}
	}

	return p;
}

static string::size_type findOpen(const string &from, int line)
{
	string::size_type p = 0;
	while(from[p] != '(') {
		++p;
		if(p >= from.size()) {
			Logger::slout() << "GraphIO::readBENCH: Error in line " << line <<
				". Expected opening bracket before EOL; Ignoring.\n";
			break;
		}
	}
	return p;
}


bool GraphIO::readBENCH(Graph &G, List<node>& hypernodes, List<edge>* shell, std::istream &is)
{
	G.clear();
	hypernodes.clear();
	if(shell) shell->clear();

	string buffer;
	HashArray<string,node> hm(nullptr);

	node si = nullptr, so = nullptr;
	if(shell) {
		si = G.newNode();
		so = G.newNode();
		shell->pushBack( G.newEdge(si,so) );
	}

	int line = 0;
	while(getline(is,buffer))
	{
		++line;

		if(buffer.empty() || buffer[0] == ' ' || buffer[0] == '#')
			continue;

		if(prefixIgnoreCase("INPUT(", buffer)) {
			string s(buffer, 6, extractIdentifierLength(buffer, 6, line));
			node n = G.newNode();
			hm[s] = n;
			hypernodes.pushBack(n);
			if(shell) shell->pushBack( G.newEdge(si,n) );

		} else if(prefixIgnoreCase("OUTPUT(", buffer)) {
			string s(buffer, 7, extractIdentifierLength(buffer, 7, line));
			node n = G.newNode();
			hm[s] = n;
			hypernodes.pushBack(n);
			if(shell) shell->pushBack( G.newEdge(n,so) );

		} else {
			string::size_type p = extractIdentifierLength(buffer, 0, line);
			string s(buffer, 0, p); // gatename
			node m = hm[s]; // found as outputname -> refOut
			if(!m) {
				m = hm[s + "%$@"]; // found as innernode input.
				if(!m) { // generate it anew.
					node in = G.newNode();
					node out = G.newNode();
					hm[s + "%$@"] = in;
					hm[s] = out;
					hypernodes.pushBack(out);
					G.newEdge(in,out);
					m = in;
				}
			}
			p = findOpen(buffer, line);
			do {
				p = newStartPos(buffer, p+1, line);
				string::size_type pp = extractIdentifierLength(buffer, p, line);
				string str(buffer, p, pp);
				p += pp;
				node mm = hm[str];
				if(!mm) {
					// new
					node in = G.newNode();
					node out = G.newNode();
					hm[str + "%$@"] = in;
					hm[str] = out;
					hypernodes.pushBack(out);
					G.newEdge(in,out);
					mm = out;
				}
				G.newEdge(mm,m);
			} while(buffer[p] == ',');
		}
	}

	return true;
}


bool GraphIO::readPLA(Graph &G, List<node>& hypernodes, List<edge>* shell, std::istream &is)
{
	G.clear();
	hypernodes.clear();
	if(shell) shell->clear();

	int i;
	int numGates = -1;
	is >> numGates;

	if(numGates < 0) {
		return false;
	}

	Array<node> outport(1,numGates);
	for(i = 1; i <= numGates; ++i) {
		node out = G.newNode();
		outport[i] = out;
		hypernodes.pushBack(out);
	}

	for(i = 1; i <= numGates; ++i) {
		int id, type, numinput;
		is >> id >> type >> numinput;
		if(id != i) {
			Logger::slout() << "GraphIO::readPLA: ID and linenum do not match\n";
			return false;
		}

		node in = G.newNode();
		G.newEdge(in,outport[i]);
		for(int j = 0; j < numinput; ++j) {
			int from = -1;
			is >> from;
			if(from < 1 || from > numGates) {
				Logger::slout() << "GraphIO::readPLA: illegal node index\n";
				return false;
			}
			G.newEdge(outport[from],in);
		}
		while(!is.eof() && is.get() != '\n')
			;
	}

	if(shell) {
		node si = G.newNode();
		node so = G.newNode();
		shell->pushBack( G.newEdge(si,so) );

		for(node n : G.nodes) {
			if(n->degree() == 1) {
				if(n->outdeg() == 1) { //input
					shell->pushBack( G.newEdge( si, n ) );
				} else { // output
					shell->pushBack( G.newEdge( n, so ) );
				}
			}
		}
	}

	return true;
}


}
