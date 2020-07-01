[OGDF](../../README.md) » [Release Notes](../relnotes.md) » Sakura

# OGDF Sakura (2012.07)

Released 2012-07-18.

This release is mainly a clean-up and bug-fix release,
bringing support for new compilers and fixing some annoying bugs.

Highlights:
 * Improved compiler support:
   * Support for the latest compiler versions added (gcc 4.7, Visual Studio 2012).
   * Support for new compilers / platforms: MinGW on Windows and LLVM/clang on
     Linux.
   * Adjusted generated project files for Visual Studio, so that source files
     can now be compiled in parallel.
   * Support for shared libraries (DLLs for Visual Studio 2008-2012, shared
     libraries for gcc, LLVM/clang).
 * Clean-up of classes for planarity testing and embedding:
   * `PlanarModule` has been renamed to `BoothLueker`.
   * Introduced a base class `PlanarityModule` for planarity testing and
     embedding and adjusted the interface of `BoothLueker` and `BoyerMyrvold`.
   * `extended_graph_alg.h` now contains simple functions for planarity
     testing and embedding.
 * Changed the interface of embedder modules from `PlanRep` to `Graph`.
   `PlanarStraightLayout` and `PlanarDrawLayout` now have a module option for
   the embedder.
 * Various important bugfixes:
   * Fixed crashes when compiling with gcc and `-O2` or `-O3`.
     By default, OGDF release builds now use `-O2`.
   * Fixed crashes of some embedder modules when the input graph contained
     blocks just consisting of two parallel edges.
   * Fixed a bug in the special handling of isolated nodes when minimizing
     crossings with `SugiyamaLayout`. Previous code did not work as intended,
     the revised code can decrease the number of crossings in many cases.

## New Features

 * Clean-up of classes for planarity testing and embedding:
   * `PlanarModule` has been renamed to `BoothLueker`; both have been
     adjusted to the new interface specified by `PlanarityModule`.
   * Introduced a base class `PlanarityModule` specifiying the interface for
     planarity testing and embedding.
   * Derived `BoothLueker` and `BoyerMyvold` from `PlanarityModule` and
     adjusted their interface.
   * Added new functions for planarity testing and embedding to
     `extended_graph_alg.h`, making it easier to call this functionality:
     `isPlanar()`, `planarEmbed()`, `planarEmbedPlanarGraph()`
   * Remark: If you used `PlanarModule` for planarity testing or embedding
     in your programs, you have to rewrite your code! The simplest way is
     to use the above functions from `extended_graph_alg.h` instead.
 * Changed the interface of embedder modules from `PlanRep` to `Graph`.
 * Added module option for the embedder to `PlanarStraightLyout` and
   `PlanarDrawLayout`.

## Minor Modifications

 * Renamed some graph load functions (for consistent naming):
   * `loadRomeGraphStream` → `loadRomeGraph`
   * `loadChacoStream` → `loadChacoGraph`
   * `loadSimpleGraphStream` → `loadSimpleGraph`
   * `loadBenchHypergraphStream` → `loadBenchHypergraph`
 * Made `Array2D::det()` method const
 * Moved definition of `MemElem` to `MallocMemoryAllocator`
 * Replaced ogdf’s `min`/`max` by `std::min`/`std::max`
 * Renamed `eLabelTyp` → `eLabelType`
 * Made destructor of class `Thread` virtual
 * Documentation adjusted to latest doxygen version (1.8.1.1).

## Bug Fixes

 * Fixed crashes when compiling with gcc and `-O2` or `-O3`. By default,
   OGDF release builds now use `-O2`.
 * Fixed crashes of some embedder modules when the input graph contained
   blocks just consisting of two parallel edges. Affected where
   `EmbedderMaxFace`, `EmbedderMaxFaceLayers`, `EmbedderMinDepth`,
   `EmbedderMinDepthMaxFace`, `EmbedderMinDepthMaxFaceLayers`.
 * Fixed a bug in the special handling of isolated nodes when minimizing
   crossings with `SugiyamaLayout`. Previous code did not work as intended,
   the revised code can decrease the number of crossings in many cases.
 * Fixed a bug in `makeConnected()`: now also works for empty graphs (just
   returns)
 * Added missing header files for `SteinLibParser`.
 * Corrected various errors when compiling `HyperGraph.h`.

## Build System

 * Added support for gcc 4.7.
 * Added support for Visual Studio 2012.
 * Added support for MinGW on Windows (just 32-bit MinGW version).
 * Added support for LLVM/clang on Linux.
 * Adjusted generated project files for Visual Studio, so that source files
   can now be compiled in parallel.
 * OGDF can now be built as DLL with Visual Studio 2008-2012.
 * OGDF can now be built as a shared library on Linux with gcc and LLVM/clang.
 * Clean-up of OGDF’s main directory:
   * Moved project templates to the subdirectory `config`
   * Moved OGDF’s doxygen configuration file `ogdf-doxygen.cfg`
     to the subdirectory doc
