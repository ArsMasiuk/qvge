[OGDF](../../README.md) » [Release Notes](../relnotes.md) » Bubinga

# OGDF Bubinga (2007.11)

Released 2007-11-23.

This is the second public release of OGDF.
This release focuses on improved usability, but also contains new functionality.

Highlights:
 * New algorithm for planar augmentation with fixed embedding.
 * The planar drawing algorithms Planar-Straight, Planar-Draw, and Mixed-Model can now be called with a given planar embedding.
 * New class `DualGraph` representing the geometric dual graph of a combinatorial embedding.
 * Consistent interface for planarization layout that allows to call with `GraphAttributes`.
 * Sugiyama layout now produces drawings with given node ranks that are also respected across different connected components.
 * Hashing functions can now be passed as template parameters; the implementation of two-dimensional hash-arrays has been revised and allows now using different types for each index.
 * Optional template parameter for index type of array-based classes.
 * Unified naming conventions and interfaces.
 * Improved build system for Visual Studio, including support for compiling with Osi Coin and creating projects for Visual Studio 2003.
 * Significantly improved documentation.

## New Features

 * Additional (optional) template parameter for used size/index type in
   `Array`, `ArrayBuffer`, `BoundedQueue`, `BoundedStack`, `MinHeap`,
   and `Top10Heap`.
 * Specification of hash function as (optional) template parameter in
   `Hashing` and `HashArray`.
   * The default hash function is implemented by `DefHashFunc<K>` (instead
     of function `hash()`); this can be extended to further types by
     specializing `DefHashFunc`.
 * Revised implementation of `HashArray2D`.
   * Supports now different types for each index.
   * The hash function can be passed as (optional) template parameter.
   * `entry(const I&, const I&)` → `operator()(const I1&,const I2&)`
   * Iterators: `key(I&, I&)` → `key1()` and `key2()`
 * The new class `DualGraph` represents the geometric dual graph of a
   combinatorial embedding.
 * New augmentation algorithm for planar biconnected augmentation with
   fixed embedding (class `PlanarAugmentationFix`).
 * Planar drawing algorithms now implement a call for drawing with a given
   planar embedding.
   * The base class `PlanarGridLayoutModule` defines this interface.
   * `PlanarStraightLayout`, `PlanarDrawLayout`, and `MixedModelLayout`
     implement this interface.
 * The planarization layout can now be called with `GraphAttributes` like
   other layout algorithms; setting/getting of options made consistent with
   ogdf naming style. The following changes were done:
   * `UMLPlanarizationLayout` → `PlanarizationLayout`
   * `UMLLayoutModule` inherits from `LayoutModule`
   * `PlanarizationLayout` has now a `call(GraphAttributes&)`
   * `setCliqueSize(int)` → `minCliqueSize(int)`
   * added `int minCliqueSize()`
   * `preProcessCliques(bool)` → `preprocessCliques(bool)`
   * added `bool preprocessCliques()`
 * `SugiyamaLayout` has a new option `arrangeCCs` (deciding whether components
   are laid out separately and arranged afterwards) and a new module option
   packer (for arranging connected components). Setting `arrangeCCs` to false
   and passing node ranks directly allows to get a layout which truly respects
   the layering across all connected components.
 * `LongestPathRanking` has a new option `optimizeEdgeLength`;
   setting this option to false gives a longest-path ranking as known from
   the literature; default is true which is same behavior as before (performs
   additional optimization for reducing edge lengths).

## Minor Modifications

 * Unified interface for containers (`ArrayBuffer`, `BinaryHeap`,
   `BoundedQueue`, `BoundedStack`, `MinHeap`, `Top10Heap`).
   * `size()` returns the current number of elements in the container.
   * `capacity()` returns the maximal number of elements that can be stored in the container (if applicable).
   * `empty()` returns true if the container contains no elements.
   * `full()` returns true if the current number of elements has reached the capacity (if applicable).
   * `clear()` removes all elements from the container.
 * Unified naming conventions for array classes:
   * `Array2` → `Array2D`
   * `HashingArray` → `HashArray`
   * `TwoDHashArray` → `HashArray2D`
   * `TwoDHashIterator` → `HashConstIterator2D`
 * Usage of `size_t` instead of `int` in:
   * `BendString::BendString(char, size_t)`
   * `BendString::operator[](size_t)`
   * `BendString::size()`
   * `BendString::set(char, size_t)`
   * `BendString::init(char, size_t)`
   * `String::String(size_t, const char *)`
   * `String::length()`
   * `String::operator[](size_t)`
 * Removed `String::operator const char *()`; use the new method `String::cstr()` instead.
 * Usage of `const String&` instead of `const char*` in:
   * `CliqueFinder::writeGraph(Graph &, NodeArray<int> &, const String &)`
   * `GraphAttributes::readGML(Graph &, const String &)`
   * `GraphAttributes::writeGML(const String &)`
   * `GraphAttributes::readXML(Graph &G, const String &fileName)`
   * `GraphAttributes::writeXML(const String &, const char*, const char*)`
   * `GraphAttributes::readRudy(Graph &, const String &)`
   * `GraphAttributes::writeRudy(const String &)`
   * `String::compare(const String &,const String &)`
 * `GraphAttributes` return now default values for type(node) and type(edge)
   even if the respective arrays are not initialized.
 * Renamed `GraphStructure` → `GraphObserver`.
 * The methods `assignNode()`, `unassignNode()`, and `removeNodeAssignment()`
   in `ClusterGraph` are now private (not meant for public use).
 * Added constructor `PlanRepUML(const GraphAttributes&)`.
 * Changed default augmenter of `PlanarStraightLayout` and `PlanarDrawLayout`
   to `PlanarAugmentation`.
 * Removed `OrthoFormerUML` (obsolete).
 * Renamed `OrthoFormerGENERIC → OrthoShaper`.
 * Renamed `UMLOrthoLayout → OrthoLayout`.
 * Renamed `UMLPlanarLayoutModule → LayoutPlanRepModule`.
 * Revised design of `ClusterPlanarizationLayout`:
   * `ClusterPlanarizationLayout` does not inherit from `UMLLayoutModule` anymore.
   * Removed unsupported call methods.
   * Removed (unused) module options subgraph and inserter.
   * Changed type of `planarLayouter` to new module type
     `LayoutClusterPlanRepModule`.
   * Changed base class of `ClusterOrthoLayout` to `LayoutClusterPlanRepModule`.
   * Renamed `ClusterOrthoFormer` to `ClusterOrthoShaper`.
 * Renamed `ClustererBase` → `ClustererModule`
   and moved header `ClustererModule.h` to `ogdf/module/`.
 * Logging output of Coin Osi solver turned off by default
   (previous version produced some output in debug builds).

## Bug Fixes

 * Fixed possible rounding error in `LPSolver::checkFeasibility()`.
 * Fixed C++ template syntax for `print()` function of `BoundedQueue`.
 * Removed `LPSolver::` in declaration of `LPSolver::checkFeasibility()`
   (did not compile with some g++ versions).

## Build System

 * Visual Studio project file: Added configuration file `makeVCProj.config`
   for specifying project template and optional settings for LP-solver.
 * Support for Visual Studio 2003 (Visual C++ 7.1) by selecting
   `ogdf.vcproj.vs2003.template` as project template.
 * Linux/g++ makefile: Build targets renamed to `release`, `cleanrelease`,
   etc. (instead of `release_all`, `release_clean`).
 * Available make targets are now `debug`, `saferelease` (`-O0`), and
   `release` (`-O1`); default is `release` which yields a typical
   performance gain over the previous release (with `-O0`) by a factor
   of 2.5−12. We discourage using `-O2` or `-O3` with g++, since this is
   not stable.
