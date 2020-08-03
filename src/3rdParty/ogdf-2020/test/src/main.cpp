/** \file
 * \brief Implementation of a command line based tool to run
 * tests.
 *
 * \author Christoph Schulz, Stephan Beyer, Tilo Wiedera
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

#include <iostream>
#include <resources.h>

int main(int argc, char **argv)
{
	bool verbose = false;
	bool help = false;

	for(int i = 1; i < argc; i++) {
		verbose |= string(argv[i]) == "--ogdf-verbose";
		help |= string(argv[i]) == "--help";
	}

	if(!verbose) {
		Logger::globalLogLevel(Logger::Level::Force);
	}

	resources::load_resources();

	int result = run(argc, argv);

	if(help) {
		std::cout << "OGDF specific options:" << std::endl;
		std::cout << "  --ogdf-verbose\t\tEnable verbose OGDF logging." << std::endl;
	}

	return result;
}
