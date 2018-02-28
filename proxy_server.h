#ifndef PROXY_SERVER
#define PROXY_SERVER

#include <memory>
#include <functional>

#include <boost/asio.hpp>

namespace proxy_server
{

using callback_type = std::function< void( const char* const, uint64_t, uint64_t  ) >;

namespace detail
{

class session;

}// detail

struct connection_settings
{
    std::string own_ip;
    uint16_t own_port;
    std::string server_ip;
    uint16_t server_port;
};

// Listnes to incoming connections, creates new sessions
class listner
{
public:
    listner( const connection_settings& settings,
             boost::asio::io_service& io_service );

    void start();
    void stop();
    bool is_running() const;

    std::string get_own_ip() const;
    std::string get_server_ip() const;
    uint16_t get_own_port() const noexcept;
    uint16_t get_server_port() const noexcept;

    void set_on_client_data_callback( const callback_type& callback );

private:
    void handle_connetion(const boost::system::error_code& error);

private:
    boost::asio::ip::address_v4 m_own_ip;
    uint16_t m_own_port{ 0 };
    boost::asio::ip::address_v4 m_server_ip;
    uint16_t m_server_port{ 0 };

    boost::asio::io_service& m_io_service;
    boost::asio::ip::tcp::acceptor m_acceptor;

    std::shared_ptr< class detail::session > m_waiting_session;
    callback_type m_callback;
};

namespace detail
{

static constexpr uint64_t default_buffer_size{ 8192 };

class session : public std::enable_shared_from_this< session >
{
public:
    session( boost::asio::io_service& io_service,
             uint64_t buffer_size = default_buffer_size );

    void connect_to_server( const boost::asio::ip::address_v4 &ip, uint16_t port );
    boost::asio::ip::tcp::socket& get_client_socket() noexcept;

    void set_on_client_data_callback( const callback_type& callback );

private:
    void set_handlers( const boost::system::error_code& error );
    void teardown( const boost::system::error_code& error = {} );

    void write_to_client( const boost::system::error_code& error, const uint64_t &data_size);
    void read_from_client( const boost::system::error_code& error);
    void write_to_server( const boost::system::error_code& error, const uint64_t& data_size );
    void read_from_server( const boost::system::error_code& error );

private:
    boost::asio::ip::tcp::socket m_server_socket;
    boost::asio::ip::tcp::socket m_client_socket;

    uint64_t m_client_packet_num{ 0 };
    std::vector< char > m_client_data_buffer;
    std::vector< char > m_server_data_buffer;

    callback_type m_callback;
};

}// detail

}// tcp_proxy

#endif
