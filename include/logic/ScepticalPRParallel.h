#ifndef SCEPTICAL_PR_PARALLEL_H
#define SCEPTICAL_PR_PARALLEL_H

#include <iostream>
#include <stdio.h>
#include <stdint.h>

extern "C" {
#include "Actives.h"
#include "AF.h"
#include "Decodings.h"
#include "ListActives.h"
#include "Reduct.h"
#include "ScepticalPRCheck.h"

#include "../util/List.h"
#include "../util/MemoryWatchDog.h"
}

#include "Encodings_cms.h"
#include "Decodings_cms.h"
#include "ExternalSolver.h"
#include "InitialSetSolver.h"
#include "omp.h"

class ScepticalPRParallel {
public:

	/// <summary>
	/// Checks if a specified argument's sceptical acceptance can be rejected, because there exists an initial set in the specified 
	///  framework or any of its reducts, which attacks the argument. This method uses recursive method calls.
	/// </summary>
	/// <param name="argument">The argument, which could be sceptical accepted or not.</param>
	/// <param name="framework">The abstract argumentation framework, specifying the underlying attack relations between the arguments.</param>
	/// <param name="activeArgs">The active arguments of the current state of the framework.</param>
	/// <param name="proof_extension"> Extension proving, that the argument cannot be sceptically accepted.</param>
	/// <param name="numCores"> Number of cores requested to be used to solve the problem. Actual number can be lower depending on the OS scheduler.
	/// <returns>TRUE iff the sceptical acceptance was rejected. FALSE otherwise.</returns>
	static bool check_rejection_parallel(uint32_t argument, argFramework_t *framework, activeArgs_t *activeArgs, nodeUInt32_t **proof_extension, uint8_t numCores);
};

#endif
