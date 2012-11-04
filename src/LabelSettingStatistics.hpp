#ifndef LABELSETTING_STATS_H_
#define LABELSETTING_STATS_H_

#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <numeric>

//NodeID and EdgdeID
#include "utility/datastructure/graph/GraphTypes.hpp"
#include "utility/datastructure/graph/GraphMacros.h"

#define DATA_COUNT 4

enum StatisticalElement {
	BEST_NODE,
	NEW_LABEL_DOMINATED,
	NEW_LABEL_NONDOMINATED,
	NEW_BEST_LABEL
};

class LabelSettingStatistics {
private:
	unsigned int data[DATA_COUNT];
	std::vector<unsigned int> labels_set_size;
	unsigned int max_temp_set_size;

	// consecutive calls to markBestLabelAsPermantent without inserts
	std::vector<unsigned int> depletion_counter; 
	std::vector<unsigned int> depletion_sequences;
public:

	
	LabelSettingStatistics(unsigned int node_count) :
		labels_set_size(node_count),
		max_temp_set_size(0),
		depletion_counter(node_count)
	{
		std::fill_n(data, DATA_COUNT, 0);
	}

	void report(StatisticalElement stat, NodeID node, unsigned int payload=0) {
			data[stat]++;

			if (stat == NEW_LABEL_NONDOMINATED) {
				labels_set_size[node] = payload;
				max_temp_set_size = std::max(max_temp_set_size, payload);

				if (depletion_counter[node] > 0) {
					depletion_sequences.push_back(depletion_counter[node]);
					depletion_counter[node] = 0;
				}
			}
			if (stat == BEST_NODE) {
				depletion_counter[node]++;
			}
	}

	std::string toString() {
		std::ostringstream out_stream;
		
		unsigned int total_label_count = data[NEW_LABEL_NONDOMINATED] + data[NEW_LABEL_DOMINATED];
		double dom_percent = 100.0 * data[NEW_LABEL_DOMINATED] / total_label_count;
		out_stream << "\nCreated Labels: " << total_label_count << "\n";
		out_stream << "  initially dominated" << ": " << dom_percent << "% (=" << data[NEW_LABEL_DOMINATED] <<")\n";
		out_stream << "  initially non-dominated" << ": " << 100-dom_percent << "% (=" << data[NEW_LABEL_NONDOMINATED] <<")\n";

		double new_best_percent = 100.0 * data[NEW_BEST_LABEL] / data[NEW_LABEL_NONDOMINATED];
		out_stream << "    new best label" << ": " << new_best_percent << "% (=" << data[NEW_BEST_LABEL] <<")\n";

		unsigned int final_total_label_count = std::accumulate(labels_set_size.begin(), labels_set_size.end(), 0);
		unsigned int finally_dominated = data[NEW_LABEL_NONDOMINATED] - final_total_label_count;
		double finally_dom_percent = 100.0 * finally_dominated / data[NEW_LABEL_NONDOMINATED];
		out_stream << "    finally dominated" << ": " << finally_dom_percent << "% (=" << finally_dominated <<")\n";

		double avg_final_set_size = final_total_label_count / labels_set_size.size();
		unsigned int max_final_set_size = *std::max_element(labels_set_size.begin(), labels_set_size.end());
		out_stream << "LabelSet sizes: " << "\n";
		out_stream << "  avg (final)" << ": " << avg_final_set_size << "\n";
		out_stream << "  max (final)" << ": " << max_final_set_size << "\n";
		out_stream << "  max (temp)"  << ": " << max_temp_set_size  << "\n";

		for (std::vector<unsigned int>::const_iterator i = depletion_counter.begin(); i != depletion_counter.end(); ++i) {
			if (*i > 0) {
				depletion_sequences.push_back(*i);
			}
		}
		depletion_counter.clear();
		unsigned int total_depletions = std::accumulate(depletion_sequences.begin(), depletion_sequences.end(), 0);
		double avg_depletion_run = total_depletions / depletion_sequences.size();
		unsigned int max_depletion_run = *std::max_element(depletion_sequences.begin(), depletion_sequences.end());
		out_stream << "markBestLabelAsPermantent without insert into set:" << "\n";
		out_stream << "  avg" << ": " << avg_depletion_run << "\n";
		out_stream << "  max" << ": " << max_depletion_run << "\n";
		return out_stream.str();
	}
};

#endif 
