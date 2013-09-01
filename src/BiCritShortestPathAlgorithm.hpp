#ifndef LABELSETTING_H_
#define LABELSETTING_H_



#include "options.hpp"

#ifndef BRANCHING_PARAMETER_B
#define BRANCHING_PARAMETER_B 32
#endif
#ifndef LEAF_PARAMETER_K
#define LEAF_PARAMETER_K 660
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
#include "Graph.hpp"


class LabelSettingAlgorithm : public LABEL_SETTING_ALGORITHM {
public:
	LabelSettingAlgorithm(const Graph& graph_, const unsigned short _num_threads=my_default_thread_count):
#ifdef PARALLEL_BUILD
		LABEL_SETTING_ALGORITHM(graph_, _num_threads)
#else 
		LABEL_SETTING_ALGORITHM(graph_)
#endif
	 {if(_num_threads == 0){} /* prevent unused variable warning */}
};

#endif
