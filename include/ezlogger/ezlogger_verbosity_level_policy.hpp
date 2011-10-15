#ifndef EZLOGGER_VERBOSITY_LEVEL_POLICY_HPP_HEADER_GRD_
#define EZLOGGER_VERBOSITY_LEVEL_POLICY_HPP_HEADER_GRD_

namespace axter
{
	enum verbosity{log_default_verbosity_level = 3, log_verbosity_not_set = 0, log_always = 1, log_often, log_regularly, log_rarely, log_very_rarely};

/*! @struct ezlogger_verbosity_level_policy
@brief Defines implementation for setting initial global verbosity level.
See \ref VERBOSITY_LEVEL_LOGGING detail description of verbosity level logic.
@section Description
This is the default policy for setting the initial state of the global 
verbosity level.
It may be desirable to replace this policy so that the initial state is
set by a registry key value, or by an environmental value.\n
To replace this policy, define EZLOGGER_CUSTOM_VERBOSITY_LEVEL_POLICY 
before including ezlogger.hpp\n
And also include the header for the replacement policy.\n
Example:\n
@verbatim
#define EZLOGGER_CUSTOM_VERBOSITY_LEVEL_POLICY
#include "MyVerbosityLevelPolicy.h"
#include "ezlogger.hpp"
@endverbatim
The replacement policy is required to have a get_verbosity_level_tolerance() method.
That's the only method called by ezlogger.\n
A policy that requires the global verbosity level to 
stay the same for the entire execution of the program, should not
have a public set_verbosity_level_tolerance() method.

*/
	struct ezlogger_verbosity_level_policy
	{
		static inline verbosity get_verbosity_level_tolerance(){return set_or_get_verbosity_level_tolerance(true);}
		static void set_verbosity_level_tolerance(verbosity NewValue){set_or_get_verbosity_level_tolerance(false, NewValue);}
	private:
		inline static verbosity initial_verbosity_level(){return log_default_verbosity_level;}
		static verbosity set_or_get_verbosity_level_tolerance(bool GetLevel, verbosity NewValue = log_default_verbosity_level)
		{
			static verbosity verbosity_level = initial_verbosity_level();
			if (!GetLevel) verbosity_level = NewValue;
			return verbosity_level;
		}
	};
}

#endif //EZLOGGER_VERBOSITY_LEVEL_POLICY_HPP_HEADER_GRD_
