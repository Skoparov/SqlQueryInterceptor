#ifndef SQL_PARSERS
#define SQL_PARSERS

#include <string>

#include "db_types.h"

namespace sql_parser
{

std::string get_query( const char* const packet_data,
                       uint64_t packet_size,
                       const db_type& type );

}// sql_parsers

#endif
