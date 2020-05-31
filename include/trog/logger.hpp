#ifndef trog_guard_logger_HPP
#define trog_guard_logger_HPP
//
#include <iostream>
#include <sstream>
#include <set>
#include <cstdint>
#include <pthread.h>
#include <boost/filesystem.hpp>
#include <trog/call_data.hpp>
#include <trog/formatter.hpp>
#include <trog/writer_interface.hpp>
#include <trog/writer_threaded.hpp>
#include <trog/writer_simple.hpp>
#include <trog/loglevel.hpp>

namespace Trog {

/** a class the collects logging data at the site of a call toi the logger system.
 * there can be multiple of these objects.
 * Each one is distinguised by a 'name'
*/
class Logger
{
private:
    LogLevelType m_global_threshold;
    WriterInterface*    m_writer_ptr;
    Formatter*          m_formatter_ptr;

public:

    Logger(WriterInterface* writer, Formatter* formatter): m_writer_ptr(writer), m_formatter_ptr(formatter)
    {
        std::cout << "Trog::Logger destructor" << std::endl;
        m_global_threshold = LogLevelAll;
    }
    ~Logger()
    {
        std::cout << "Trog::Logger destructor" << std::endl;
    }
    
    void set_threshold(LogLevelType threshold)
    {
        m_global_threshold = threshold;
    }
    LogLevelType get_threshold()
    {
        return m_global_threshold;
    }

    template<typename T, typename... Types>
    void collect(
        LogLevelType level,
        const char* channel,
        const char* file_name,
        const char* func_name,
        int line_number,
        const T& firstArg,
        const Types&... args)
    {
        LogLevelType file_threshold = LogLevelAll;
        if(!levelIsActive(level, file_threshold, m_global_threshold)) {
            return;
        }
        LogCallDataSPtr call_sptr = std::make_shared<LogCallData>(
            channel, 
            level, 
            file_name, 
            line_number, 
            func_name, 
            pthread_self(), 
            getpid()
        );
        // tests suggest something like the following would save about 1/3
        // compared to the ostringstream version in the else clause
        #if 0
        char dummy[] = "This is a dummy string"; 
        char* p = (char*)malloc(1000);
        char* rp = p;
        for(int i = 0; i < 10; i++) {
            int n = sprintf(rp, "%s", dummy);
            rp = rp+n;
        }
        #else
        std::ostringstream os;
        m_formatter_ptr->format_types(os, firstArg, args...);
        call_sptr->message_sptr = std::make_shared<std::string>(os.str());
        m_writer_ptr->submit(call_sptr);
        #endif
    }
    void wait()
    {
        m_writer_ptr->wait();
    }

}; //class   

} // namespace
#endif