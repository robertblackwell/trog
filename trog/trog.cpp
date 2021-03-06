//
#include <iostream>
#include <sstream>
#include <stdarg.h>
#include <bitset>
#include <cstdint>
#include <trog/trog.hpp>

bool Trog::logger_enabled = true;
Trog::LogLevelType Trog::Trogger::allEnabled = 
    Trog::LogLevelVerbose | Trog::LogLevelFDTrace | Trog::LogLevelTrace | Trog::LogLevelCTorTrace;

Trog::LogLevelType Trog::Trogger::globalThreshold = Trog::Trogger::allEnabled; 

Trog::Trogger Trog::Trogger::activeTrogger{};


std::ostringstream& Trog::preamble(
    std::ostringstream& os,
    std::string filename,
    long pid,
    long tid,
    std::string function_name,
    long linenumber
){
    os 
        #ifdef RBLOG_FILENAME
        << filename.c_str() 
        #endif
        #ifdef RBLOG_PIDTID
        << "[" 
        <<pid 
        << ":" 
        << tid 
        <<"]" 
        #endif
        #ifdef RBLOG_FUNCTION_NAME
        << "::"
        << function_name 
        #endif
        #ifdef RBLOG_LINE_NUMBER
        << "["<< linenumber <<"]"
        << ":" 
        #endif
        << "";
    return os;
}

void Trog::setEnabled(bool on_off)
{
    logger_enabled = on_off;
}
void Trog::enableForLevel(LogLevelType level)
{
    Trog::Trogger::globalThreshold  = level;
    logger_enabled = true;
}

std::string Trog::Trogger::p_className(std::string& func_name){
    
    return "";
}

Trog::Trogger::Trogger(std::ostream& os) : m_outStream(os)
{
    Trog::logger_enabled = true;
}


void Trog::Trogger::logWithFormat(
     LogLevelType           level,
     LogLevelType           threshold,
      const char*    file_name,
     const char*    func_name,
     int            line_number,
     char*          format,
     ...)
{
    using namespace boost::filesystem;
    std::ostringstream os;
    if( levelIsActive(level, threshold) ){
        std::lock_guard<std::mutex> lg(_loggerMutex);
        os << Trog::LogLevelText(level) << "|";
        path tmp2 = path(file_name);
        path filename_tmp3 = tmp2.filename();
        path tmp4 = filename_tmp3.stem();
        auto tmp5 = tmp4.string();
        auto pid = ::getpid();
        auto tid = pthread_self();

        #ifndef RBLOG_USE_PREAMBLE
        os 
            << tmp3.c_str() 
            << ":" 
            << "[" 
            <<pid 
            << ":" 
            << tid 
            <<"]" 
            << func_name 
            << "["<< line_number <<"]"
            << ":" ;
        #else
        preamble(os, filename_tmp3.string(), pid, tid, func_name, line_number);
        #endif

        va_list argptr;
        va_start(argptr,format);
        char* bufptr;
        vasprintf(&bufptr, format, argptr);
        va_end(argptr);
        std::string outStr = os.str() + std::string(bufptr, strlen(bufptr));
        const char* outCharStar = outStr.c_str();
        size_t len = strlen(outCharStar);
        write(STDERR_FILENO, (void*)outCharStar, len);
    }
}
void Trog::Trogger::torTraceLog(
    LogLevelType           level,
    LogLevelType           threshold,
    const char* file_name,
    const char* func_name,
    int line_number,
    void* this_arg)
{
    if( levelIsActive(level, threshold) ){
        std::ostringstream os;
        std::lock_guard<std::mutex> lg(_loggerMutex);
        
        os << "CTR" <<"|";
        auto tmp2 = boost::filesystem::path(file_name);
        auto filename_tmp3 = tmp2.filename();
        auto tmp4 = filename_tmp3.stem();
        auto pid = ::getpid();
        auto tid = pthread_self();

        #ifndef RBLOG_USE_PREAMBLE
        os 
            << filename_tmp3.c_str() 
            << ":" 
            << "[" 
            <<pid 
            << ":" 
            << tid 
            <<"]" 
            << func_name 
            << "["<< line_number <<"]"
            << ":" ;
        #else
        preamble(os, filename_tmp3.string(), pid, tid, func_name, line_number);
        #endif

        os << std::hex << (long)this_arg << std::dec << std::endl;;
        //
        // Only use the stream in the last step and this way we can send the log record somewhere else
        // easily
        //
        write(STDERR_FILENO, os.str().c_str(), strlen(os.str().c_str()) );
    }
}
void Trog::Trogger::fdTraceLog(
    LogLevelType level,
    LogLevelType threshold,
    const char* file_name,
    const char* func_name,
    int line_number,
    long fd_arg)
{
    if( levelIsActive(level, threshold) ){
        std::ostringstream os;
        std::lock_guard<std::mutex> lg(_loggerMutex);
        
        os << "FD " <<"|";
        auto tmp2 = boost::filesystem::path(file_name);
        auto filename_tmp3 = tmp2.filename();
        auto tmp4 = filename_tmp3.stem();
        auto pid = ::getpid();
        auto tid = pthread_self();

        #ifndef RBLOG_USE_PREAMBLE
        os 
            << filename_tmp3.c_str() 
            << ":" 
            << "[" 
            <<pid 
            << ":" 
            << tid 
            <<"]" 
            << func_name 
            << "["<< line_number <<"]"
            << ":" ;
        #else
        preamble(os, filename_tmp3.string(), pid, tid, func_name, line_number);
        #endif
        os << " fd:" << fd_arg << std::endl;;
        //
        // Only use the stream in the last step and this way we can send the log record somewhere else
        // easily
        //
        write(STDERR_FILENO, os.str().c_str(), strlen(os.str().c_str()) );
    }
}


bool Trog::Trogger::enabled()
{
    /// this function is only used for Trace functions
    /// we want these active with DEBUG levels
    LogLevelType lvl = LOG_LEVEL_DEBUG;
    LogLevelType tmp = globalThreshold;
    return ( ((int)lvl <= (int)tmp) && Trog::logger_enabled );
}

std::string Trog::LogLevelText(Trog::LogLevelType level)
{
    static std::string tab[] = {
        "",
        "ERR",
        "WRN",
        "INF",
        "DBG",
        "VRB",
    };
    static std::string other_tab[] = {
        "BAD1",
        "TRC","TOR",
        "BAD3","FD "
    };
    const int bitWidth = 64;
    const int64_t traceMax = 0b10000000;
    const int64_t traceMask = 0b11111111;
    int64_t adjusted_level;
    int64_t level_long = level;

    if (level > traceMax) {
        // the level value is not a trace value
        std::bitset<bitWidth> blevel(level);
        adjusted_level = level >> Trog::TraceBits;
        std::bitset<bitWidth> badjusted_level(adjusted_level);
        // std::cout << "LogLevelText level: " << blevel << " adjusted_level : " << badjusted_level << std::endl;
        assert(adjusted_level <= (Trog::LogLevelVerbose >> Trog::TraceBits));
        return tab[adjusted_level];
    } else {
        adjusted_level = (level & traceMask);
        switch(adjusted_level) {
            case Trog::LogLevelCTorTrace:
                return "TOR";
                break;
            case Trog::LogLevelFDTrace:
                return "FD ";
                break;
            case Trog::LogLevelTrace:
                return "TRC";
                break;
            case Trog::LogLevelTrace2:
                return "TR2";
                break;
            case Trog::LogLevelTrace3:
                return "TR3";
                break;
            case Trog::LogLevelTrace4:
                return "TR4";
                break;
            default:
                assert(false);
        }
    }
}

bool Trog::testLevelForActive(long level, long threshold )
{

    const int bitWidth = 32;
    const int32_t traceMax = 0b10000000;
    const int32_t traceMask = 0b11111111;
	int32_t result;
	int32_t threshold_bits;
	std::bitset<bitWidth> blevel(level);
	std::bitset<bitWidth> bthreshold(threshold);
	// std::cout << "testLevels entry level: " << blevel << " threshold: " << bthreshold << std::endl;
	if (level <= traceMax) {
		threshold_bits = (threshold & traceMask);
		std::bitset<bitWidth> bthreshold_bits(threshold);
		result = (level & threshold_bits);
		std::bitset<bitWidth> bresult(result);
		// std::cout << "testLevels level =< 4 threshold_bits: " << bthreshold_bits << " result: " << bresult << std::endl;
		return result;
	} else {
		return (level <= threshold);
	}
}

bool Trog::Trogger::levelIsActive(LogLevelType lvl, LogLevelType threshold)
{
    if (! Trog::logger_enabled)
        return false;
    if (testLevelForActive(lvl, threshold)) {
        if (testLevelForActive(lvl, globalThreshold)) {
            return true;
        } else {
            return false;
        }
    }
    return false;
    /// use the lowest threshold - local or global
    LogLevelType tmp = (threshold <= globalThreshold) ? threshold : globalThreshold;
    return ( ((int)lvl <= (int)tmp) && Trog::logger_enabled );
//    return ( ((int)lvl <= (int)threshold) && Trog::logger_enabled );
}

void Trog::Trogger::myprint(std::ostringstream& os)
{
//        write(STDERR_FILENO, "\n", 2);
    os << std::endl;
}
