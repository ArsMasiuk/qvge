[OGDF](../../README.md) » [Release Notes](../relnotes.md) » Baobab

# OGDF Baobab (2015.05)

Released 2015-05-30.

This release introduces various new algorithms and modules but also
improves usability and elegance. To accomplish the latter, some changes
had to be made that break compatibility to Sakura. Also, COIN-OR and ABACUS
are now included in the OGDF package to make every feature work out-of-the-box.

Noteworthy changes:
 * COIN-OR and ABACUS are integrated in the package for easier installation
 * improvements to `GraphAttributes` class
   * restructuring
   * more consistent naming scheme
   * improved color management (`Color` class instead of strings)
   * basic 3d support
 * new `LayoutStandards`: changeable default parameters (sizes, distances,
   colors) that allow more comparable drawings
 * basic support for hypergraphs (`Hypergraph` class)
 * reader and writer for fileformats
   * new support for DL, DOT, GDF, GEXF, GraphML, Tulip graph formats
   * new support for Bench and PLA hypergraph formats
   * improved GML and OGML parser
   * reading and writing files is now handled by `GraphIO` class
 * new layered crossing minimization for Sugiyama algorithm:
   grid sifting and global sifting heuristics (`GridSifting`,
   `GlobalSifting`; Bachmaier et al, 2011)
 * new `EmbedderOptimalFlexDraw` class for bend minimization in planar,
   orthogonal drawings (Bläsius et al, 2012)
 * new force-directed layout `BertaultLayout` that preserves
   edge-crossing properties (Bertault, 2000)
 * new parallelized version of `FastPlanarSubgraph`
 * new methods for some (restricted) upward planarity problems
   (`UpwardPlanarity` class)
 * new classes for stress majorization (`StressMinimization`),
   pivot multi-dimensional scaling (`PivotMDS`)
 * revised code for edge insertion into planar graphs (`EdgeInsertionModule`,
   `FixedEmbeddingInserter`, `VariableEmbeddingInserter*`,
   `SubgraphPlanarizer` classes)
 * new `Graph::CCsInfo` class to store information about connected components
 * new `LCA` class to compute lowest common ancestors in arborescences with
   linear preprocessing time and constant query time (Bender and
   Farach-Colton, 2000)
 * new `Voronoi` class to compute Voronoi regions in graphs
 * new graph generators, e.g., `randomSeriesParallelDAG()`
 * new simple graph algorithms, e.g., `makeBimodal()` or
   `makeMinimumSpanningTree()`
 * and as usual: bugfixes, (a little) code cleanup, and improvement of
   code quality (like const-correctness)
