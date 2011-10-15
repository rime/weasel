#ifndef EZLOGGER_HEADERS_HPP_HEADER_GRD_
#define EZLOGGER_HEADERS_HPP_HEADER_GRD_

/*! @file ezlogger_headers.hpp
@brief ezlogger_headers.hpp includes all the default policies for ezlogger.
@section Description
Include this header to get all the required default headers to perform logging.
To replace one or more policies, the developer should create a similar header list,
and replace the header for the target policies with the custom policies.
*/

#include "ezlogger_misc.hpp"
#include "ezlogger_output_policy.hpp"
#include "ezlogger_format_policy.hpp"
#include "ezlogger_verbosity_level_policy.hpp"
#include "ezlogger.hpp"
#include "ezlogger_macros.hpp"

#endif //EZLOGGER_HEADERS_HPP_HEADER_GRD_
