/** \file
 * \brief Implementation of filesystem functions
 *
 * \author Carsten Gutwenger
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

#include <ogdf/basic/filesystem.h>

#ifdef OGDF_SYSTEM_WINDOWS
# define WIN32_EXTRA_LEAN
# define WIN32_LEAN_AND_MEAN
# undef NOMINMAX
# define NOMINMAX
# include <windows.h>
# include <direct.h>
//# if defined(_MSC_VER) && defined(UNICODE)
# if defined(UNICODE)
#  undef GetFileAttributes
#  undef FindFirstFile
#  undef FindNextFile
#  define GetFileAttributes  GetFileAttributesA
#  define FindFirstFile  FindFirstFileA
#  define WIN32_FIND_DATA WIN32_FIND_DATAA
#  define FindNextFile  FindNextFileA
# endif
#endif
#ifdef __BORLANDC__
# define _chdir chdir
#endif
#ifdef OGDF_SYSTEM_UNIX
# include <unistd.h>
# include <dirent.h>
# include <sys/stat.h>
# include <fnmatch.h>
#endif

namespace ogdf {

#ifdef OGDF_SYSTEM_WINDOWS

bool isFile(const char *fileName)
{
	DWORD att = GetFileAttributes(fileName);

	if (att == 0xffffffff) return false;
	return (att & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool isDirectory(const char *fileName)
{
	DWORD att = GetFileAttributes(fileName);

	if (att == 0xffffffff) return false;
	return (att & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool changeDir(const char *dirName)
{
	return (_chdir(dirName) == 0);
}

void getEntriesAppend(const char *dirName,
		FileType t,
		List<string> &entries,
		const char *pattern)
{
	OGDF_ASSERT(isDirectory(dirName));

	string filePattern = string(dirName) + "\\" + pattern;

	WIN32_FIND_DATA findData;
	HANDLE handle = FindFirstFile(filePattern.c_str(), &findData);

	if (handle != INVALID_HANDLE_VALUE)
	{
		do {
			DWORD isDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
			if(isDir && (
				strcmp(findData.cFileName,".") == 0 ||
				strcmp(findData.cFileName,"..") == 0)
			)
				continue;

			if (t == FileType::Entry || (t == FileType::File && !isDir) ||
				(t == FileType::Directory && isDir))
			{
				entries.pushBack(findData.cFileName);
			}
		} while(FindNextFile(handle, &findData));

		FindClose(handle);
	}
}
#endif

#ifdef OGDF_SYSTEM_UNIX

bool isDirectory(const char *fname)
{
	struct stat stat_buf;
	return stat(fname, &stat_buf) == 0 && (stat_buf.st_mode & S_IFMT) == S_IFDIR;
}

bool isFile(const char *fname)
{
	struct stat stat_buf;
	return stat(fname, &stat_buf) == 0 && (stat_buf.st_mode & S_IFMT) == S_IFREG;
}

bool changeDir(const char *dirName)
{
	return (chdir(dirName) == 0);
}

void getEntriesAppend(const char *dirName,
	FileType t,
	List<string> &entries,
	const char *pattern)
{
	OGDF_ASSERT(isDirectory(dirName));

	DIR* dir_p = opendir(dirName);

	dirent* dir_e;
	while ( (dir_e = readdir(dir_p)) != nullptr )
	{
		const char *fname = dir_e->d_name;
		if (pattern != nullptr && fnmatch(pattern,fname,0)) continue;

		string fullName = string(dirName) + "/" + fname;

		bool isDir = isDirectory(fullName.c_str());
		if(isDir && (
			strcmp(fname,".") == 0 ||
			strcmp(fname,"..") == 0)
			)
			continue;

		if (t == FileType::Entry || (t == FileType::File && !isDir) ||
			(t == FileType::Directory && isDir))
		{
			entries.pushBack(fname);
		}
	}

	closedir(dir_p);
}
#endif

void getEntries(const char *dirName,
		FileType t,
		List<string> &entries,
		const char *pattern)
{
	entries.clear();
	getEntriesAppend(dirName, t, entries, pattern);
}

void getFiles(const char *dirName,
	List<string> &files,
	const char *pattern)
{
	getEntries(dirName, FileType::File, files, pattern);
}

void getSubdirs(const char *dirName,
	List<string> &subdirs,
	const char *pattern)
{
	getEntries(dirName, FileType::Directory, subdirs, pattern);
}

void getEntries(const char *dirName,
	List<string> &entries,
	const char *pattern)
{
	getEntries(dirName, FileType::Entry, entries, pattern);
}

void getFilesAppend(const char *dirName,
	List<string> &files,
	const char *pattern)
{
	getEntriesAppend(dirName, FileType::File, files, pattern);
}

void getSubdirsAppend(const char *dirName,
	List<string> &subdirs,
	const char *pattern)
{
	getEntriesAppend(dirName, FileType::Directory, subdirs, pattern);
}

void getEntriesAppend(const char *dirName,
	List<string> &entries,
	const char *pattern)
{
	getEntriesAppend(dirName, FileType::Entry, entries, pattern);
}


}
