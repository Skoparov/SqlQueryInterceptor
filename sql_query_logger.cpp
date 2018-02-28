#include "sql_query_logger.h"

#include <iostream>
#include <arpa/inet.h>

namespace sql_query_logging
{

concurrent_logger::concurrent_logger( const db_type& type,
                                      std::ostream& stream ) noexcept:
    m_db_type( type ),
    m_out_stream( stream ){}

concurrent_logger::~concurrent_logger()
{
    stop();
}

void concurrent_logger::on_message( const char* const data, uint64_t data_size, uint64_t query_num )
{
    try
    {
        // Skip the first packet, as it carries only service info
        if( query_num )
        {
            std::string query{ sql_parser::get_query( data, data_size, m_db_type ) };
            if( !query.empty() )
            {
                std::lock_guard< std::mutex > l{ m_mutex };
                m_query_queue.push( std::move( query ) );
                m_cv.notify_one();
            }
        }
    }
    catch( const std::exception& e )
    {
        std::cerr << e.what() << std::endl;
    }
}

void concurrent_logger::start()
{
    if( !m_running )
    {
        m_thread = std::unique_ptr< std::thread >( new std::thread{ &concurrent_logger::cycle, this } );
        m_running = true;
    }
}

void concurrent_logger::stop()
{
    if( m_running )
    {
        m_running = false;

        m_cv.notify_one();
        m_thread->join();
    }
}

bool concurrent_logger::is_running() const noexcept
{
    return m_running;
}

void concurrent_logger::cycle()
{
    while( m_running )
    {
        std::unique_lock< std::mutex > l{ m_mutex };
        m_cv.wait( l, [ this ](){ return !m_query_queue.empty() || !m_running; } );

        while( !m_query_queue.empty() )
        {
            const std::string& query = m_query_queue.front();
            m_out_stream << query << std::endl;
            m_query_queue.pop();
        }
    }
}

}// sql_logger
