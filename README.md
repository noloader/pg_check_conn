# pg_check_conn

A PostgreSQL utility to check a user's access to a database. `pg_check_conn` will use supplied options to connect to a PostgreSQL database and return `0` on success, and `PQstatus` otherwise.

`pg_check_conn` is a direct replacement for `pg_isready`. The new utility accepts the same options as `pg_isready`: database (`-d`), hostname (`-h`), username (`-U`), port (`-p`), and timeout (`-t`). Environmental variables, like `PGPASSWORD`, should work as expected as `pg_check_conn` leaves them in tact for the underlying `libpg` library.

The reason for the `pg_check_conn` utility is, `pg_isready` does not use options when it tests a connection. If you use a fictitious or incorrect option like `-d foo`, `-U bar` or `PGPASSWORD=baz`, then `pg_isready` will still return success if the PostgreSQL server is running and accepting connections. For the details, see [Determine if a user and database are available](https://www.postgresql.org/message-id/CAH8yC8%3DfbU-DWSNso7qKDSNCj52hTop8ENKxF7_4a%3DhjY-nn6w%40mail.gmail.com) on the pgsql-general mailing list.

## Prerequisites

`pg_check_conn` requires the PostgreSQL development package and a C++ compiler. C++ simplifies the program by handling memory management.

The PostgreSQL development packages are `libpq-dev` on Debian and derivatives like Ubuntu; and `libpq-devel` on Red Hat derivatives, like CentOS and Fedora.

## Compiling pg_check_conn

You can compile `pg_check_conn` with the following command:

```bash
g++ -g -O2 `pkg-config --cflags libpq` pg_check_conn.cpp -o pg_check_conn `pkg-config --libs libpq`
```

## Running pg_check_conn

`pg_check_conn` is a direct replacement for `pg_isready`. Running `pg_check_conn` with no issues will result in success and a return code of 0. On error, an error string written to standard output as reported by `PQerrorMessage` and the value of `PQstatus` is returned by the program. The examples below show two typical failues.

```bash
$ PGPASSWORD=${password} pg_check_conn -h "${hostname}" -U "${username}" -d "${database}" -p "${port}"
Error: connection to server at "localhost" (::1), port 5432 failed: FATAL: Ident authentication failed for user "postgres"
```

and

```bash
$ PGPASSWORD=${password} pg_check_conn -U "${username}" -d "${database}" -p "${port}"
Error: connection to server on socket "/var/run/postgresql/.s.PGSQL.5432" failed: FATAL: Peer authentication failed for user "postgres"
```

The exit code for `pg_check_conn` is different than `pg_isready` since `pg_isready` returns success for nearly everything, including a missing database, a missing user, bad credentials and an incorrect authentication method. `pg_isready` would have reported success in the examples since the server was running and accepting connections.
