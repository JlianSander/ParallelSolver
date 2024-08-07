#ifndef PARSER_ICCMA
#define PARSER_ICCMA

#include <cstdint>
#include <iostream>			
#include <fstream>			
#include <algorithm>
#include <sstream>

#include <omp.h>

#include "AF.h"

extern "C" {
	#include "../util/MemoryWatchDog.h"
}

using namespace std;

/// <summary>
/// This class is responsible for parsing a file into a data type representing an abstract argumentation framework.
/// </summary>
class ParserICCMA {
public:

	/// <summary>
	/// This method parses an abstract argumentation framework from a file, which complies to the ICCMA'23 format, located 
	/// at the specified path.
	/// </summary>
	/// <param name="framework"> Object of the framework to create.</param>
	/// <param name="file">String that describes the location of the file to open.</param>
	static void parse_af(AF &framework, string file);
};

#endif