#include "input_parser.h++"

#include <util/logging.h++>

#include <iostream>
#include <string>
#include <vector>

#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/tuple.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/spirit/home/x3.hpp>

namespace {
    template <typename T>
    struct as_type {
        template <typename Expr>
            auto operator[](Expr&& expr) const {
                return boost::spirit::x3::rule<struct _, T>{"as"} = boost::spirit::x3::as_parser(std::forward<Expr>(expr));
            }
    };

    template <typename T> static const as_type<T> as = {};
}

namespace parsing {
namespace input {

std::tuple<TrackNetwork,StatPassCollection, bool> parse_data(std::istream& is) {

	auto indent = dout(DL::DATA_READ1).indentWithTitle("Reading Data");

	TrackNetwork::BackingGraphType network;
	StatPassCollection statpsgrs;

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

		namespace x3 = boost::spirit::x3;
		namespace chars = boost::spirit::x3::ascii;

		const auto identifier = as<std::string>[
			x3::raw[ x3::lexeme[ +( chars::alpha | x3::lit('_') ) >> *( chars::alnum | x3::lit('_') ) ] ]
		];

		std::vector< std::tuple<
			std::string, std::vector<std::string>, std::vector<std::string>, double, double
		> > parse_results;

		auto it = begin(passenger_string);
		const bool is_match = x3::phrase_parse( it, end(passenger_string),
			(
				identifier >> ':' >> (identifier % ',') >> "->" >> (identifier % ',') >> "@rate=" >> x3::double_ >> '/' >> x3::double_
			) % ',',
			chars::space,
			parse_results
		);

		struct PassengerData {
			const std::string& basename;
			TrackNetwork::NodeID entrance;
			TrackNetwork::NodeID exit;
			double rate;
		};

		std::vector<PassengerData> pdata;

		// make a list of all data for all the statpsgrs
		for (const auto& elem : parse_results) {
			for(const auto& entrance_str : std::get<1>(elem)) {
				for (const auto& exit_str : std::get<2>(elem)) {
					pdata.emplace_back(PassengerData{
						std::get<0>(elem),
						tn.getVertex(entrance_str),
						tn.getVertex(exit_str),
						std::get<3>(elem)*100/std::get<4>(elem)
					});
				}
			}
		};

		// actually make the statpsgrs
		std::transform(begin(pdata), end(pdata), std::back_inserter(statpsgrs), [&](auto&& elem) {
			return StatisticalPassenger{
				elem.basename,
				elem.entrance,
				elem.exit,
				elem.rate
			};
		});

		const bool matches_full = is_match && it == end(passenger_string);

		const bool match_is_good = matches_full; // && statpsgrs.back().getEntryID() == vdesc;

		if (!match_is_good) {
			::util::print_and_throw<std::invalid_argument>([&](auto&& str) {
				str << "bad passenger spec: " << passenger_string << '\n';
			});
		}
	}

	{ auto pindent = dout(DL::DATA_READ1).indentWithTitle("Passenger Info");
		::util::print_container(statpsgrs, dout(DL::DATA_READ1), "\n", "", "");
		dout(DL::DATA_READ1) << '\n';
	}

	return std::make_tuple(std::move(tn), std::move(statpsgrs), true);
}

} // end namepace input
} // end namepace parsing
