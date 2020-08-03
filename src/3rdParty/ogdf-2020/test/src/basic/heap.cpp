/** \file
 * \brief Tests for implementations of various heaps.
 *
 * \author Łukasz Hanuszczak, Tilo Wiedera
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

#include <vector>
#include <queue>
#include <set>
#include <algorithm>
#include <numeric>

#include <ogdf/basic/comparer.h>
#include <ogdf/basic/graph_generators.h>
#include <ogdf/basic/PriorityQueue.h>
#include <ogdf/graphalg/Dijkstra.h>

#include <ogdf/basic/heap/BinaryHeap.h>
#include <ogdf/basic/heap/BinomialHeap.h>
#include <ogdf/basic/heap/FibonacciHeap.h>
#include <ogdf/basic/heap/RMHeap.h>
#include <ogdf/basic/heap/RadixHeap.h>
#include <ogdf/basic/heap/HotQueue.h>

#include <testing.h>

static std::vector<int> randomVector(size_t n)
{
	std::default_random_engine rng(n);
	std::uniform_int_distribution<int> dist;

	std::vector<int> values;
	for (size_t i = 0; i < n; i++) {
		values.push_back(dist(rng));
	}

	return values;
}

template<template<typename T, typename C> class H>
void simpleScenarioTest(bool supportsDecrease, bool supportsMerge)
{
	describe("simple scenario test", [&]() {
		using Handle = typename H<int, std::less<int>>::Handle;
		constexpr void *invalidHandle = nullptr;
		Handle handle = nullptr;

		it("pushes values", [&]() {
			H<int, std::less<int>> heap;

			handle = heap.push(3);
			AssertThat(handle, Is().Not().EqualTo(invalidHandle));
			AssertThat(heap.value(handle), Equals(3));

			handle = heap.push(10);
			AssertThat(handle, Is().Not().EqualTo(invalidHandle));
			AssertThat(heap.value(handle), Equals(10));

			handle = heap.push(5);
			AssertThat(handle, Is().Not().EqualTo(invalidHandle));
			AssertThat(heap.value(handle), Equals(5));

			handle = heap.push(7);
			AssertThat(handle, Is().Not().EqualTo(invalidHandle));
			AssertThat(heap.value(handle), Equals(7));
		});

		it("pops in the right order", [&]() {
			H<int, std::less<int>> heap;
			heap.push(3);
			heap.push(10);
			heap.push(5);
			heap.push(7);

			AssertThat(heap.top(), Equals(3));
			heap.pop();
			AssertThat(heap.top(), Equals(5));
			heap.pop();
			AssertThat(heap.top(), Equals(7));
			heap.pop();
			AssertThat(heap.top(), Equals(10));
		});

		if (supportsDecrease) {
			it("decreases values and pops in the right order", []() {
				H<int, std::less<int>> heap;
				heap.push(3);
				heap.push(10);
				Handle node5 = heap.push(5);
				Handle node7 = heap.push(7);

				AssertThat(heap.top(), Equals(3));
				heap.decrease(node7, 2);
				AssertThat(heap.value(node7), Equals(2));
				AssertThat(heap.top(), Equals(2));
				heap.pop();
				AssertThat(heap.top(), Equals(3));
				heap.pop();
				AssertThat(heap.top(), Equals(5));
				heap.decrease(node5, 1);
				AssertThat(heap.value(node5), Equals(1));
				AssertThat(heap.top(), Equals(1));
				heap.pop();
				AssertThat(heap.top(), Equals(10));
			});
		}

		if(supportsMerge) {
			it("merges two heaps", []() {
				H<int, std::less<int>> h1, h2;
				h1.push(3);
				h1.push(5);
				h1.push(-2);

				h2.push(1);
				h2.push(-1);
				h2.push(4);

				h1.merge(h2);

				AssertThat(h1.top(), Equals(-2));
				h1.pop();
				AssertThat(h1.top(), Equals(-1));
				h1.pop();
				AssertThat(h1.top(), Equals(1));
				h1.pop();
				AssertThat(h1.top(), Equals(3));
				h1.pop();
				AssertThat(h1.top(), Equals(4));
				h1.pop();
				AssertThat(h1.top(), Equals(5));
			});
		}
	});
}

template<typename T, typename C, template<typename X, typename Y> class H>
void sortingDatasetTest(const std::vector<T> &values)
{
	using Handle = typename H<T, C>::Handle;

	it("pushes and pops values in correct order", [&]() {
		H<T, C> heap;

		for(const T &v : values) {
			Handle node = heap.push(v);
			AssertThat(heap.value(node), Equals(v));
		}

		C compare;
		std::vector<int> sorted(values);
		std::sort(sorted.begin(), sorted.end(), compare);

		for (const T &v : sorted) {
			// compare is either "greater" or "less"
			AssertThat(compare(heap.top(), v), IsFalse());
			AssertThat(compare(v, heap.top()), IsFalse());
			heap.pop();
		}
	});
}

template<typename T, typename C, template<typename X, typename Y> class H>
void mergingDatasetTest(const std::vector<T> &a, const std::vector<T> &b)
{
	using Handle = typename H<T, C>::Handle;

	it("pushes values and merges heaps", [&]() {
		H<T, C> heapA, heapB;

		for(const T &v : a) {
			Handle node = heapA.push(v);
			AssertThat(heapA.value(node), Equals(v));
		}
		for(const T &v : b) {
			Handle node = heapB.push(v);
			AssertThat(heapB.value(node), Equals(v));
		}

		std::vector<int> merged, sortedA(a), sortedB(b);
		std::sort(sortedA.begin(), sortedA.end(), C());
		std::sort(sortedB.begin(), sortedB.end(), C());
		std::merge(
				sortedA.begin(), sortedA.end(),
				sortedB.begin(), sortedB.end(),
				std::back_inserter(merged));

		heapA.merge(heapB);

		for(const T &v : merged) {
			AssertThat(heapA.top(), Equals(v));
			heapA.pop();
		}
	});
}

template<template<typename T, typename C> class H>
void sortingRandomTest(size_t n)
{
	std::string desc = "sorting on " + std::to_string(n) + " random values";
	describe(desc.data(), [&]() {
		std::vector<int> data;

		before_each([&](){
			data = randomVector(n);
		});

		sortingDatasetTest<int, std::less<int>, H>(data);
	});
}

template<template<typename T, typename C> class H>
void sortingComparerTest(size_t n)
{
	std::string descStandard = "sorting on " + std::to_string(n) + " random values with standard comparer";
	describe(descStandard.data(), [&]() {
		std::vector<int> data;

		before_each([&](){
			data = randomVector(n);
		});

		sortingDatasetTest<int, ogdf::StlGreater<int, ogdf::StdComparer<int>>, H>(data);
	});

	class ModComparer {
	public:
		static int compare(const int &x, const int &y) {
			return x % 3 - y % 3;
		}
	OGDF_AUGMENT_STATICCOMPARER(int)
	};

	std::string descCustom = "sorting on " + std::to_string(n) + " random values with custom comparer";
	describe(descCustom.data(), [&]() {
		std::vector<int> data;

		before_each([&](){
			data = randomVector(n);
		});

		sortingDatasetTest<int, ogdf::StlLess<int, ModComparer>, H>(data);
	});
}

template<template<typename T, typename C> class H>
void mergingRandomTest(size_t n)
{
	std::string desc = "merging on " + std::to_string(n) + " random values";
	describe(desc.data(), [&]() {
		std::vector<int> a;
		std::vector<int> b;

		before_each([&](){
			a = (randomVector(n / 3));
			b = (randomVector(2 * (n / 3) + n % 3));
		});

		mergingDatasetTest<int, std::less<int>, H>(a, b);
	});
}

template<template<typename T, typename C> class H>
void destructorTest()
{
	describe("destructor test", []() {
		size_t N = 1000;

		it("should push random values and release memory", [&]() {
			std::vector<int> data(randomVector(N));

			H<int, std::less<int>> h;
			std::for_each(data.begin(), data.end(), [&](int v) { h.push(v); });
		});

		it("shoud push sorted values and release memory", [&]() {
			std::vector<int> data(N);
			std::iota(data.begin(), data.end(), 1);

			H<int, std::less<int>> h;
			std::for_each(data.begin(), data.end(), [&](int v) { h.push(v); });
		});

		it("should push and pop random values and release memory", [&]() {
			std::vector<int> data(randomVector(N));

			H<int, std::less<int>> h;
			for(size_t i = 0; i < N; i++) {
				if(i % 3 < 2) {
					h.push(data[i]);
				} else {
					h.pop();
				}
			}
		});
	});
}

template<template<typename T, class C> class Impl>
void prioritizedQueueWrapperTest(std::size_t n)
{
	std::string desc = "prioritized queue wrapper test on " + std::to_string(n) + " rands";
	describe(desc.data(), [&]() {

		std::default_random_engine rng(n);

		it("works for integers", [&]() {
			std::vector<int> data(randomVector(n));
			PrioritizedQueue<int, int, std::greater<int>, Impl> queue;

			std::set<int> indices;
			for(int i = 0; i < static_cast<int>(data.size()); i++) {
				indices.insert(i);
			}

			std::uniform_int_distribution<int> dist;

			// randomly insert elements
			for(int i = 0; i < static_cast<int>(data.size()); i++) {
				int pos = dist(rng) % (int) indices.size();
				std::set<int>::iterator it = indices.begin();
				advance(it, pos);
				queue.push(data[*it], *it);
				indices.erase(it);
			}

			AssertThat(queue.size(), Equals(data.size()));

			// pop elements in order
			for(int i = (int) queue.size() - 1; !queue.empty(); i--) {
				AssertThat(queue.topElement(), Equals(data.back()));
				AssertThat(queue.topPriority(), Equals(i));
				queue.pop();
				data.pop_back();
			}

			queue.clear();
		});

		it("works for nodes", [&]() {
			std::uniform_int_distribution<int> dist(0, (int) n);

			Graph graph;
			int m = (int) dist(rng);
			randomGraph(graph, (int) std::sqrt(m), m);
			PrioritizedMapQueue<node, int, std::less<int>, Impl> queue(graph);

			for(node v : graph.nodes) {
				AssertThat(queue.contains(v), Is().False());
				queue.push(v, v->degree());
				AssertThat(queue.contains(v), Is().True());
			}

			AssertThat((int) queue.size(), Equals(graph.numberOfNodes()));

			int lastDegree = 0;
			while(!queue.empty()) {
				node v = queue.topElement();
				AssertThat(queue.contains(v), Is().True());
				AssertThat(v->degree(), Equals(queue.topPriority()));
				AssertThat(v->degree(), !IsLessThan(lastDegree));
				lastDegree = v->degree();
				queue.pop();
				AssertThat(queue.contains(v), Is().False());
			}

			queue.clear();
		});

		it("works for edges", [&]() {
			Graph graph;
			randomTree(graph, (int)(n+1));
			PrioritizedMapQueue<edge, int, std::less<int>, Impl> queue(graph);

			for(edge e : graph.edges) {
				AssertThat(queue.contains(e), Is().False());
				queue.push(e, e->index());
				AssertThat(queue.contains(e), Is().True());
			}

			AssertThat((int) queue.size(), Equals(graph.numberOfEdges()));

			for(int i = 0; i < graph.numberOfEdges(); i++) {
				edge e = queue.topElement();
				AssertThat(queue.contains(e), Is().True());
				AssertThat(e->index(), Equals(i));
				queue.pop();
				AssertThat(queue.contains(e), Is().False());
			}

			AssertThat(queue.empty(), IsTrue());
			queue.clear();
		});
	});
}

template<template<typename T, class C> class Impl>
void priorityQueueWrapperTest(std::size_t n, bool supportsMerge = true)
{
	using PQ = PriorityQueue<int, std::greater<int>, Impl>;

	std::string desc = "queue wrapper test on " + std::to_string(n) + " rands";
	describe(desc.data(), [&]() {
		std::vector<int> data;
		auto init = { 3, 1, 6, -20, 4, 2, -4, 1, 6 };
		PQ ogdfPQ;

		before_each([&](){
			ogdfPQ.clear();
			data = randomVector(n);
		});

		it("behaves like std::priority_queue", [&]() {
			std::priority_queue<int> stdPQ;

			for(int e : data) {
				ogdfPQ.push(e);
				stdPQ.push(e);

				AssertThat(ogdfPQ.size(), Equals(stdPQ.size()));
				AssertThat(ogdfPQ.top(), Equals(stdPQ.top()));
			}

			while(!stdPQ.empty()) {
				AssertThat(ogdfPQ.empty(), IsFalse());
				AssertThat(ogdfPQ.top(), Equals(stdPQ.top()));
				AssertThat(ogdfPQ.size(), Equals(stdPQ.size()));

				ogdfPQ.pop();
				stdPQ.pop();
			}

			AssertThat(ogdfPQ.empty(), IsTrue());
		});


		it("allows to be initialized with initializer list", [&]() {
			ogdfPQ = init;
			AssertThat(ogdfPQ.size(), Equals(init.size()));
			std::vector<int> elems = init;

			while(!ogdfPQ.empty()) {
				int value = ogdfPQ.top();
				auto it = std::find(elems.begin(), elems.end(), value);

				AssertThat(it != elems.end(), IsTrue());

				elems.erase(it);
				ogdfPQ.pop();
			}
		});

		it("supports move-construction", [&]() {
			ogdfPQ = init;
			PQ tmp(std::move(ogdfPQ));
			AssertThat(tmp.size() == init.size(), IsTrue());
			std::vector<int> elems = init;

			while(!tmp.empty()) {
				int value = tmp.top();
				auto it = std::find(elems.begin(), elems.end(), value);

				AssertThat(it != elems.end(), IsTrue());

				elems.erase(it);
				tmp.pop();
			}
		});

		it("allows swapping operation", [&]() {
			ogdfPQ.clear();
			AssertThat(ogdfPQ.size(), Equals(0u));

			PQ tmp = { 1, 2, 3 };

			using std::swap;
			swap(tmp, ogdfPQ);
			AssertThat(ogdfPQ.size(), Equals(3u));
			AssertThat(tmp.size(), Equals(0u));
		});

		if (supportsMerge) {
			it("correctly merges and clears another PriorityQueue", [&]() {
				ogdfPQ = init;
				AssertThat(ogdfPQ.size(), Equals(init.size()));

				PQ tmp = { 1, 2, 3 };
				ogdfPQ.merge(tmp);
				AssertThat(ogdfPQ.size(), Equals(init.size() + 3));
				AssertThat(tmp.size(), Equals(0u));

				tmp = { 1, 2, 3 };
				PQ orig = init;
				while (!tmp.empty() && !orig.empty()) {
					AssertThat(ogdfPQ.empty(), IsFalse());
					int val = ogdfPQ.top();
					AssertThat((val == orig.top() || val == tmp.top()), IsTrue());
					if (val == orig.top()) {
						orig.pop();
					}
					else { // val == tmp.top()
						tmp.pop();
					}
					ogdfPQ.pop();
				}
			});
		}
	});
}

void radixHeapSortingTest(std::size_t n)
{
	std::string desc = "sorting test on " + std::to_string(n) + " rands";
	describe(desc.data(), [&]() {
		using RadixHeapType = RadixHeap<std::string, std::size_t>;
		std::unique_ptr<RadixHeapType> heap;

		before_each([&](){
			std::default_random_engine rng(n);
			std::uniform_int_distribution<std::size_t> size_dist(1, 100);
			std::uniform_int_distribution<char> char_dist('a', 'z');

			heap.reset(new RadixHeapType());

			for(std::size_t i = 0; i < n; i++) {
				std::string str(size_dist(rng), char_dist(rng));
				heap->push(str, str.length());
			}
		});

		it("has correct size after insertions", [&]() {
			AssertThat(heap->size(), Equals(n));
		});

		it("correctly sorts inserted values", [&]() {
			std::size_t last = 0;
			while(!heap->empty()) {
				std::string str = heap->pop();
				AssertThat(str.size(), Is().Not().LessThan(last));
				last = str.size();
			}
		});
	});
}

template<template<typename T, class C> class H>
void hotQueueSimpleScenario(std::size_t levels, bool supportsDecrease)
{
	using Queue = HotQueue<std::string, int, H>;

	it("creates empty queue", [&]() {
		Queue queue(100, levels);
		AssertThat(queue.empty(), IsTrue());
	});

	it("inserts elements", [&]() {
		Queue queue(100, levels);
		queue.push("abc", 10);
		queue.push("def", 31);
		queue.push("ghi", 15);
		queue.push("xyz", 12);
		queue.push("ror", 22);

		AssertThat(queue.size(), Equals(5u));
	});

	if (supportsDecrease) {
		it("pops elements in the right order and decreases keys", [&]() {
			Queue queue(100, levels);
			queue.push("abc", 10);
			queue.push("def", 31);
			queue.push("ghi", 15);
			queue.push("xyz", 12);
			queue.push("ror", 22);

			AssertThat(queue.top(), Equals("abc"));
			queue.pop();
			AssertThat(queue.top(), Equals("xyz"));
			queue.pop();

			queue.push("uvw", 17);
			AssertThat(queue.top(), Equals("ghi"));
			queue.pop();
			AssertThat(queue.top(), Equals("uvw"));
			queue.pop();

			auto handle = queue.push("poiuyt", 35);
			queue.decrease(handle, 28);
			queue.push("qwerty", 32);

			AssertThat(queue.top(), Equals("ror"));
			queue.pop();
			AssertThat(queue.top(), Equals("poiuyt"));
			queue.pop();
			AssertThat(queue.top(), Equals("def"));
			queue.pop();
			AssertThat(queue.top(), Equals("qwerty"));
			queue.pop();

			AssertThat(queue.empty(), IsTrue());
		});
	}
}

template<template<typename T, class C> class H>
void hotQueueSimpleTest(std::size_t levels, bool supportsDecrease)
{
	std::string desc =
			"simple scenario test using " + std::to_string(levels) + " levels";
	describe(desc.data(), [&]() {
		/*
		 * clang parser crashes from too many lambdas or something, so this
		 * additional function is needed to make it not crash. When the bug
		 * will be fixed, it may be copy-n-pasted here again.
		 */
		hotQueueSimpleScenario<H>(levels, supportsDecrease);
	});
}

template<template<typename T, class C> class H>
void dijkstraTest(int n)
{
	std::string title =
			"yields the same result as the PairingHeap for Dijkstra on a graph with " + to_string(n) + " nodes";
	it(title.data(), [&]() {
		Graph graph;
		randomBiconnectedGraph(graph, n, randomNumber(n, n*(n-1)/2));
		EdgeArray<int> costs(graph);

		for(edge e : graph.edges) {
			costs[e] = randomNumber(1, n);
		}

		Dijkstra<int, PairingHeap> dijkstra;
		Dijkstra<int, H> dijkstraCustom;
		node source = graph.chooseNode();
		NodeArray<edge> preds(graph);
		NodeArray<int> distances(graph);
		NodeArray<int> distancesCustom(graph);

		dijkstra.call(graph, costs, source, preds, distances);
		dijkstraCustom.call(graph, costs, source, preds, distancesCustom);

		for(node v : graph.nodes) {
			AssertThat(distancesCustom[v], Equals(distances[v]));
		}
	});
}

template<template<typename T, class C> class H>
void describeHeapBasic(bool supportsDecrease, bool supportsMerge) {
	simpleScenarioTest<H>(supportsDecrease, supportsMerge);
	destructorTest<H>();
	sortingComparerTest<H>(100);
	sortingRandomTest<H>(100);
	sortingRandomTest<H>(10000);
	sortingRandomTest<H>(1000000);
	if(supportsMerge) {
		mergingRandomTest<H>(100);
		mergingRandomTest<H>(10000);
		mergingRandomTest<H>(1000000);
	}
	if (supportsDecrease) {
		priorityQueueWrapperTest<H>(100, supportsMerge);
		priorityQueueWrapperTest<H>(10000, supportsMerge);
		//priorityQueueWrapperTest<H>(1000000, supportsMerge);
		prioritizedQueueWrapperTest<H>(10);
		prioritizedQueueWrapperTest<H>(100);
		prioritizedQueueWrapperTest<H>(10000);
		dijkstraTest<H>(10);
		dijkstraTest<H>(100);
		dijkstraTest<H>(1000);
	}
}

template<template<typename T, class C> class H>
void describeHeap(const char *title, bool supportsDecrease = true,  bool supportsMerge = true) {
	describe(title, [&](){
		describeHeapBasic<H>(supportsDecrease, supportsMerge);

		describe("Heap-on-Top queue", [&]() {
			hotQueueSimpleTest<H>(3, supportsDecrease);
			hotQueueSimpleTest<H>(5, supportsDecrease);
			hotQueueSimpleTest<H>(7, supportsDecrease);
			hotQueueSimpleTest<H>(11, supportsDecrease);
		});
	});
}

go_bandit([]() {
	describe("Heaps", [](){
		describeHeap<BinaryHeap>("Binary heap", true, false);
		describeHeap<PairingHeap>("Pairing heap");
		describeHeap<BinomialHeap>("Binomial heap", false);
		describeHeap<FibonacciHeap>("Fibonacci heap");
		describeHeap<RMHeap>("Randomized mergable heap");

		describe("Radix heap", [](){
			radixHeapSortingTest(1000);
			radixHeapSortingTest(10000);
			radixHeapSortingTest(100000);
		});
	});
});
