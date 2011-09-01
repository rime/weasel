// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <string>

#pragma warning(disable : 4819)

#include <boost/interprocess/streams/bufferstream.hpp>
#include <boost/python.hpp>

#pragma warning(default : 4819)

using namespace std;
using boost::interprocess::wbufferstream;
namespace python = boost::python;
