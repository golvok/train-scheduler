#include "input_parser.h++"

#include <util/logging.h++>

#include <iostream>
#include <string>
#include <vector>

#include <boost/graph/graphviz.hpp>
#include <boost/phoenix/bind/bind_function_object.hpp>
#include <boost/phoenix/object/construct.hpp>
#include <boost/phoenix/stl/container.hpp>
#include <boost/spirit/include/qi.hpp>

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
			std::make_tuple("pA2-E2"  , "A2", "E2", 1),
			std::make_tuple("pB1-E2_0", "B1", "E2", 1),
			std::make_tuple("pB1-E2_1", "B1", "E2", 1),
		},
	};
}

std::tuple<TrackNetwork,PassengerList, bool> parse_data(std::istream& is) {

	auto indent = dout(DL::DATA_READ1).indentWithTitle("Reading Data");

	TrackNetwork::BackingGraphType network;
	PassengerList passengers;

	TrackNetwork::OffNodeDataPropertyMap off_node_data;

	boost::vector_property_map<std::string> passenger_desc;
	auto vertex_names = boost::make_transform_value_property_map(
		[](auto& pm_value) -> std::string& {
			return pm_value.name;
		},
		off_node_data
	);
	auto x_values = boost::make_transform_value_property_map(
		[](auto& pm_value) -> TrackNetwork::CoordType& {
			return pm_value.location.x;
		},
		off_node_data
	);
	auto y_values = boost::make_transform_value_property_map(
		[](auto& pm_value) -> TrackNetwork::CoordType& {
			return pm_value.location.y;
		},
		off_node_data
	);
	auto distances = get(&TrackNetwork::EdgeProperties::weight, network);
	// boost::vector_property_map<TrackNetwork::EdgeIndexType> edge_ids;

	boost::dynamic_properties dp;
	dp.property("vertex_name", vertex_names);
	dp.property("passenger", passenger_desc);
	dp.property("x", x_values);
	dp.property("y", y_values);
	dp.property("distance", distances);
	// dp.property("edge_id", edge_ids);

	bool status = boost::read_graphviz(is, network, dp, "vertex_name");

	{ size_t i = 0; for (const auto& edesc : make_iterable(edges(network))) {
		// because there seems to not be a way to determine if there was as "edge_id"
		// set, if we read it in, and that they should be set to something, I set them manually
		get(&TrackNetwork::EdgeProperties::index, network, edesc) = i;
		++i;
	}}

	dout(DL::DATA_READ1) << "file parse successful: " << std::boolalpha << status << '\n';

	{ auto vindent = dout(DL::DATA_READ1).indentWithTitle("Vertex Info");
		for (size_t i = 0; i < 7; ++i) {
			dout(DL::DATA_READ1)
				<< get(vertex_names, i) << ' '
				<< get(passenger_desc, i) << ' '
				<< get(x_values, i) << ' '
				<< get(y_values, i) << '\n'
			;
		}
	}

	{ auto eindent = dout(DL::DATA_READ1).indentWithTitle("Edge Info");
		for (const auto& e : make_iterable(edges(network))) {
			dout(DL::DATA_READ1)
				<< source(e, network) << "->" << target(e, network) << ' '
				<< get(distances, e) << ' '
				<< get(&TrackNetwork::EdgeProperties::index, network, e) << '\n'
			;
		}
	}

	TrackNetwork tn(std::move(network), std::move(off_node_data));

	for (const auto& vdesc : make_iterable(vertices(tn.g()))) {
		const auto& passenger_string = get(passenger_desc, vdesc);

		if (passenger_string.empty()) {
			continue; // skip empties
		}

		namespace qi = boost::spirit::qi;
		namespace chars = boost::spirit::ascii;
		namespace phoenix = boost::phoenix;

		const auto identifier = qi::as_string[+chars::alnum];
		const auto get_corrisponding_vertex = [&](const std::string& id_str) { return tn.getVertex(id_str); };

		auto it = begin(passenger_string);
		const bool is_match = qi::phrase_parse( it, end(passenger_string),
			(
				identifier >> ':' >> identifier >> "->" >> identifier >> "@t=" >> qi::double_
			) [
				phoenix::push_back( phoenix::ref(passengers), phoenix::construct<Passenger>(
					qi::_1,
					::util::make_id<PassengerId>(passengers.size()),
					phoenix::bind(get_corrisponding_vertex, qi::_2),
					phoenix::bind(get_corrisponding_vertex, qi::_3),
					qi::_4
				))
			],
			chars::space
		);

		const bool matches_full = is_match && it == end(passenger_string);

		const bool match_is_good = matches_full && passengers.back().getEntryID() == vdesc;

		if (!match_is_good) {
			passengers.pop_back();
			dout(DL::WARN) << "bad passenger spec: " << passenger_string << '\n';
		}
	}

	{ auto pindent = dout(DL::DATA_READ1).indentWithTitle("Passenger Info");
		::util::print_container(passengers, dout(DL::DATA_READ1), "\n", "", "");
		dout(DL::DATA_READ1) << '\n';
	}

	return std::make_tuple(std::move(tn), std::move(passengers), true);
}

} // end namepace input
} // end namepace parsing
