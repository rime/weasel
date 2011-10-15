#ifndef EZLOGGER_MACROS_HPP_HEADER_GRD_
#define EZLOGGER_MACROS_HPP_HEADER_GRD_

/*! \file ezlogger_macros.hpp
\brief ezlogger_macros.hpp defines default macros for ezlogger class.
@section Description
EzLogger is used through macros, which allows source code
file name, line number, and function name to be automatically
inserted into the logging.
*/

#if defined(_DEBUG) || defined(_DEBUG_)
/*! @def EZLOGGER_IMPLEMENT_DEBUGLOGGING
@brief This macro is defined if _DEBUG or _DEBUG_ is defined.
@note
Optionally, EZLOGGER_IMPLEMENT_DEBUGLOGGING can be defined
by the developer so-as implement debug-logging in release
version.
*/
#define EZLOGGER_IMPLEMENT_DEBUGLOGGING
#endif

/*! @page FUNCTIONMACRO FUNCTION Macro
@def __FUNCTION__
@brief The __FUNCTION_ macro is defined by some implementations.
@section Description
The __FUNCTION_ is supported by some compilers like VC++ 7.x, 8.x and GNU 3.x, 4.x.
Compilers that support this macro replace the macro with the function name
calling the macro.
By default, for compilers that do not support this macro, __FUNCTION__
is replaced with an empty string by EzLogger.\n\n
For non-supporting compilers, optionally, the developer could choose to define this macro
to a common name used in functions to declare a local variable with the name of the
function.
@verbatim
#define __FUNCTION__ function_name
#include "ezlogger.hpp"
void Myfunction()
{
   const char* function_name = "Myfunction";
}
@endverbatim 
If this approach is used, than all functions that call an EzLogger macro within
that translation unit would have to declare the function_name variable.
Moreover, this define would have to be included in each translation unit that
requires this approach.
*/
// @cond INCLUDE_ALL_OBJS_
#if !defined(__FUNCTION__) && !defined(__GNUC__) //The GNU compiler and VC++ 7.x supports this macro
#define __FUNCTION__ ""  //If compiler does not support it, then use empty string
#endif //!defined(__FUNCTION__) && !defined(__GNUC__)
// @endcond 

////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*! @name Logging macros that implement in both debug and release version, and use default verbosity level */
//@{
////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*! @def EZLOGGER
@brief A macro used to log 1, 2, or 3 arguments.
@section ExampleUsage Example Usage
@verbatim
EZLOGGER(MyStr);
EZLOGGER(a, b);
EZLOGGER(a, b, c);
@endverbatim 
The argument can be any type that stringstream can convert to a string.
@note
EZLOGGER has a fixed verbosity level logic, and it's implemented and called on
both debug and release version.
@see EZLOGGER, EZLOGGERVL, EZDBGONLYLOGGER, EZDBGONLYLOGGERVL
*/
#define EZLOGGER axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__)

/*! @def EZLOGGERSTREAM
@brief A macro use for logging with iostream type syntax
@section ExampleUsage Example Usage
@verbatim
EZLOGGERSTREAM << "This is data1 " << data1 << "This is data2 " << data2 << std::endl;
@endverbatim 
@note
EZLOGGERSTREAM has a fixed verbosity level logic, and it's implemented and called on
both debug and release version.
@see EZLOGGERSTREAM, EZLOGGERVLSTREAM, EZDBGONLYLOGGERSTREAM, EZDBGONLYLOGGERVLSTREAM
*/
#define EZLOGGERSTREAM axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__, axter::log_default_verbosity_level, true)

/*! @def EZLOGGERSTREAM2
@brief A macro to use an alternate output stream
@section ExampleUsage Example Usage
@verbatim
EZLOGGERSTREAM2(std::cout) << "This is data1 " << data1 << "This is data2 " << data2 << std::endl;
@endverbatim 
@note
EZLOGGERSTREAM2 has a fixed verbosity level logic, and it's implemented and called on
both debug and release version.
@see EZLOGGERSTREAM, EZLOGGERVLSTREAM, EZDBGONLYLOGGERSTREAM, EZDBGONLYLOGGERVLSTREAM
*/
#define EZLOGGERSTREAM2(alterante_stream) axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__, axter::log_default_verbosity_level, true, &alterante_stream)

/*! @def EZLOGGERPRINT
@brief A macro use for logging with C-Style printf syntax.
@section ExampleUsage Example Usage
@verbatim
EZLOGGERPRINT("i = %i and somedata = %s", i, somedata.c_str());
@endverbatim 
@note
EZLOGGERPRINT has a fixed verbosity level logic, and it's implemented and called on
both debug and release version.
@see EZLOGGERPRINT, EZLOGGERVLPRINT, EZDBGONLYLOGGERPRINT, EZDBGONLYLOGGERVLPRINT
*/
#define EZLOGGERPRINT axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__).cprint

/*! @def EZLOGGERVARIFY
@brief A macro that logs failed verifications and asserts
@section ExampleUsage Example Usage
@verbatim
bool SomeConditionVar = true;
EZLOGGERVARIFY(SomeConditionVar == false);
@endverbatim
In the debug version, an assertion is done on the results.
@note
EZLOGGERVARIFY has a fixed verbosity level logic, and it's implemented and called on
both debug and release version.
@see EZLOGGERVARIFY, EZLOGGERVLVARIFY
*/
#ifdef EZLOGGER_IMPLEMENT_DEBUGLOGGING
#define EZLOGGERVARIFY(x) assert(axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__, axter::log_default_verbosity_level).log_if_fails_verification((x), #x))
#else
#define EZLOGGERVARIFY(x) axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__, axter::log_default_verbosity_level).log_if_fails_verification((x), #x)
#endif

/*! @def EZLOGGERVAR
@brief An easy to use macro to log both variable name and value.
@section ExampleUsage Example Usage
@verbatim
EZLOGGERVAR(somevariable);
@endverbatim 
Only takes one argument, but it will work with the following syntax:
@verbatim
bool SomeConditionVar = true;
EZLOGGERVAR(SomeConditionVar == false);
@endverbatim 
@note
EZLOGGERVAR has a fixed verbosity level logic, and it's implemented and called on
both debug and release version.
@see EZLOGGERVAR, EZLOGGERVLVAR, EZDBGONLYLOGGERVAR, EZDBGONLYLOGGERVLVAR
*/
#define EZLOGGERVAR(x) axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__)(axter::ezlogger<>::to_str(#x) + axter::ezlogger<>::to_str(" = '") + axter::ezlogger<>::to_str(x) + axter::ezlogger<>::to_str("'")), x

/*! @def EZLOGGERMARKER
@brief The EZLOGGERMARKER macro is used as a maker for condition logic.
@section ExampleUsage Example Usage
@verbatim
EZLOGGERMARKER;
@endverbatim 
This macro takes not arguments, and it's main purpose is to log the source
code path of an executable.
@note
EZLOGGERMARKER has a fixed verbosity level logic, and it's implemented and called on
both debug and release version.
@see EZLOGGERMARKER, EZLOGGERVLMARKER, EZDBGONLYLOGGERMARKER, EZDBGONLYLOGGERVLMARKER
*/
#define EZLOGGERMARKER axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__)("marker")

/*! @def EZLOGGERFUNCTRACKER
@brief The EZLOGGERFUNCTRACKER macro is used to trace the entering and exiting of a function.
@section ExampleUsage Example Usage
@verbatim
void FunctFoo()
{
EZLOGGERFUNCTRACKER;
//Function Foo code here....
}
@endverbatim 
@note
EZLOGGERFUNCTRACKER has a fixed verbosity level logic, and it's implemented and called on
both debug and release version.
@see EZLOGGERFUNCTRACKER, EZLOGGERVLFUNCTRACKER, EZDBGONLYLOGGERFUNCTRACKER, EZDBGONLYLOGGERVLFUNCTRACKER
*/
#define EZLOGGERFUNCTRACKER axter::ezfunction_tracker my_function_tracker##__LINE__(__FILE__, __LINE__, __FUNCTION__)

/*! @def EZLOGGERDISPLAY_STACK
@brief The EZLOGGERDISPLAY_STACK macro is use to display the current stack.
Only functions that have used the EZLOGGERFUNCTRACKER or associated xxxxFUNCTRACKER macro are included in the stack.
@section ExampleUsage Example Usage
@verbatim
void FunctFoo()
{
	EZLOGGERFUNCTRACKER;
	EZLOGGERDISPLAY_STACK;  //Display includes current function
}
@endverbatim 
@note
EZLOGGERDISPLAY_STACK has a fixed verbosity level logic, and it's implemented and called on
both debug and release version.
@see EZLOGGERDISPLAY_STACK, EZLOGGERVLDISPLAY_STACK, EZDBGONLYLOGGERDISPLAY_STACK, EZDBGONLYLOGGERVLDISPLAY_STACK
*/
#define EZLOGGERDISPLAY_STACK axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__).display_stack();

/*! @def EZLOGGER_PRG_MAIN_ARG
@brief The EZLOGGER_PRG_MAIN_ARG macro is used to log the program's input parameters.
@section ExampleUsage Example Usage
@verbatim
int main(int argc, char**argv)
{
	EZLOGGER_PRG_MAIN_ARG(argc, argv);
@endverbatim 
@note
EZLOGGER_PRG_MAIN_ARG has a fixed verbosity level logic, and it's implemented and called on
both debug and release version.
@see EZLOGGER_PRG_MAIN_ARG, EZLOGGERVL_PRG_MAIN_ARG, EZDBGONLYLOGGER_PRG_MAIN_ARG, EZDBGONLYLOGGERVL_PRG_MAIN_ARG
*/
#define EZLOGGER_PRG_MAIN_ARG(argc, argv) axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__).prg_main_arg(argc, argv)
//@}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*! @name Verbosity level logging macros, which implement in both debug and release version */
//@{
////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*! @def EZLOGGERVL
@brief A macro used to log 1, 2, or 3 arguments.
@section ExampleUsage Example Usage
@verbatim
EZLOGGERVL(axter::log_often)(MyStr);
EZLOGGERVL(axter::log_rarely)(a, b);
EZLOGGERVL(axter::log_very_rarely)(a, b, c);
@endverbatim 
The argument can be any type that stringstream can convert to a string.
@note
EZLOGGERVL has verbosity level logic, and it's implemented and called on
both debug and release version.
@see EZLOGGER, EZLOGGERVL, EZDBGONLYLOGGER, EZDBGONLYLOGGERVL
*/
#define EZLOGGERVL(verbosity_level) axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__, verbosity_level)

/*! @def EZLOGGERVLSTREAM
@brief A macro use for logging with iostream type syntax
@section ExampleUsage Example Usage
@verbatim
EZLOGGERVLSTREAM(axter::log_rarely) << "This is data1 " << data1 << "This is data2 " << data2 << std::endl;
@endverbatim 
@note
EZLOGGERVLSTREAM has verbosity level logic, and it's implemented and called on
both debug and release version.
@see EZLOGGERSTREAM, EZLOGGERVLSTREAM, EZDBGONLYLOGGERSTREAM, EZDBGONLYLOGGERVLSTREAM
*/
#define EZLOGGERVLSTREAM(verbosity_level) axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__, verbosity_level, true)

/*! @def EZLOGGERVLSTREAM2
@brief A macro to use an alternate output stream
@section ExampleUsage Example Usage
@verbatim
EZLOGGERVLSTREAM2(axter::log_rarely, std::cerr) << "This is data1 " << data1 << "This is data2 " << data2 << std::endl;
@endverbatim 
@note
EZLOGGERVLSTREAM2 has verbosity level logic, and it's implemented and called on
both debug and release version.
@see EZLOGGERSTREAM, EZLOGGERVLSTREAM, EZDBGONLYLOGGERSTREAM, EZDBGONLYLOGGERVLSTREAM2
*/
#define EZLOGGERVLSTREAM2(verbosity_level, alterante_stream) axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__, verbosity_level, true, &alterante_stream)

/*! @def EZLOGGERVLPRINT
@brief A macro use for logging with C-Style printf syntax.
@section ExampleUsage Example Usage
@verbatim
EZLOGGERVLPRINT(axter::log_often)("i = %i and somedata = %s", i, somedata.c_str());
@endverbatim 
@note
EZLOGGERVLPRINT has verbosity level logic, and it's implemented and called on
both debug and release version.
@see EZLOGGERPRINT, EZLOGGERVLPRINT, EZDBGONLYLOGGERPRINT, EZDBGONLYLOGGERVLPRINT
*/
#define EZLOGGERVLPRINT(verbosity_level) axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__, verbosity_level).cprint

/*! @def EZLOGGERVLVARIFY
@brief A macro that logs failed verifications and asserts
@section ExampleUsage Example Usage
@verbatim
bool SomeConditionVar = true;
EZLOGGERVLVARIFY(axter::log_always, SomeConditionVar == false);
@endverbatim
In the debug version, an assertion is done on the results.
@note
EZLOGGERVLVARIFY has verbosity level logic, and it's implemented and called on
both debug and release version.
@see EZLOGGERVARIFY, EZLOGGERVLVARIFY
*/
#ifdef EZLOGGER_IMPLEMENT_DEBUGLOGGING
#define EZLOGGERVLVARIFY(verbosity_level, x) assert(axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__, verbosity_level).log_if_fails_verification((x), #x))
#else
#define EZLOGGERVLVARIFY(verbosity_level, x) axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__, verbosity_level).log_if_fails_verification((x), #x)
#endif

/*! @def EZLOGGERVLVAR
@brief An easy to use macro to log both variable name and value.
@section ExampleUsage Example Usage
@verbatim
EZLOGGERVLVAR(axter::log_often, somevariable);
@endverbatim 
Only takes one argument, but it will work with the following syntax:
@verbatim
bool SomeConditionVar = true;
EZLOGGERVLVAR(axter::log_always, SomeConditionVar == false);
@endverbatim 
@note
EZLOGGERVLVAR has verbosity level logic, and it's implemented and called on
both debug and release version.
@see EZLOGGERVAR, EZLOGGERVLVAR, EZDBGONLYLOGGERVAR, EZDBGONLYLOGGERVLVAR
*/
#define EZLOGGERVLVAR(verbosity_level, x) axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__, verbosity_level)(axter::ezlogger<>::to_str(#x) + axter::ezlogger<>::to_str(" = '") + axter::ezlogger<>::to_str(x) + axter::ezlogger<>::to_str("'")),x

/*! @def EZLOGGERVLMARKER
@brief The EZLOGGERVLMARKER macro is used as a maker for condition logic.
@section ExampleUsage Example Usage
@verbatim
EZLOGGERVLMARKER(axter::log_often);
@endverbatim 
This macro takes not arguments, and it's main purpose is to log the source
code path of an executable.
@note
EZLOGGERVLMARKER has verbosity level logic, and it's implemented and called on
both debug and release version.
@see EZLOGGERMARKER, EZLOGGERVLMARKER, EZDBGONLYLOGGERMARKER, EZDBGONLYLOGGERVLMARKER
*/
#define EZLOGGERVLMARKER(verbosity_level) axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__, verbosity_level)("marker")

/*! @def EZLOGGERVLFUNCTRACKER
@brief The EZLOGGERVLFUNCTRACKER macro is used to trace the entering and exiting of a function.
@section ExampleUsage Example Usage
@verbatim
void FunctFoo()
{
    EZLOGGERVLFUNCTRACKER(axter::log_often);
	//Function Foo code here....
}
@endverbatim 
@note
EZLOGGERVLFUNCTRACKER has verbosity level logic, and it's implemented and called on
both debug and release version.
@see EZLOGGERFUNCTRACKER, EZLOGGERVLFUNCTRACKER, EZDBGONLYLOGGERFUNCTRACKER, EZDBGONLYLOGGERVLFUNCTRACKER
*/
#define EZLOGGERVLFUNCTRACKER(verbosity_level) axter::ezfunction_tracker my_function_tracker##__LINE__(__FILE__, __LINE__, __FUNCTION__, verbosity_level)

/*! @def EZLOGGERVLDISPLAY_STACK
@brief The EZLOGGERVLDISPLAY_STACK macro is use to display the current stack.
Only functions that have used the EZLOGGERFUNCTRACKER or associated xxxxFUNCTRACKER macro are included in the stack.
@section ExampleUsage Example Usage
@verbatim
void FunctFoo()
{
	EZLOGGERVLFUNCTRACKER(axter::log_often);
	EZLOGGERVLDISPLAY_STACK(axter::log_often);  //Display includes current function
}
@endverbatim 
@note
EZLOGGERVLDISPLAY_STACK has verbosity level logic, and it's implemented and called on
both debug and release version.
@see EZLOGGERDISPLAY_STACK, EZLOGGERVLDISPLAY_STACK, EZDBGONLYLOGGERDISPLAY_STACK, EZDBGONLYLOGGERVLDISPLAY_STACK
*/
#define EZLOGGERVLDISPLAY_STACK(verbosity_level) axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__, verbosity_level).display_stack();

/*! @def EZLOGGERVL_PRG_MAIN_ARG
@brief The EZLOGGERVL_PRG_MAIN_ARG macro is used to log the program's input parameters.
@section ExampleUsage Example Usage
@verbatim
int main(int argc, char**argv)
{
EZLOGGERVL_PRG_MAIN_ARG(axter::log_always, argc, argv);
@endverbatim 
@note
EZLOGGERVL_PRG_MAIN_ARG has verbosity level logic, and it's implemented and called on
both debug and release version.
@see EZLOGGER_PRG_MAIN_ARG, EZLOGGERVL_PRG_MAIN_ARG, EZDBGONLYLOGGER_PRG_MAIN_ARG, EZDBGONLYLOGGERVL_PRG_MAIN_ARG
*/
#define EZLOGGERVL_PRG_MAIN_ARG(verbosity_level, argc, argv) axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__, verbosity_level).prg_main_arg(argc, argv)
//@}


////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Logging macros that only implement in debug version
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef EZLOGGER_IMPLEMENT_DEBUGLOGGING
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*! @name Logging macros that only implement in debug version, and use default verbosity level */
//@{
////////////////////////////////////////////////////////////////////////////////////////////////////////////


/*! @def EZDBGONLYLOGGER
@brief A macro used to log 1, 2, or 3 arguments.
@section ExampleUsage Example Usage
@verbatim
EZDBGONLYLOGGER(MyStr);
EZDBGONLYLOGGER(a, b);
EZDBGONLYLOGGER(a, b, c);
@endverbatim 
The argument can be any type that stringstream can convert to a string.
*/
#define EZDBGONLYLOGGER axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__)

/*! @def EZDBGONLYLOGGERSTREAM
@brief A macro use for logging with iostream type syntax
@section ExampleUsage Example Usage
@verbatim
EZDBGONLYLOGGERSTREAM << "This is data1 " << data1 << "This is data2 " << data2 << std::endl;
@endverbatim 
*/
#define EZDBGONLYLOGGERSTREAM axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__, axter::log_default_verbosity_level, true)

/*! @def EZDBGONLYLOGGERSTREAM2
@brief A macro to use an alternate output stream
@section ExampleUsage Example Usage
@verbatim
EZDBGONLYLOGGERSTREAM2(std::cout) << "This is data1 " << data1 << "This is data2 " << data2 << std::endl;
@endverbatim 
@note
EZDBGONLYLOGGERSTREAM2 has a fixed verbosity level logic, and it's implemented and called only on debug version.
@see EZLOGGERSTREAM, EZLOGGERVLSTREAM, EZDBGONLYLOGGERSTREAM, EZDBGONLYLOGGERVLSTREAM
*/
#define EZDBGONLYLOGGERSTREAM2(alterante_stream) axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__, axter::log_default_verbosity_level, true, &alterante_stream)

/*! @def EZDBGONLYLOGGERPRINT
@brief A macro use for logging with C-Style printf syntax.
@section ExampleUsage Example Usage
@verbatim
EZDBGONLYLOGGERPRINT("i = %i and somedata = %s", i, somedata.c_str());
@endverbatim 
*/
#define EZDBGONLYLOGGERPRINT axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__).cprint

/*! @def EZDBGONLYLOGGERVAR
@brief An easy to use macro to log both variable name and value.
@section ExampleUsage Example Usage
@verbatim
EZDBGONLYLOGGERVAR(somevariable);
@endverbatim 
Only takes one argument, but it will work with the following syntax:
@verbatim
bool SomeConditionVar = true;
EZDBGONLYLOGGERVAR(SomeConditionVar == false);
@endverbatim 
*/
#define EZDBGONLYLOGGERVAR(x) axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__)(axter::ezlogger<>::to_str(#x) + axter::ezlogger<>::to_str(" = '") + axter::ezlogger<>::to_str(x) + axter::ezlogger<>::to_str("'"))

/*! @def EZDBGONLYLOGGERMARKER
@brief The EZDBGONLYLOGGERMARKER macro is used as a maker for condition logic.
@section ExampleUsage Example Usage
@verbatim
EZDBGONLYLOGGERMARKER;
@endverbatim 
This macro takes not arguments, and it's main purpose is to log the source
code path of an executable.
*/
#define EZDBGONLYLOGGERMARKER axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__)("marker")

/*! @def EZDBGONLYLOGGERFUNCTRACKER
@brief The EZDBGONLYLOGGERFUNCTRACKER macro is used to trace the entering and exiting of a function.
@section ExampleUsage Example Usage
@verbatim
EZDBGONLYLOGGERFUNCTRACKER;
void FunctFoo()
{
	EZDBGONLYLOGGERFUNCTRACKER;
	//Function Foo code here....
}
@endverbatim 
@note
EZDBGONLYLOGGERFUNCTRACKER has a fixed verbosity level logic, and it's implemented and called on
both debug and release version.
@see EZLOGGERFUNCTRACKER, EZLOGGERVLFUNCTRACKER, EZDBGONLYLOGGERFUNCTRACKER, EZDBGONLYLOGGERVLFUNCTRACKER
*/
#define EZDBGONLYLOGGERFUNCTRACKER axter::ezfunction_tracker my_function_tracker##__LINE__(__FILE__, __LINE__, __FUNCTION__)

/*! @def EZDBGONLYLOGGERDISPLAY_STACK
@brief The EZDBGONLYLOGGERDISPLAY_STACK macro is use to display the current stack.
Only functions that have used the EZLOGGERFUNCTRACKER or associated xxxxFUNCTRACKER macro are included in the stack.
@section ExampleUsage Example Usage
@verbatim
void FunctFoo()
{
	EZLOGGERFUNCTRACKER;
	EZDBGONLYLOGGERDISPLAY_STACK;  //Display includes current function
}
@endverbatim 
@note
EZDBGONLYLOGGERDISPLAY_STACK has a fixed verbosity level logic, and it's implemented and called on
both debug and release version.
@see EZLOGGERDISPLAY_STACK, EZLOGGERVLDISPLAY_STACK, EZDBGONLYLOGGERDISPLAY_STACK, EZDBGONLYLOGGERVLDISPLAY_STACK
*/
#define EZDBGONLYLOGGERDISPLAY_STACK axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__).display_stack();

/*! @def EZDBGONLYLOGGER_PRG_MAIN_ARG
@brief The EZDBGONLYLOGGER_PRG_MAIN_ARG macro is used to log the program's input parameters.
@section ExampleUsage Example Usage
@verbatim
int main(int argc, char**argv)
{
EZDBGONLYLOGGER_PRG_MAIN_ARG(argc, argv);
@endverbatim 
*/
#define EZDBGONLYLOGGER_PRG_MAIN_ARG(argc, argv) axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__).prg_main_arg(argc, argv)
//@}


////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*! @name Verbosity level logging macros, which implement ONLY in debug version */
//@{
////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*! @def EZDBGONLYLOGGERVL
@brief A macro used to log 1, 2, or 3 arguments.
@section ExampleUsage Example Usage
@verbatim
EZDBGONLYLOGGERVL(axter::log_often)(MyStr);
EZDBGONLYLOGGERVL(axter::log_rarely)(a, b);
EZDBGONLYLOGGERVL(axter::log_very_rarely)(a, b, c);
@endverbatim 
The argument can be any type that stringstream can convert to a string.
@see EZLOGGER, EZLOGGERVL, EZDBGONLYLOGGER, EZDBGONLYLOGGERVL
*/
#define EZDBGONLYLOGGERVL(verbosity_level) axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__, verbosity_level)

/*! @def EZDBGONLYLOGGERVLSTREAM
@brief A macro use for logging with iostream type syntax
@section ExampleUsage Example Usage
@verbatim
EZDBGONLYLOGGERVLSTREAM(axter::log_rarely) << "This is data1 " << data1 << "This is data2 " << data2 << std::endl;
@endverbatim 
@see EZLOGGERSTREAM, EZLOGGERVLSTREAM, EZDBGONLYLOGGERSTREAM, EZDBGONLYLOGGERVLSTREAM
*/
#define EZDBGONLYLOGGERVLSTREAM(verbosity_level) axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__, verbosity_level, true)

/*! @def EZDBGONLYLOGGERVLSTREAM2
@brief A macro to use an alternate output stream
@section ExampleUsage Example Usage
@verbatim
EZDBGONLYLOGGERVLSTREAM2(axter::log_rarely, std::cerr) << "This is data1 " << data1 << "This is data2 " << data2 << std::endl;
@endverbatim 
@see EZLOGGERSTREAM, EZLOGGERVLSTREAM, EZDBGONLYLOGGERSTREAM, EZDBGONLYLOGGERVLSTREAM
*/
#define EZDBGONLYLOGGERVLSTREAM2(verbosity_level, alterante_stream) axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__, verbosity_level, true, &alterante_stream)

/*! @def EZDBGONLYLOGGERVLPRINT
@brief A macro use for logging with C-Style printf syntax.
@section ExampleUsage Example Usage
@verbatim
EZDBGONLYLOGGERVLPRINT(axter::log_often)("i = %i and somedata = %s", i, somedata.c_str());
@endverbatim 
@see EZLOGGERPRINT, EZLOGGERVLPRINT, EZDBGONLYLOGGERPRINT, EZDBGONLYLOGGERVLPRINT
*/
#define EZDBGONLYLOGGERVLPRINT(verbosity_level) axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__, verbosity_level).cprint

/*! @def EZDBGONLYLOGGERVLVAR
@brief An easy to use macro to log both variable name and value.
@section ExampleUsage Example Usage
@verbatim
EZDBGONLYLOGGERVLVAR(axter::log_often, somevariable);
@endverbatim 
Only takes one argument, but it will work with the following syntax:
@verbatim
bool SomeConditionVar = true;
EZDBGONLYLOGGERVLVAR(axter::log_always, SomeConditionVar == false);
@endverbatim 
@see EZLOGGERVAR, EZLOGGERVLVAR, EZDBGONLYLOGGERVAR, EZDBGONLYLOGGERVLVAR
*/
#define EZDBGONLYLOGGERVLVAR(verbosity_level, x) axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__, verbosity_level)(axter::ezlogger<>::to_str(#x) + axter::ezlogger<>::to_str(" = '") + axter::ezlogger<>::to_str(x) + axter::ezlogger<>::to_str("'")),x

/*! @def EZDBGONLYLOGGERVLMARKER
@brief The EZDBGONLYLOGGERVLMARKER macro is used as a maker for condition logic.
@section ExampleUsage Example Usage
@verbatim
EZDBGONLYLOGGERVLMARKER(axter::log_often);
@endverbatim 
This macro takes not arguments, and it's main purpose is to log the source
code path of an executable.
@see EZLOGGERMARKER, EZLOGGERVLMARKER, EZDBGONLYLOGGERMARKER, EZDBGONLYLOGGERVLMARKER
*/
#define EZDBGONLYLOGGERVLMARKER(verbosity_level) axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__, verbosity_level)("marker")

/*! @def EZDBGONLYLOGGERVLFUNCTRACKER
@brief The EZDBGONLYLOGGERVLFUNCTRACKER macro is used to trace the entering and exiting of a function.
@section ExampleUsage Example Usage
@verbatim
void FunctFoo()
{
	EZDBGONLYLOGGERVLFUNCTRACKER(axter::log_often);
	//Function Foo code here....
}
@endverbatim 
@see EZLOGGERFUNCTRACKER, EZLOGGERVLFUNCTRACKER, EZDBGONLYLOGGERFUNCTRACKER, EZDBGONLYLOGGERVLFUNCTRACKER
*/
#define EZDBGONLYLOGGERVLFUNCTRACKER(verbosity_level) axter::ezfunction_tracker my_function_tracker##__LINE__(__FILE__, __LINE__, __FUNCTION__, verbosity_level)

/*! @def EZDBGONLYLOGGERVLDISPLAY_STACK
@brief The EZDBGONLYLOGGERVLDISPLAY_STACK macro is use to display the current stack.
Only functions that have used the EZLOGGERFUNCTRACKER or associated xxxxFUNCTRACKER macro are included in the stack.
@section ExampleUsage Example Usage
@verbatim
void FunctFoo()
{
	EZDBGONLYLOGGERVLFUNCTRACKER(axter::log_often);
	EZDBGONLYLOGGERVLDISPLAY_STACK(axter::log_often);  //Display includes current function
}
@endverbatim 
@see EZLOGGERDISPLAY_STACK, EZLOGGERVLDISPLAY_STACK, EZDBGONLYLOGGERDISPLAY_STACK, EZDBGONLYLOGGERVLDISPLAY_STACK
*/
#define EZDBGONLYLOGGERVLDISPLAY_STACK(verbosity_level) axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__, verbosity_level).display_stack();

/*! @def EZDBGONLYLOGGERVL_PRG_MAIN_ARG
@brief The EZDBGONLYLOGGERVL_PRG_MAIN_ARG macro is used to log the program's input parameters.
@section ExampleUsage Example Usage
@verbatim
int main(int argc, char**argv)
{
EZDBGONLYLOGGERVL_PRG_MAIN_ARG(axter::log_always, argc, argv);
@endverbatim 
@see EZLOGGER_PRG_MAIN_ARG, EZLOGGERVL_PRG_MAIN_ARG, EZDBGONLYLOGGER_PRG_MAIN_ARG, EZDBGONLYLOGGERVL_PRG_MAIN_ARG
*/
#define EZDBGONLYLOGGERVL_PRG_MAIN_ARG(verbosity_level, argc, argv) axter::ezlogger<>(__FILE__, __LINE__, __FUNCTION__, verbosity_level).prg_main_arg(argc, argv)
//@}


#else //EZLOGGER_IMPLEMENT_DEBUGLOGGING

#define EZDBGONLYLOGGER if (1);else 
#define EZDBGONLYLOGGERSTREAM if (1);else std::cout
#define EZDBGONLYLOGGERSTREAM2(alterante_stream) if (1);else std::cout
#define EZDBGONLYLOGGERPRINT if (1);else 
#define EZDBGONLYLOGGERVAR(x)  
#define EZDBGONLYLOGGERMARKER  
#define EZDBGONLYLOGGERFUNCTRACKER  
#define EZDBGONLYLOGGERDISPLAY_STACK  
#define EZDBGONLYLOGGER_PRG_MAIN_ARG(argc, argv) 

#define EZDBGONLYLOGGERVL(verbosity_level) if (1);else std::cout
#define EZDBGONLYLOGGERVLSTREAM(verbosity_level) if (1);else std::cout
#define EZDBGONLYLOGGERVLSTREAM2(verbosity_level, alterante_stream) if (1);else std::cout
#define EZDBGONLYLOGGERVLPRINT(verbosity_level)  if (1);else 
#define EZDBGONLYLOGGERVLVAR(verbosity_level, x)
#define EZDBGONLYLOGGERVLMARKER(verbosity_level)
#define EZDBGONLYLOGGERVLFUNCTRACKER(verbosity_level)
#define EZDBGONLYLOGGERVLDISPLAY_STACK(verbosity_level)
#define EZDBGONLYLOGGERVL_PRG_MAIN_ARG(verbosity_level, argc, argv)

#endif //EZLOGGER_IMPLEMENT_DEBUGLOGGING

#ifdef _WIN32
/*! @def EZLOGGER_BEEP_AND_SLEEP
@brief The EZLOGGER_BEEP_AND_SLEEP macro is used to cause a delay in program execution.
@section ExampleUsage Example Usage
@verbatim
int main(int argc, char**argv)
{
EZLOGGER_BEEP_AND_SLEEP(45);
@endverbatim 
This macro can be used in any function, and it's main purpose is to allow the developer
time to attach to a running process for debugging.
The macro will beep twice before going to sleep mode, and than beep once
in the middle of sleep mode, and than beep three times when complete.
*/
#define EZLOGGER_BEEP_AND_SLEEP(qty_seconds) Beep(100, 300);Beep(50, 300);Sleep((qty_seconds/2)*1000);Beep(50, 300);Sleep((qty_seconds/2)*1000);Beep(50, 200);Beep(50, 200);Beep(50, 400);EZLOGGER("BeepAndSleep")
#endif //_WIN32

/*!
\n
@section  example_usage Example Usage
@include ezlogger_example.hxx
*/

#endif //EZLOGGER_MACROS_HPP_HEADER_GRD_
