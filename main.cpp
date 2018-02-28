#include <fstream>
#include <iostream>

#include <boost/format.hpp>
#include <boost/asio/signal_set.hpp>

#include "proxy_server.h"
#include "sql_query_logger.h"

enum command_list_args_pos
{
    own_ip_pos = 1,
    own_port_pos,
    server_ip_pos,
    server_port_pos,
    output_target_pos,
    db_type_pos,
    args_size
};

enum class output_mode{ cout, file };

db_type db_type_from_string( const std::string& str )
{
    db_type type;

    if( str == "postgres" )
    {
        type = db_type::postgres;
    }
    else
    {
        throw std::invalid_argument{ "Unimplemented database type" };
    }

    return type;
}

output_mode output_mode_from_string( const std::string& str )
{
    return str == "cout"?  output_mode::cout : output_mode::file;
}

struct project_settings
{
    proxy_server::connection_settings conn_settings;
    std::string log_file;
    db_type database_type;
    output_mode mode;
};

project_settings get_settings( int argc, char* argv[] )
{
    if( argc != args_size )
    {
        throw std::invalid_argument
        { "Usage: ./sql_query_logger $own_ip $own_port $server_ip $server_port $log_file $db_type\n"
          "Currently avaliable db_types: \"postgres\"" };
    }

    project_settings settings;

    settings.conn_settings.own_ip = argv[ own_ip_pos ];
    settings.conn_settings.own_port = static_cast< uint16_t >( std::atoi( argv[ own_port_pos ] ) );
    settings.conn_settings.server_ip = argv[ server_ip_pos ];
    settings.conn_settings.server_port = static_cast< uint16_t >( std::atoi( argv[ server_port_pos ] ) );
    settings.database_type = db_type_from_string( argv[ db_type_pos ] );
    settings.log_file = argv[ output_target_pos ];
    settings.mode = output_mode_from_string( settings.log_file );

    return settings;
}

void print_info( const project_settings& settings )
{
    boost::format pattern{ "Started on: %s:%s\nProxying to: %s:%s\n" };
    std::cout << boost::str( pattern %
                             settings.conn_settings.own_ip %
                             settings.conn_settings.own_port %
                             settings.conn_settings.server_ip %
                             settings.conn_settings.server_port );
}

int main(int argc, char* argv[])
{
    try
    {
        project_settings settings = get_settings( argc, argv );

        std::ofstream out_file;
        if( settings.mode == output_mode::file )
        {
            out_file.open( settings.log_file, std::ios_base::app );
            if( !out_file.is_open() )
            {
                throw std::ios_base::failure{ "Failed to open file" };
            }
        }

        boost::asio::io_service io_service;
        sql_query_logging::concurrent_logger logger{ settings.database_type,
                    settings.mode == output_mode::file? out_file : std::cout };
        proxy_server::listner listner{ settings.conn_settings, io_service };

        auto callback = std::bind( &sql_query_logging::concurrent_logger::on_message,
                                   &logger,
                                   std::placeholders::_1,
                                   std::placeholders::_2,
                                   std::placeholders::_3 );

        listner.set_on_client_data_callback( callback );

        boost::asio::signal_set signals_to_handle{ io_service, SIGINT, SIGTERM };
        signals_to_handle.async_wait( [ & ]( const boost::system::error_code&, int )
        {
            listner.stop();
            io_service.stop();
            logger.stop();
        } );

        print_info( settings );

        logger.start();
        listner.start();
        io_service.run();
    }
    catch( const std::exception& e )
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
