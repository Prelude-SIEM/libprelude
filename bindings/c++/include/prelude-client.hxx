/*****
*
* Copyright (C) 2009-2017 CS-SI. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoannv@gmail.com>
*
* This file is part of the Prelude library.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2, or (at your option)
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
*****/

#ifndef _LIBPRELUDE_PRELUDE_CLIENT_HXX
#define _LIBPRELUDE_PRELUDE_CLIENT_HXX

#include "idmef.hxx"
#include "prelude-connection-pool.hxx"
#include "prelude-client-profile.hxx"


namespace Prelude {
        class IDMEF;

        class Client : public ClientProfile {
            private:
                prelude_client_t *_client;
                ConnectionPool _pool;

            protected:
                int _recv_timeout;

            public:
                enum ClientFlagsEnum {
                        ASYNC_SEND  = PRELUDE_CLIENT_FLAGS_ASYNC_SEND,
                        FLAGS_ASYNC_SEND   = PRELUDE_CLIENT_FLAGS_ASYNC_SEND,
                        ASYNC_TIMER = PRELUDE_CLIENT_FLAGS_ASYNC_TIMER,
                        FLAGS_ASYNC_TIMER  = PRELUDE_CLIENT_FLAGS_ASYNC_TIMER,
                        HEARTBEAT   = PRELUDE_CLIENT_FLAGS_HEARTBEAT,
                        FLAGS_HEARTBEAT   = PRELUDE_CLIENT_FLAGS_HEARTBEAT,
                        CONNECT     = PRELUDE_CLIENT_FLAGS_CONNECT,
                        FLAGS_CONNECT     = PRELUDE_CLIENT_FLAGS_CONNECT,
                        AUTOCONFIG  = PRELUDE_CLIENT_FLAGS_AUTOCONFIG,
                        FLAGS_AUTOCONFIG = PRELUDE_CLIENT_FLAGS_AUTOCONFIG,
                };

                enum ConnectionPermissionEnum {
                        IDMEF_READ  = PRELUDE_CONNECTION_PERMISSION_IDMEF_READ,
                        PERMISSION_IDMEF_READ = PRELUDE_CONNECTION_PERMISSION_IDMEF_READ,
                        ADMIN_READ  = PRELUDE_CONNECTION_PERMISSION_ADMIN_READ,
                        PERMISSION_ADMIN_READ  = PRELUDE_CONNECTION_PERMISSION_ADMIN_READ,
                        IDMEF_WRITE = PRELUDE_CONNECTION_PERMISSION_IDMEF_WRITE,
                        PERMISSION_IDMEF_WRITE  = PRELUDE_CONNECTION_PERMISSION_IDMEF_WRITE,
                        ADMIN_WRITE = PRELUDE_CONNECTION_PERMISSION_ADMIN_WRITE,
                        PERMISSION_ADMIN_WRITE  = PRELUDE_CONNECTION_PERMISSION_ADMIN_WRITE,
                };

                ~Client();
                Client(const char *profile);
                Client(const Client &client);

                void start();
                void init();

                prelude_client_t *getClient() const;

                void sendIDMEF(const Prelude::IDMEF &message);
                int recvIDMEF(Prelude::IDMEF &idmef, int timeout=-1);

                int getFlags() const;
                void setFlags(int flags);

                int getRequiredPermission() const;
                void setRequiredPermission(int permission);

                const char *getConfigFilename() const;
                void setConfigFilename(const char *name);

                Prelude::ConnectionPool &getConnectionPool();
                void setConnectionPool(Prelude::ConnectionPool pool);

                Client &operator << (Prelude::IDMEF &idmef);
                Client &operator >> (Prelude::IDMEF &idmef);
                Client &operator=(const Client &p);

                static Client &setRecvTimeout(Client &c, int timeout);
        };
};

#endif
