cmake_minimum_required(VERSION 2.6)
set( PROJECT sql_query_logger )
project( ${PROJECT} )

set( CMAKE_CXX_STANDARD 11 )
SET( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11 -pthread" )
FIND_PACKAGE( Boost 1.55.0 REQUIRED COMPONENTS system )

ADD_EXECUTABLE( ${PROJECT}
                main.cpp
                db_types.h
                proxy_server.h
                proxy_server.cpp
                sql_parsers.h
                sql_parsers.cpp
                sql_query_logger.h
                sql_query_logger.cpp )

INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIRS} )
LINK_DIRECTORIES( ${Boost_LIBRARY_DIRS} )
TARGET_LINK_LIBRARIES( ${PROJECT} ${Boost_SYSTEM_LIBRARY} )
