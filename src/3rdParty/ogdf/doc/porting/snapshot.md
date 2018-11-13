[OGDF](README.md) » [Porting Guide](porting.md) » Snapshot

# Porting from Baobab to current snapshot

## General

### C++11

OGDF now requires C++11 features.
Make sure you do not use another standard when compiling your user programs.


### Compiling for debug mode

If you want to compile for debug mode (even if it is user code, linking
against OGDF), you do not need to define `OGDF_DEBUG` any longer. This is
automatically defined using CMake.


### Header files

It might be necessary that you need to include additional header files
for code that worked before. For example, `#include <ogdf/basic/NodeArray.h>`
does not ensure that you also have an `EdgeArray`.

We also removed header files that contained declarations of classes whose
implementation has already been removed in former releases.
This includes `FixedUpwardEmbeddingInserter.h` and `UpwardPlanarizationLayout.h`.

The directory structure for internal headers (for example, helper classes)
has also changed.

### Global namespace

In former versions of the OGDF some symbols were added to the global namespace.
This includes but is not limited to `ifstream`, `ofstream`, `cout`, `ios`, `endl`,
and `numeric_limits`. Thus, you may be required to explicitly use the `std` namespace.

Also many `std` functions and classes that were also available in the `ogdf` namespace are now
*only* available in the `std` namespace. For example, string-to-number conversion functions
like `std::stoul` cannot be called using `ogdf::stoul` any longer.

### Exceptions

When a `PreconditionViolationException` or `AlgorithmFailureException` was only thrown
in debug mode, it is now replaced by an assertion (`OGDF_ASSERT`).


### Enum Classes

All enums are now enum classes or const static.
Since the enumerators are scoped, the distinguishing prefixes
of the enumerators have been dropped.
Unscoped enums are now forbidden in the OGDF.


### COIN-OR

We have removed Symphony and Cgl from the included COIN-OR package.
The only LP solver we ship is hence Clp.

If your code used Cgl or Symphony, you have to link your objects to
the original versions of these libraries now.


## Macros

Some macros have been removed because they are not necessary any longer.

This includes
 * `OGDF_NEW` (simply use `new` instead)
 * `ABACUS_LP_OSI` (Abacus always uses COIN)
 * `OGDF_NO_COMPILER_TLS` (thread-local storage is a C++11 feature)
 * `OGDF_DECL_THREAD` (simply use `thread_local` instead)

We also have removed most of the iteration macros since C++11 offers range-based for loops.
This includes the removal of `forall_nodes` and `forall_edges`.

```cpp
node v;
Graph G;

forall_nodes(v, G) { ... }
```
should be replaced by
```cpp
for(node v : G.nodes) { ... }
```

`OGDF_ASSERT_IF` was replaced by `OGDF_HEAVY_ASSERT`. The debug level argument was dropped.

There must now be a semicolon after usage of the macro `OGDF_ASSERT` and `OGDF_HEAVY_ASSERT`.

## Changed class, function, header names

### stNumber(), testSTnumber()

To access the st-numbering functions, include `ogdf/basic/STNumbering.h`
(instead of `ogdf/basic/extended_graph_alg.h`).

The respective functions were renamed:

| Former       | New                 |
|--------------|---------------------|
| stNumber     | computeSTNumbering  |
| testSTnumber | isSTNumbering       |

### DTreeMultilevelEmbedder

The `DTreeMultilevelEmbedder` header file is moved
from `include/ogdf/internal/energybased/` to `include/ogdf/energybased`
because it is not internal.

Its internal header files however have moved to
`include/ogdf/energybased/dtree`.

### MixedForceLayout

The class `MixedForceLayout` was removed.
Instead of calling this layout algorithm, call the `FastMultipoleEmbedder`
directly followed by a call to the `SpringEmbedderGridVariant`.

### TricComp

The (formerly internal) class `TricComp` was renamed to `Triconnectivity`
and can now be found in `include/ogdf/graphalg/Triconnectivity.h`.

### PlanRepInc

The method `writeGML(const char*, GraphAttributes&, bool)`
was deleted. Use `writeGML(ostream &os, const GraphAttributes&)` instead.

## Changed method signatures

The parameter `BlockOrder* order` was deleted from the constructors of `Block` in `BlockOrder.h`.
Use the constructors `Block(edge e)` and `Block(node v)` instead.

## Graph class

### List of Adjacent Edges

`NodeElement::adjEdges` was renamed to `NodeElement::adjEntries`.
This means you have to change `for(adjEntry adj : v->adjEdges)` to `for(adjEntry adj : v->adjEntries)`.

The following getter-methods were moved from `Graph` to `NodeElement`.
All of these methods take a single list as the only parameter now.
 * `adjEntries()` (renamed to `allAdjEntries()`)
 * `adjEdges()`
 * `inEdges()`
 * `outEdges()`


### Hiding and restoring edges

Hiding and restoring edges was not a safe operation in former releases.
Replace code like
```c++
    G.hideEdge(e);
    // ...
    G.restoreEdge(e);
```
by
```c++
   Graph::HiddenEdgeSet hiddenSet(G);
   hiddenSet.hide(e);
   // ...
   hiddenSet.restore(e);
```
All edges are restored by `hiddenSet.restore()` or automatically by the `HiddenEdgeSet` destructor.


### Typofix: collaps

The method `collaps` has been renamed to `collapse`.

### Method to compute the next Power of 2

The static method `nextPower2` was slightly changed and moved to `Math`.
The computed result is no longer strictly larger than the second parameter.
`nextPower2(0)` is now `0` instead of `1`.

## GraphCopy class

The `newEdge()` methods to add edges at predefined positions in the adjacency list
have been removed from `GraphCopy`.
You can instead use the respective `newEdge()` function inherited from `Graph`,
followed by `GraphCopy::setEdge()`.

## Stack & StackPure & BoundedStack classes

The classes `Stack`, `StackPure` and `BoundedStack` are deleted.
Use `ArrayBuffer` instead (which provides the functionality of all three
classes (and more) and outperforms all implementations). Change
`pop()` in your implementations to `popRet()`.

## List & ListPure class

The deprecated methods `List::exchange()` and `ListPure::exchange()` were removed.
Use `List::swap()` and `ListPure::swap()` instead.

## Planar Subgraph Algorithms

`BoyerMyrvoldSubgraph` was renamed to `PlanarSubgraphBoyerMyrvold` and `FastPlanarSubgraph` to `PlanarSubgraphFast`.
`MaximalPlanarSubgraphSimple` now supports arbitrary start heuristics.
As such, the `doMaximize` option was removed from `PlanarSubgraphBoyerMyrvold` (formerly `BoyerMyrvoldSubgraph`).
`MaximalPlanarSubgraphSimple::clone()` should be used to obtain a copy of the respective instance
since `MaximalPlanarSubgraphSimple(const MaximalPlanarSubgraphSimple &mps)` is no longer available.

Support for edge weights was added to `MaximumCPlanarSubgraph`.
The signature of `call` includes the new parameter `EdgeArray<int> *pCost` that can be set to `nullptr` for uniform weight.

The Module `PlanarSubgraphModule` is now a template.
The implementations `PlanarSubgraphCactus`, `MaximalPlanarSubgraphSimple`,
`PlanarSubgraphEmpty` and `PlanarSubgraphFast` are templates itself.
All other implementations of the module inherit from `PlanarSubgraphModule<int>`.

### MaximumCPlanarSubgraph

`MaximumCPlanarSubgraph` supports advanced calls that also return the edges required to connect the input graph.
To avoid conflicts with the method defined by `CPlanarSubgraphModule`, `MaximumCPlanarSubgraph::call` was renamed to
`MaximumCPlanarSubgraph::callAndConnect`.

## LayoutClusterPlanRepModule

The signature of `call()` changed. The last two parameters `List<NodePair>& npEdges` and `List<edge>& newEdges`
became the single parameter `List<edge>& origEdges` that contains all edges in the original that still need to be inserted.
The original graph must now contain all edges prior to the call.
This also means that the input graph is no longer modified (except for embedding it).

## Comparers

`NodeComparer` was removed. Instead of `NodeComparer<T> foo(array, asc)` use `GenericComparer<node, T, asc> foo(array)`.
`IndexComparer` also was removed. Use `GenericComparer<node, int>([](node v) { return v->index(); });` instead.
If you need a default constructor you may use `OGDF_DECLARE_COMPARER`.

## Heaps and Priority Queues

In an attempt to unify the existing heap classes, `BinaryHeap` and `BinaryHeap2` were replaced by a new `BinaryHeap`.
All heap implementations are derived from the common interface `HeapBase`.
Additionally, the classes `MinPriorityQueue` and `PQueue` were removed.

The following heap implementations were introduced:
 * `BinaryHeap`
 * `BinomialHeap`
 * `FibonacciHeap`
 * `PairingHeap`
 * `RadixHeap`

Priority queues might be realized using the newly introduced `PriorityQueue` independently of the desired heap implementation.
In contrast to `PriorityQueue` that merely stores directly comparable elements, `PrioritizedQueue` is used to store elements with priorities assigned upon insertion.
For even simpler usage see `PrioritizedMapQueue` that keeps track of handles for the inserted elements but requires elements to be unique.

The tests in `test/src/basic/heap.cpp` show exemplary usage of the new classes.

### Method equivalents

#### BinaryHeap
`BinaryHeap` was replaced by `PrioritizedQueue`.
Accessing elements at arbitrary positions is no longer supported.

| Former                    | New                                                      |
|---------------------------|----------------------------------------------------------|
| `BinaryHeap::clear`       | `PrioritizedQueue::clear`                                |
| `BinaryHeap::decPriority` | `PrioritizedQueue::decrease`                             |
| `BinaryHeap::empty`       | `PrioritizedQueue::empty`                                |
| `BinaryHeap::extractMin`  | `PrioritizedQueue::topElement` & `PrioritizedQueue::pop` |
| `BinaryHeap::getMin`      | `PrioritizedQueue::topElement`                           |
| `BinaryHeap::insert`      | `PrioritizedQueue::push`                                 |
| `BinaryHeap::operator[]`  | -                                                        |
| `BinaryHeap::pop`         | `PrioritizedQueue::pop`                                  |
| `BinaryHeap::push`        | `PrioritizedQueue::push`                                 |
| `BinaryHeap::size`        | `PrioritizedQueue::size`                                 |
| `BinaryHeap::top`         | `PrioritizedQueue::topElement`                           |

#### BinaryHeap2

`BinaryHeap2` was replaced by `PrioritizedMapQueue`. Querying the capacity of the heap is no longer supported.

| Former                       | New                                                            |
|------------------------------|----------------------------------------------------------------|
| `BinaryHeap2::capacity`      | -                                                              |
| `BinaryHeap2::clear`         | `PrioritizedMapQueue::clear`                                   |
| `BinaryHeap2::decreaseKey`   | `PrioritizedMapQueue::decrease`                                |
| `BinaryHeap2::empty`         | `PrioritizedMapQueue::empty`                                   |
| `BinaryHeap2::extractMin`    | `PrioritizedMapQueue::topElement` & `PrioritizedMapQueue::pop` |
| `BinaryHeap2::getMinimumKey` | `PrioritizedMapQueue::topPriority`                             |
| `BinaryHeap2::getPriority`   | `PrioritizedMapQueue::priority`                                |
| `BinaryHeap2::insert`        | `PrioritizedMapQueue::push`                                    |
| `BinaryHeap2::size`          | `PrioritizedMapQueue::size`                                    |
| `BinaryHeap2::topElement`    | `PrioritizedMapQueue::topElement`                              |

#### PQueue

`PQueue` was replaced by `PrioritizedQueue`.

| Former                       | New            |
|------------------------------|----------------|
| `PQueue::del_min`  | `PrioritizedQueue::pop`  |
| `PQueue::find_min` | `PrioritizedQueue::top`  |
| `PQueue::insert`   | `PrioritizedQueue::push` |

#### MinPriorityQueue

`MinPriorityQueue` was replaced by `PrioritizedQueue`. Note that `PrioritizedQueue::size` returns the number of elements in the heap instead of the capacity.

| Former                               | New                          |
|--------------------------------------|------------------------------|
| `MinPriorityQueue::count`            | `PrioritizedQueue::size`     |
| `MinPriorityQueue::decreasePriority` | `PrioritizedQueue::decrease` |
| `MinPriorityQueue::empty`            | `PrioritizedQueue::empty`    |
| `MinPriorityQueue::getMin`           | `PrioritizedQueue::top`      |
| `MinPriorityQueue::insert`           | `PrioritizedQueue::push`     |
| `MinPriorityQueue::pop`              | `PrioritizedQueue::pop`      |
| `MinPriorityQueue::size`             | -                            |

## Layout algorithms and graph constraints

Layout algorithms no longer support `GraphConstraints`.
Use `call(GraphAttributes)` instead of `call(GraphAttributes, GraphConstraints)`.
Graph constraints were removed entirely.
Removed classes: `Constraint`, `ConstraintManager`, and `GraphConstraints`.

## GraphIO

Methods that take filenames (instead of streams) are deprecated and will be removed in future versions.
For example, instead of using

```c++
	GraphIO::readLEDA(G, "circulant.lgr");
	GraphIO::writeChaco(G, "circulant.gml");
```

you have to create the streams manually, i.e.,

```c++
	std::ifstream is("circulant.lgr");
	GraphIO::readLEDA(G, is);
	std::ofstream os("circulant.gml");
	GraphIO::writeGML(G, os);
```

There are also helper functions `GraphIO::read()` and `GraphIO::write()`
that accept filenames for `GraphAttribute`s and `Graph`s,
so you can also write

```c++
	GraphIO::read(G, "circulant.lgr", GraphIO::readLEDA);
	GraphIO::write(G, "circulant.gml", GraphIO::writeGML);
```

Do not confuse the new filename-based `GraphIO::read()` helper function with
our new (experimental) generic reader function of the same name.
However, this generic reader even allows to write

```c++
	GraphIO::read(G, "circulant.lgr");
	GraphIO::write(G, "circulant.gml", GraphIO::writeGML);
```

## GmlParser

If you happened to use the `GmlParser` class directly (instead of
`GraphIO`), the constructor with filename argument is also gone
now. Use the constructor for input streams instead.

## Filesystem functions

If you have used filesystem functions in your code,
you can include `ogdf/basic/filesystem.h` now and you get them back.
They are marked deprecated now since there are maintained and portable
alternatives like [tinydir](https://github.com/cxong/tinydir).
Also C++17 will have filesystem functions.

## CombinatorialEmbedding

`CombinatorialEmbedding::splitFace(adjEntry, node)` and `CombinatorialEmbedding::splitFace(node, adjEntry)`
are used to insert degree-0 nodes without having to call `CombinatorialEmbedding::computeFaces()` again.
In previous version both methods could be used with non-isolated nodes
if the last adjacency entry belonged to the same face as the other adjacency entry.
This is no longer supported and the node has to be isolated.
To reflect this functional change the respective versions of `splitFace` were renamed to `addEdgeToIsolatedNode`.
Use `myEmbedding.splitFace(v->lastAdj(), adj)` instead of `myEmbedding.splitFace(v, adj)` if `v` isn't isolated (and you are sure that the adjacency entries lie on the same face!).
Use `myEmbedding.addEdgeToIsolatedNode(v, adj)` if `v` is isolated.

## OGDF_USE_SSE2, OGDF_CHECK_SSE2

The macros mentioned above have been removed.
You can check for the SSE2 CPU feature directly using
`ogdf::System::cpuSupports(ogdf::CPUFeature::SSE2)`.

## ModuleOption

The template class `ModuleOption` was removed. `std::unique_ptr` should be used instead.

## Angle functions for DPoint and GenericPoint

The static methods `angle` and `angleDegrees` of `DPoint` and `GenericPoint` are now
non-static. That means, old calls of

```c++
	double angle = DPoint::angle(point1, point2, point3);
```

now become

```c++
	double angle = point1.angle(point2, point3);
```

## PoolMemoryAllocator

Some types / constants of `PoolMemoryAllocator` were deleted or renamed.

| Former              | New          |
|---------------------|--------------|
| `BlockChainPtr`     | -            |
| `eBlockSize`        | `BLOCK_SIZE` |
| `eMinBytes`         | `MIN_BYTES`  |
| `ePoolVectorLength` | -            |
| `eTableSize`        | `TABLE_SIZE` |
| `MemElemEx`         | -            |
| `MemElemExPtr`      | -            |
| `PoolVector`        | -            |


## NonPlanarCore

`NonPlanarCore<TCost>::mincut(edge e)` now returns an `NonPlanarCore<TCost>::CutEdge` which is
a struct with the two parameters `edge e`, the edge itself and a `bool dir` which indicates the
direction of the edge, i.e. indicates if the edge goes from s to t or the other way round.
Furthermore the `NonPlanarCore` can now handle weighted edges and therefore is templated with
the type of the weights.

## MMMExample layouts

Some very simple example layout algorithms (using the `ModularMultilevelMixer`) were removed.
This includes `MMMExampleFast`, `MMMExampleNice`, and `MMMExampleNoTwist`.
The respective code can still be found in `doc/examples/layout/multilevelmixer.cpp`.

## FMMMLayout

The enumerators from `FMMMLayout` are now in a new class `FMMMOptions`.
Hence, for example,
`FMMMLayout::apInteger` became `FMMMOptions::AllowedPositions::Integer` and
`FMMMLayout::gcNonUniformProbLowerMass` became `FMMMLayout::GalaxyChoice::NonUniformProbLowerMass`.

## DisjointSets

The basic data structure `DisjointSets` is highly customizable using
template parameters. The enumerator names to do so have now changed.

| Former  | New                                         |
|---------|---------------------------------------------|
| `NL`    | `LinkOptions::Naive`                        |
| `LI`    | `LinkOptions::Index`                        |
| `LS`    | `LinkOptions::Size`                         |
| `LR`    | `LinkOptions::Rank`                         |
| `PC`    | `CompressionOptions::PathCompression`       |
| `PS`    | `CompressionOptions::PathSplitting`         |
| `PH`    | `CompressionOptions::PathHalving`           |
| `R1`    | `CompressionOptions::Type1Reversal`         |
| `CO`    | `CompressionOptions::Collapsing`            |
| `NF`    | `CompressionOptions::Disabled`              |
| `NI`    | `InterleavingOptions::Disabled`             |
| `Rem`   | `InterleavingOptions::Rem`                  |
| `TvL`   | `InterleavingOptions::Tarjan`               |
| `IR0`   | `InterleavingOptions::Type0Reversal`        |
| `IPSPC` | `InterleavingOptions::SplittingCompression` |

The following arrays (containing the string expansions of the settings)
are removed:
 * `linkOptionNames`
 * `compressionOptionNames`
 * `interleavingOptionNames`

## isForest() and isFreeForest()

The function `isForest()` in `basic/simple_graph_alg.h` has been deprecated in favor of `isArborescenceForest()`.
`isForest()` formerly returned `true` even when the graph contained multi-edges or self-loops.
This is no longer the case for `isArborescenceForest()`.

Furthermore, `isFreeForest()` has been deprecated in favor of `isAcyclicUndirected()`.

## connectedIsolatedComponents()

The function `connectedIsolatedComponents()` in `basic/simple_graph_alg.h` has been deprecated in favor of `connectedComponents()`.
`connectedIsolatedComponents(graph, isolatedNodes, component)` can be rewritten as `connectedComponents(graph, component, &isolatedNodes)`.

## makeParallelFreeUndirected()

The overloaded functions of `makeParallelFreeUndirected()` in
`basic/simple_graph_alg.h` have been deprecated in favor of a single function
that takes pointers as parameters (their default being `nullptr`).
`makeParallelFreeUndirected(graph, parEdges, cardPositive, cardNegative)` can be rewritten as
`makeParallelFreeUndirected(graph, &parEdges, &cardPositive, &cardNegative)`.

## doDestruction()

The function template `template<class E> doDestruction(const E *)` has been removed
in favor of `!std::is_trivially_destructible<E>::value`. Hence, lists of elements with
trivial destructors are now deallocated in constant time automatically, without
the necessity of writing a template specialization.

## GraphAttributes

The methods `setDirected`, `setStrokeType`, and `setFillPattern` have been removed from `GraphAttributes`.
Use the respective getters (`directed`, `strokeType`, and `fillPattern`) instead to obtain a non-const reference to the underlying value.

The method `initAttributes` was renamed to `addAttributes` as it does not disable previously enabled attributes.

## Sets of faces and nodes

Some set classes were removed / renamed.

| Former          | New                                       |
|-----------------|-------------------------------------------|
| `FaceSetSimple` | `FaceSet<false>`                          |
| `FaceSetPure`   | `FaceSet<false>`                          |
| `FaceSet`       | `FaceSet<>` (defaults to `FaceSet<true>`) |
| `NodeSetSimple` | `NodeSet<false>`                          |
| `NodeSetPure`   | `NodeSet<false>`                          |
| `NodeSet`       | `NodeSet<>` (defaults to `FaceSet<true>`) |

## Math class

`Math` is no longer a class with static methods but a namespace.

## Geometry

The classes `DPoint` and `DVector` are now merged. Note that `DVector::length()` is now `DPoint::norm()`.
Furthermore, `DVector::operator^` is now `DPoint::determinant`.

The class `DScaler` and the methods `ogdf::DRound` and `DPolyline::convertToInt` were deleted.

The classes `DLine` and `DSegment` were rewritten in such a way that `DLine`
represents infinite lines and `DSegment` represents finite line segments with
start and end points. The methods to access the sides of a `DRect` now return
`DSegment`s instead of `DLine`s, and they were renamed appropriately.

| Former                            | New                             |
|-----------------------------------|---------------------------------|
| `DLine::intersectionOfLines()`    | `DLine::intersection()`         |
| `DSegment::intersectionOfLines()` | `DLine::intersection()`         |
| `DRect::bottomLine()`             | `DRect::bottom()`               |
| `DRect::leftLine()`               | `DRect::left()`                 |
| `DRect::rightLine()`              | `DRect::right()`                |
| `DRect::topLine()`                | `DRect::top()`                  |

The methods `intersection()`, `verIntersection()` and `horIntersection()` of
`DLine` and `DSegment` now return an `IntersectionType` instead of a `bool`.

## HyperGraph

The class `HyperGraph` was removed. Use `Hypergraph` instead.

## EFreeList, EList, and EStack

The classes `EFreeList`, `EList` and `EStack` were removed. Use `List` and `Stack` instead.
