#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <util/track_network.h++>
#include <util/utils.h++>

namespace algo {

int schedule(TrackNetwork& network, std::vector<Passenger>& passengers);

} // end namespace algo

#endif /* SCHEDULER_H */