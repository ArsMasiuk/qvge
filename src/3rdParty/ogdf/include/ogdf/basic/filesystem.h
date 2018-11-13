/** \file
 * \brief Declaration of deprecated file system functions
 *
 * \author Stephan Beyer (responsible only for deprecation)
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

#include <ogdf/basic/List.h>

namespace ogdf {

/**
 * @addtogroup file-system
 */
//@{

//! The type of an entry in a directory.
enum class FileType {
	Entry,     /**< file or directory */
	File,      /**< file */
	Directory  /**< directory */
};

//! Returns true iff \p fileName is a regular file (not a directory).
OGDF_DEPRECATED("Please use another library (or C++17) for filesystem functions.")
OGDF_EXPORT bool isFile(const char *fileName);

//! Returns true iff \p fileName is a directory.
OGDF_DEPRECATED("Please use another library (or C++17) for filesystem functions.")
OGDF_EXPORT bool isDirectory(const char *fileName);

//! Changes current directory to \p dirName; returns true if successful.
OGDF_DEPRECATED("Please use another library (or C++17) for filesystem functions.")
OGDF_EXPORT bool changeDir(const char *dirName);

//! Returns in \p files the list of files in directory \p dirName.
/** The optional argument \p pattern can be used to filter files.
 *
 *  \pre \p dirName is a directory
 */
OGDF_DEPRECATED("Please use another library (or C++17) for filesystem functions.")
OGDF_EXPORT void getFiles(const char *dirName,
                          List<string> &files,
                          const char *pattern = "*");

//! Appends to \p files the list of files in directory \p dirName.
/** The optional argument \p pattern can be used to filter files.
 *
 *  \pre \p dirName is a directory
 */
OGDF_DEPRECATED("Please use another library (or C++17) for filesystem functions.")
OGDF_EXPORT void getFilesAppend(const char *dirName,
                                List<string> &files,
                                const char *pattern = "*");

//! Returns in \p subdirs the list of directories contained in directory \p dirName.
/** The optional argument \p pattern can be used to filter files.
 *
 *  \pre p dirName is a directory
 */
OGDF_DEPRECATED("Please use another library (or C++17) for filesystem functions.")
OGDF_EXPORT void getSubdirs(const char *dirName,
                            List<string> &subdirs,
                            const char *pattern = "*");

//! Appends to \p subdirs the list of directories contained in directory \p dirName.
/** The optional argument \p pattern can be used to filter files.
 *
 *  \pre \p dirName is a directory
 */
OGDF_DEPRECATED("Please use another library (or C++17) for filesystem functions.")
OGDF_EXPORT void getSubdirsAppend(const char *dirName,
                                  List<string> &subdirs,
                                  const char *pattern = "*");

//! Returns in \p entries the list of all entries contained in directory \p dirName.
/** Entries may be files or directories. The optional argument \p pattern
 *  can be used to filter files.
 *
 *  \pre \p dirName is a directory
 */
OGDF_DEPRECATED("Please use another library (or C++17) for filesystem functions.")
OGDF_EXPORT void getEntries(const char *dirName,
                            List<string> &entries,
                            const char *pattern = "*");

//! Appends to \p entries the list of all entries contained in directory \p dirName.
/** Entries may be files or directories. The optional argument \p pattern
 *  can be used to filter files.
 *
 *  \pre \p dirName is a directory
 */
OGDF_DEPRECATED("Please use another library (or C++17) for filesystem functions.")
OGDF_EXPORT void getEntriesAppend(const char *dirName,
                                  List<string> &entries,
                                  const char *pattern = "*");

//! Returns in \p entries the list of all entries of type \p t contained in directory \p dirName.
/** The optional argument \p pattern can be used to filter files.
 *
 *  \pre \p dirName is a directory
 */
OGDF_DEPRECATED("Please use another library (or C++17) for filesystem functions.")
OGDF_EXPORT void getEntries(const char *dirName,
                            FileType t,
                            List<string> &entries,
                            const char *pattern = "*");

//! Appends to \p entries the list of all entries of type \p t contained in directory \p dirName.
/** The optional argument \p pattern can be used to filter files.
 *
 *  \pre \p dirName is a directory
 */
OGDF_DEPRECATED("Please use another library (or C++17) for filesystem functions.")
OGDF_EXPORT void getEntriesAppend(const char *dirName,
                                  FileType t,
                                  List<string> &entries,
                                  const char *pattern = "*");

//@}

}
