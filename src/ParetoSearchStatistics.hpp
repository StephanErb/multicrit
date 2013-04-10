#ifndef PARETOSEARCH_STATS_H_
#define PARETOSEARCH_STATS_H_

#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <numeric>
#include <vector>
#include <deque>

#define STAT_ELEMENT_COUNT 8

enum StatElement {
	IDENTICAL_TARGET_NODE,
	LABEL_DOMINATED,
	LABEL_NONDOMINATED,
	MINIMA_COUNT,
	UPDATE_COUNT,
	PQ_SIZE_DELTA,
	ITERATION,
	DOMINATION_SHORTCUT
};

#ifdef GATHER_STATS

template<typename Label>
class ParetoSearchStatistics {
private:
	unsigned long data[STAT_ELEMENT_COUNT];
	unsigned long pq_size;
	unsigned long peak_pq_size;
	unsigned long minima_size;
	unsigned long peak_minima_size;
	unsigned long update_size;
	unsigned long peak_update_size;
	unsigned long pq_size_delta;
	unsigned long peak_size_delta;
	unsigned long identical_target_node;
	unsigned long peak_identical_target_node;

public:

	ParetoSearchStatistics() :
		pq_size(0),
		peak_pq_size(0),
		minima_size(0),
		peak_minima_size(0),
		update_size(0),
		peak_update_size(0),
		pq_size_delta(0),
		peak_size_delta(0),
		identical_target_node(0),
		peak_identical_target_node(0)
	{
		std::fill_n(data, STAT_ELEMENT_COUNT, 0);
	}

	void report(StatElement stat, unsigned long payload=0) {
			data[stat]++;

			if (stat == ITERATION) {
				pq_size += payload;
				peak_pq_size = std::max(peak_pq_size, payload);
			}
			if (stat == MINIMA_COUNT) {
				minima_size += payload;
				peak_minima_size = std::max(peak_minima_size, payload);
			}
			if (stat == UPDATE_COUNT) {
				update_size += payload;
				peak_update_size = std::max(peak_update_size, payload);
			}
			if (stat == PQ_SIZE_DELTA) {
				pq_size_delta += payload;
				peak_size_delta = std::max(peak_size_delta, payload);
			}
			if (stat == IDENTICAL_TARGET_NODE) {
				identical_target_node += payload;
				peak_identical_target_node = std::max(peak_identical_target_node, payload);
			}
	}

#ifdef PARALLEL_BUILD

	std::string toString() {
		std::ostringstream out_stream;
		out_stream << "\nIterations: " << data[ITERATION] << "\n";

		double avg_pq_size = pq_size / data[ITERATION];
		out_stream << "ParetoQueue sizes: " << "\n";
		out_stream << "  avg" << ": " << avg_pq_size << "\n";
		out_stream << "  max" << ": " << peak_pq_size;
		
		return out_stream.str();
	}

#else

	template<class T>
	std::string toString(std::vector<T>& labels) {
		std::ostringstream out_stream;
		out_stream << "\nIterations: " << data[ITERATION] << "\n";

		unsigned long total_label_count = data[LABEL_NONDOMINATED] + data[LABEL_DOMINATED];
		double dom_percent = 100.0 * data[LABEL_DOMINATED] / total_label_count;
		double dom_shortcut_percent = 100.0 * data[DOMINATION_SHORTCUT] / data[LABEL_DOMINATED];

		out_stream << "Created Labels: " << total_label_count << "\n";
		out_stream << "  initially dominated" << ": " << dom_percent << "% (=" << data[LABEL_DOMINATED] <<")\n";
		out_stream << "    via candidate shortcut" << ": " << dom_shortcut_percent << "% (=" << data[DOMINATION_SHORTCUT] <<")\n";
		out_stream << "  initially non-dominated" << ": " << 100-dom_percent << "% (=" << data[LABEL_NONDOMINATED] <<")\n";


		unsigned long final_total_label_count = std::accumulate(labels.begin(), labels.end(), 0,
			[](const unsigned long accum, const T& ls) { return accum + ls.labels.size() -2; /*sentinal correction*/ });
		unsigned long finally_dominated = data[LABEL_NONDOMINATED] - final_total_label_count;
		double finally_dom_percent = 100.0 * finally_dominated / data[LABEL_NONDOMINATED];
		out_stream << "    finally dominated" << ": " << finally_dom_percent << "% (=" << finally_dominated <<")\n";

		double avg_set_size = final_total_label_count / labels.size();
		unsigned long max_set_size = (*std::max_element(labels.begin(), labels.end(), 
			[](const T& x, const T& y) { return x.labels.size() < y.labels.size(); })).labels.size() -2; //sentinal correction
		out_stream << "LabelSet sizes: " << "\n";
		out_stream << "  avg" << ": " << avg_set_size << "\n";
		out_stream << "  max" << ": " << max_set_size << "\n";

		double avg_pq_size = pq_size / data[ITERATION];
		out_stream << "ParetoQueue sizes: " << "\n";
		out_stream << "  avg" << ": " << avg_pq_size << "\n";
		out_stream << "  max" << ": " << peak_pq_size;

		if (data[MINIMA_COUNT] > 0) {
			out_stream << "\n";
			double avg_minima_size = minima_size / data[MINIMA_COUNT];
			out_stream << "Pareto Optimal Elements: " << "\n";
			out_stream << "  avg" << ": " << avg_minima_size << "\n";
			out_stream << "  max" << ": " << peak_minima_size;
		}
		if (data[UPDATE_COUNT] > 0) {
			out_stream << "\n";
			double avg_update_size = update_size / data[UPDATE_COUNT];
			out_stream << "Pareto Queue Updates: " << "\n";
			out_stream << "  avg" << ": " << avg_update_size << "\n";
			out_stream << "  max" << ": " << peak_update_size;
		}
		if (data[PQ_SIZE_DELTA] > 0) {
			out_stream << "\n";
			double avg_size_delta = pq_size_delta / data[PQ_SIZE_DELTA];
			out_stream << "Pareto Queue Size Delta: " << "\n";
			out_stream << "  avg" << ": " << avg_size_delta << "\n";
			out_stream << "  max" << ": " << peak_size_delta;
		}
		if (data[IDENTICAL_TARGET_NODE] > 0) {
			out_stream << "\n";
			double avg_ident_target_nodes = identical_target_node / data[IDENTICAL_TARGET_NODE];
			out_stream << "Identical Target Nodes per Iteration: " << "\n";
			out_stream << "  avg" << ": " << avg_ident_target_nodes << "\n";
			out_stream << "  max" << ": " << peak_identical_target_node;
		}
		return out_stream.str();
	}

#endif
};

#else

template<typename Label>
class ParetoSearchStatistics {
public:

	ParetoSearchStatistics() {}

	void report(StatElement) {}
	void report(StatElement, unsigned long) {}

#ifdef PARALLEL_BUILD
	std::string toString() {
#else
	template<class T>
	std::string toString(std::vector<T>&) {
#endif
	std::ostringstream out_stream;
	out_stream << " Statistics disabled at compile time. See options file.";
		return out_stream.str();
	}
};
#endif

#endif 
