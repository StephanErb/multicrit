#ifndef LABELSETTING_H_
#define LABELSETTING_H_

#include "utility/datastructure/graph/KGraph.hpp"
#include "utility/datastructure/graph/Edge.hpp"
#include "utility/datastructure/graph/GraphTypes.hpp"
#include "utility/datastructure/graph/GraphMacros.h"

#include "options.hpp"

#include "ParetoSearch.hpp"
#include "SeqLabelSetting.hpp"

typedef utility::datastructure::IntegerBiWeightedEdge Edge;
typedef utility::datastructure::KGraph<Edge> Graph;
typedef utility::datastructure::NodeID NodeID;
typedef Edge::edge_data Label;

class LabelSettingAlgorithm : public LABEL_SETTING_ALGORITHM<Graph> {
public:
	LabelSettingAlgorithm(const Graph& graph_):
		LABEL_SETTING_ALGORITHM<Graph>(graph_)
	 {}
};

#endif
