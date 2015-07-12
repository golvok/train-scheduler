#include "input_parser.h++"

#include <iostream>
#include <string>
#include <vector>

namespace parsing {
namespace input {

namespace fake_data {
	std::vector<std::vector<std::tuple<std::string, std::string, uint>>> graphs {
		{
			std::make_tuple("A","Z",1),
		},
		{
			std::make_tuple("A","B",1),
			std::make_tuple("A","C",2),
			std::make_tuple("A","D",2),
			std::make_tuple("B","Z",2),
			std::make_tuple("C","Z",2),
			std::make_tuple("D","Z",1),
		},
	};

	std::vector<std::vector<std::tuple<std::string, std::string, std::string, uint>>> passengers {
		{
			std::make_tuple("P1","A","Z", 1),
		},
		{
			std::make_tuple("P1","A","Z", 1),
			std::make_tuple("P2","A","Z", 3),
		},
	};
}

std::tuple<TrackNetwork,std::vector<Passenger>, bool> parse_graph(std::istream& is, std::ostream& err) {
	static size_t call_num = 0;
	call_num += 1;
	(void)is;
	(void)err;

	TrackNetwork tn;
	std::vector<Passenger> passengers;

	if (call_num > fake_data::graphs.size()) {
		return std::make_tuple(tn,passengers,false);
	}

	for (auto& elem : fake_data::graphs[call_num-1]) {
		boost::add_edge(
			tn.getOrCreateVertex(std::get<0>(elem)),
			tn.getOrCreateVertex(std::get<1>(elem)),
			std::get<2>(elem),
			tn.g()
		);
	}

	for (auto& train : fake_data::passengers[call_num-1]) {
		passengers.emplace_back(
			std::get<0>(train),
			tn.getOrCreateVertex(std::get<1>(train)),
			tn.getOrCreateVertex(std::get<2>(train)),
			std::get<3>(train)			
		);
	}

	return std::make_tuple(std::move(tn),std::move(passengers), true);
}

} // end namepace input
} // end namepace parsing
