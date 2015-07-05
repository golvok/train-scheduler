#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <util/graph.h++>
#include <util/utils.h++>

namespace algo {

int schedule(Graph::TrackGraph& network, std::vector<Train>& trains);
	
} // end namespace algo

#endif /* SCHEDULER_H */