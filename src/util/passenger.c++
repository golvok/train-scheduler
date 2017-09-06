#include "passenger.h++"

#include <iostream>

namespace {
	void print_first_part(std::ostream& os, const StatisticalPassenger& p) {
		os << p.getBaseName();
	}

	void print_first_part(std::ostream& os, const Passenger& p) {
		os << p.getName() << " (" << p.getID().getValue() << ") : ";
	}
}

void StatisticalPassenger::print(std::ostream& os) const {
	print_first_part(os,*this);
	os << getEntryID() << " -> " << getExitID();
}

void StatisticalPassenger::print(std::ostream& os, const TrackNetwork& tn) const {
	print_first_part(os,*this);
	os << tn.getVertexName(getEntryID()) << " -> " << tn.getVertexName(getExitID());
}

void Passenger::print(std::ostream& os) const {
	print_first_part(os,*this);
	os << getEntryID() << "@t=" << getStartTime() << " -> " << getExitID();
}

void Passenger::print(std::ostream& os, const TrackNetwork& tn) const {
	print_first_part(os,*this);
	os << tn.getVertexName(getEntryID()) << "@t=" << getStartTime() << " -> " << tn.getVertexName(getExitID());
}
