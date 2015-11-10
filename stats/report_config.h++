
#ifndef STATS__REPORT_CONFIG_HPP
#define STATS__REPORT_CONFIG_HPP

namespace stats {

class ReportConfig {
public:
	enum class ReportType {
		PASSENGER_ROUTE_STATS,
	};

	ReportConfig(ReportType report_type)
		: report_type(report_type)
	{ }

	ReportType getReportType() const { return report_type; }

private:
	ReportType report_type;
};

} // end namespace stats

#endif /* STATS__REPORT_CONFIG_HPP */
