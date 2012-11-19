#ifndef PARETOSEARCH_STATS_H_
#define PARETOSEARCH_STATS_H_

#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <numeric>

//NodeID and EdgdeID
#include "utility/datastructure/graph/GraphTypes.hpp"
#include "utility/datastructure/graph/GraphMacros.h"

#define STAT_ELEMENT_COUNT 5

enum StatElement {
	CANDIDATE,
	LABEL_DOMINATED,
	LABEL_NONDOMINATED,
	MINIMA_COUNT,
	ITERATION
};

#ifdef GATHER_STATS

class ParetoSearchStatistics {
private:
	unsigned int data[STAT_ELEMENT_COUNT];
	std::vector<unsigned int> pq_size;
	unsigned int peak_pq_size;
	std::vector<unsigned int> minima_size;
	unsigned int peak_minima_size;

public:

	ParetoSearchStatistics(unsigned int node_count) :
		peak_pq_size(0),
		peak_minima_size(0)
	{
		std::fill_n(data, STAT_ELEMENT_COUNT, 0);
	}

	void report(StatElement stat, unsigned int payload=0) {
			data[stat]++;

			if (stat == ITERATION) {
				pq_size.push_back(payload);
				peak_pq_size = std::max(peak_pq_size, payload);
			}
			if (stat == MINIMA_COUNT) {
				minima_size.push_back(payload);
				peak_minima_size = std::max(peak_pq_size, payload);
			}			
	}

	std::string toString() {
		std::ostringstream out_stream;
		out_stream << "\nIterations: " << data[ITERATION] << "\n";

		unsigned int total_label_count = data[LABEL_NONDOMINATED] + data[LABEL_DOMINATED];
		double dom_percent = 100.0 * data[LABEL_DOMINATED] / total_label_count;
		out_stream << "Created Labels: " << total_label_count << "\n";
		out_stream << "  initially dominated" << ": " << dom_percent << "% (=" << data[LABEL_DOMINATED] <<")\n";
		out_stream << "  initially non-dominated" << ": " << 100-dom_percent << "% (=" << data[LABEL_NONDOMINATED] <<")\n";

		unsigned int total_pq_size = std::accumulate(pq_size.begin(), pq_size.end(), 0);
		double avg_pq_size = total_pq_size / pq_size.size();
		out_stream << "ParetoQueue sizes: " << "\n";
		out_stream << "  avg" << ": " << avg_pq_size << "\n";
		out_stream << "  max" << ": " << peak_pq_size << "\n";

		unsigned int total_minima_size = std::accumulate(minima_size.begin(), minima_size.end(), 0);
		double avg_minima_size = total_minima_size / minima_size.size();
		out_stream << "Pareto Optimal Elements: " << "\n";
		out_stream << "  avg" << ": " << avg_minima_size << "\n";
		out_stream << "  max" << ": " << peak_minima_size;
		
		return out_stream.str();

	}
};

#else

class LabelSettingStatistics {
public:

	LabelSettingStatistics(unsigned int node_count) {}

	void report(StatElement stat, unsigned int payload=0) {}

	std::string toString() {
		std::ostringstream out_stream;
		out_stream << " Statistics disabled at compile time. See options file.";
		return out_stream.str();

	}
};

#endif

#endif 
