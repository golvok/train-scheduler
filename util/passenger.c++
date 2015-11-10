#include "passenger.h++"

#include <iostream>

namespace {
	void print_first_part(std::ostream& os, const Passenger& p) {
		os << p.getName() << " (" << p.getId() << ") : ";
	}
}

// std::ostream& operator<<(std::ostream& os, const Passenger& p) {
// 	print_first_part(os,p);
// 	os << p.getEntryId() << "@t=" << p.getStartTime() << " -> " << p.getExitId();
// 	return os;
// }

std::ostream& operator<<(std::ostream& os, std::pair<const Passenger&,const TrackNetwork&> pair) {
	const auto& p = pair.first;
	const auto& tn = pair.second;
	print_first_part(os,p);
	os << tn.getVertexName(p.getEntryId()) << "@t=" << p.getStartTime() << " -> " << tn.getVertexName(p.getExitId());
	return os;
}
