[OGDF](../../README.md) » [Release Notes](../relnotes.md) » Catalpa

# OGDF Catalpa (2020.02)

Released 2020-02-09.

This release has been in the making for a very long time. So-called "snapshots"
have been introduced in the meantime to shorten the waiting time for the next
release. Now, even after more than 3000 commits, our original ambitions of the
"next release" are still not fully met, but it is about time for a new snapshot.
We have however noticed that releasing snapshots has not proven to be useful;
it just caused irritation and confusion among OGDF users. Hence, starting with
this release, called Catalpa, we will not publish any further snapshots.
Instead, we will try to publish ordinary releases more often, i.e., whenever
there has been some noteworthy progress in OGDF.

Moreover, OGDF got a new website, a new logo, and the abbreviation now has a
second meaning that reflects OGDF's matter in a better way. OGDF now stands
for both "Open Graph Drawing Framework" and "Open Graph algorithms and Data
structures Framework".

## Noteworthy changes since last snapshot

If you used the snapshots, you might be interested in noteworthy changes
since the last OGDF snapshot:
 * file formats:
   * DL:
     * improved (and fixed) logic when to use matrix and when to use
       adjacency list
   * DOT:
     * fixes regarding cluster graphs and parsing bugs
     * support for subgraphs, edge styles, edge types, custom node ids,
       node label positions, arrow directions
   * GEXF:
     * support for subgraphs, labels, bend points, edge styles,
       custom node ids, node label positions
     * compatibility fixes (e.g., regarding edge weights)
     * fix handling of directed vs. undirected graphs
   * GML:
     * compatibility changes regarding subgraphs
     * support for node types, node label positions, integral edge weights
       (including compatibility fix), and cluster graphs
   * Graph6 family:
     * new support for Sparse6 (s6) and Digraph6 (d6) file format
     * fixed support for the case of 63 nodes
   * GraphML:
     * compatibility changes regarding subgraphs
     * support for bend points, edge styles, custom node ids, node label positions
     * fix handling of directed vs. undirected graphs
   * STP:
     * support for coordinates, directed edges, node shapes for terminals
     * support for simplified format (from PACE 2018)
   * support for background pattern's fill color of nodes in many file formats
   * fix self-loops in Chaco file format
   * the generic reader now supports more file formats and GraphAttributes
   * some readers (e.g., Rudy) are stricter now
   * readers with additional information (like edge weights or terminal nodes)
     are now available in `Graph`-only form
   * `GraphAttributes` can now be used for readers with edge weights
   * new generic writer (`GraphIO::write()`) based on file name extension
   * fix edge arrow issues in SVG printer
   * deprecated `GraphIO` functions based on filenames (instead of streams)
     are removed
 * graph generators:
   * faster `randomSimpleGraph()` and `randomSimpleConnectedGraph()`
   * new `randomSimpleGraphByProbability()` for fast random graph generation
 * new basic functions for convenience:
   * cast from `AdjElement` to corresponding `node`
   * `EdgeElement::nodes()` function for `for (node v : anEdge->nodes()) { ... }`
   * `EdgeElement::isParallelUndirected()`
   * `EdgeElement::isParallelDirected()`
   * `EdgeElement::isInvertedDirected()`
   * `GraphAttributes::transferTo{Original,Copy}()` to replace
     `GraphCopyAttributes` class (which is removed)
   * `GraphAttributes::all` flag
   * `hasNonSelfLoopEdges()` to check whether a graph has edges
     which are not self-loops
   * `removeSelfLoops()`
   * `Graph::searchEdge()` with and without edge direction
   * `GraphAttributes::point(node)` instead of querying single x- and
     y-coordinates
   * `graphUnion()` to form a (disjoint and non-disjoint) union of two graphs
   * `graphProduct()` to form the product of two graphs using a given function,
     and its common use-cases:
     * `cartesianProduct()`
     * `tensorProduct()`
     * `lexicographicalProduct()`
     * `strongProduct()`
     * `coNormalProduct()`
     * `modularProduct()`
     * `rootedProduct()`
 * algorithms:
   * new algorithms:
     * `PlanarSubgraphTriangles` for finding planar subgraphs
     * `EdgeIndependentSpanningTrees` for k edge-independent spanning trees
     * simple algorithms regarding matchings, e.g., Matching::findMaximalMatching()
   * algorithms for finding maximum planar subgraphs are now weight-templated
   * `CliqueFinder` is split into `CliqueFinderHeuristic` and `CliqueFinderSPQR`
   * Steiner tree:
     * new `SteinerTreeLowerBoundDualAscent` algorithm that can be activated
       for preprocessing (`SteinerTreePreprocessing::reduceFastAndDualAscent()`)
     * overhaul of constructing and managing full components (for approximation
       algorithms that use full components)
     * slight overhaul of `SteinerTreePreprocessing` (including some fixes),
       `MinSteinerTreeGoemans139`, `MinSteinerTreeRZLoss`
 * other noteworthy changes
   (besides typofixes, less compiler warnings, improved documentation, etc.):
   * many layouts/algorithms now run on corner cases like empty graphs or single nodes
   * overhaul of `ClusterGraphAttributes`, now similar to `GraphAttributes`
   * `AdjacencyOracle` can now trade memory usage versus speed
     by ignoring nodes of small degree
   * `Layout::computeBoundingBox()` can now handle negative coordinates
   * `ConnectivityTester` returns correct values also for node-connectivity now
   * `GraphCopy` copy assignment operator fixed for uninitialized `GraphCopy`
   * removed `PreconditionViolatedException`

## Noteworthy changes since Baobab

Now let us talk about changes since our last non-snapshot release Baobab.

First of all, we are now using C++11 features, hence a C++11-capable compiler
is necessary to use OGDF. Note that for this transition, macros like
`forall_nodes()` have been removed in favor of range-based for-loops.

Check the [porting guide](../porting/catalpa.md) for compatibility-breaking
changes.

Our build system has been modernized to use CMake instead of self-written
Python scripts. Our documentation (see the [Build Guide](../build.md))
provides examples how to use it if you are not familiar with CMake.

Noteworthy changes since Baobab:
 * file formats:
   * there is now a generic reader (`GraphIO::read()`) recognizing many
     graph formats...
   * ...and a generic writer (`GraphIO::write()`) based on file name extension
   * new support for file formats:
     * Graph6 (g6)
     * Sparse6 (s6)
     * Digraph6 (d6)
     * format of the DIMACS maximum flow challenge (DMF)
   * additional features and fixes in support for
     Chaco, DL, DOT, GDF, GEXF, GML, GraphML, STP, TLP
   * some readers (e.g., Rudy, GEXF, TLP) are stricter now (not accepting rubbish)
   * buggy XML parser is replaced by [pugixml](https://pugixml.org/)
   * dead OGML file format no longer supported
   * SVG printer is rewritten (bugfixes included)
   * readers with additional information (like edge weights or terminal nodes)
     are now available in `Graph`-only form
   * `GraphAttributes` can now be used for readers with edge weights
   * `GraphIO` functions based on filenames (instead of streams) are removed
 * layouts:
   * new `LinearLayout`, places nodes next to each other
     and draws edges as bows above the nodes
   * new `NodeRespecterLayout`, a force-directed layout
     respecting node shapes and sizes
   * `SchnyderLayout` can now compute the 1989's paper version by using
     `SchnyderLayout::setCombinatorialObjects(SchnyderLayout::CombinatorialObjects::Faces)`
   * fixed setting of random seeds in `FMMMLayout`
   * `LayoutStatistics` changes:
     * new `numberOfNodeOverlaps()` method
     * new `numberOfNodeCrossings()` method
     * `numberOfCrossings()` can now return statistical measures on crossings
 * new graph generators:
   * `customGraph()` for quick generation of specific custom graphs
   * `emptyGraph()` for an empty graph or n isolated nodes
   * `circulantGraph()` for circulant graphs
   * `completeKPartiteGraph()` and `completeBipartiteGraph()`
   * `randomSimpleConnectedGraph()`
   * `randomSimpleGraphByProbability()`
   * `randomGeometricCubeGraph()` for random geometric graphs in a unit n-cube
   * `randomRegularGraph()` for random regular graphs
   * `preferentialAttachmentGraph()`
   * `regularLatticeGraph()`
   * `randomWattsStrogatzGraph()`
   * `randomChungLuGraph()`
   * `randomGeographicalThresholdGraph()`
   * `randomWaxmanGraph()`
   * generic `randomEdgesGraph()`
 * algorithms:
   * new algorithms:
     * s-t-planarity (planar and nodes s and t share a face):
       * `isSTPlanar()` to check if a graph is s-t-planar
       * `planarSTEmbed()` to embed a graph s-t-planarly
     * maximum flows in networks:
       * `MaxFlowEdmondsKarp`
       * `MaxFlowGoldbergTarjan`
       * `MaxFlowSTPlanarDigraph`
       * `MaxFlowSTPlanarItaiShiloach`
     * minimum s-t-cuts:
       * `MinSTCutBFS`
       * `MinSTCutDijkstra`
       * `MinSTCutMaxflow` (replaces old `MinSTCut`)
     * Steiner trees:
       * `MinSteinerTreeDirectedCut`
       * `MinSteinerTreeDualAscent`
       * `MinSteinerTreeGoemans139`
       * `MinSteinerTreeKou`
       * `MinSteinerTreeMehlhorn`
       * `MinSteinerTreePrimalDual`
       * `MinSteinerTreeRZLoss`
       * `MinSteinerTreeShore`
       * `MinSteinerTreeTakahashi`
       * `MinSteinerTreeZelikovsky`
       * `SteinerTreePreprocessing`
       * `SteinerTreeLowerBoundDualAscent`
     * planar subgraphs:
       * `PlanarSubgraphTriangles`
       * `PlanarSubgraphTree`
       * `PlanarSubgraphCactus`
       * `PlanarSubgraphEmpty`
       * `MaximalPlanarSubgraphSimple`
     * canonical orderings:
       * `LeftistOrdering`
       * `BitonicOrdering`
     * `MaxAdjOrdering` to compute one or all maximum adjacency orderings
     * `AStarSearch` for finding a shortest path between two given nodes
     * `MaximalFUPS` for the maximal feasible upward-planar subgraph
       based on a SAT formulation
     * `EdgeIndependentSpanningTrees` for k edge-independent spanning trees
     * simple algorithms regarding matchings, e.g.,
       `Matching::findMaximalMatching()`
     * `isTwoEdgeConnected()` to check a graph for 2-edge-connectivity
     * `isBipartite()`
   * planar subgraph algorithms:
     * `MaximumPlanarSubgraph` now supports weighted edges
     * renamed `BoyerMyrvoldSubgraph` to `PlanarSubgraphBoyerMyrvold`
     * renamed `FastPlanarSubgraph` to `PlanarSubgraphFast`
   * `NonPlanarCore`:
     * can now process weighted graphs
     * new `NonPlanarCore::retransform()`
     * improved (returns simple graphs)
   * `MinCostFlowReinelt` is now cost-templated
   * `CliqueFinder` is split into `CliqueFinderHeuristic` and `CliqueFinderSPQR`
   * renamed and deprecated some forest functions with misleading names
 * new basic functions (e.g., for convenience):
   * cast from `AdjElement` to corresponding node
   * `AdjElement::isSource()`
   * `AdjElement::isBetween()`
   * `EdgeElement::nodes()` function for `for (node v : anEdge->nodes()) { ... }`
   * `EdgeElement::isAdjacent()`
   * `EdgeElement::getAdj()`
   * `EdgeElement::isParallelUndirected()`
   * `EdgeElement::isParallelDirected()`
   * `EdgeElement::isInvertedDirected()`
   * `operator()` indexing (as synonym for `operator[]`) for `NodeArray`s and alike
   * `operator==` and `operator!=` for `Array` and `ArrayBuffer`
   * `GraphCopy::isReversedCopyEdge()`
   * `Graph::searchEdge()` with and without edge direction
   * `GraphAttributes::nodeBoundingBoxes(boundingBoxes)`
   * `GraphAttributes::has()` to check for attributes
   * `GraphAttributes::transferTo{Original,Copy}()` to replace
      `GraphCopyAttributes` class (which is removed)
   * `GraphAttributes::all` flag
   * `GraphAttributes::point(node)` instead of querying single x- and
     y-coordinates
   * `hasNonSelfLoopEdges()` to check whether a graph has edges
     which are not self-loops
   * `removeSelfLoops()`
   * `Graph::insert()`
   * `graphUnion()` to form a (disjoint and non-disjoint) union of two graphs
   * `graphProduct()` to form the product of two graphs using a given function, and its common use-cases:
     * `cartesianProduct()`
     * `tensorProduct()`
     * `lexicographicalProduct()`
     * `strongProduct()`
     * `coNormalProduct()`
     * `modularProduct()`
     * `rootedProduct()`
   * methods like `Graph::chooseNode()` or
     `CombinatorialEmbedding::chooseFace()` can now adhere to constraints
   * `nodeDistribution()` and `degreeDistribution()` as a special case
   * `isRegular()` to check if a graph is regular
   * `GenericComparer` replaces simple custom comparers by using lambdas
   * `Graph::allNodes()` and `Graph::allEdges()` for arrays
   * `GraphAttributes::{x,y,z}Label()` methods for the coordinates of
     a label relative to its node
   * `Graph::HiddenEdgeSet` replaces old and buggy (un)hiding mechanism for edges
   * new `EpsilonTest` class
 * geometry classes:
   * `DPoint`, `IPoint` are now template specializations of `GenericPoint`
   * `DPolyline, `IPolyline` are now template specializations of new
     `GenericPolyline`
   * new `GenericLine` and new `GenericSegment` represent (infinite) lines and
     (finite) line segments, respectively, replacing `DLine` being a mix
     of both (for double only)
   * improved intersection methods for these classes
   * removed: `DVector` (simply use `DPoint`), `DScaler`, `DRound`
   * new `DRect::intersection()`
   * fixed bug and generalized `DPolyline::normalize()`
   * new `DIntersectableRect` replaces `IntersectionRectangle`
 * `Math` utility changes:
   * `Math` is now a namespace instead of a static class
   * deprecate functions where one can use STL functions instead
   * template some functions such that they can be used,
     e.g., for ints and doubles
   * new `harmonic(n)` to compute the n-th harmonic number
   * new `minValue()`, `maxValue()`, `sum()`, `mean()`, `standardDeviation()`
   * new `radiansToDegrees()` and `degreesToRadians()`
   * new `updateMin()` and `updateMax()`
   * new `nextPower2()`
 * overhaul of `ClusterGraphAttributes`, now similar to `GraphAttributes`
 * removed classes `Stack`, `StackPure`, `BoundedStack` in favor of `ArrayBuffer`
 * removed OGDF file system functions (outside of scope of OGDF)
 * rather internal but noteworthy changes:
   * all include guards are replaced by `#pragma once`
   * all header files are now self-sufficient (i.e., have no other dependencies)
   * enum classes (scoped enums) are now used throughout OGDF
     instead of unscoped enums
   * cleanup of global namespace
   * some auxiliary classes are moved into sub-namespaces of `ogdf`
   * many `std` members imported into the `ogdf` namespace are now removed
     from it (e.g., `swap`, `[io]stream`, `numeric_limits`, `cin`, `cout`,
     `endl`, `flush`, `ios`)
   * `rbegin()` and `rend()` now work with reverse iterators instead
     of forward iterators
   * `rbegin()` is renamed to `backIterator()` where a reverse iterator
     does not make sense
   * removed `ModuleOption` (replaced by `std::unique_ptr`)
   * `AdjacencyOracle` can now trade memory usage versus speed
     by ignoring nodes of small degree
   * the graph size is no longer bounded by the stack size
     for the following algorithms:
     * `biconnectedComponents()`
     * `isAcyclic()`
     * `isAcyclicUndirected()`
     * `isBiconnected()`
     * `isForest()`
     * `makeBiconnected()`
     * `strongComponents()`
     * and all algorithms based on these functions
   * new `safeForEach()` and `safeTestForEach()` as simple functions to iterate
     over containers destructively (i.e., the current member of an iteration
     may be destroyed)
   * removed `PreconditionViolatedException`
 * many many bugfixes, code cleanup, improvements in code quality,
   documentation, and test coverage

This release contains (huge and small) contributions by Anuj Agarwal,
Hendrick Brückler, Carsten Gutwenger, Daniel L. Lu, Denis Kurz, Ivo Hedtke,
Jens Schmidt, Jöran Schierbaum, Karsten Klein, Kévin Szkudłapski,
Łukasz Hanuszczak, Manuel Fiedler, Markus Chimani, Max Ilsen, Mirko Wagner,
Mihai Popa, raven-worx on GitHub, Sebastian Semper, Stephan Beyer,
Tilo Wiedera, and Yosuke Onoue.
