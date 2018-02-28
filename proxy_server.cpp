#include "proxy_server.h"

#include <iostream>

namespace ba = boost::asio;
namespace bs = boost::system;

namespace proxy_server
{

namespace detail
{

session::session( ba::io_service& io_service, uint64_t buffer_size ) :
    m_server_socket( io_service ),
    m_client_socket( io_service )
{
    m_server_data_buffer.reserve( buffer_size );
    m_client_data_buffer.reserve( buffer_size );
}

void session::set_on_client_data_callback( const callback_type& callback )
{
    m_callback = callback;
}

ba::ip::tcp::socket& session::get_client_socket() noexcept
{
    return m_client_socket;
}

void session::connect_to_server( const ba::ip::address_v4& ip, uint16_t port )
{
    auto handler = std::bind( &session::set_handlers,
                              shared_from_this(),
                              std::placeholders::_1 );

    m_server_socket.async_connect( ba::ip::tcp::endpoint{ ip, port },
                                   handler );
}

void session::set_handlers( const bs::error_code& error )
{
    if( !error )
    {
        auto server_handler = std::bind( &session::write_to_client,
                                         shared_from_this(),
                                         std::placeholders::_1,
                                         std::placeholders::_2 );

        auto client_handler = std::bind( &session::write_to_server,
                                         shared_from_this(),
                                          std::placeholders::_1,
                                          std::placeholders::_2 );

        m_server_socket.async_read_some
        (
            ba::buffer( m_server_data_buffer.data(), m_server_data_buffer.capacity() ),
            server_handler
        );

        m_client_socket.async_read_some
        (
            ba::buffer( m_client_data_buffer.data(), m_server_data_buffer.capacity() ),
            client_handler
        );
    }
    else
    {
        teardown( error );
    }
}

void session::write_to_client( const bs::error_code &error, const uint64_t& data_size )
{
    if( !error )
    {
        auto handler = std::bind( &session::read_from_server,
                                  shared_from_this(),
                                  std::placeholders::_1 );

        async_write( m_client_socket,
                     ba::buffer( m_server_data_buffer.data(), data_size ),
                     handler );
    }
    else
    {
        teardown( error );
    }
}

void session::read_from_client( const bs::error_code& error )
{
    if( !error )
    {
        auto handler = std::bind( &session::write_to_server,
                                  shared_from_this(),
                                  std::placeholders::_1,
                                  std::placeholders::_2 );

        m_client_socket.async_read_some
        (
            ba::buffer( m_client_data_buffer.data(), m_client_data_buffer.capacity() ),
            handler
        );
    }
    else
    {
        teardown( error );
    }
}

void session::write_to_server( const bs::error_code& error, const uint64_t& data_size )
{
    if( !error )
    {
        // write to server socket, initialte new client read in handler
        auto handler = std::bind( &session::read_from_client,
                                  shared_from_this(),
                                  std::placeholders::_1 );

        if( m_callback )
        {
            m_callback( m_client_data_buffer.data(), data_size, m_client_packet_num );
        }

        ++m_client_packet_num;

        async_write( m_server_socket,
                     boost::asio::buffer( m_client_data_buffer.data(), data_size ),
                     handler );
    }
    else
    {
        teardown( error );
    }
}

void session::read_from_server( const boost::system::error_code& error )
{
    if( !error )
    {
        auto handler = std::bind( &session::write_to_client,
                                  shared_from_this(),
                                  std::placeholders::_1,
                                  std::placeholders::_2 );

        m_server_socket.async_read_some
        (
            ba::buffer( m_server_data_buffer.data(), m_server_data_buffer.capacity() ),
            handler
        );
    }
    else
    {
        teardown( error );
    }
}

void session::teardown( const bs::error_code& error )
{
    // Do not report regular or abnormal connection closures
    if( error.value() != boost::asio::error::eof &&
        error.value() != boost::asio::error::operation_aborted )
    {
        std::cerr << error.message() << std::endl;
    }

    if( m_client_socket.is_open() )
    {
        m_client_socket.close();
    }

    if( m_server_socket.is_open() )
    {
        m_server_socket.close();
    }
}

}// detail

listner::listner( const connection_settings& settings,
                  ba::io_service& io_service) :
    m_own_ip( ba::ip::address_v4::from_string( settings.own_ip ) ),
    m_own_port( settings.own_port ),
    m_server_ip( ba::ip::address_v4::from_string( settings.server_ip ) ),
    m_server_port( settings.server_port ),
    m_io_service( io_service ),
    m_acceptor( m_io_service, ba::ip::tcp::endpoint{ m_own_ip, m_own_port } ){}

void listner::start()
{
    m_waiting_session = std::make_shared< detail::session >( m_io_service );

    if( m_callback )
    {
        m_waiting_session->set_on_client_data_callback( m_callback );
    }

    auto accept_handler = std::bind( &listner::handle_connetion,
                                     this,
                                     std::placeholders::_1 );

    m_acceptor.async_accept( m_waiting_session->get_client_socket(), accept_handler );
}

void listner::stop()
{
    if( m_acceptor.is_open() )
    {
        m_acceptor.close();
    }
}

bool listner::is_running() const
{
    return m_acceptor.is_open();
}

std::string listner::get_own_ip() const
{
    return m_own_ip.to_string();
}

std::string listner::get_server_ip() const
{
    return m_server_ip.to_string();
}

uint16_t listner::get_own_port() const noexcept
{
    return m_own_port;
}

uint16_t listner::get_server_port() const noexcept
{
    return m_server_port;
}

void listner::set_on_client_data_callback( const callback_type& callback )
{
    m_callback = callback;
}

void listner::handle_connetion( const boost::system::error_code& error )
{
    if( !error )
    {
        m_waiting_session->connect_to_server( m_server_ip, m_server_port );
        start();
    }
    else
    {
        std::cerr << error.message() << std::endl;
    }
}

}// proxy_server
