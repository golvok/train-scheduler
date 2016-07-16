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
		dout(DL::DATA_READ1) << "name x y passenger_desc\n";
		for (const auto& v : make_iterable(vertices(network))) {
			dout(DL::DATA_READ1)
				<< get(vertex_names, v) << ' '
				<< get(x_values, v) << ' '
				<< get(y_values, v) << ' '
				<< get(passenger_desc, v) << '\n'
			;
		}
	}

	{ auto eindent = dout(DL::DATA_READ1).indentWithTitle("Edge Info");
		dout(DL::DATA_READ1) << "src -> dest distance edge_id\n";
		for (const auto& e : make_iterable(edges(network))) {
			dout(DL::DATA_READ1)
				<< source(e, network) << " -> " << target(e, network) << ' '
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

		const auto identifier = qi::as_string[ +( chars::alnum || qi::lit('_') ) ];
		const auto get_corrisponding_vertex = [&](const std::string& id_str) { return tn.getVertex(id_str); };
		const auto get_next_passenger_id = [&]() { return ::util::make_id<PassengerId>(passengers.size()); };

		auto it = begin(passenger_string);
		const bool is_match = qi::phrase_parse( it, end(passenger_string),
			(
				identifier >> ':' >> identifier >> "->" >> identifier >> "@t=" >> qi::double_
			) [
				phoenix::push_back( phoenix::ref(passengers), phoenix::construct<Passenger>(
					qi::_1,
					phoenix::bind(get_next_passenger_id),
					phoenix::bind(get_corrisponding_vertex, qi::_2),
					phoenix::bind(get_corrisponding_vertex, qi::_3),
					qi::_4
				))
			] % ',',
			chars::space
		);

		const bool matches_full = is_match && it == end(passenger_string);

		const bool match_is_good = matches_full && passengers.back().getEntryID() == vdesc;

		if (!match_is_good) {
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
