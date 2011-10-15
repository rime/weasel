#ifndef EZLOGGER_MISC_HPP_HEADER_GRD_
#define EZLOGGER_MISC_HPP_HEADER_GRD_


namespace axter
{
	enum severity{no_severity, debug, info, status, warn, error, fatal};

	/*! @struct ext_data
		@brief ext_data is a struct used to hold extended data for format policy.
		The current default format policy only uses m_severity_level,
		but the other fields are available for custom format policies that which
		to add additional logging.\n
		To add ext_data to a debug macro, use the derived class axter::levels.
		Example Usage
		@verbatim
		EZLOGGERVLSTREAM(axter::levels(axter::log_often, axter::warn, "Xyz Facility")) << somedata << " " << i << std::endl;
		@endverbatim 
		@see levels
	*/
	struct ext_data
	{
		ext_data(severity severity_level, const char* pretty_function = NULL, const char* facility = NULL, const char* tag = NULL, int code = 0)
			:m_severity_level(severity_level), m_pretty_function(pretty_function), m_facility(facility), m_tag(tag), m_code(code){}
			severity m_severity_level;
			const char* m_pretty_function;
			const char* m_facility;
			const char* m_tag;
			int m_code;
	};
}

#endif //EZLOGGER_MISC_HPP_HEADER_GRD_
