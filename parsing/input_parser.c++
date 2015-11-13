#include "input_parser.h++"

#include <iostream>
#include <string>
#include <vector>

namespace parsing {
namespace input {

namespace fake_data {
	std::vector<std::unordered_map<std::string,std::pair<float,float>>> vertex_locations {
		{
			{"A",{  0,  0}},
			{"B",{ 20,  0}},
			{"C",{ 40,  0}},
			{"D",{ 60,  0}},
			{"E",{ 80,  0}},
			{"F",{100,  0}},
			{"Z",{120,  0}},
		},
		{
			{"A",{  0,  0}},
			{"B",{ 20,  0}},
			{"C",{ 40,  0}},
			{"D",{ 60,  0}},
			{"E",{ 80,  0}},
			{"F",{100,  0}},
			{"G",{120,  0}},
			{"H",{140,  0}},
			{"I",{160,  0}},
			{"J",{180,  0}},
			{"K",{200,  0}},
			{"Z",{220,  0}},
		},
		{
			{"A", {  0,  0}},
			{"B", { 40,  0}},
			{"C0",{ 80,-40}},
			{"C1",{ 80,+60}},
			{"D0",{120,-40}},
			{"D1",{120,+60}},
			{"E", {160,  0}},
			{"F", {200,  0}},
			{"G0",{240,-40}},
			{"G1",{240,+40}},
		},
		{
			{"C", {  0,  0}},
			{"A1",{-80,  0}},
			{"B1",{-40,  0}},
			{"D1",{ 40,  0}},
			{"E1",{ 80,  0}},
			{"A2",{  0,-80}},
			{"B2",{  0,-40}},
			{"D2",{  0, 40}},
			{"E2",{  0, 80}},
		},
	};

	std::vector<std::vector<std::tuple<std::string, std::string, uint>>> graphs {
		{
			std::make_tuple("A"," ",0), // first one is spawn location
			std::make_tuple("A","B",1),
			std::make_tuple("B","C",1),
			std::make_tuple("C","D",1),
			std::make_tuple("D","E",1),
			std::make_tuple("E","F",1),
			std::make_tuple("F","Z",1),
		},
		{
			std::make_tuple("A"," ",0),
			std::make_tuple("A","B",1),
			std::make_tuple("B","C",1),
			std::make_tuple("C","D",1),
			std::make_tuple("D","E",1),
			std::make_tuple("E","F",1),
			std::make_tuple("F","G",1),
			std::make_tuple("G","H",1),
			std::make_tuple("H","I",1),
			std::make_tuple("I","J",1),
			std::make_tuple("J","K",1),
			std::make_tuple("K","Z",1),
		},
		{
			std::make_tuple("A", " ", 0),
			std::make_tuple("A", "B", 1),
			std::make_tuple("B", "C0",1),
			std::make_tuple("B", "C1",2),
			std::make_tuple("C0","D0",1),
			std::make_tuple("C1","D1",1),
			std::make_tuple("D0","E", 1),
			std::make_tuple("D1","E", 2),
			std::make_tuple("E", "F", 1),
			std::make_tuple("F", "G0",1),
			std::make_tuple("F", "G1",1),
		},
		{
			std::make_tuple("A1"," ", 0),
			std::make_tuple("A1","A2",1),
			std::make_tuple("A1","B1",1),
			std::make_tuple("B1","C" ,1),
			std::make_tuple("C", "D1",1),
			std::make_tuple("D1","E1",1),
			std::make_tuple("A2","B2",1),
			std::make_tuple("B2","C" ,1),
			std::make_tuple("C", "D2",1),
			std::make_tuple("D2","E2",1),
		},
	};

	std::vector<std::vector<std::tuple<std::string, std::string, std::string, uint>>> passengers {
		{
			std::make_tuple("  "," "," ", 0), // first one is graph id used
			std::make_tuple("pA","A","B", 1),
			std::make_tuple("pB","B","C", 2),
			std::make_tuple("pC","C","D", 3),
			std::make_tuple("pD","D","E", 4),
			std::make_tuple("pE","E","F", 5),
			std::make_tuple("pF","F","Z", 6),
		},
		{
			std::make_tuple("  "," "," ", 0),
			std::make_tuple("p1","A","D", 1),
			std::make_tuple("p2","B","F", 1),
			std::make_tuple("p3","C","D", 1),
			std::make_tuple("p4","C","F", 1),
			std::make_tuple("p5","D","F", 1),
			std::make_tuple("p6","E","Z", 1),
		},
		{
			std::make_tuple("  "," "," ", 1),
			std::make_tuple("p1","A","Z", 1),
			std::make_tuple("p2","A","Z", 3),
			std::make_tuple("p3","B","K", 3),
		},
		{
			std::make_tuple(" "      ," " ," " , 2),
			std::make_tuple("pA-G0",  "A" ,"G0", 1),
			std::make_tuple("pA-G1_0","A" ,"G1", 1),
			std::make_tuple("pA-G1_1","A" ,"G1", 1),
			std::make_tuple("pC0-G0", "C0","G0", 1),
			std::make_tuple("pC1-G0", "C0","G0", 1),
		},
		{
			std::make_tuple(" "       , " ",  " ",  3),
			std::make_tuple("pA1-E1_0", "A1", "E1", 1),
			std::make_tuple("pA1-E1_1", "A1", "E1", 1),
			std::make_tuple("pA1-E1_2", "A1", "E1", 1),
			std::make_tuple("pA2-E2",   "A2", "E2", 1),
			std::make_tuple("pB1-E2",   "B1", "E2", 1),
		},
	};
}

std::tuple<TrackNetwork,PassengerList, bool> parse_data(std::istream& is) {
	static size_t call_num = 0;
	call_num += 1;
	(void)is;

	TrackNetwork tn;
	PassengerList passengers;

	if (call_num > fake_data::passengers.size()) {
		return std::make_tuple(tn,passengers,false);
	}

	bool first = true;
	for (auto& passenger : fake_data::passengers[call_num-1]) {
		if (first) {
			first = false;
			// get graph id from first "passenger"'s arrival time
			size_t graph_id = std::get<3>(passenger);

			// create vertices
			for (auto& elem : fake_data::vertex_locations[graph_id]) {
				tn.createVertex(elem.first, {elem.second.first,elem.second.second} );
			}

			// add edges
			bool first_edge = true;
			uint e_index = 0;
			for (auto& elem : fake_data::graphs[graph_id]) {
				if (first_edge) {
					tn.setTrainSpawnLocation(tn.getVertex(std::get<0>(elem)));
					first_edge = false;
				} else {
					boost::add_edge(
						tn.getVertex(std::get<0>(elem)),
						tn.getVertex(std::get<1>(elem)),
						TrackNetwork::EdgeProperties (
							std::get<2>(elem),
							e_index
						),
						tn.g()
					);
					e_index += 1;
				}
			}
		} else {
			passengers.emplace_back(
				std::get<0>(passenger),
				::util::make_id<PassengerId>(passengers.size()),
				tn.getVertex(std::get<1>(passenger)),
				tn.getVertex(std::get<2>(passenger)),
				// WC doesn't usderstand passengers that don't start at 0 yet
				0 // std::get<3>(passenger)
			);
		}
	}

	return std::make_tuple(std::move(tn),std::move(passengers), true);
}

} // end namepace input
} // end namepace parsing
