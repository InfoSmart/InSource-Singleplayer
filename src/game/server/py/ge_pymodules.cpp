///////////// Copyright © 2009 LodleNet. All rights reserved. /////////////
//
//   Project     : Server
//   File        : ge_pymodules.cpp
//   Description :
//      [TODO: Write the purpose of ge_pymodules.cpp.]
//
//   Created On: 9/1/2009 10:19:52 PM
//   Created By: Mark Chandler <mailto:mark@moddb.com>
////////////////////////////////////////////////////////////////////////////

#include "cbase.h"

#include "baseentity.h"
#include <boost/python.hpp>
namespace bp = boost::python;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define REG( Name )	extern void init##Name(); PyImport_AppendInittab( #Name , & init##Name );	

void pyMsg(const char* msg)
{
	Msg(msg);
}

void pyWarning(const char* msg)
{
	Warning(msg);
}

BOOST_PYTHON_MODULE(HLUtil)
{
	bp::def("Msg", pyMsg);
	bp::def("Warning", pyWarning);
}


extern "C"
{

void RegisterPythonModules()
{
	REG( HLUtil );
}

}