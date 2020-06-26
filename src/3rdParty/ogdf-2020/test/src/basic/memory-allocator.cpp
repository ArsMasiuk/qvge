/** \file
 * \brief Tests for ogdf::PoolMemoryAllocator, ogdf::MallocMemoryAllocator and the respective macros.
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

#include <cmath>
#include <iostream>
#include <ogdf/basic/memory.h>
#include <ogdf/basic/System.h>
#include <testing.h>

template<size_t size>
class OGDFObject {
	char x[size];

	OGDF_NEW_DELETE
};

template<size_t size>
class MallocObject {
	char x[size];

	OGDF_MALLOC_NEW_DELETE
};

template<template<size_t> class ObjectOfSize>
void describeMemoryManager(const std::string &name) {
	describe(name + " allocator", [] {
		after_each([] {
			size_t managerAllocated = System::memoryAllocatedByMemoryManager();
			size_t mallocAllocated = System::memoryAllocatedByMalloc();
			AssertThat(managerAllocated, IsLessThanOrEqualTo(mallocAllocated));
		});

		it("allocates objects that need exactly 1 byte", [] {
			delete new ObjectOfSize<1>;
		});

		it("does not deallocate nullptr", [] {
			ObjectOfSize<1> *ptr = nullptr;
			delete ptr;
		});
	});
}

go_bandit([] {
	describeMemoryManager<OGDFObject>("OGDF");
	describeMemoryManager<MallocObject>("Malloc");
});
