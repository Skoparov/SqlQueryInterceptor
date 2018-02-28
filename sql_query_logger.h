#ifndef SQL_QUERY_LOGGER
#define SQL_QUERY_LOGGER

#include <queue>
#include <fstream>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include <condition_variable>

#include "sql_parsers.h"

namespace sql_query_logging
{

class concurrent_logger
{
public:
    concurrent_logger( const db_type& type, std::ostream& stream ) noexcept;
    ~concurrent_logger();

    // Class is supposed to be started and stop from the same thread
    void start();
    void stop();
    bool is_running() const noexcept;

    void on_message( const char* const data, uint64_t data_size, uint64_t query_num );

private:
    void cycle();

private:
    db_type m_db_type;
    std::ostream& m_out_stream;
    std::queue< std::string > m_query_queue;

    std::atomic_bool m_running{ false };

    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::unique_ptr< std::thread > m_thread;
};

}// sql_query_logger

#endif
