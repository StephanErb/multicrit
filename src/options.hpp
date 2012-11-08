
/**
 * If defined, use std::set to store labels, otherwise use std::vector
 */
//#define TREE_SET

/**
 * Configure the label setting algorithm to use.
 * Only one may be active at a time!
 */ 
//#define LABEL_SETTING_ALGORITHM NodeHeapLabelSettingAlgorithm
#define LABEL_SETTING_ALGORITHM SharedHeapLabelSettingAlgorithm // will always use SharedHeapLabelSet

/**
 * The specific LabelSet Implementation type to be used by the NodeHeaplabelSettingAlgo
 * Only one may be active at a time!
 */
//#define LABEL_SET SplittedNaiveLabelSet
#define LABEL_SET NaiveLabelSet

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