/** \file
 * \brief Resource file abstraction to be used in tests. Resources
 * are compiled into binary format and can be accessed with the
 * classes provided by this file.
 *
 * \author Tilo Wiedera, Jöran Schierbaum
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

#pragma once

#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <ogdf/fileformats/GraphIO.h>
#include <testing.h>

namespace resources {

//! Abstract base class for resources
class AbstractResource {
protected:
	string m_path; //!< The relative path in the resource directory
	string m_name; //!< The name of this resource

public:
	AbstractResource(const string& path, const string& name) : m_path(path), m_name(name) { }

	inline const string& path() const { return m_path; }
	inline const string& name() const { return m_name; }
	inline string fullPath() const { return (m_path.empty() ? m_name : m_path + '/' + m_name); }
};

//! A resource file whose contents can be retrieved
/**
 * Use the ResourceFile::get() methods to get const pointers for test cases.
 */
class ResourceFile : public AbstractResource {
protected:
	string m_data; //!< File contents

public:
	ResourceFile() : AbstractResource("", "") { }
	ResourceFile(const string& path, const string& name, const string& data) : AbstractResource(path, name), m_data(data) { }

	inline const string& data() const { return m_data; }

	//! Retrieves the data of a resource with given path
	//! @param file filename with path given relative to original resources folder
	static inline const string& data(const string& file) {
		return get(file)->data();
	}

	//! Retrieves a resource with given path
	/**
	 * @param path file path and name, relative to original resources folder
	 * @return pointer to file if found, throws exception otherwise
	 */
	static const ResourceFile* get(const string& path);
};

//! A resource folder, holding subfolders and files
/**
 * Use the ResourceDirectory::get() methods to get const pointers for test cases.
 */
class ResourceDirectory : public AbstractResource {
protected:
	std::unordered_map<string, ResourceDirectory*> m_directories;
	std::unordered_map<string, ResourceFile*> m_files;

public:
	ResourceDirectory() : AbstractResource("", "") { }
	ResourceDirectory(const string& path, const string& name) : AbstractResource(path, name) { }

	//! Registers a new file in this directory
	void addFile(ResourceFile* file);

	//! Returns a file in this directory
	/**
	 * @param name the filename
	 * @return pointer to file if found, \c nullptr otherwise
	 */
	const ResourceFile* getFile(const string& name) const;

	//! Recursively look for a file
	/**
	 * @param path the file's path relative to this directory
	 * @return pointer to file if found, \c nullptr otherwise
	 */
	const ResourceFile* getFileByPath(const string& path) const;

	//! Registers a new directory as a subdirectory of the current object
	void addDirectory(ResourceDirectory*);

	//! Registers a new directory as a subdirectory of the current object
	ResourceDirectory* addDirectory(const string& name);

	//! Returns a subdirectory
	/**
	 * @param name name of the subdirectory
	 * @return pointer to subdirectory if found, \c nullptr otherwise
	 */
	const ResourceDirectory* getDirectory(const string& name) const;

	//! Returns a subdirectory
	/**
	 * @param name name of the subdirectory
	 * @param create if \c true, create directory if not found
	 * @return pointer to subdirectory if found, \c nullptr otherwise
	 */
	ResourceDirectory* getDirectory(const string& name, bool create = false);

	//! Recursively looks for a directory with the given path
	/**
	 * @param path the directory's path relative to this directory
	 * @return pointer to directory if found, \c nullptr otherwise
	 */
	const ResourceDirectory* getDirectoryByPath(const string& path) const;

	//! Recursively looks for a directory with the given path
	/**
	 * @param path the directory's path relative to this directory
	 * @param createPath whether empty directories should be created when a
	 *        non-existant path gets queried
	 * @return pointer to directory if found, \c nullptr otherwise
	 */
	ResourceDirectory* getDirectoryByPath(const string& path, bool createPath = false);

	//! Returns all files in this directory
	std::vector<const ResourceFile*> getFiles() const;

	//! Returns all subdirectories
	std::vector<const ResourceDirectory*> getDirectories() const;

	//! Iterates over each file contained in this directory.
	/**
	 * \param callback A function that will be called for each file in the directory.
	 * \param recurse Whether to include sub directories.
	 */
	void forEachFile(std::function<void(const ResourceFile*)> callback, bool recurse = false) const;

	//! Retrieves a resource directory with given path
	/**
	 * @param path directory path, relative to original resources folder
	 * @return pointer to directory if found, \c nullptr otherwise
	 */
	static const ResourceDirectory* get(const string& path);

	//! Retrieves a resource directory with given path and name
	/**
	 * @param dir a directory relative to original resources folder
	 * @param name filename of the requested directory
	 * @return pointer to directory if found, \c nullptr otherwise
	 */
	static const ResourceDirectory* get(const string& dir, const string& name);
};

namespace internal {

/* Internal global variables holding the folder structure */
extern std::unordered_map<string, ResourceDirectory> g_directories;
extern std::unordered_map<string, ResourceFile> g_resources;
extern ResourceDirectory g_resources_root;

//! Registers a resource and stores it in global file structure
/**
 * This function is called by the generated resource source file to register all loaded
 * resource files for use in other areas.
 *
 * @note You should not call this manually from tests as it changes the global state of
 * the virtual file tree and thus directly affects other test cases.
 *
 * @param directory the file's path
 * @param filename the file's name
 * @param contents the contents to store
 */
void registerResource(const string& directory, const string& filename, const string& contents);

} // namespace internal

//! Loads the generated resource files into the global data structure
void load_resources();

} // namespace resources

// Export the two important classes for easier access
using resources::ResourceFile;
using resources::ResourceDirectory;

//! Iterates over each file contained in the specified directory.
/**
 * \param directory The path of the directory.
 * \param callback A function that will be called for each file in the directory.
 * \param recurse Whether to include sub directories.
 */
inline void for_each_file(const string &directory, std::function<void(const ResourceFile*)> callback, bool recurse = false) {
	const ResourceDirectory* dir = ResourceDirectory::get(directory);
	if (dir != nullptr) {
		dir->forEachFile(callback, recurse);
	}
}

//! Reads the specified files and creates a test for each graph.
/**
 * \param title The base title for the test cases.
 * \param filenames The names of the files to be read.
 * \param testFunc The actual test to be performed.
 * \param reader The function used to parse the files, defaults to GraphIO::readGML.
 */
inline void for_each_graph_it(const string &title, const std::vector<string> &filenames, std::function<void(Graph&)> testFunc, GraphIO::ReaderFunc reader = GraphIO::readGML) {
	for(const string filename : filenames) {
		bandit::it(title + " [" + filename.c_str() + "]", [&] {
			Graph graph;
			const ResourceFile* file = ResourceFile::get(filename);
			std::stringstream ss{file->data()};
			AssertThat(reader(graph, ss), IsTrue());
			testFunc(graph);
		});
	}
}
