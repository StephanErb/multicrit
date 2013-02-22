#ifndef LABELSETTING_H_
#define LABELSETTING_H_

#include "utility/datastructure/graph/KGraph.hpp"
#include "utility/datastructure/graph/Edge.hpp"
#include "utility/datastructure/graph/GraphTypes.hpp"
#include "utility/datastructure/graph/GraphMacros.h"

#include "options.hpp"

// Width of nodes given as number of cache-lines
#ifndef INNER_NODE_WIDTH
#define INNER_NODE_WIDTH 8
#endif
#ifndef LEAF_NODE_WIDTH
#define LEAF_NODE_WIDTH 8
#endif

#ifdef PARALLEL_BUILD
#include "tbb/task_scheduler_init.h"
#include "ParetoSearch_parallel.hpp"
const unsigned short my_default_thread_count = tbb::task_scheduler_init::default_num_threads();
#else
#include "ParetoSearch_sequential.hpp"
const unsigned short my_default_thread_count = 0;
#endif

#include "SeqLabelSetting.hpp"

typedef utility::datastructure::IntegerBiWeightedEdge Edge;
typedef utility::datastructure::KGraph<Edge> Graph;
typedef utility::datastructure::NodeID NodeID;
typedef Edge::edge_data Label;

class LabelSettingAlgorithm : public LABEL_SETTING_ALGORITHM<Graph> {
public:
	LabelSettingAlgorithm(const Graph& graph_, const unsigned short _num_threads=my_default_thread_count):
#ifdef PARALLEL_BUILD
		LABEL_SETTING_ALGORITHM<Graph>(graph_, _num_threads)
#else 
		LABEL_SETTING_ALGORITHM<Graph>(graph_)
#endif
	 {if(_num_threads == 0){} /* prevent unused variable warning */}
};

#endif
