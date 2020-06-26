[OGDF](../../README.md) » [Release Notes](../relnotes.md) » Sassafras

# OGDF Sassafras (2010.10)

Released 2010-10-21.

This release brings various new algorithms and features.

Highlights:
 * Completely revised memory management; pool-memory allocator now supports
   thread-safe allocation.
 * Basic multithreading support for OGDF algorithms.
 * New class System provides methods for accessing system-dependent
   information and functionality (processor features, memory usage, file system).
 * New hierarchical graph layout method using upward planarization (class
   `UpwardPlanarization`); outperforms traditional Sugiyama-based methods
   with respect to crossings by far.
 * New multilevel layout algorithm (`FastMultipoleMultilevelEmbedder`),
   based on the multipole method, well-separated pair decomposition, and
   a new quadtree space partitioning (Martin Gronemann's Diploma thesis);
   makes use of SSE and multicore processors and is significantly faster
   than `FMMMLayout`.
 * New modular framework for multilevel graph layout (class
   `ModularMultilevelMixer`) with various options for coarsening, placement,
   and single-level layout.
 * New force-directed layout algorithms: Kamada-Kawai (class
   `SpringEmbedderKK`) and stress majorization (class `StressMajorization`).
 * New layout algorithms for directed graphs: `DominanceLayout` and
   `VisibilityLayout`.
 * Implementation of Prim's algorithm for minimum spanning tree computation.
 * Added support for OGML graph file format.
 * Exact algorithms for computing maximum planar and maximum c-planar
   subgraphs (classes `MaximumPlanarSubgraph` and `MaximumCPlanarSubgraph`).
 * OGDF can now be compiled as DLL under Windows.
 * Support for Visual Studio 2010 and latest g++ compilers.

## New Features

 * Completely revised memory management, with a new thread-safe pool-memory
   allocator. OGDF provides now three allocators (which can be selected with
   compiler predefines):
   * thread-safe pool allocator (`OGDF_MEMORY_POOL_TS`; usually the deafult),
   * non-thread-safe pool allocator (`OGDF_MEMORY_POOL_NTS`), and
   * ordinary malloc/free (`OGDF_MEMORY_MALLOC_TS`).
 * Basic, platform-independent multithreading support for OGDF algorithms;
   new classes `Thread`, `CriticalSection`, `Mutex`, `Barrier`.
 * New class `System` provides methods for accessing system-dependent
   information and functionality (processor features, memory usage, file system).
 * New layout algorithms for directed (hierarchical) graphs:
   * `DominanceLayout`, based on dominance layout of st-digraphs.
   * `VisibilityLayout`, based on computation of a visibility representation.
 * New hierarchical graph layout method using upward planarization (class
   `UpwardPlanarization`); implements the algorithm by Chimani, Gutwenger,
   Mutzel and Wong. Outperforms traditional Sugiyama-based methods with
   respect to number of crossings by far. Uses module options for upward
   planarization and layout:
   * `UpwardPlanarizerModule`: its implementation `SubgraphUpwardPlanarizer`
     applies a 2-phase approach: first compute a feasible upward planar
     subgraph (`FUPSModule` with implementation `FUPSSimple`), then insert
     remaining edges (`UpwardEdgeInserterModule` with implementation
     `FixedEmbeddingUpwardEdgeInserter`).
   * `UPRLayoutModule`: implementation `LayerBasedUPRLayout` which makes use
     of hierarchy layout modules from the Sugiyama framework.
 * New force-directed layout algorithms:
   * Kamada-Kawai (class `SpringEmbedderKK`)
   * stress majorization (class `StressMajorization`).
 * New multilevel layout algorithm (`FastMultipoleMultilevelEmbedder`), based
   on the multipole method, well-separated pair decomposition, and a new
   quadtree space partitioning (Martin Gronemann’s Diploma thesis); makes use
   of SSE and multicore processors and is significantly faster than `FMMMLayout`.
 * New modular framework for multilevel graph layout (class
   `ModularMultilevelMixer`) with various options for coarsening, placement,
   and single-level layout.
   * `MultilevelBuilder` module (coarsening): `EdgeCoverMerger`,
     `IndependentSetMerger`, `LocalBiconnectedMerger`, `MatchingMerger`,
     `RandomMerger`, `SolarMerger`.
   * `InitialPlacer` module (placement): `BarycenterPlacer`, `CirclePlacer`,
     `MedianPlacer`, `RandomPlacer`, `SolarPlacer`, `ZeroPlacer`.
 * Implementation of Prims’ algorithm for minimum spanning tree computation
   (function `computeMinST` in `ogdf/basic/extended_graph_alg.h`.
 * Added support for OGML graph file format:
   * Class `OgmlParser` implements a parser for OGML files.
   * Methods `writeOGML` and `readClusterGraphOGML` in `ClusterGraphAttributes`
     provide reading and writing of OGML files.
 * Exact algorithm for computing a maximum planar subgraph (class
   `MaximumPlanarSubgraph`); requires COIN and ABACUS.
 * Exact algorithm for computing a maximum c-planar subgraph (class
   `MaximumCPlanarSubgraph`); requires COIN and ABACUS.

## Minor Modifications

 * `GraphAttributes` provides now a flag indicating if the graph is directed
   or not (default is true): methods directed for getting / setting, support
   in reading and writing GML files (`readGML` and `writeGML`).
 * Hash values are now of type `size_t` for providing better compatibility
   with 64-bit systems.
 * Default parameters of `GEMLayout` revised.
 * `ClusterPlanarizationLayout` now allows to pass edge weights in its call,
   which are used for computing a c-planar subgraph (edges with low weight
   are preferred).
 * Clarified `Comparer` interfaces:
   * Class `Comparer<E>` renamed to `VComparer<E>` (since it relies on
     virtual functions).
   * Added the macros `OGDF_AUGMENT_COMPARER` and `OGDF_AUGMENT_STATICCOMPARER`
     to allow easy generation of comparers with full interfaces.
   * Added `StdComparer` (standard comparers) and `TargetComparer` (for
     comparing targets of pointers).
 * `Array::grow()` allows now to enlarge an array with empty index set.
 * `PlanarizationLayout`: changed default planar subgraph to `FastPlanarSubgraph`.
 * Changed return type of `BCTree::findPathBCTree()` to pointer (instead of
   reference) to reflect the fact that the returned object has been allocated
   with `new`.
 * `isConnected()`, `makeConnected()`: improved performance by replacing
   recursive with iterative implementation.
 * New, more efficient implementation of list-based stacks.

## Bug Fixes

 * Fixed bug in `SugiyamaLayout`: possibly incorrect setting of number of
   crossings if there are several connected components and `arrangeCCs = true`.
 * Fixed bug in `BoyerMyrvold`: incorrect embedding of self-loops.
 * Fixed memory leaks in `PlanarAugmentationFix`, `EmbedderMaxFace`,
   `EmbedderMaxFaceLayers`, `EmbedderMinDepth`, `EmbedderMinDepthMaxFace`,
   `EmbedderMinDepthMaxFace`, `EmbedderMinDepthPiTa`, `PlanarizationLayout`.
 * Fixed bug with copy constructor of graph arrays (`NodeArray`,
   `EdgeArray`, etc.): crashed if copied array was not initialized for a graph.
 * Fixed bug with Hierarchy: memory leaks occurred when using `initByNodes()`.
 * Revised compiler condition for systems providing only 15 random bits
   with rand() function; fixes a non-termination bug with `FMMMLayout` on
   Solaris/SPARC.
 * Fixed a bug in `makeConnected()`.
 * Fixed a memory leak in `LPSolver_coin`.

## Build System

 * Added support for Visual Studio 2010, which introduces an new project
   format (`.vcxproj`). For creating such a project, use `makeVCXProj.py`
   (instead of `makeVCProj.py`). At the moment, there is just one possible
   template file for VS 2010 (`ogdf.vcxproj.vs2010.template`; which is the
   default).
 * Added support for compiling OGDF as DLL under Windows (currently VS 2005
   only); simply select project template `ogdf.dll.vcproj.vs2005.template` in
   `makeVCProj.config`.
 * Added support for latest g++ compilers (4.1.x – 4.4.x) on Linux and Mac OS X.
 * Windows only: OGDF now requires to also link against `Psapi.lib`.
 * g++ on Linux/Mac only: OGDF now requires to link against pthreads;
   add option `-pthread` when compiling and linking with g++.
