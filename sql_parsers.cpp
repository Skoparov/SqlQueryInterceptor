#include "sql_parsers.h"

#include <stdexcept>
#include <arpa/inet.h>

namespace sql_parser
{

#pragma pack( push, 1 )
struct postgres_header
{
    uint8_t message_type; // absent in the first message
    uint32_t message_length;
};
#pragma pack( pop )

std::string postgres_parser( const char* const packet_data, uint64_t packet_size )
{
    std::string result;

    if( packet_size < sizeof( postgres_header ) )
    {
        throw std::invalid_argument{ "Packet is too small" };
    }

    if( packet_size > sizeof( postgres_header ) )
    {
        const postgres_header* header{
            reinterpret_cast< const postgres_header* >( packet_data ) };

        uint64_t size_according_to_header = htonl( header->message_length );

        // Verify packet size and construct the query string
        if( size_according_to_header == packet_size - 1 )
        {
            const char* const query_data{ packet_data + sizeof( postgres_header ) };
            uint64_t query_size{ packet_size - sizeof( postgres_header ) - 1 };
            result.assign( query_data, query_data + query_size );
        }
        else
        {
            throw std::invalid_argument{ "Corrputed packet" };
        }
    }

    return result;
}

std::string get_query( const char* const packet_data, uint64_t packet_size, const db_type& type )
{
    std::string result;

    switch( type )
    {
    case db_type::postgres:
        result = postgres_parser( packet_data, packet_size );
        break;
    default:
        throw std::invalid_argument{ "Unimplemented database type" };
    }

    return result;
}

}// sql_parser
