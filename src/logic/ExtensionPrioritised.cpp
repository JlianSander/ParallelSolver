#include "../../include/logic/ExtensionPrioritised.h"

ExtensionPrioritised::ExtensionPrioritised() {
	Priority = UINT32_MAX;
}

ExtensionPrioritised::ExtensionPrioritised(AF framework, list<uint32_t> extension, Heuristic1 heuristic) {
	Extension = extension;
	Priority = heuristic.calculate_priority(framework, extension);
}

ExtensionPrioritised::ExtensionPrioritised(AF framework, list<uint32_t> extension, Heuristic1 heuristic, vector<vector<int64_t>> complement_clauses) {
	Extension = extension;
	Priority = heuristic.calculate_priority(framework, extension);
	Complement_clauses = complement_clauses;
}


ExtensionPrioritised::~ExtensionPrioritised() {
	Extension.clear();
	Complement_clauses.clear();
}