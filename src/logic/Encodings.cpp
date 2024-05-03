#include "../../include/logic/Encodings.h"

using namespace std;
using std::vector;

static int64_t get_literal_accepted(uint32_t argument, bool isInverted)
{
	int64_t variable = static_cast<int64_t>(argument);
	return isInverted ? -1 * variable: variable;
}

static int64_t get_literal_rejected(uint32_t activeArgsSize, uint32_t argument, bool isInverted)
{
	int64_t variable = static_cast<int64_t>(argument) + activeArgsSize;
	return isInverted ? -1 * variable : variable;
}

/*===========================================================================================================================================================*/
/*===========================================================================================================================================================*/

static vector<int64_t> add_rejected_clauses(SatSolver &solver, uint32_t activeArgsSize, uint32_t argument)
{
	// basic acceptance and rejection clause
	//Part I:  models that an argument cannot be accepted and rejected at the same time
	//create disjunction

	solver.add_clause_short(
		get_literal_rejected(activeArgsSize, argument, true),
		get_literal_accepted(argument, true));

	//Part III: constitutes that if an argument 'a' is rejected, one of its attackers must be accepted
	//create disjunction
	vector<int64_t> rejection_reason_clause;
	rejection_reason_clause.push_back(get_literal_rejected(activeArgsSize, argument, true));
	return rejection_reason_clause;
}

/*===========================================================================================================================================================*/
/*===========================================================================================================================================================*/

static void add_rejected_clauses_per_attacker(SatSolver &solver, uint32_t activeArgsSize, uint32_t argument, uint32_t attacker, vector<int64_t> &rejection_reason_clause)
{
	//Part II: ensures that if an attacker 'b' of an argument 'a' is accepted, then 'a' must be rejected
	//create disjunction for active attacker

	solver.add_clause_short(
		get_literal_rejected(activeArgsSize, argument, false),
		get_literal_accepted(attacker, true));

	//Part III: constitutes that if an argument 'a' is rejected, one of its attackers must be accepted
	//create disjunction
	rejection_reason_clause.push_back(get_literal_accepted(attacker, false));
}

/*===========================================================================================================================================================*/
/*===========================================================================================================================================================*/

static void add_conflict_free_per_attacker(SatSolver &solver, uint32_t argument, uint32_t attacker)
{
	//create disjunction

	if (argument != attacker)
	{
		solver.add_clause_short(
			get_literal_accepted(argument, true),
			get_literal_accepted(attacker, true));
	}
	else
	{
		solver.add_clause_short(get_literal_accepted(argument, true), 0);
	}
}

/*===========================================================================================================================================================*/
/*===========================================================================================================================================================*/

static void add_defense_per_attacker(SatSolver &solver, uint32_t activeArgsSize, uint32_t arguments, uint32_t attacker)
{
	if (arguments == attacker)
	{
		return;
	}
	//is not a self-attack

	//models the notion of defense in an abstract argumentation framework: 
	// if an argument is accepted to be in the admissible set, all its attackers must be rejected
	//create disjunction

	solver.add_clause_short(
		get_literal_accepted(arguments, true),
		get_literal_rejected(activeArgsSize, attacker, false));
}

/*===========================================================================================================================================================*/
/*===========================================================================================================================================================*/

static void add_admissible(SatSolver &solver, AF &framework, unordered_set<uint32_t> &activeArgs, uint32_t argument)
{
	vector<int64_t> rejection_reason_clause = add_rejected_clauses(solver, activeArgs.size(), argument);
	
	vector<uint32_t> attackers = framework.attackers[argument];

	for (int i = 0; i < attackers.size(); i++)
	{
		if(activeArgs.count(attackers[i]))
		{
			add_rejected_clauses_per_attacker(solver, activeArgs.size(), argument, attackers[i], rejection_reason_clause);
			add_conflict_free_per_attacker(solver, argument, attackers[i]);
			add_defense_per_attacker(solver, activeArgs.size(), argument, attackers[i]);
		}
	}

	solver.add_clause(rejection_reason_clause);
	rejection_reason_clause.clear();
}

/*===========================================================================================================================================================*/
/*===========================================================================================================================================================*/

void Encodings_SatSolver::add_clauses_nonempty_admissible_set(SatSolver &solver, AF &framework, unordered_set<uint32_t> &activeArgs)
{
	vector<int64_t> non_empty_clause;
	//iterate through all active arguments


	for (size_t bno = 0; bno < activeArgs.bucket_count(); ++bno) {
		for (auto bit = activeArgs.begin(bno), end = activeArgs.end(bno); bit != end; ++bit) {
			const auto &argument = *bit;

			non_empty_clause.push_back(get_literal_accepted(argument, false));
			add_admissible(solver, framework, activeArgs, argument);
		}
	}

	solver.add_clause(non_empty_clause);
	non_empty_clause.clear();
}

/*===========================================================================================================================================================*/
/*===========================================================================================================================================================*/

void Encodings_SatSolver::add_complement_clause(SatSolver &solver, unordered_set<uint32_t> &activeArgs)
{
	vector<int64_t> complement_clause;

	for (size_t bno = 0; bno < activeArgs.bucket_count(); ++bno) {
		for (auto bit = activeArgs.begin(bno), end = activeArgs.end(bno); bit != end; ++bit) {
			const auto &argument = *bit;
			int64_t arg_64 = static_cast<int64_t>(argument);

			if (solver.check_var_model(arg_64))
			{
				int64_t arg_64_inv = -1 * arg_64;
				complement_clause.push_back(arg_64_inv);
			}
		}
	}

	solver.add_clause(complement_clause);
	complement_clause.clear();
}