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

	std::vector<std::vector<std::tuple<std::string, std::string, std::string, uint>>> trains {
		{
			std::make_tuple("T1","A","Z", 1),
		},
		{
			std::make_tuple("T1","A","Z", 1),
			std::make_tuple("T2","A","Z", 3),
		},
	};
}

std::tuple<TrackNetwork,std::vector<Train>, bool> parse_graph(std::istream& is, std::ostream& err) {
	static size_t call_num = 0;
	call_num += 1;
	(void)is;
	(void)err;

	TrackNetwork tn;
	std::vector<Train> trains;

	if (call_num > fake_data::graphs.size()) {
		return std::make_tuple(tn,trains,false);
	}

	for (auto& elem : fake_data::graphs[call_num-1]) {
		boost::add_edge(
			tn.getOrCreateVertex(std::get<0>(elem)),
			tn.getOrCreateVertex(std::get<1>(elem)),
			std::get<2>(elem),
			tn.g()
		);
	}

	for (auto& train : fake_data::trains[call_num-1]) {
		trains.emplace_back(
			std::get<0>(train),
			tn.getOrCreateVertex(std::get<1>(train)),
			tn.getOrCreateVertex(std::get<2>(train)),
			std::get<3>(train)			
		);
	}

	return std::make_tuple(std::move(tn),std::move(trains), true);
}

} // end namepace input
} // end namepace parsing
