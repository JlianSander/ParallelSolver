#include "../../include/logic/Solver_DS_PR.h"
using namespace std;

static list<uint32_t> ExtendExtension(list<uint32_t> &extension_build, list<uint32_t> &initial_set)
{
	list<uint32_t> tmpCopy_1, tmpCopy_2;
	std::copy(extension_build.begin(), extension_build.end(), std::back_inserter(tmpCopy_1));
	std::copy(initial_set.begin(), initial_set.end(), std::back_inserter(tmpCopy_2));
	tmpCopy_1.merge(tmpCopy_2);
	return tmpCopy_1;
}

/*===========================================================================================================================================================*/
/*===========================================================================================================================================================*/

static void check_rejection(uint32_t argument, AF &framework, ArrayBitSet &activeArgs, bool &isRejected, bool &isTerminated, bool isMain,
	ExtensionPrioritised state_info, Heuristic1 heuristic, list<uint32_t> &output_extension,
	std::priority_queue<ExtensionPrioritised, std::vector<ExtensionPrioritised>, extPrioLess_t> &extension_priority_queue,
	omp_lock_t *lock_prio_queue, omp_lock_t *lock_has_entry, uint32_t lim_iterations)
{
	int id = omp_get_thread_num();
	bool isTerminated_tmp = false;

#pragma omp flush(isTerminated)
#pragma omp atomic read
	isTerminated_tmp = isTerminated;
	if (isTerminated_tmp)
	{
		return;
	}

	ArrayBitSet reduct = state_info.Extension.empty() ? activeArgs.copy() : Reduct::get_reduct_set(activeArgs, framework, state_info.Extension);

	if (reduct._array.size() < 2)
	{
		// there is only 1 active argument, this has to be the argument to check, 
		// if not then there should have been a rejection check earlier who did not work
		return;
	}

	//flag used to signal that clauses need to be extended
	bool *isSolved = NULL;
	isSolved = (bool *)malloc(sizeof * isSolved);
	*isSolved = false;
	//flag used to signal that reduct has only empty set as admissible set
	bool *isFirstCalculation = NULL;
	isFirstCalculation = (bool *)malloc(sizeof * isFirstCalculation);
	*isFirstCalculation = true;
	uint64_t numVars = reduct._array.size();
	SatSolver *solver = NULL;
	solver = new SatSolver_cadical(numVars);
	Encoding::add_clauses_nonempty_admissible_set(*solver, framework, reduct);
	uint32_t num_iterations = 0;

	if (!state_info.Complement_clauses.empty()) {
		for (int i = 0; i < state_info.Complement_clauses.size(); i++) {
			solver->add_clause(state_info.Complement_clauses[i]);
		}
	}

#pragma omp flush(isTerminated)
#pragma omp atomic read
	isTerminated_tmp = isTerminated;
	if (isTerminated_tmp == true)
	{
		free(isSolved);
		free(isFirstCalculation);
		delete solver;
		return;
	}

	bool has_Solution = true;

	//iterate through initial sets
	do {
		*isSolved = true;
		has_Solution = (*solver).solve();
		if(!isMain) num_iterations++;

		if (!has_Solution)
		{
			//no more initial sets to calculate after this one
			if (*isFirstCalculation)
			{
				// since this is the first calculation and no IS was found, this reduct has only the empty set as an admissible set, 
				// therefore the extension_build is complete and since only extensions not containing the query argument proceed 
				// in the calculation, extension_build cannot contain the query argument, so that there exists
				// an extension not containing the query argument, so that the argument gets sceptical rejected				
#pragma atomic write
				isRejected = true;
#pragma omp flush(isRejected)
#pragma atomic write
				isTerminated = true;
#pragma omp flush(isTerminated)

				free(isSolved);
				free(isFirstCalculation);
				delete solver;
				return;
			}

			break;
		}

		list<uint32_t> initial_set = Decoding::get_set_from_solver(*solver, reduct);		
		if (initial_set.empty())
		{
			if (*isFirstCalculation)
			{
				// since this is the first calculation and only the empty set as IS was found, the extension_build is complete
				// and since only extensions not containing the query argument proceed in the calculation, extension_build 
				// cannot contain the query argument, so that there exists
				// an extension not containing the query argument, so that the argument gets sceptical rejected				
#pragma atomic write
				isRejected = true;
#pragma omp flush(isRejected)
#pragma atomic write
				isTerminated = true;
#pragma omp flush(isTerminated)
				free(isSolved);
				free(isFirstCalculation);
				delete solver;
				initial_set.clear();
				return;
			}
			else
			{
				printf("ERROR impossible that empty IS calculated and is not first calculation");
				exit(1);
			}
		}

		*isFirstCalculation = false;

		if (ScepticalCheck::check_rejection(argument, initial_set, framework))
		{
#pragma atomic write
			isRejected = true;
#pragma omp flush(isRejected)
#pragma atomic write
			isTerminated = true;
#pragma omp flush(isTerminated)

			list<uint32_t> new_extension_build = ExtendExtension(state_info.Extension, initial_set);
#pragma atomic write
			output_extension = new_extension_build;

			free(isSolved);
			free(isFirstCalculation);
			delete solver;
			initial_set.clear();
			return;
		}
		else if (ScepticalCheck::check_terminate_extension_build(argument, initial_set))
		{
			initial_set.clear();
			continue;
		}

		list<uint32_t> new_extension_build = ExtendExtension(state_info.Extension, initial_set);
		initial_set.clear();

		if (has_Solution)
		{
			vector<int64_t> complement_clause = Encoding::add_complement_clause(*solver, reduct);
			solver->add_clause(complement_clause);
			if (!isMain) {
				state_info.Complement_clauses.push_back(complement_clause);
			}
		}
		
		ExtensionPrioritised newEntryQueue = ExtensionPrioritised(framework, new_extension_build, heuristic);

		push_priority_queue(lock_prio_queue, extension_priority_queue, newEntryQueue, lock_has_entry);

#pragma omp flush(isTerminated)
#pragma omp atomic read
		isTerminated_tmp = isTerminated;
	} while (has_Solution && !isTerminated_tmp &&( isMain || num_iterations < lim_iterations));

	if (!isMain && num_iterations >= lim_iterations) {
		push_priority_queue(lock_prio_queue, extension_priority_queue, state_info, lock_has_entry);
	}

	free(isSolved);
	free(isFirstCalculation);
	delete solver;
	return;
}

/*===========================================================================================================================================================*/
/*===========================================================================================================================================================*/

void push_priority_queue(omp_lock_t *lock_prio_queue, 
	std::priority_queue<ExtensionPrioritised, std::vector<ExtensionPrioritised>, extPrioLess_t> &extension_priority_queue, 
	ExtensionPrioritised &newEntryQueue, omp_lock_t *lock_has_entry)
{
	omp_set_lock(lock_prio_queue);
	extension_priority_queue.push(newEntryQueue);
#pragma omp flush(extension_priority_queue)
	omp_unset_lock(lock_prio_queue);
	omp_unset_lock(lock_has_entry);
}

static ExtensionPrioritised pop_prio_queue(std::priority_queue<ExtensionPrioritised, std::vector<ExtensionPrioritised>, extPrioLess_t> &extension_priority_queue
	, omp_lock_t *lock_queue) {
	omp_set_lock(lock_queue);
	ExtensionPrioritised entry = extension_priority_queue.top();
	extension_priority_queue.pop();
#pragma omp flush(extension_priority_queue)
	omp_unset_lock(lock_queue);
	return entry;
}

static uint64_t check_prio_queue_size(std::priority_queue<ExtensionPrioritised, std::vector<ExtensionPrioritised>, extPrioLess_t> &extension_priority_queue
	, omp_lock_t *lock_queue) {
	omp_set_lock(lock_queue);
	uint64_t result = extension_priority_queue.size();
	omp_unset_lock(lock_queue);
	return result;
}

/*===========================================================================================================================================================*/
/*===========================================================================================================================================================*/

static bool start_checking_rejection(uint32_t argument, AF &framework, ArrayBitSet &active_args, list<uint32_t> &proof_extension, uint8_t numCores)
{
	int lim_iterations = 10;

	omp_lock_t *lock_queue = NULL;
	lock_queue = (omp_lock_t *)malloc(sizeof * lock_queue);
	if (lock_queue == NULL) {
		printf("Memory allocation failed\n");
		exit(1);
	}
	omp_init_lock(lock_queue);

	omp_lock_t *lock_has_entry = NULL;
	lock_has_entry = (omp_lock_t *)malloc(sizeof * lock_has_entry);
	if (lock_queue == NULL) {
		printf("Memory allocation failed\n");
		exit(1);
	}
	omp_init_lock(lock_has_entry);

	Heuristic1 heuristic = Heuristic1();

	if (numCores > 0)
	{
		omp_set_num_threads(numCores);
	}

	std::priority_queue<ExtensionPrioritised, std::vector<ExtensionPrioritised>, extPrioLess_t> extension_priority_queue;

	bool isTerminated = false;
	bool isFinished = false;
	bool isRejected = false;
	omp_set_lock(lock_has_entry);
#pragma omp parallel shared(argument, framework, active_args, isRejected, isTerminated, isFinished, proof_extension, heuristic, extension_priority_queue)
	{
#pragma omp single nowait
		{
			list<uint32_t> extension; 
			ExtensionPrioritised empty_extension = ExtensionPrioritised();
			check_rejection(argument, framework, active_args, isRejected, isTerminated, true, empty_extension, heuristic, proof_extension,
				extension_priority_queue, lock_queue, lock_has_entry, lim_iterations);
			update_isFinished(isTerminated, isFinished, extension_priority_queue, lock_queue, lock_has_entry);
		}

		bool isFinished_tmp = false;
		while (true) {
			omp_set_lock(lock_has_entry);
#pragma omp flush(isFinished)
#pragma omp atomic read
			isFinished_tmp = isFinished;
			if (isFinished_tmp) {
				omp_unset_lock(lock_has_entry);
				break;
			}

			uint64_t size = check_prio_queue_size(extension_priority_queue, lock_queue);
			
			if (size > 0) {
				ExtensionPrioritised state = pop_prio_queue(extension_priority_queue, lock_queue);

				if (size > 1) {
					omp_unset_lock(lock_has_entry);
				}

				check_rejection(argument, framework, active_args, isRejected, isTerminated, false, state, heuristic, proof_extension,
					extension_priority_queue, lock_queue, lock_has_entry, lim_iterations);
				update_isFinished(isTerminated, isFinished, extension_priority_queue, lock_queue, lock_has_entry);
			}
			else {
				throw new exception();
			}
		}
	}
	
	omp_destroy_lock(lock_queue);
	free(lock_queue);
	omp_destroy_lock(lock_has_entry);
	free(lock_has_entry);
	return isRejected;
}

/*===========================================================================================================================================================*/
/*===========================================================================================================================================================*/

bool check_if_finished(bool &isTerminated, bool &isFinished,
	std::priority_queue<ExtensionPrioritised, std::vector<ExtensionPrioritised>, extPrioLess_t> &extension_priority_queue,
	omp_lock_t *lock_queue) {
	bool isTerminated_tmp = false;
#pragma omp flush(isTerminated)
#pragma omp atomic read
	isTerminated_tmp = isTerminated;

	uint64_t size = check_prio_queue_size(extension_priority_queue, lock_queue);
	return size == 0 || isTerminated_tmp;
}

/*===========================================================================================================================================================*/
/*===========================================================================================================================================================*/

void update_isFinished(bool &isTerminated, bool &isFinished, 
	std::priority_queue<ExtensionPrioritised, std::vector<ExtensionPrioritised>, extPrioLess_t> &extension_priority_queue, 
	omp_lock_t *lock_queue, omp_lock_t *lock_has_entry)
{
	bool isFinished_tmp = check_if_finished(isTerminated, isFinished, extension_priority_queue, lock_queue);
#pragma atomic write
	isFinished = isFinished_tmp;
#pragma omp flush(isFinished)
	if (isFinished_tmp) {
		omp_unset_lock(lock_has_entry);
	}
}

/*===========================================================================================================================================================*/
/*===========================================================================================================================================================*/

bool Solver_DS_PR::solve(uint32_t argument, AF &framework, list<uint32_t> &proof_extension, uint8_t numCores) 
{
	ArrayBitSet initial_reduct = ArrayBitSet();
	pre_proc_result result_preProcessor = PreProc_DS_PR::process(framework, argument, initial_reduct);

	switch (result_preProcessor){

		case accepted:
			return true;

		case rejected:
			return false;

		case unknown:
			return !start_checking_rejection(argument, framework, initial_reduct, proof_extension, numCores);

		default:
			return unknown;
	}
}