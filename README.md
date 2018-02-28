Simple tcp proxy server able to log sql queries passing through. Uses boost::asio under the hood. As for now only postgres parser is implemented. SSL should be disabled for the session.

#### Usage: ./sql_query_logger $own_ip $own_port $server_ip $server_port $output $db_type

#### output can be:
* cout
* any other text will be treated as a destination file name

#### db_type can be:
* postgres
