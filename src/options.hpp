
#include <unistd.h>
#include <sstream>
#include <string.h>

#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)

/**
 * If defined, use std::set to store labels, otherwise use std::vector
 */
//#define TREE_SET

/**
 * Configure the label setting algorithm to use.
 * Only one may be active at a time!
 */ 
//#define LABEL_SETTING_ALGORITHM NodeHeapLabelSettingAlgorithm
//#define LABEL_SETTING_ALGORITHM SharedHeapLabelSettingAlgorithm // will always use SharedHeapLabelSet
#define LABEL_SETTING_ALGORITHM SequentialParetoSearch

/**
 * The specific LabelSet Implementation type to be used by the NodeHeaplabelSettingAlgo
 * Only one may be active at a time!
 */
#define LABEL_SET NaiveLabelSet
//#define LABEL_SET SplittedNaiveLabelSet
//#define LABEL_SET HeapLabelSet

// Configure the linear combination used to select the best label
// Only one may be active at a time!
//#define PRIORITY_SUM
#define PRIORITY_LEX
//#define PRIORITY_MAX

/**
 * Keep this defined to gather runtime stats during label setting
 */
#define GATHER_STATS

/**
 * Maximal costs used to compute the lexicographic priority.
 * Must be appropriate to the problem size!
 */
 #define MAX_COST 20000


 std::string currentConfig() {
	std::ostringstream out_stream;

	out_stream << STR(LABEL_SETTING_ALGORITHM) << "_";
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

	return out_stream.str();
}