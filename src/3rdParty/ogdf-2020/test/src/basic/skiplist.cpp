/** \file
 * \brief Tests for ogdf::Skiplist and skiplist-based ogdf::SortedSequence.
 *
 * \author Tilo Wiedera
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

#include <ogdf/basic/geometry.h>
#include <ogdf/basic/Math.h>
#include <ogdf/basic/Skiplist.h>
#include <ogdf/basic/SortedSequence.h>

#include <testing.h>

constexpr int MAX_ELEMENTS = 100;

template<typename T>
void describeSkiplist(const string& typeName, std::function<T()> randomValue) {
	describe("Skiplist<" + typeName + ">", [&] {
		Skiplist<T*> list;

		after_each([&] {
			list.clear(true);
		});

		it("recognizes empty lists", [&] {
			AssertThat(list.empty(), IsTrue());

			list.add(new T);
			AssertThat(list.empty(), IsFalse());

			list.clear(true);
			AssertThat(list.empty(), IsTrue());
		});

		it("returns its size", [&] {
			for (int i = 0; i < MAX_ELEMENTS; i++) {
				AssertThat(list.size(), Equals(i));
				list.add(new T(randomValue()));
			}
		});

		it("sorts inserted values", [&] {
			std::map<T,int> counter;

			for (int i = 0; i < MAX_ELEMENTS; i++) {
				T j = randomValue();

				if(counter.find(j) == counter.end()) {
					counter[j] = 1;
				} else {
					counter[j]++;
				}

				list.add(new T(j));
			}

			T prev = std::numeric_limits<T>::lowest();
			for(T* p : list) {
				T j = *p;
				counter[j]--;

				if(counter[j] == 0) {
					counter.erase(j);
				}

				if(j != prev) {
					AssertThat(prev, IsLessThan(j));
					prev = j;
				}
			}

			AssertThat(counter.empty(), IsTrue());
		});

		it("works with many duplicate values", [&] {
			T small = randomValue();
			T big;
			do {
				big = randomValue();
			} while (big == small);

			if(big < small) {
				std::swap(big, small);
			}

			for(int i = 0; i < MAX_ELEMENTS; i++) {
				list.add(&big);
			}

			list.add(&small);
			list.add(&big);
			list.add(&small);

			AssertThat(list.size(), Equals(MAX_ELEMENTS+3));

			int counter = 0;
			for(auto p : list) {
				counter++;

				if(counter < 3) {
					AssertThat(p, Equals(&small));
				} else {
					AssertThat(p, Equals(&big));
				}
			}

			AssertThat(counter, Equals(list.size()));

			list.clear();
		});
	});
}

template<typename T>
void describeSortedSequence(const string& typeName) {
	class MyInfoObject {
	public:
		T x;
		MyInfoObject(T p) : x(p) {}
		MyInfoObject() : MyInfoObject(0) {}
	};

	SortedSequence<int, MyInfoObject> sequence;

	auto toInfo = [](T i) {
		i = std::abs(i);
		return (i+1)*(i+2);
	};

	auto insert = [&](T i) {
		MyInfoObject info = MyInfoObject(toInfo(i));
		sequence.insert(i, info);
	};

	List<T> perm;

	for(T i = 0; i < MAX_ELEMENTS; i++) {
		perm.pushBack(i);
	}

	describe("SortedSequence<" + typeName + ">", [&] {
		before_each([&] {
			perm.permute();
		});

		after_each([&] {
			sequence.clear();
		});

		it("recognizes empty sequences", [&] {
			AssertThat(sequence.empty(), IsTrue());

			insert(1);
			AssertThat(sequence.empty(), IsFalse());

			sequence.clear();
			AssertThat(sequence.empty(), IsTrue());
		});

		it("returns its size", [&] {
			int counter = 0;

			for(T i : perm) {
				AssertThat(sequence.size(), Equals(counter));
				insert(i);
				counter++;
			}
		});

		it("returns its info object", [&] {
			for (T i : perm) {
				insert(i);
			}

			for (auto it = sequence.begin(); it.valid(); it++) {
				AssertThat(it.info().x, Equals(toInfo(it.key())));
			}
		});

		it("sorts inserted values", [&] {
			for(T i : perm) {
				AssertThat(sequence.lookup(i).valid(), IsFalse());
				insert(i);
				AssertThat(sequence.lookup(i).valid(), IsTrue());
			}

			AssertThat(sequence.size(), Equals(perm.size()));

			T prev{};
			for (auto it = sequence.begin(); it.valid(); it++) {
				T k = it.key();

				AssertThat(sequence.lookup(k).valid(), IsTrue());
				if(it != sequence.begin()) {
					AssertThat(k, IsGreaterThan(prev));
				}

				prev = k;
			}
		});

		it("deletes values", [&] {
			insert(42);
			insert(17);
			sequence.del(42);
			insert(2017);

			AssertThat(sequence.lookup(42).valid(), IsFalse());
			AssertThat(sequence.size(), Equals(2));

			sequence.delItem(sequence.lookup(2017));

			AssertThat(sequence.lookup(2017).valid(), IsFalse());
			AssertThat(sequence.size(), Equals(1));
		});

		it("identifies min and max values", [&] {
			AssertThat(sequence.minItem().valid(), IsFalse());
			AssertThat(sequence.maxItem().valid(), IsFalse());

			for(int i : perm) {
				insert(i);
			}

			AssertThat(sequence.minItem().key(), Equals(0));
			AssertThat(sequence.maxItem().key(), Equals(MAX_ELEMENTS-1));
		});

		it("locates the smallest feasible value", [&] {
			AssertThat(sequence.locate(0).valid(), IsFalse());

			int max = 0;
			bool firstIteration = true;

			for(int i : perm) {
				Math::updateMax(max, i);
				insert(i);

				if(!firstIteration) {
					AssertThat(sequence.locate(0).key(), !Equals(max));
				}

				AssertThat(sequence.locate(max).key(), Equals(max));
				AssertThat(sequence.locate(max + 1).valid(), IsFalse());

				firstIteration = false;
			}
		});
	});
}

go_bandit([] {
	auto ranD = [] { return randomDouble(0, MAX_ELEMENTS); };

	describeSkiplist<int>("int", [] { return randomNumber(0, (MAX_ELEMENTS*5)/6); });
	describeSkiplist<double>("double", ranD);
	describeSkiplist<DPoint>("DPoint", [&] { return DPoint(ranD(), ranD()); });

	describeSortedSequence<int>("int");
	describeSortedSequence<double>("double");
});
