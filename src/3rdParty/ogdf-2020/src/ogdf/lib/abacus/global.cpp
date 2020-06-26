/*!\file
* \author Matthias Elf
*
* \par License:
* This file is part of ABACUS - A Branch And CUt System
* Copyright (C) 1995 - 2003
* University of Cologne, Germany
*
* \par
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* \par
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* \par
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
* \see http://www.gnu.org/copyleft/gpl.html
*/

#include <ogdf/lib/abacus/global.h>
#include <sstream>
#include <cstring>

namespace abacus {


std::ostream &operator<<(std::ostream &out, const AbacusGlobal &rhs)
{
	out << "zero tolerance:         " << rhs.eps_ << std::endl;
	out << "machine zero tolerance: " << rhs.machineEps_ << std::endl;
	out << "infinity:               " << rhs.infinity_ << std::endl;
	return out;
}


bool AbacusGlobal::isInteger(double x, double eps) const
{
	double frac = fracPart(x);

	if ((frac > eps) && (frac < 1.0 - eps))
		return false;
	else
		return true;
}



void AbacusGlobal::insertParameter(const char *name, const char *value)
{
	if(!name||!value) {
		Logger::ifout() << "AbacusGlobal:insertParameter(): both arguments must\nbe non-zero pointers\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Global);
	}
	string stName(name);
	string stValue(value);
	paramTable_.overWrite(stName, stValue);
}


void AbacusGlobal::readParameters(const string &fileName)
{
	string line;
	string name;
	string value;

	// open the parameter file \a fileName
	std::ifstream paramFile(fileName, std::ios::in); // XXX: removed ios::nocreate

	if (!paramFile) {
		Logger::ifout() << "AbacusGlobal::readParameters(): opening file " << fileName << " failed\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Global);
	}
	// read the parameter file line by line
	/* Lines in a parameter file starting with \a '#' are comments and hence they
	*   are skipped. Every other non-void line must contain two strings. The
	*   first one is the name of the parameter, the second one its value.
	*/
	std::stringstream is;
	while( std::getline(paramFile, line) ) {
		if (line.empty() || line[0] == '#')
			continue;

		is.str(line);
		is.clear();

		if( !(is >> name) )
			continue; // empty line

		if( !(is >> value) ) {
			Logger::ifout() << "AbacusGlobal::readParameters " << fileName << " value missing for parameter " << name << "\n";
			OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Global);
		}
		else {
			paramTable_.overWrite(name, value);
		}
	}
}


void AbacusGlobal::assignParameter(
	int &param,
	const char *name,
	int minVal,
	int maxVal) const
{
	if(getParameter(name, param)){
		Logger::ifout() << "AbacusGlobal::assignParameter(): parameter " << name << " not found in parameter table\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Global);
	}
	if(param < minVal || param > maxVal) {
		Logger::ifout() << "AbacusGlobal::assignParameter(): parameter " << name << " is out of range.\nvalue: " << param << "\nfeasible range: " << minVal << " ... " << maxVal << "\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Global);
	}
}


void AbacusGlobal::assignParameter(
	unsigned &param,
	const char *name,
	unsigned minVal,
	unsigned maxVal) const
{
	if(getParameter(name, param)){
		Logger::ifout() << "AbacusGlobal::assignParameter(): parameter " << name << " not found in parameter table.\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Global);
	}
	if(param < minVal || param > maxVal) {
		Logger::ifout() << "AbacusGlobal::assignParameter(): parameter " << name << " is out of range.\nvalue: " << param << "\nfeasible range: " << minVal << " ... " << maxVal << "\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Global);
	}
}


void AbacusGlobal::assignParameter(
	double &param,
	const char *name,
	double minVal,
	double maxVal) const
{
	if(getParameter(name, param)){
		Logger::ifout() << "AbacusGlobal::assignParameter(): parameter " << name << " not found in parameter table.\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Global);
	}
	if(param < minVal || param > maxVal) {
		Logger::ifout() << "AbacusGlobal::assignParameter(): parameter " << name << " is out of range.\nvalue: " << param << "\nfeasible range: " << minVal << " ... " << maxVal << "\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Global);

	}
}


void AbacusGlobal::assignParameter(bool &param, const char *name) const
{
	if(getParameter(name, param)){
		Logger::ifout() << "AbacusGlobal::assignParameter(): parameter " << name << " not found in parameter table.\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Global);
	}
}


void AbacusGlobal::assignParameter(
	string &param,
	const char *name,
	unsigned nFeasible,
	const char *feasible[]) const
{
	if(getParameter(name, param)){
		Logger::ifout() << "AbacusGlobal::assignParameter(): parameter " << name << " not found in parameter table.\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Global);
	}

	if(nFeasible){
		unsigned i;
		for(i = 0; i < nFeasible; i++) {
			string stFeas(feasible[i]);
			if(param == stFeas)
				break;
		}

		if(i == nFeasible) {
			Logger::ifout() << "AbacusGlobal::assignParameter(): parameter " << name << " is not feasible!\n"
			 << "value of parameter: " << param << "\n"
			 << "fesible Values are:";
			for(i = 0; i < nFeasible; i++)
				Logger::ifout() << " " << feasible[i];
			Logger::ifout() << "\n";
			OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Global);
		}
	}
}


void AbacusGlobal::assignParameter(char &param, const char *name, const char *feasible) const
{
	if(getParameter(name, param)){
		Logger::ifout() << "AbacusGlobal::assignParameter(): parameter " << name << " not found in parameter table.\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Global);
	}
	if(feasible){
		string stFeasible(feasible);
		string::size_type len = stFeasible.size(), i;
		for(i = 0; i < len; i++)
			if(stFeasible[i] == param)
				break;
		if(feasible && i == len) {
			Logger::ifout() << "AbacusGlobal::assignParameter(): parameter " << name << " is not feasible.\nvalue: " << param << "\nfeasible settings: " << feasible << "\n";
			OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Global);
		}
	}
}


void AbacusGlobal::assignParameter(
	int &param,
	const char *name,
	int minVal,
	int maxVal,
	int defVal) const
{
	if(getParameter(name, param)){
		param = defVal;
	}
	if(param < minVal || param > maxVal) {
		Logger::ifout() << "AbacusGlobal::assignParameter(): parameter " << name << " is out of range.\nvalue: " << param << "\nfeasible range: " << minVal << " ... " << maxVal << "\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Global);
	}
}


void AbacusGlobal::assignParameter(
	unsigned &param,
	const char *name,
	unsigned minVal,
	unsigned maxVal,
	unsigned defVal) const
{
	if(getParameter(name, param)){
		param = defVal;
	}
	if(param < minVal || param > maxVal) {
		Logger::ifout() << "AbacusGlobal::assignParameter(): parameter " << name << " is out of range.\nvalue: " << param << "\nfeasible range: " << minVal << " ... " << maxVal << "\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Global);
	}
}


void AbacusGlobal::assignParameter(
	double &param,
	const char *name,
	double minVal,
	double maxVal,
	double defVal) const
{
	if(getParameter(name, param)){
		param = defVal;
	}
	if(param < minVal || param > maxVal) {
		Logger::ifout() << "AbacusGlobal::assignParameter(): parameter " << name << " is out of range.\nvalue: " << param << "\nfeasible range: " << minVal << " ... " << maxVal << "\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Global);
	}
}


void AbacusGlobal::assignParameter(bool &param, const char *name, bool defVal) const
{
	if(getParameter(name, param)){
		param = defVal;
	}
}


void AbacusGlobal::assignParameter(
	string &param,
	const char *name,
	unsigned nFeasible,
	const char *feasible[],
	const char *defVal) const
{
	if(getParameter(name, param)) {
		param = defVal;
	}
	if(nFeasible){
		unsigned i;
		for(i = 0; i < nFeasible; i++) {
			string stFeas(feasible[i]);
			if(param == stFeas)
				break;
		}
		if(i == nFeasible) {
			Logger::ifout() << "AbacusGlobal::assignParameter(): parameter " << name << " is not feasible!\n"
			 << "value of parameter: " << param << "\n"
			 << "fesible Values are:";
			for(i = 0; i < nFeasible; i++)
				Logger::ifout() << " " << feasible[i];
			Logger::ifout() << "\n";
			OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Global);
		}
	}
}


void AbacusGlobal::assignParameter(
	char &param,
	const char *name,
	const char* feasible,
	char defVal) const
{
	if(getParameter(name, param)){
		param = defVal;
	}
	if(feasible){
		string stFeasible(feasible);
		string::size_type len = stFeasible.size(), i;
		for(i = 0; i < len; i++)
			if(stFeasible[i] == param)
				break;
		if(i == len) {
			Logger::ifout() << "AbacusGlobal::assignParameter(): parameter " << name << " is not feasible.\nvalue: " << param << "\nfeasible settings: " << feasible << "\n";
			OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Global);
		}
	}
}


int AbacusGlobal::findParameter(const char *name, unsigned nFeasible, const int *feasible) const
{
	unsigned i;
	int param;
	assignParameter(param, name, std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
	for(i = 0; i < nFeasible; i++)
		if(feasible[i] == param)
			break;
	if(i == nFeasible) {
		Logger::ifout() << "AbacusGlobal::assignParameter(): parameter " << name << " is not feasible!\n"
		 << "value of parameter: " << param << "\n"
		 << "fesible Values are:";
		for(i = 0; i < nFeasible; i++)
			Logger::ifout() << " " << feasible[i];
		Logger::ifout() << "\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Global);
	}
	return i;
}


int AbacusGlobal::findParameter(const char *name, unsigned nFeasible, const char *feasible[]) const
{
	unsigned i;
	string param;
	assignParameter(param,name);
	for(i = 0; i < nFeasible; i++) {
		string stFeas(feasible[i]);
		if(stFeas == param)
			break;
	}
	if(i == nFeasible) {
		Logger::ifout() << "AbacusGlobal::assignParameter(): parameter " << name << " is not feasible!\n"
		 << "value of parameter: " << param << "\n"
		 << "fesible Values are:";
		for(i = 0; i < nFeasible; i++)
			Logger::ifout() << " " << feasible[i];
		Logger::ifout() << "\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Global);
	}
	return i;
}


int AbacusGlobal::findParameter(const char *name, const char *feasible) const
{
	if(!feasible){
		Logger::ifout() << "AbacusGlobal::findParameter(const char*, const char*): second argument must be non-zero.\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Global);
	}
	char param;
	assignParameter(param,name);
	int len = (int)strlen(feasible), i;
	for(i = 0; i < len; i++) {
		if(feasible[i]==param)
			break;
	}
	if(i == len) {
		Logger::ifout() << "AbacusGlobal::assignParameter(): parameter " << name << " is not feasible.\nvalue: " << param << "\nfeasible settings: " << feasible << "\n";
		OGDF_THROW_PARAM(AlgorithmFailureException, ogdf::AlgorithmFailureCode::Global);
	}
	return i;
}


int AbacusGlobal::getParameter(const char *name, int &parameter) const
{
	const string  nameString(name);

	const string *s = paramTable_.find(nameString);
	if  (s == nullptr)
		return 1;
	else {
		//parameter = s->ascii2int();
		parameter = std::stoi(*s);
		return 0;
	}
}


int AbacusGlobal::getParameter(const char *name, unsigned int &parameter) const
{
	const string  nameString(name);

	const string *s = paramTable_.find(nameString);
	if  (s == nullptr)
		return 1;
	else {
		//parameter = s->ascii2unsignedint();
		parameter = (unsigned int)std::stoul(*s);
		return 0;
	}
}


int AbacusGlobal::getParameter(const char *name, double &parameter) const
{
	const string  nameString(name);

	const string *s = paramTable_.find(nameString);
	if  (s == nullptr)
		return 1;
	else {
		//parameter = s->ascii2double();
		parameter = std::stod(*s);
		return 0;
	}
}


int AbacusGlobal::getParameter(const char *name, string &parameter) const
{
	const string  nameString(name);

	const string *s = paramTable_.find(nameString);
	if  (s == nullptr)
		return 1;
	else {
		parameter = *s;
		return 0;
	}
}


int AbacusGlobal::getParameter(const char *name, bool &parameter) const
{
	const string  nameString(name);

	const string *s = paramTable_.find(nameString);
	if  (s == nullptr)
		return 1;
	else {
		parameter = ascii2bool(*s);
		return 0;
	}
}


int AbacusGlobal::getParameter(const char *name, char &parameter) const
{
	const string  nameString(name);

	const string *s = paramTable_.find(nameString);
	if  (s == nullptr)
		return 1;
	else {
		parameter = (s->size() > 0) ? (*s)[0] : '\0';
		return 0;
	}
}
}
