#ifndef ALGO_OPTIONS_
#define ALGO_OPTIONS_

#include <unistd.h>
#include <sstream>
#include <iostream>
#include <string.h>

#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)

/**
 * Configure the label setting algorithm to use.
 */ 
//#define LABEL_SETTING_ALGORITHM NodeHeapLabelSettingAlgorithm
//#define LABEL_SETTING_ALGORITHM SharedHeapLabelSettingAlgorithm // will always use SharedHeapLabelSet
#define LABEL_SETTING_ALGORITHM ParetoSearch // will always use custom pareto label set

/**
 * The specific LabelSet Implementation type to be used by the NodeHeaplabelSettingAlgo
 */
#define LABEL_SET NaiveLabelSet
//#define LABEL_SET SplittedNaiveLabelSet
//#define LABEL_SET HeapLabelSet

/**
 * Configure the linear combination used to select the best label
 */ 
//#define PRIORITY_SUM
#define PRIORITY_LEX
//#define PRIORITY_MAX

/**
 * If defined, use std::set to store labels, otherwise use an std::vector
 */
//#define TREE_SET


/**
 * If defined, a pareto queue will be based ontop of a tree, otherwise it uses std::vector
 */
#define PARETO_QUEUE VectorParetoQueue
//#define PARETO_QUEUE BTreeParetoQueue


/**
 * Keep this defined to gather runtime stats during label setting
 */
//#define GATHER_STATS

/**
 * Enable the following flag to gather how label sets and the pareto queue are modified
 */ 
//#define GATHER_DATASTRUCTURE_MODIFICATION_LOG

/**
 * Enable the following flag to time individual substeps of the ParetoSearch algorithms
 */ 
//#define GATHER_SUBCOMPNENT_TIMING


/**
 * Maximal costs used to compute the lexicographic priority.
 * Must be appropriate to the problem size!
 */
 #define MAX_COST 20000


/**
 * A suitable size for dynamic but pre-allocated data structures where we want
 * to actually reserve memory only when we trigger a page fault.
 */
 #define LARGE_ENOUGH_FOR_EVERYTHING 16777216


 std::string currentConfig() {
	std::ostringstream out_stream;

	#ifdef PARALLEL_BUILD
		out_stream << "Parallel";
	#else 
		out_stream << "Sequential";
	#endif
	out_stream << STR(LABEL_SETTING_ALGORITHM) << "_";

	if (strcmp(STR(LABEL_SETTING_ALGORITHM), "ParetoSearch") == 0) {
		#ifdef PARALLEL_BUILD
			out_stream << "ParallelBTreeParetoQueue";
		#else 
			out_stream << STR(PARETO_QUEUE);
		#endif
	} else {
		if (strcmp(STR(LABEL_SETTING_ALGORITHM), "NodeHeapLabelSettingAlgorithm") == 0) {
			out_stream << STR(LABEL_SET) << "_";
		}
		#ifdef TREE_SET
			out_stream << "TreeSet_";
		#else
			out_stream << "VectorSet_";
		#endif

		#ifdef PRIORITY_LEX
			out_stream << "Lex";
		#endif
		#ifdef PRIORITY_SUM
			out_stream << "Sum";
		#endif
		#ifdef PRIORITY_MAX
			out_stream << "Max";
		#endif	
	}
	return out_stream.str();
}
#endif
