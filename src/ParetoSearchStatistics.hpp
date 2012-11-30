#ifndef PARETOSEARCH_STATS_H_
#define PARETOSEARCH_STATS_H_

#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <numeric>

#define STAT_ELEMENT_COUNT 6

enum StatElement {
	IDENTICAL_TARGET_NODE,
	LABEL_DOMINATED,
	LABEL_NONDOMINATED,
	MINIMA_COUNT,
	ITERATION,
	DOMINATION_SHORTCUT
};

#ifdef GATHER_STATS

class ParetoSearchStatistics {
private:
	unsigned int data[STAT_ELEMENT_COUNT];
	std::vector<unsigned int> pq_size;
	unsigned int peak_pq_size;
	std::vector<unsigned int> minima_size;
	unsigned int peak_minima_size;
	std::vector<unsigned int> identical_target_node;
	unsigned int peak_identical_target_node;

public:

	ParetoSearchStatistics() :
		peak_pq_size(0),
		peak_minima_size(0),
		peak_identical_target_node(0)
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
				peak_minima_size = std::max(peak_minima_size, payload);
			}
			if (stat == IDENTICAL_TARGET_NODE) {
				identical_target_node.push_back(payload);
				peak_identical_target_node = std::max(peak_identical_target_node, payload);
			}
	}

	std::string toString() {
		std::ostringstream out_stream;
		out_stream << "\nIterations: " << data[ITERATION] << "\n";

		unsigned int total_label_count = data[LABEL_NONDOMINATED] + data[LABEL_DOMINATED];
		double dom_percent = 100.0 * data[LABEL_DOMINATED] / total_label_count;
		double dom_shortcut_percent = 100.0 * data[DOMINATION_SHORTCUT] / data[LABEL_DOMINATED];

		out_stream << "Created Labels: " << total_label_count << "\n";
		out_stream << "  initially dominated" << ": " << dom_percent << "% (=" << data[LABEL_DOMINATED] <<")\n";
		out_stream << "    via candidate shortcut" << ": " << dom_shortcut_percent << "% (=" << data[DOMINATION_SHORTCUT] <<")\n";

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
		out_stream << "  max" << ": " << peak_minima_size << "\n";

		unsigned int total_ident_target_nodes = std::accumulate(identical_target_node.begin(), identical_target_node.end(), 0);
		double avg_ident_target_nodes = total_ident_target_nodes / identical_target_node.size();
		out_stream << "Identical Target Nodes per Iteration: " << "\n";
		out_stream << "  avg" << ": " << avg_ident_target_nodes << "\n";
		out_stream << "  max" << ": " << peak_identical_target_node;
		
		return out_stream.str();

	}
};

#else

class ParetoSearchStatistics {
public:

	ParetoSearchStatistics() {}

	void report(StatElement stat, unsigned int payload=0) {}

	std::string toString() {
		std::ostringstream out_stream;
		out_stream << " Statistics disabled at compile time. See options file.";
		return out_stream.str();

	}
};

#endif

#endif 
