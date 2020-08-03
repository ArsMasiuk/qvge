[OGDF](../../README.md) » [Porting Guide](../porting.md) » Baobab

# Porting from Sakura to Baobab

## Classes

### List, SList, ListPure, SListPure search() method

The `search(element)` method returned a position index.
It now returns a `ListIterator` or `ListConstIterator`.

| use case                                  | old code                            | new code                            |
| ----------------------------------------- | ----------------------------------- | ------------------------------------|
| check if element is a member of the list  | `list.search(element) != -1`      | `list.search(element).valid()`    |
| find the iterator for an element          | `list.get(list.search(element))`  | `list.search(element)`            |
| find the position index of an element     | `list.search(element)`            | `list.pos(list.search(element))`  |

### ClusterGraph

Some methods for reading and writing graphs have been **moved** to `GraphIO`
(as static methods, where the `ClusterGraph` is the first parameter):

| old method               | new method            |
| ------------------------ | --------------------- |
| `readClusterGML`       | `GraphIO::readGML`  |
| `writeGML`             | `GraphIO::writeGML` |
| `readClusterGraphOGML` | `GraphIO::readOGML` |

### ClusterGraphAttributes

Some **attribute access methods** have been renamed for consistency.
The following table shows the changes.

| old name                  | new name | remark |
| ------------------------- | -------- | ------ |
| `clusterBackColor`      | `fillBgColor` | colors are now represented by `Color` (instead of `String`) |
| `clusterColor`          | `strokeColor` | colors are now represented by `Color` (instead of `String`) |
| `clusterFillColor`      | `fillColor` | colors are now represented by `Color` (instead of `String`) |
| `clusterFillPattern`    | `fillPattern` | attribute type has been changed from `GraphAttributes::BrushPattern` to `FillPattern`; use `setFillPattern` to change the attribute value |
| `clusterHeight`         | `height` | |
| `clusterLabel`          | `label` | |
| `clusterLineStyle`      | `strokeType` | attribute type has been changed from `GraphAttributes::EdgeStyle` to `StrokeType`; use `setStrokeType` to change the attribute value |
| `clusterLineWidth`      | `strokeWidth` | attribute type has been changed from `double` to `float` |
| `clusterWidth`          | `width` | |
| `clusterXPos`           | `x` | |
| `clusterYPos`           | `y` | |
| `setClusterFillPattern` | `setFillPattern` | attribute type has been changed from `GraphAttributes::BrushPattern` to `FillPattern` |
| `setClusterLineStyle` | `setStrokeType` | attribute type has been changed from `GraphAttributes::EdgeStyle` to `StrokeType` |

Access to cluster attributes by **cluster index** has been removed.
Use index by cluster instead.

The method `clusterID(node v)` has been **removed**.
Use `cga.clusterOf(v)->index()` instead (if `cga` is an instance
of `ClusterGraphAttributes`).

Some methods for reading and writing graphs have been **moved** to `GraphIO`
(as static methods, where `ClusterGraphAttributes` is the first parameter):

| old method               | new method             |
| ------------------------ | ---------------------- |
| `readClusterGML`       | `GraphIO::readGML`   |
| `writeGML`             | `GraphIO::writeGML`  |
| `readClusterGraphOGML` | `GraphIO::readOGML`  |
| `writeOGML`            | `GraphIO::writeOGML` |

### DisjointSets

DisjointSets manages disjoint sets of *integers* (set IDs) from zero
to the maximum number of sets. The mapping (for example from nodes
to set IDs) is the responsibility of the user.
The first template parameter (the type of the set elements) is now
gone because it was not necessary.

The code
```c++
include <ogdf/basic/DisjointSets.h>
…
    DisjointSets<edge> uf(G.numberOfEdges());
    EdgeArray<int> setID(G);
    …
    edge e;
    …
    setID[e] = uf.makeSet(e);
…
```
simply becomes
```c++
include <ogdf/basic/DisjointSets.h>
…
    DisjointSets<> uf(G.numberOfEdges());
    EdgeArray<int> setID(G);
    …
    edge e;
    …
    setID[e] = uf.makeSet();
…
```

### Graph

Some methods for reading and writing graphs have been **moved** to `GraphIO`
(as static methods, where the `Graph` is the first parameter):

| old method        | new method            |
| ----------------- | --------------------- |
| `readGML`       | `GraphIO::readGML`  |
| `writeGML`      | `GraphIO::writeGML` |
| `readLEDAGraph` | `GraphIO::readLEDA` |


### GraphAttributes

Some **types** have been renamed and moved to `graphics.h`:

| old name                          | new name        |
| --------------------------------- | --------------- |
| `GraphAttributes::BrushPattern` | `FillPattern` |
| `GraphAttributes::EdgeArrow`    | `EdgeArrow`   |
| `GraphAttributes::EdgeStyle`    | `StrokeType`  |

The scope of some **attribute bit specifiers** (for the constructor of
`GraphAttributes`) has grown such that some other could be removed:
`GraphAttributes::nodeStyle` now implies `GraphAttributes::nodeColor`
which has hence been removed. In the same way, use `GraphAttributes::edgeStyle`
instead of `GraphAttributes::edgeColor`. `GraphAttributes::nodeLevel` has
just been removed.

Some **attribute access methods** have been renamed for consistency.
The following table shows the changes.

| old name          | new name        | remark |
| ----------------- | --------------- | ------ |
| `arrowEdge`     | `arrowType`   | attribute type has been changed from `GraphAttributes::EdgeArrow` to `EdgeArrow` |
| `colorEdge`     | `strokeColor` | colors are now represented by `Color` (instead of `String`) |
| `colorNode`     | `fillColor`   | colors are now represented by `Color` (instead of `String`) |
| `edgeWidth`     | `strokeWidth` | attribute type has been changed from `double` to `float` |
| `labelEdge`     | `label`       | |
| `labelNode`     | `label`       | |
| `lineWidthNode` | `strokeWidth` | attribute type has been changed from `double` to `float` |
| `nodeLine`      | `strokeColor` | colors are now represented by `Color` (instead of `String`) |
| `nodePattern`   | `fillPattern` | attribute type has been changed from `GraphAttributes::BrushPattern` to `FillPattern`; use `setFillPattern` to change the attribute value |
| `shapeNode`     | `shape`       | attribute type has been changed from `int` to `Shape` |
| `styleEdge`     | `strokeType`  | attribute type has been changed from `GraphAttributes::EdgeStyle` to `StrokeType`; use `setStrokeType` to change the attribute value |
| `styleNode`     | `strokeType`  | attribute type has been changed from `GraphAttributes::EdgeStyle` to `StrokeType`; use `setStrokeType` to change the attribute value |

The setter method `directed` has been **renamed** to `setDirected`.

Some types and attributes have been **removed** as they are not used anymore:
 * types: `GraphAttributes::ImageAlignment`, `GraphAttributes::ImageStyle`
 * methods: `imageAlignmentNode`, `imageDrawLineNode`, `imageHeightNode`, `imageStyleNode`, `imageUriNode`, `imageWidthNode`

Some methods for reading and writing graphs have been **moved** to GraphIO
(as static methods, where `GraphAttributes` is the first parameter):

| old method    | new method             |
| ------------- | ---------------------- |
| `readGML`   | `GraphIO::readGML`   |
| `writeGML`  | `GraphIO::writeGML`  |
| `readRudy`  | `GraphIO::readRudy`  |
| `writeRudy` | `GraphIO::writeRudy` |
| `writeSVG`  | `GraphIO::drawSVG`   |

### GraphCopy

Use `delNode()` and `delEdge()` (as you would do in a `Graph`) instead of `delCopy()`.

### SteinLibParser

This class has been **removed**.
The SteinLib file reader (`SteinLibParser::readSteinLibInstance`) is now in
`GraphIO` (`GraphIO::readSTP`).

### String

This class has been **removed**; use `std::string` instead.

Users of the `sprintf` methods should replace code like
```c++
    GA.label(v).sprintf("Node %d", v->index() + 1);
```
by code like
```c++
    GA.label(v) = "Node " + to_string(v->index() + 1);
```
For more sophisticated formatting, use a stringstream.


### PlanarizationLayout

Different modules, use now `SubgraphPlanarizer`

### TwoLayerCrossMin

Has been renamed to `LayerByLayerSweep`.

## Methods

Some methods that are common in some classes are changed/renamed/moved.

### read* and write*

These methods are usually moved to `GraphIO`.

### delCopy()

Use `delEdge()` or `delNode()` instead.

### getGraph()

If the graph is read-only, use `constGraph()`.

## Files

### simple_graph_load.h

This file has been completely **removed**.
All its functions are now static methods in the class `GraphIO`, though with a different name.
The following table explains the name changes.

| old name                 | new name                           |
| ------------------------ | ---------------------------------- |
| `loadRomeGraph`        | `GraphIO::readRome`              |
| `loadChacoGraph`       | `GraphIO::readChaco`             |
| `loadSimpleGraph`      | `GraphIO::readPMDissGraph`       |
| `loadYGraph`           | `GraphIO::readYGraph`            |
| `loadChallengeGraph`   | `GraphIO::readChallengeGraph`    |
| `saveChallengeGraph`   | `GraphIO::writeChallengeGraph`   |
| `loadEdgeListSubgraph` | `GraphIO::readEdgeListSubgraph`  |
| `saveEdgeListSubgraph` | `GraphIO::writeEdgeListSubgraph` |
| `loadBenchHypergraph`  | `GraphIO::writeChaco`            |
| `loadPlaHypergraph`    | `GraphIO::readPLA`               |
| `loadBenchHypergraph`  | `GraphIO::readBENCH`             |

For `loadYGraph`, note that the input file is now specified
by `istream` or a filename string, instead of a C-style `FILE` handle.

### SteinLibParser.h

This file has been completely **removed**.
The SteinLib file reader (`SteinLibParser::readSteinLibInstance`) is now in `GraphIO` (`GraphIO::readSTP`).

## General Concepts

### Colors

Colors are now represented by the class `Color`,
and no longer by strings of the form `"#RRGGBB"`.
For porting these string representations, `Color` offers you several options:
  * You can construct a `Color` from its string representation by passing the string in the constructor: `Color color("ff0000");`
  * You can set a color from the string representation: `Color color; ... color.fromString("ff0000");`
  * You can decompose the string representation and construct the color from its components: `Color color(0xff,0x00,0x00);`
  * You can use named colors from the `Color::Name` enumeration: `Color color(Color::Red);`

The most readable form is the latter one, so it might be useful to adopt this version where possible;
the `Color::Name` enumeration is quite extensive and corresponds to the
[SVG color keywords](http://www.w3.org/TR/SVG/types.html#ColorKeywords).

### GraphIO

GraphIO handles reading from and writing to several graph formats, saving to SVG vector graphics format, and so on.

Simple usage example:
```c++
include <ogdf/fileformats/GraphIO.h>

…
    GraphIO::readRome(G, "foo");
    …
    GraphIO::writeGML(G, "foo.gml");
    …
    GraphIO::drawSVG(GA, "foo.svg");
…
```

## Third-party Libraries

Some code uses LP solvers or SAT solvers. To keep the third-party dependencies
of OGDF small and ease the use of that solvers, such solvers are now included
or integrated into OGDF.

### Abacus

Abacus 3.2 is now integrated into OGDF, in its own namespace named `abacus`.

#### Abacus Classes

All classes like `ABA_FOOBAR` have been renamed
such that the `ABA_` prefix is removed and the suffix is camel-cased,
like `FooBar`.
Examples are `Master`, `VarType`, `Variable`, `Sub`, `CSense`, `FSVarStat`.

There are exceptions and some classes have also been removed. See the following list:

| old name         | use instead      |
| ---------------- | ---------------- |
| `ABA_BUFFER`   | `ArrayBuffer`  |
| `ABA_CPUTIMER` | `StopwatchCPU` |
| `ABA_STRING`   | `string`       |

#### Missing Header Files

Because Abacus' own timers are replaced by OGDF timers, the file `abacus/timer.h`
has been removed. Instead include `ogdf/basic/Stopwatch.h` and use the
`StopwatchWallClock` or `StopwatchCPU` classes. Note that they must not be
initialized with Abacus objects.

#### Const Correctness

Abacus is not fully const correct.
For the OGDF integration, missing `const` declarations have been inserted.
However, when using old Abacus code (written against the original Abacus)
this may lead to non-compiling code when you inherit from Abacus classes
with virtual methods that were non-const and are now const.

For example, virtual `Constraint` methods like `print` and `coeff` are
all declared const now. If you define non-const `print` and `coeff` in
your own constraints, the const `print` and `coeff` version are still
undefined. C++ compilers like `g++` may hence display errors like
"`cannot allocate an object of abstract type ‘YourConstraintType’`".
Adding `const` keywords in the right position(s) should solve the problems.

### COIN-OR

COIN-OR (Clp 1.14.7 and Symphony 5.4.5) is included in the OGDF source tree.
However, it is not linked to the OGDF binary.

Only minor changes have been made to the COIN-OR source which are documented in `src/coin/Readme.txt`.
