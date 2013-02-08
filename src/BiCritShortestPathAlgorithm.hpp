#ifndef LABELSETTING_H_
#define LABELSETTING_H_

#include "utility/datastructure/graph/KGraph.hpp"
#include "utility/datastructure/graph/Edge.hpp"
#include "utility/datastructure/graph/GraphTypes.hpp"
#include "utility/datastructure/graph/GraphMacros.h"

#include "options.hpp"


#ifdef PARALLEL_BUILD
#include "ParetoSearch_parallel.hpp"
#else
#include "ParetoSearch_sequential.hpp"
#endif

#include "SeqLabelSetting.hpp"

typedef utility::datastructure::IntegerBiWeightedEdge Edge;
typedef utility::datastructure::KGraph<Edge> Graph;
typedef utility::datastructure::NodeID NodeID;
typedef Edge::edge_data Label;

class LabelSettingAlgorithm : public LABEL_SETTING_ALGORITHM<Graph> {
public:
	LabelSettingAlgorithm(const Graph& graph_, const unsigned short _num_threads=0):
#ifdef PARALLEL_BUILD
		LABEL_SETTING_ALGORITHM<Graph>(graph_, _num_threads)
#else 
		LABEL_SETTING_ALGORITHM<Graph>(graph_)
#endif
	 {if(_num_threads == 0){} /* prevent unused variable warning */}
};

#endif
