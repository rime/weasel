/*! 
@mainpage EzLogger
@author David Maisonave (Axter) (609-345-1007) (<A HREF="http://www.axter.com" TARGET="_top">www.axter.com</A>)\n
Copyright (C) 2006\n
@section DownloadSourceCode Download
Download EzLogger source code using following link:\n
http://axter.com/ezlogger/ezlogger.zip
\n\n
Windows users can optionally use (CHM) help file, which
can be downloaded through the following link:\n
http://axter.com/ezlogger/ezlogger_chm.zip\n
\n
<P> 
\ref DEBUG_ONLY_MACROS\n
\ref VERBOSITY_LEVEL_LOGGING\n
\ref Revision_History\n
</P>

<H1><CENTER>Overview</CENTER></H1> 
\n
EzLogger is a C++ logging library that can easily be added to an implementation.
It can be used with both C++ style stream operators and C style printf functions.\n
For windows, the default policy logs the data to a text file with the same name and
path of the executable, and having a postfix _debuglog.txt.\n
For none windows platform, the default policy logs the data to a file named ezlogger_output.txt.
For details see ezlogger_output_policy\n\n
All logging is performmed through macros.  This allows the logging class to 
automatically receive source file name, source line number, and source function name.
There are four sets of macros.  Two sets have fixed value verbosity level logging,
and the other two sets have variable verbosity level logging.\n
The macro prefix determines what set it belongs to.\n
@verbatim
1. [EZLOGGER] Logging macros that implement in both debug and release version, and use default verbosity level
2. [EZLOGGERVL] Verbosity level logging macros, which implement in both debug and release version
3. [EZDBGONLYLOGGER] Logging macros that only implement in debug version, and use default verbosity level
4. [EZDBGONLYLOGGERVL] Verbosity level logging macros, which implement ONLY in debug version
@endverbatim 
For details on macro usage and description, see ezlogger_macros.hpp\n
For details on debug only macros, see \ref DEBUG_ONLY_MACROS\n
For details on verbosity level logging, see \ref VERBOSITY_LEVEL_LOGGING\n

\n
@section  example_simple_usage Example Usage
@include ezlogger_simple_example.hxx
\n
@section  Details
For detailed EzLogger usage, see ezlogger_macros.hpp\n
*/
#ifndef EZLOGGER_HPP_HEADER_GRD_
#define EZLOGGER_HPP_HEADER_GRD_

/*! @file ezlogger.hpp
@brief ezlogger.hpp defines ezlogger class and implementation.
@section Description
EzLogger is used through macros, which allows source code
file name, line number, and function name to be automatically
inserted into the logging.
*/

#include <iostream>
#include <sstream>
#include <string>
#include <stdarg.h>
#include <stdio.h>

#include "ezlogger_misc.hpp"

namespace axter
{
	/*! @struct levels
	@brief levels is a struct used to hold extended data for the format policy.
	Example Usage
	@verbatim
	EZLOGGERVLSTREAM(axter::levels(axter::log_often, axter::warn, "Xyz Facility")) << somedata << " " << i << std::endl;
	@endverbatim 
	@see ext_data
	*/
	struct levels : public ext_data
	{
		levels(verbosity verbosity_level, severity severity_level, 
			const char* pretty_function = NULL, const char* facility = NULL, 
			const char* tag = NULL, int code = 0)
			:ext_data(severity_level, pretty_function, facility, tag, code), 
			m_verbosity_level(verbosity_level){}
		verbosity m_verbosity_level;
	};

	template<class EZLOGGER_OUTPUT_POLICY = ezlogger_output_policy, 
			class EZLOGGER_FORMAT_POLICY = ezlogger_format_policy,
			class EZLOGGER_VERBOSITY_LEVEL_POLICY = ezlogger_verbosity_level_policy>
	class ezlogger : public EZLOGGER_OUTPUT_POLICY, public EZLOGGER_FORMAT_POLICY, public EZLOGGER_VERBOSITY_LEVEL_POLICY
	{
	public:
		inline ezlogger(const char*filename, int lineno, const char*functionname, 
			verbosity verbosity_level = log_default_verbosity_level, 
			bool isstreamoutput = false, std::ostream* alternate_output = NULL)
			:m_src_file_name(filename), m_src_line_num(lineno), 
			m_src_function_name(functionname), m_verbosity_level(verbosity_level),
			m_levels_format_usage(no_severity), m_alternate_output(alternate_output)
		{
			common_constructor_imp(isstreamoutput);
		}

		inline ezlogger(const char*filename, int lineno, const char*functionname, 
			levels levels_data, bool isstreamoutput = false, std::ostream* alternate_output = NULL)
			:m_src_file_name(filename), m_src_line_num(lineno), 
			m_src_function_name(functionname), m_verbosity_level(levels_data.m_verbosity_level),
			m_levels_format_usage(levels_data), m_alternate_output(alternate_output)
		{
			common_constructor_imp(isstreamoutput);
		}

		template<typename T> inline ezlogger& operator<<(T& Data) {
			if (m_verbosity_level <= EZLOGGER_VERBOSITY_LEVEL_POLICY::get_verbosity_level_tolerance()) 
			{
				if (m_alternate_output)
					(*m_alternate_output) << Data;
				else
					EZLOGGER_OUTPUT_POLICY::get_log_stream() << Data;
			}
			return *this;
		}
		inline ezlogger& operator<<(std::ostream& (*func)(std::ostream&))
		{
			if (m_verbosity_level <= EZLOGGER_VERBOSITY_LEVEL_POLICY::get_verbosity_level_tolerance())
			{
				if (m_alternate_output)
					(*m_alternate_output) << func;
				else
					EZLOGGER_OUTPUT_POLICY::get_log_stream() << func;
			}
			return *this;
		}

		template<class T> void operator()(const T&Data) const{
			if (m_verbosity_level <= EZLOGGER_VERBOSITY_LEVEL_POLICY::get_verbosity_level_tolerance()) 
				log_out(m_src_file_name, m_src_line_num, m_src_function_name, m_levels_format_usage, true, Data);
		}
		template<class T1, class T2>
			void operator()(const T1 &Data1, const T2 &Data2) const{
			if (m_verbosity_level <= EZLOGGER_VERBOSITY_LEVEL_POLICY::get_verbosity_level_tolerance()) 
				log_out(m_src_file_name, m_src_line_num, m_src_function_name, m_levels_format_usage, true, Data1, Data2);
		}
		template<class T1, class T2, class T3>
			void operator()(const T1 &Data1, const T2 &Data2, const T3 &Data3) const{
			if (m_verbosity_level <= EZLOGGER_VERBOSITY_LEVEL_POLICY::get_verbosity_level_tolerance()) 
				log_out(m_src_file_name, m_src_line_num, m_src_function_name, m_levels_format_usage, true, Data1, Data2, Data3);
		}
#ifndef EZLOGGER_EXCLUDE_CPRINT_METHOD
		void cprint(const char * format, ...)
		{
			if (m_verbosity_level <= EZLOGGER_VERBOSITY_LEVEL_POLICY::get_verbosity_level_tolerance()) 
			{
				char Data[4096];
				va_list v;
				va_start(v,format);
				vsnprintf(Data, sizeof(Data), format,v);
				va_end(v);
				log_out(m_src_file_name, m_src_line_num, m_src_function_name, m_levels_format_usage, true, Data);
			}
		}
#endif //EZLOGGER_EXCLUDE_CPRINT_METHOD
		template<class T1>
		void prg_main_arg(int argc, T1 argv)
		{
			if (m_verbosity_level <= EZLOGGER_VERBOSITY_LEVEL_POLICY::get_verbosity_level_tolerance()) 
			{
				std::string Data = "main() arg(s) {";
				for (int i = 0;i < argc;++i) {Data += " arg#" + to_str(i) + " = '" + to_str(argv[i]) + "' ";}
				Data += "}";
				std::string PrgCallSig = "   Program Call Signature --->>   " + to_str(argv[0]);
				for (int ii = 1;ii < argc;++ii) {PrgCallSig += " \"" + to_str(argv[ii]) + "\"";}

				log_out(m_src_file_name, m_src_line_num, m_src_function_name, m_levels_format_usage, true, Data + PrgCallSig);
			}
		}
		void display_stack()
		{
			if (m_verbosity_level <= EZLOGGER_VERBOSITY_LEVEL_POLICY::get_verbosity_level_tolerance()) 
			{
				EZLOGGER_OUTPUT_POLICY::display_stack_main(m_src_line_num);
			}
		}
		bool log_if_fails_verification(bool eval, const char* evaluation)
		{
			if (!eval && m_verbosity_level <= EZLOGGER_VERBOSITY_LEVEL_POLICY::get_verbosity_level_tolerance())
				EZLOGGER_OUTPUT_POLICY::get_log_stream() << EZLOGGER_FORMAT_POLICY::get_log_prefix_format(m_src_file_name, m_src_line_num, m_src_function_name, m_levels_format_usage) << 
					"Failed verification:  '" << evaluation << "'" << std::endl;
			return eval;
		}


		static const std::string to_str(const wchar_t *Data)
		{
			if (!Data) return std::string("Error: Bad pointer");
			const int SizeDest = (int)wcslen(Data);
			char *AnsiStr = new char[SizeDest+1];
			wcstombs(AnsiStr, Data, SizeDest);
			AnsiStr[SizeDest] = 0;
			std::stringstream ss;
			ss << AnsiStr;
			delete [] AnsiStr;
			return ss.str();
		}
		static const std::string to_str(wchar_t *Data)
		{
			if (!Data) return std::string("Error: Bad pointer");
			const int SizeDest = (int)wcslen(Data);
			char *AnsiStr = new char[SizeDest+1];
			wcstombs(AnsiStr, Data, SizeDest);
			AnsiStr[SizeDest] = 0;
			std::stringstream ss;
			ss << AnsiStr;
			delete [] AnsiStr;
			return ss.str();
		}
		static const std::string to_str(const std::wstring &Data){return to_str(Data.c_str());}
		template<class T>
			static const std::string to_str(const T &Data)
		{
			std::stringstream ss;
			ss << Data;
			return ss.str();
		}
		static const std::string to_str(){return std::string();}
	protected:
		const char* m_src_file_name;
		int m_src_line_num;
		const char* m_src_function_name;
		verbosity m_verbosity_level;
		ext_data m_levels_format_usage;
		std::ostream*	m_alternate_output;
		inline void common_constructor_imp(bool isstreamoutput)
		{
			if (isstreamoutput && m_verbosity_level <= EZLOGGER_VERBOSITY_LEVEL_POLICY::get_verbosity_level_tolerance()) 
			{
				if (m_alternate_output) 
					(*m_alternate_output) << EZLOGGER_FORMAT_POLICY::get_log_prefix_format(m_src_file_name, m_src_line_num, m_src_function_name, m_levels_format_usage);
				else
					EZLOGGER_OUTPUT_POLICY::get_log_stream() << EZLOGGER_FORMAT_POLICY::get_log_prefix_format(m_src_file_name, m_src_line_num, m_src_function_name, m_levels_format_usage);
			}
		}

		template<class T>
			static void log_out(const char*FileName, int LineNo, const char*FunctionName, 
			ext_data levels_format_usage_data, bool endline, const T &Data)
		{
			EZLOGGER_OUTPUT_POLICY::get_log_stream() << EZLOGGER_FORMAT_POLICY::get_log_prefix_format(FileName, LineNo, FunctionName, levels_format_usage_data) << to_str(Data);
			if (endline) EZLOGGER_OUTPUT_POLICY::get_log_stream() << std::endl;
		}
		template<class T1, class T2>
		static void log_out(const char*FileName, int LineNo, const char*FunctionName, 
		ext_data levels_format_usage_data, bool endline, const T1 &Data1, const T2 &Data2)
		{
			EZLOGGER_OUTPUT_POLICY::get_log_stream() << EZLOGGER_FORMAT_POLICY::get_log_prefix_format(FileName, LineNo, FunctionName, levels_format_usage_data) << to_str(Data1) << ", "  << to_str(Data2);
			if (endline) EZLOGGER_OUTPUT_POLICY::get_log_stream() << std::endl;
		}
		template<class T1, class T2, class T3>
			static void log_out(const char*FileName, int LineNo, const char*FunctionName, 
			ext_data levels_format_usage_data, bool endline, const T1 &Data1, const T2 &Data2, const T3 &Data3)
		{
			EZLOGGER_OUTPUT_POLICY::get_log_stream() << EZLOGGER_FORMAT_POLICY::get_log_prefix_format(FileName, LineNo, FunctionName, levels_format_usage_data) << to_str(Data1) << ", "  << to_str(Data2) << ", "  << to_str(Data3);
			if (endline) EZLOGGER_OUTPUT_POLICY::get_log_stream() << std::endl;
		}
	};

	class ezfunction_tracker : public ezlogger<>
	{
	public:
		inline ezfunction_tracker(const char*filename, int lineno, const char*functionname, 
			verbosity verbosity_level = log_default_verbosity_level, 
			bool isstreamoutput = false, std::ostream* alternate_output = NULL)
			:ezlogger<>(filename, lineno, functionname, verbosity_level, isstreamoutput, alternate_output)
		{
			operator()("Entering function ", m_src_function_name);
			add_to_stack(m_src_function_name);
		}
		inline ~ezfunction_tracker()
		{
			operator()("Exiting function ", m_src_function_name);
			pop_stack();
		}
	private:
		ezfunction_tracker(const ezfunction_tracker&);
		ezfunction_tracker& operator=(const ezfunction_tracker&);
	};
}

/*! @page  Revision_History Revision History
@section RevisionHistorySection History
\verbatim
$Date: 4/15/11 0:00a $	(Last Update)
$Revision: 7 $			(Last Revision)
$Log : $
$History: ezlogger.hpp $
 * 
 * *****************  Version 7  *****************
 * User: lotem        Date: 4/15/11    Time: 0:00a
 * Updated in $/Logger.root/Logger_1
 * Work-around gcc 4.2.1 on mac.
 * 
 * *****************  Version 6  *****************
 * User: Axter        Date: 3/24/06    Time: 7:23a
 * Updated in $/Logger.root/Logger_1
 * Added stack logging feature
 * 
 * *****************  Version 5  *****************
 * User: Axter        Date: 3/19/06    Time: 12:33a
 * Updated in $/Logger.root/Logger_1
 * Added ezfunction_tracker class, to track function start and exit.
 * 
 * *****************  Version 4  *****************
 * User: Axter        Date: 3/12/06    Time: 9:21p
 * Updated in $/Logger.root/Logger_1
 * 
 * *****************  Version 3  *****************
 * User: Axter        Date: 3/12/06    Time: 12:08a
 * Updated in $/Logger.root/Logger_1
 * 
 * *****************  Version 2  *****************
 * User: Axter        Date: 3/11/06    Time: 10:22a
 * Updated in $/Logger.root/Logger_1
 * Added logic for alternate out put stream
\endverbatim 
*/

#endif //EZLOGGER_HPP_HEADER_GRD_
