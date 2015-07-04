#ifndef UTIL_H
#define UTIL_H

#include "graph.h++"
#include <string>

class Train {
	std::string name;
	Graph::TrackGraph::ID entry_id;
	Graph::TrackGraph::ID exit_id;
	uint start_time;

public:
	Train(
		const std::string name,
		Graph::TrackGraph::ID entry_id,
		Graph::TrackGraph::ID exit_id,
		uint start_time
	)
		: name(name)
		, entry_id(entry_id)
		, exit_id(exit_id)
		, start_time(start_time)
	{}

	const std::string& getName() const { return name; }
	Graph::TrackGraph::ID getEntryId() const { return entry_id; }
	Graph::TrackGraph::ID getExitId() const { return exit_id; }
	uint getStartTime() const { return start_time; }

};

#endif /* UTIL_H */