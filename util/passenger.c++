#include "passenger.h++"

#include <iostream>

namespace {
	void print_first_part(std::ostream& os, const Passenger& p) {
		os << p.getName() << " (" << p.getID().getValue() << ") : ";
	}
}

std::ostream& operator<<(std::ostream& os, const Passenger& p) {
	print_first_part(os,p);
	os << p.getEntryID() << "@t=" << p.getStartTime() << " -> " << p.getExitID();
	return os;
}

std::ostream& operator<<(std::ostream& os, const std::tuple<const Passenger&,const TrackNetwork&>& pair) {
	const auto& p = std::get<0>(pair);
	const auto& tn = std::get<1>(pair);
	print_first_part(os,p);
	os << tn.getVertexName(p.getEntryID()) << "@t=" << p.getStartTime() << " -> " << tn.getVertexName(p.getExitID());
	return os;
}
