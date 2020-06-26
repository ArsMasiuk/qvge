/** \file
 * \brief Small helper program to generate a source file with the
 * contents of all test resources, so that they may be compiled
 * into the test binaries.
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

#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <algorithm>
#include <tinydir.h>

/*
 * These constants define where to read the resources from and
 * where to place the generated source file respectively. These
 * paths need to be relative to where this program is run from.
 */
const std::string RESOURCE_DIR = "test/resources";

std::ofstream gfs;

// VisualStudio does not like string literals over 16380 bytes.
#ifdef _MSC_VER
#define STRING_LITERAL_MAX_LENGTH 16379
#else
#define STRING_LITERAL_MAX_LENGTH 65530
#endif

/**
 * Tests whether the resource directory is present (i.e. the working directory is correct).
 *
 * \return true iff the resource directory was found
 */
inline bool resourceCheck() {
	tinydir_dir dir;
	bool result = (tinydir_open(&dir, RESOURCE_DIR.c_str()) != -1);
	tinydir_close(&dir);

	return result;
}

/**
 * Load a file and write its content into the generated source file
 *
 * @param directory the directory the file is in, relative to execution point
 * @param filename the name of the file in the directory
 */
void load_file(const std::string& directory, const std::string& filename) {
	std::string filepath = directory + '/' + filename;
	std::ifstream fs(filepath.c_str());
	char next;

	// Write file path and name, but strip the common resource folder prefix
	gfs << "  registerResource(\"" << directory.substr(RESOURCE_DIR.length() + 1) << "\", \"" << filename << "\", std::string(\"";
	int count = 0;
	while (fs.get(next)) {
		if (count == STRING_LITERAL_MAX_LENGTH) {
			// Make sure our string literals don't get too big.
			// Apparently concatenation is not a problem for any compiler.
			gfs << "\") + std::string(\"";
			count = 0;
		}
		// Escape special characters to not accidentally close the string early
		switch (next) {
		case '\t': gfs << "\\t"; break;
		case '\r': gfs << "\\r"; break;
		case '\n': gfs << "\\n"; break;
		case '\\': gfs << "\\\\"; break;
		case '"': gfs << "\\\""; break;
		default: gfs << next;
		}
		count++;
	}
	gfs << "\"));\n";
}

/**
 * Recursively load files and write their contents into the generated source
 *
 * @param directory the current directory to look at, relative to execution point
 * @return true on success, false on any error
 */
bool load_files(const std::string& directory) {
	tinydir_dir dir;

	if (tinydir_open(&dir, directory.c_str()) == -1) {
		return false;
	}

	while (dir.has_next) {
		tinydir_file file;

		if (tinydir_readfile(&dir, &file) == -1) {
			return false;
		}

		if (!file.is_dir) {
			load_file(directory, file.name);
		} else if (strcmp(file.name, ".") && strcmp(file.name, "..")) {
			load_files(directory + "/" + file.name);
		}

		tinydir_next(&dir);
	}

	tinydir_close(&dir);
	return true;
}

int main(int argc, char** argv) {
	if (!resourceCheck()) {
		std::cerr << "Could not find the resource directory.\n"
		    << "Make sure you run this program from within the OGDF source directory." << std::endl;
		return 1;
	}

	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <generated source path>\n"
		    << "Make sure you run this program from within the OGDF source directory." << std::endl;
		return 1;
	}

	gfs.open(argv[1]);

	if (gfs.fail()) {
		std::cerr << "Could not open file " << argv[1] << "!" << std::endl;
		return 1;
	}

	gfs << "/* FILE GENERATED AUTOMATICALLY BY " __FILE__ ". DO NOT EDIT. */\n"
		"#include <resources.h>\n"
		"namespace resources {\n"
		"using internal::registerResource;\n"
		"void load_resources() {\n";

	load_files(RESOURCE_DIR);

	gfs << "}\n"
		"}\n";
	gfs.close();

	return 0;
}
