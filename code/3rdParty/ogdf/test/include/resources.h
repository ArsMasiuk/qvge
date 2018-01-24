/** \file
 * \brief Helper functions to be used in tests for accessing resource files.
 *
 * \author Tilo Wiedera
 */

#pragma once

#include <vector>
#include <ogdf/fileformats/GraphIO.h>
#include <tinydir.h>
#include <testing.h>

const string RESOURCE_DIR = "test/resources";

/**
 * Tests whether the resource directory is present (i.e. the working directory is correct).
 *
 * \return true iff the resource directory was found
 */
inline bool resourceCheck() {
	tinydir_dir dir;
	bool result = tinydir_open(&dir, RESOURCE_DIR.c_str())  != -1;
	tinydir_close(&dir);

	return result;
}
/**
 * Iterates over each file contained in the specified directory.
 *
 * \param directory The path of the directory.
 * \param callback A function that will be called for each file in the directory.
 * \param recurse Whether to include sub directories.
 * \return False, if an error occured, True otherwise
 */
inline bool for_each_file(const string &directory, std::function<void (const string&)> callback, bool recurse = false) {
	string resourceDirectory = RESOURCE_DIR + "/" + directory;
	tinydir_dir dir;

	if(tinydir_open(&dir, resourceDirectory.c_str())  == -1) {
		return false;
	} else while (dir.has_next) {
		tinydir_file file;

		if (tinydir_readfile(&dir, &file) == -1) {
			return false;
		}

		if (!file.is_dir) {
			callback(resourceDirectory + "/" + file.name);
		} else if (recurse && strcmp(file.name, ".") && strcmp(file.name, "..")) {
			for_each_file(directory + "/" + file.name, callback, true);
		}

		tinydir_next(&dir);
	}

	tinydir_close(&dir);
	return true;
}

/**
 * Reads the specified files and creates a test for each graph.
 *
 * \param title The base title for the test cases.
 * \param callback filenames The names of the files to be read.
 * \param recurse testFunc The actual test to be performed.
 * \param reader The function used to parse the files, defaults to GraphIO::readGML.
 */
inline void for_each_graph_it(const string &title, const std::vector<string> &filenames, std::function<void (Graph &graph, const string&)> testFunc, GraphIO::ReaderFunc reader = GraphIO::readGML) {
	for(const string filename : filenames) {
		bandit::it(string(title + " [" + filename.c_str() + "] "), [&](){
			Graph graph;
			AssertThat(GraphIO::read(graph, (RESOURCE_DIR + "/" + filename).c_str(), reader), snowhouse::IsTrue());
			testFunc(graph, filename);
		});
	}
}
