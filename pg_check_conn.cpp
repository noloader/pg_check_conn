// Written and placed in public domain by Jeffrey Walton.
//
// pg_check_conn verifies you can connect to a PostgreSQL database using the
// supplied credentials and options. The program is a drop-in for pg_isready.
// In fact, pg_check_conn was written to fill the gaps in pg_isready because
// pg_isready does not fail if the database or user (or both) does not exist.
//
// A user's password should be passed through the environmental variable
// PGPASSWORD. This avoids copying the password into insecure memory or
// exposing it in command line arguments.
//
// The GitHub repo for this file is located at
// https://github.com/noloader/pg_check_conn.
//
// PostgreSQL source file for pg_isready can be found in the source tree at
// src/bin/scripts/pg_isready.c.
//
// Compile with:
//    g++ -g -O2 `pkg-config --cflags libpq` pg_check_conn.cpp -o pg_check_conn `pkg-config --libs libpq`

#include <iostream>
#include <exception>
#include <stdexcept>
#include <sstream>
#include <cstdlib>
#include <cstring>

#include <libpq-fe.h>

// Unnamed namespace
namespace {

    const char whitespace[] = " \t\n\r\f\v";

    inline std::string ltrim(std::string str)
    {
        return str.erase(0, str.find_first_not_of(whitespace));
    }

    inline std::string rtrim(std::string str)
    {
        std::string::size_type pos = str.find_last_not_of(whitespace);

        if (pos != std::string::npos)
            return str.erase(pos + 1);

        return std::string();
    }

    inline std::string trim(std::string str)
    {
        return ltrim(rtrim(str));
    }

    inline std::string parse_next_arg(int argc, const char* const argv[], size_t index, const char* const errMsg)
    {
        if (index+1 < static_cast<size_t>(argc) && argv[index+1][0] != '-')
        {
            std::string a = trim(argv[index + 1]);
            if (! a.empty() )
                return a;
        }

        throw std::out_of_range(errMsg);
    }

    inline std::string parse_equal_arg(const std::string& arg, const char* const errMsg)
    {
        std::string::size_type pos = arg.find('=', 0);

        if (pos != std::string::npos && pos < arg.length())
        {
            std::string a = trim(arg.substr(pos + 1));
            if (! a.empty() )
                return a;
        }

        throw std::runtime_error(errMsg);
    }

    inline bool is_empty(const char* const arg)
    {
        return arg == NULL || std::strlen(arg) == 0;
    }

    inline bool is_empty(const std::string& arg)
    {
        return arg.empty();
    }

    struct ConnException : public std::runtime_error
    {
        ConnException(const char* msg) : std::runtime_error(msg) { }
    };

}; // namespace

int main(int argc, char* argv[])
{
    PGconn *conn = NULL;
    int ret = 0;

    try
    {
        std::string database, username, /*password,*/ host, hostaddr, port, timeout;

        for (size_t index=1; index<static_cast<size_t>(argc); ++index)
        {
            std::string opt(argv[index]);

            ////////// Database //////////
            if (opt == "-d")
                database = parse_next_arg(argc, argv, index, "missing database argument"), index++;

            else if (opt.substr(0, 8) == "--dbname")
                database = parse_equal_arg(opt, "missing database argument");

            ////////// Username //////////
            else if (opt == "-U")
                username = parse_next_arg(argc, argv, index, "missing username argument"), index++;

            else if (opt.substr(0, 10) == "--username")
                username = parse_equal_arg(opt, "missing username argument");

            ////////// Hostname //////////
            else if (opt == "-h")
                host = parse_next_arg(argc, argv, index, "missing hostname argument"), index++;

            else if (opt.substr(0, 10) == "--hostname")
                host = parse_equal_arg(opt, "missing hostname argument");

            ////////// Hostaddr //////////
            else if (opt.substr(0, 10) == "--hostaddr")  // No short opt
                hostaddr = parse_equal_arg(opt, "missing hostaddr argument");

            ////////// Port //////////
            else if (opt == "-p")
                port = parse_next_arg(argc, argv, index, "missing port argument"), index++;

            else if (opt.substr(0, 6) == "--port")
                port = parse_equal_arg(opt, "missing port argument");

            ////////// Timeout //////////
            else if (opt == "-t")
                timeout = parse_next_arg(argc, argv, index, "missing timeout argument"), index++;

            else if (opt.substr(0, 9) == "--timeout")
                timeout = parse_equal_arg(opt, "missing timeout argument");
        }

        // https://www.postgresql.org/docs/current/libpq-connect.html#LIBPQ-CONNSTRING
        std::stringstream connstr;

        if (! is_empty(database) ) {
            connstr << "dbname=" << database << " ";
        }

        if (! is_empty(username) ) {
            connstr << "user=" << username << " ";
        }

        //const char* pgpass = getenv("PGPASSWORD");
        //if (pgpass) {
        //    connstr << "password=" << pgpass << " ";
        //}

        // Both hostaddr and host can be supplied; hostaddr avoids a DNS lookup. See
        // https://www.postgresql.org/docs/current/libpq-connect.html#LIBPQ-PARAMKEYWORDS
        if (! is_empty(hostaddr) ) {
            connstr << "hostaddr=" << hostaddr << " ";
        }

        if (! is_empty(host) ) {
            connstr << "host=" << host << " ";
        }

        if (! is_empty(port) ) {
            connstr << "port=" << port << " ";
        }

        if (! is_empty(timeout) ) {
            connstr << "connect_timeout=" << timeout << " ";
        }

        // Debug logging
        const char* pgdebug = getenv("PGDEBUG");
        if (pgdebug && pgdebug[0] == '1') {
            std::cout << "Conn string: " << connstr.str() << std::endl;
        }

        // Paydirt
        conn = PQconnectdb(connstr.str().c_str());
        if (conn == NULL || PQstatus(conn) != CONNECTION_OK)
        {
            std::stringstream errstr;
            errstr << PQerrorMessage(conn);

            throw ConnException(errstr.str().c_str());
        }

        ret = 0;
    }
    catch (const ConnException& ex)
    {
        // PostgreSQL cnnections errors are expected. Write them to standard out
        std::cout << "Error: " << ex.what() << std::endl;

        if (conn != NULL) {
            ret = PQstatus(conn);
        } else {
            ret = -1;
        }
    }
    catch (const std::exception& ex)
    {
        // These errors are not expected. Write them to standard error.
        std::cerr << "Error: " << ex.what() << std::endl;

        ret = -1;
    }

    if (conn != NULL) {
        PQfinish(conn), conn = NULL;
    }

    return ret;
}
