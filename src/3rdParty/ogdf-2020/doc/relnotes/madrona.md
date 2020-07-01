[OGDF](../../README.md) » [Release Notes](../relnotes.md) » Madrona

# OGDF Madrona (2012.05)

Released 2012-05-10.

This release brings various new algorithms and modules,
as well as bug fixes and adaptions for current compilers.

Highlights:
 * First implementation of the approximation algorithm for multi-edge
   insertion (class `MultiEdgeApproxInserter`).
 * New planar straight-line layout methods: de Fraysseix, Pach, Pollack
   (class `FPPLayout`) and Schnyder (class `SchnyderLayout`).
 * Various new modules to be used with `SugiyamaLayout`:
   * New two-layer crossing minimization heuristics based on sifting (class
     `SiftingHeuristic`) and greedy approaches (classes
     `GreedyInsertHeuristic` and `GreedySwitchHeuristic`).
   * New ranking module based on the Coffman-Graham algorithm (class
     `CoffmanGrahamRanking`).
   * New module for coordinate assignment based on the algorithm by Brandes
     and Köpf (class `FastSimpleHierarchyLayout`).
 * New Union/Find data structure (class `DisjointSets`).
 * Support for some additional file formats (GD Challenge format, Chaco format).
 * Further graph generators ((toroidal) grid graphs, Petersen graphs,
   planar graphs).

## New Features

 * New edge insertion module (`MultiEdgeApproxInserter`) implementing an
   approximation algorithm for multi-edge insertion.
 * New planar straight-line layout algorithms:
   * `FPPLayout` implements the algorithm by de Fraysseix, Pach, Pollack.
   * `SchnyderLayout` implements the algorithm by Schnyder.
 * New ranking module (`CoffmanGrahamRanking`) which allows to limit the
   number of nodes on a layer.
 * New two-layer crossing minimization heuristics:
   * `GreedyInsertHeuristic`
   * `GreedySwitchHeuristic`
   * `SiftingHeuristic`
 * New module for coordinate assignment (`FastSimpleHierarchyLayout`)
   in Sugiyama’s framework implementing the algorithm by Brandes and Köpf.
 * New Union/Find data structure (`DisjointSets`).
 * New force-directed layout algorithm (`SpringEmbedderFRExact`):
   Fruchterman/Reingold spring-embedder with exact force calculations;
   also features OpenMP and SSE3 parallelization.
 * New functions for parsing file formats:
   * `loadChallengeGraph()`, `saveChallengeGraph()`:
     read and write graphs in GD Challenge file format.
   * `loadChacoGraph()`: read graph in Chaco (graph partitioning software)
     file format.
   * `loadEdgeListSubgraph()`, `saveEdgeListSubgraph()`:
     read and write graphs with specified subgraphs in a simple file format
     (used in experimental evaluation of edge insertion algorithms by
     Chimani and Gutwenger).
   * `SteinLibParser`: reads instances from SteinLib.
 * New graph generators:
   * `planarConnectedGraph()`: creates a connected (simple) planar (embedded)
     graph.
   * `gridGraph()`: creates a (toroidal) grid graph.
   * `petersenGraph()`: creates a generalized Petersen graph.

## Minor Modifications

 * `changeDir()` now returns a boolean value (true = successful).
 * `SList`, `SListPure`: added `search()` method.
 * Renamed `ModularMultilevelMixerLayout.h` to `ModularMultilevelMixer.h`
   (since it defines class `ModularMultilevelMixer`).
 * `ClusterGraphAttributes`: made method `readClusterGraphGML()` private,
   since it should not be called by a user.
 * `CrossingsMatrix`: changed return type of `operator()` from double to int.
 * `ScalingLayout`, `PreprocessorLayout`: proper usage of module options.
 * Moved definitions of constants for pi and e to class `Math` and renamed
   `euler` to `e`; added definitions of constants `pi_2`, `pi_4`, and `two_pi`.
 * Documentation adjusted to latest doxygen version (1.8.0).

## Bug Fixes

 * Fixed a bug in `GraphAttributes`: `writeGML()` now uses edge arrow
   attributes if specified.
 * Fixed bug in `GmlParser`: Access to uninitialized pointer in destructor
   if file could not be opened in constructor.
 * `ModularMultilevelMixer`: implemented correct usage of module options;
   fixes a potential memory leak.
 * `Cluster-Sugiyama`: fixed bug in `ExtendedNestingGraph::tryEdge()` (integer
   overflow); reimplemented computation of “levels” for fast acyclicity testing
   such that levels are < 2n.
 * `DynamicPlanarSPQRTree.h`: fixed include statement (missing subdirectory
   decomposition).
 * Fixed a 0-pointer exception in `GraphListBase::swap()` (wrong swap function
   was called).
