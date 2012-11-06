
/**
 * If defined, use std::set to store labels, otherwise use std::vector
 */
//#define TREE_SET

/**
 * The specific LabelSet Implementation type
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
 * Maximal costs used to compute the lexicographic priority.
 * Must be appropriate to the problem size!
 */
 #define MAX_COST 1000