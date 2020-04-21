#ifndef trog_guard_collector_HPP
#define trog_guard_collector_HPP
//
#include <iostream>
#include <sstream>
#include <set>
#include <cstdint>
#include <pthread.h>
#include <boost/filesystem.hpp>
#include <trog/call_data.hpp>
#include <trog/formatter.hpp>
#include <trog/writer_threaded.hpp>
#include <trog/writer_simple.hpp>
#include <trog/loglevel.hpp>

namespace Trog {


/** a class the collects logging data at the site of a call toi the logger system.
 * there can be multiple of these objects.
 * Each one is distinguised by a 'name'
*/
template<typename F, typename S, template<typename ...>class W>
class Collector: W<F, S>
{
public:

    static Collector& getInstance()
    {
        static Collector instance;
        return instance;
    }

    Collector()
    {
        std::cout << "Collector destructor" << std::endl;
    }
    ~Collector()
    {
        std::cout << "Collector destructor" << std::endl;
    }
    
    template<typename T, typename... Types>
    void collect(
        LogLevelType level,
        LogLevelType file_threshold,
        LogLevelType global_threshold,
        const char* channel,
        const char* file_name,
        const char* func_name,
        int line_number,
        const T& firstArg,
        const Types&... args)
    {
        if(!levelIsActive(level, file_threshold, global_threshold)) {
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
        F::format_types(os, firstArg, args...);
        call_sptr->message_sptr = std::make_shared<std::string>(os.str());
        W<F,S>::submit(call_sptr);
        #endif
    }
    void wait()
    {
        W<F,S>::wait();
    }
private:

}; //class   

} // namespace
#endif