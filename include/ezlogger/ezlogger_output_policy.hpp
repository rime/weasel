#ifndef EZLOGGER_OUTPUT_POLICY_HPP_HEADER_GRD_
#define EZLOGGER_OUTPUT_POLICY_HPP_HEADER_GRD_

#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

namespace axter
{
	/*! @class ezlogger_output_policy
	@brief This struct defines the get_log_stream function, which 
			is used to get the stream.
	@note
	get_log_stream is the only method required to be implemented by a custom output policy.
	*/
	class ezlogger_output_policy
	{
#ifdef _WIN32
		inline static std::string GetRunningProgramName()
		{
			char ModuleFileName[2048] = "";
			GetModuleFileNameA(NULL, ModuleFileName, sizeof(ModuleFileName));
			return ModuleFileName;
		}
		inline static std::string GetFileName()
		{
			static std::string ModuleFileName = GetRunningProgramName() + "_debuglog.txt";
			return ModuleFileName;
		}
#ifndef EZLOGGER_OUTPUT_FILENAME
#define EZLOGGER_OUTPUT_FILENAME GetFileName()
#endif //EZLOGGER_OUTPUT_FILENAME
#endif //_WIN32


#ifndef EZLOGGER_OUTPUT_FILENAME
#define EZLOGGER_OUTPUT_FILENAME "ezlogger_output.txt"
#endif
	
	public:
		inline static std::ostream& get_log_stream()
		{
			static const std::string FileName = EZLOGGER_OUTPUT_FILENAME;
#ifdef EZLOGGER_REPLACE_EXISTING_LOGFILE_
			static std::ofstream logfile(FileName.c_str(), std::ios_base::out);
#else
			static std::ofstream logfile(FileName.c_str(),  std::ios_base::out | std::ios_base::app);
#endif
			static bool logfile_is_open = logfile.is_open();
			if (logfile_is_open) return logfile;
			return std::cout;
		}
		inline static void add_to_stack(const std::string& function_name)
		{
			get_stack().push_back(function_name);
		}
		inline static void pop_stack()
		{
			get_stack().pop_back();
		}
	protected:
		inline static void display_stack_main(int LineNo = 0)
		{
			get_log_stream() << "Begin Stack -------------------- (" << LineNo << ")" <<  std::endl;
			for(std::vector<std::string>::iterator i = get_stack().begin();
				i != get_stack().end();++i)
			{
				get_log_stream() << *i << std::endl;
			}
			get_log_stream() << "End Stack --------------------" << std::endl;
		}
	private:
		inline static std::vector<std::string>& get_stack()
		{
			static std::vector<std::string> my_stack;
			return my_stack;
		}
	};
}


#endif //EZLOGGER_OUTPUT_POLICY_HPP_HEADER_GRD_
