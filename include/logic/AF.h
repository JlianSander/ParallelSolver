#ifndef AF_H
#define AF_H

#include <vector>
#include <unordered_set>
#include <cstdint>
#include <iostream>	
#include <omp.h>

#include "../util/ArrayBitSet.h"

template <class T>
inline void hash_combine(std::size_t &seed, const T &v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

namespace std
{
	template<typename S, typename T> struct hash<pair<S, T>>
	{
		inline size_t operator()(const pair<S, T> &v) const
		{
			size_t seed = 0;
			::hash_combine(seed, v.first);
			::hash_combine(seed, v.second);
			return seed;
		}
	};
}

/// <summary>
/// This class is responsible for representing the data of an abstract argumentation framework.
/// </summary>
class AF {
public:

    /// <summary>
    /// Number of arguments in the abstract argumentation framework.
    /// </summary>
    uint32_t num_args;
    /// <summary>
    /// Lists for each argument a list of the arguments, attacking it.
    /// </summary>
    std::vector<vector<uint32_t>> attackers;
	/// <summary>
	/// Lists for each argument a list of the arguments, which are attacked by it.
	/// </summary>
	std::vector<vector<uint32_t>> victims;
	/// <summary>
	/// List of the arguments in the framework, sorted according to their number of victims.
	/// </summary>
	std::vector<uint32_t> sorted_by_num_victims;
	/// <summary>
	/// Booleans indicating if an arguments attacks itself.
	/// </summary>
	std::vector<uint8_t> self_attack;
    /// <summary>
    /// Set in which the buckets contain pairs of arguments, indicating that the first argument is attacking the second.
    /// </summary>
    std::unordered_set<std::pair<uint32_t, uint32_t>> attacks;
	/// <summary>
	/// List for all arguments their distance to the argument of the query.
	/// </summary>
	std::vector<uint32_t> distance_to_query;

    /// <summary>
    /// Adds an attack to the instance.
    /// </summary>
    /// <param name="attacker">The argument which is attacking the other.</param>
    /// <param name="victim">The argument which is been attacked by the other.</param>
    /// <returns>TRUE iff the attack was successfully added.</returns>
    bool add_attack(uint32_t attacker, uint32_t victim);
    /// <summary>
    /// This method checks if there's an attack from the one to the other specified 
	/// argument.
    /// </summary>
    /// <param name="attacker">The argument which is attacking the other.</param>
    /// <param name="victim">The argument which is been attacked by the other.</param>
    /// <returns>TRUE iff there is an attack.</returns>
    bool exists_attack(uint32_t attacker, uint32_t victim) const;
    /// <summary>
    /// This method signals the instance, that it has all necessary information to finish
	/// the initialization process.
    /// </summary>
    void finish_initilization();
	/// <summary>
	/// This method starts the initialization process on this instance.
	/// </summary>
	/// <param name="number_args">The number of arguments in the framework to initialize.</param>
	void initialize(uint32_t number_args);
};

/// <summary>
/// This class is responsible for sorting arguments based on the number of their vecitims
/// </summary>
class ComparatorNumVictims {
public:
    ComparatorNumVictims(AF &framework) : _framework(framework) {}

    bool operator()(uint32_t argumentA, uint32_t argumentB) const {
        // Custom comparison logic involving state 
        return _framework.victims[argumentA].size() > _framework.victims[argumentB].size(); // it sorts in descending order 
    }

private:
    AF _framework;
};

#endif