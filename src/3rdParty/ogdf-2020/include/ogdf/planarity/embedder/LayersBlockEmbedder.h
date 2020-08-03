/** \file
 * \brief Computes an embedding of a biconnected graph with maximum
 * external face.
 *
 * \author Thorsten Kerkhof, Tilo Wiedera
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

#include <ogdf/decomposition/BCTree.h>
#include <ogdf/basic/CombinatorialEmbedding.h>
#include <ogdf/planarity/embedder/EmbedderMaxFaceBiconnectedGraphsLayers.h>
#include <ogdf/graphalg/ShortestPathWithBFM.h>


namespace ogdf {
namespace embedder {

//! Common functionality for layer-based embedding algorithms.
template<class BaseEmbedder, class T>
class LayersBlockEmbedder : public BaseEmbedder {
protected:
	void internalEmbedBlock(
			Graph &SG,
			NodeArray<T> &nodeLengthSG,
			EdgeArray<T> &edgeLengthSG,
			NodeArray<node> &nSG_to_nG,
			EdgeArray<edge> &eSG_to_eG,
			node nodeInBlockSG,
			node cT,
			ListIterator<adjEntry> &after) {
		adjEntry m_adjExternal = nullptr;
		// 1. Compute embedding of block
		EmbedderMaxFaceBiconnectedGraphsLayers<T>::embed(
				SG, m_adjExternal, nodeLengthSG, edgeLengthSG, nodeInBlockSG);

		// 2. Copy block embedding into graph embedding and call recursively
		//    embedBlock for all cut vertices in bT
		CombinatorialEmbedding CE(SG);
		face f = CE.leftFace(m_adjExternal);

		if (*BaseEmbedder::pAdjExternal == nullptr) {
			node on = BaseEmbedder::pBCTree->original(nSG_to_nG[m_adjExternal->theNode()]);

			for (adjEntry ae : on->adjEntries) {
				if (ae->theEdge() == BaseEmbedder::pBCTree->original(eSG_to_eG[m_adjExternal->theEdge()])) {
					*BaseEmbedder::pAdjExternal = ae->twin();
					break;
				}
			}
		}

		bool DGcomputed = false;
		int extFaceID = 0;

		// when the following objects get allocated,
		// the DGcomputed bool is set to true
		Graph* p_DG = nullptr;
		ArrayBuffer<node>* p_fPG_to_nDG = nullptr;
		NodeArray<List<adjEntry>>* p_adjacencyList = nullptr;
		List<List<adjEntry>>* p_faces = nullptr;
		NodeArray<int>* p_distances = nullptr;

		for (node nSG : SG.nodes) {
			node nH = nSG_to_nG[nSG];
			node nG = BaseEmbedder::pBCTree->original(nH);
			adjEntry ae = nSG->firstAdj();

			ListIterator<adjEntry>* pAfter;
			if (BaseEmbedder::pBCTree->bcproper(nG) == cT) {
				pAfter = &after;
			} else {
				pAfter = new ListIterator<adjEntry>();
			}

			if (BaseEmbedder::pBCTree->typeOfGNode(nG) == BCTree::GNodeType::CutVertex) {
				node cT2 = BaseEmbedder::pBCTree->bcproper(nG);
				bool doRecurse = true;

				if (cT2 == cT) {
					node parent_bT_of_cT2 = nullptr;

					for (adjEntry adj : cT2->adjEntries) {
						edge e_cT2_to_bT2 = adj->theEdge();

						if (e_cT2_to_bT2->source() == cT2) {
							parent_bT_of_cT2 = e_cT2_to_bT2->target();
							break;
						}
					}

					OGDF_ASSERT(parent_bT_of_cT2 != nullptr);

					if (BaseEmbedder::treeNodeTreated[parent_bT_of_cT2]) {
						doRecurse = false;
					}
				}


				// find adjacency entry of nSG which lies on external face f:
				bool aeExtExists = false;
				for (adjEntry aeFace : f->entries) {
					if (aeFace->theNode() == nSG) {
						ae = aeFace->succ() == nullptr ? nSG->firstAdj() : aeFace->succ();
						aeExtExists = true;
						break;
					}
				}

				if (doRecurse) {
					if (!aeExtExists) {
						if (!DGcomputed) {
							p_DG = new Graph();
							p_fPG_to_nDG = new ArrayBuffer<node>();
							p_adjacencyList = new NodeArray< List<adjEntry> >();
							p_faces = new List< List<adjEntry> >;
							p_distances = new NodeArray<int>;
							DGcomputed = true;

							//compute dual graph of skeleton graph:
							p_adjacencyList->init(SG);
							for (node nBG : SG.nodes) {
								for (adjEntry ae_nBG : nBG->adjEntries) {
									(*p_adjacencyList)[nBG].pushBack(ae_nBG);
								}
							}

							NodeArray< List<adjEntry> > adjEntryTreated(SG);
							for (node nBG : SG.nodes) {
								for (adjEntry adj : nBG->adjEntries) {
									if (!adjEntryTreated[nBG].search(adj).valid()) {
										List<adjEntry> newFace;
										adjEntry adj2 = adj;

										do {
											newFace.pushBack(adj2);
											adjEntryTreated[adj2->theNode()].pushBack(adj2);
											List<adjEntry> &ladj = (*p_adjacencyList)[adj2->twinNode()];
											adj2 = *ladj.cyclicPred(ladj.search(adj2->twin()));
										} while (adj2 != adj);

										p_faces->pushBack(newFace);
									}
								}
							}

							for (int i = 0; i < p_faces->size(); i++) {
								p_fPG_to_nDG->push(p_DG->newNode());
							}

							NodeArray<List<node>> adjFaces(*p_DG);
							int i = 0;

							for (const List<adjEntry> &Li : *p_faces) {
								int f1_id = i;

								for (adjEntry adj2 : Li) {
									int f2_id = 0;
									int j = 0;

									for (List<adjEntry> &Lj : *p_faces) {
										bool do_break = false;

										for (adjEntry adj4 : Lj) {
											if (adj4 == adj2->twin()) {
												f2_id = j;
												do_break = true;
												break;
											}
										}

										if (do_break) {
											break;
										}

										j++;
									}

									if (f1_id != f2_id
											&& !adjFaces[(*p_fPG_to_nDG)[f1_id]].search((*p_fPG_to_nDG)[f2_id]).valid()
											&& !adjFaces[(*p_fPG_to_nDG)[f2_id]].search((*p_fPG_to_nDG)[f1_id]).valid()) {
										adjFaces[(*p_fPG_to_nDG)[f1_id]].pushBack((*p_fPG_to_nDG)[f2_id]);
										p_DG->newEdge((*p_fPG_to_nDG)[f1_id], (*p_fPG_to_nDG)[f2_id]);
									}

									if (adj2 == f->firstAdj()) {
										extFaceID = f1_id;
									}
								}

								i++;
							}

							// compute shortest path from every face to the external face:
							List<edge> DG_edges;
							p_DG->allEdges(DG_edges);

							for (edge e : DG_edges) {
								node s = e->source();
								node t = e->target();
								p_DG->newEdge(t, s);
							}

							ShortestPathWithBFM shortestPath;
							node efDG = (*p_fPG_to_nDG)[extFaceID];
							EdgeArray<int> el(*p_DG, 1);
							p_distances->init(*p_DG);
							NodeArray<edge> pi(*p_DG);
							shortestPath.call(*p_DG, efDG, el, *p_distances, pi);
						}

						// choose face with minimal shortest path:
						List<adjEntry> optFace;
						int optFaceDist = -1;

						for (int fID = 0; fID < p_faces->size(); ++fID) {
							List<adjEntry> theFace = *(p_faces->get(fID));
							adjEntry ae_nSG;
							bool contains_nSG = false;

							for (adjEntry adj : theFace) {
								if (adj->theNode() == nSG) {
									contains_nSG = true;
									ae_nSG = adj;
									break;
								}
							}

							if (contains_nSG) {
								int thisDist = (*p_distances)[(*p_fPG_to_nDG)[fID]];

								if (optFaceDist == -1 || optFaceDist > thisDist) {
									optFace = theFace;
									optFaceDist = thisDist;
									ae = ae_nSG->succ() == nullptr ? nSG->firstAdj() : ae_nSG->succ();
								}
							}
						}
					}

					for (adjEntry adj : cT2->adjEntries) {
						node bT2 = adj->theEdge()->opposite(cT2);

						if (!BaseEmbedder::treeNodeTreated[bT2]) {
							this->embedBlock(bT2, cT2, *pAfter);
						}
					}
				}
			}

			// embed all edges of block bT:
			bool after_ae = true;
			for (adjEntry aeNode = ae; after_ae || aeNode != ae; aeNode = aeNode->succ() == nullptr ? nSG->firstAdj() : aeNode->succ()) {
				edge eG = BaseEmbedder::pBCTree->original(eSG_to_eG[aeNode->theEdge()]);
				if (nG == eG->source()) {
					if (pAfter->valid()) {
						*pAfter = BaseEmbedder::newOrder[nG].insertAfter(eG->adjSource(), *pAfter);
					} else {
						*pAfter = BaseEmbedder::newOrder[nG].pushBack(eG->adjSource());
					}
				} else {
					if (pAfter->valid()) {
						*pAfter = BaseEmbedder::newOrder[nG].insertAfter(eG->adjTarget(), *pAfter);
					} else {
						*pAfter = BaseEmbedder::newOrder[nG].pushBack(eG->adjTarget());
					}
				}

				after_ae &= aeNode->succ() != nullptr;
			}

			if (*pAfter != after) {
				delete pAfter;
			}
		}

		if (DGcomputed) {
			delete p_DG;
			delete p_fPG_to_nDG;
			delete p_adjacencyList;
			delete p_faces;
			delete p_distances;
		}
	}
};

}}
