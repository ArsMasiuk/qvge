/** \file
 * \brief Resource file abstraction to be used in tests. Resources
 * are compiled into binary format and can be accessed with the
 * classes provided by this file.
 *
 * \author Jöran Schierbaum
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

#include <resources.h>

namespace resources {

std::unordered_map<string, ResourceDirectory> internal::g_directories;
std::unordered_map<string, ResourceFile> internal::g_resources;
ResourceDirectory internal::g_resources_root;

/* Class Members of ResourceFile */
const ResourceFile* ResourceFile::get(const string& path) {
	if (internal::g_resources.find(path) == internal::g_resources.end()) {
		throw(std::runtime_error("The file `" + path + "' could not be found in the packed resources.\n"
		  "If it is in your file system, you have to re-run cmake and recompile."));
	}
	return &internal::g_resources[path];
}

/* Class Members of ResourceDirectory */
void ResourceDirectory::addFile(ResourceFile* r) {
	OGDF_ASSERT(r != nullptr);
	m_files[r->name()] = r;
}

void ResourceDirectory::addDirectory(ResourceDirectory* r) {
	OGDF_ASSERT(r != nullptr);
	m_directories[r->name()] = r;
}
ResourceDirectory* ResourceDirectory::addDirectory(const string& name) {
	ResourceDirectory newdir(fullPath(), name);
	// Copy into global array and return address from that
	internal::g_directories[newdir.fullPath()] = newdir;
	ResourceDirectory* p = &internal::g_directories[newdir.fullPath()];
	addDirectory(p);
	return p;
}

const ResourceFile* ResourceDirectory::getFile(const string& name) const {
	auto it = m_files.find(name);
	if (it == m_files.end()) return nullptr;
	return it->second;
}
const ResourceDirectory* ResourceDirectory::getDirectory(const string& name) const {
	auto it = m_directories.find(name);
	if (it == m_directories.end()) return nullptr;
	return it->second;
}
ResourceDirectory* ResourceDirectory::getDirectory(const string& name, bool create) {
	auto it = m_directories.find(name);
	if (it == m_directories.end()) {
		if (create) return addDirectory(name);
		return nullptr;
	}
	return it->second;
}

const ResourceFile* ResourceDirectory::getFileByPath(const string& path) const {
	string::size_type pos = path.find('/');
	if (pos == string::npos) {
		return getFile(path);
	}
	const ResourceDirectory* next = getDirectory(path.substr(0, pos));
	if (next == nullptr) return nullptr; // no such directory
	return next->getFileByPath(path.substr(pos + 1));
}
const ResourceDirectory* ResourceDirectory::getDirectoryByPath(const string& path) const {
	string::size_type pos = path.find('/');
	if (pos == string::npos) {
		return getDirectory(path);
	}
	const ResourceDirectory* next = getDirectory(path.substr(0, pos));
	if (next == nullptr) return next; // no such directory
	return next->getDirectoryByPath(path.substr(pos + 1));
}
ResourceDirectory* ResourceDirectory::getDirectoryByPath(const string& path, bool createPath) {
	string::size_type pos = path.find('/');
	if (pos == string::npos) {
		ResourceDirectory* found = getDirectory(path);
		if (found == nullptr && createPath) {
			found = addDirectory(path);
		}
		return found;
	}
	ResourceDirectory* next = getDirectory(path.substr(0, pos));
	if (next == nullptr) {
		// no such directory
		if (!createPath) {
			return nullptr;
		}
		next = addDirectory(path.substr(0, pos));
	}
	return next->getDirectoryByPath(path.substr(pos + 1), createPath);
}

std::vector<const ResourceFile*> ResourceDirectory::getFiles() const {
	std::vector<const ResourceFile*> entries;
	for (auto it : m_files) {
		entries.push_back(it.second);
	}
	return entries;
}
std::vector<const ResourceDirectory*> ResourceDirectory::getDirectories() const {
	std::vector<const ResourceDirectory*> entries;
	for (auto it : m_directories) {
		entries.push_back(it.second);
	}
	return entries;
}

void ResourceDirectory::forEachFile(std::function<void(const ResourceFile*)> callback, bool recurse) const {
	for (auto it : m_files) {
		callback(it.second);
	}

	if (recurse) {
		for (auto it : m_directories) {
			if (it.second == nullptr) continue;
			it.second->forEachFile(callback, true);
		}
	}
}

const ResourceDirectory* ResourceDirectory::get(const string& dir, const string& name) {
	return get(dir + '/' + name);
}
const ResourceDirectory* ResourceDirectory::get(const string& path) {
	if (internal::g_directories.find(path) == internal::g_directories.end()) return nullptr;
	return &internal::g_directories[path];
}

/* Global methods */
void internal::registerResource(const string& directory, const string& filename, const string& contents) {
	ResourceDirectory* dir = g_resources_root.getDirectoryByPath(directory, true);
	ResourceFile f(directory, filename, contents);
	// Copy into global array, store address of that copy
	g_resources[directory + '/' + filename] = f;
	dir->addFile(&g_resources[directory + '/' + filename]);
}

}
