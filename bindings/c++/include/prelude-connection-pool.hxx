/*****
*
* Copyright (C) 2008-2017 CS-SI. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoann@prelude-ids.com>
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

#ifndef _LIBPRELUDE_PRELUDE_CONNECTION_POOL_HXX
#define _LIBPRELUDE_PRELUDE_CONNECTION_POOL_HXX

#include "prelude.h"
#include "prelude-client-profile.hxx"
#include "prelude-connection.hxx"

namespace Prelude {
        class ConnectionPool {
            private:
                prelude_connection_pool_t *_pool;

            public:
                ~ConnectionPool();
                ConnectionPool();
                ConnectionPool(prelude_connection_pool_t *pool);
                ConnectionPool(const ConnectionPool &pool);
                ConnectionPool(Prelude::ClientProfile &cp, int permission);

                void init();

                void setConnectionString(const char *str);
                const char *getConnectionString() const;
                std::vector<Prelude::Connection> getConnectionList() const;

                void setFlags(int flags);
                int getFlags() const;

                void setData(void *data);
                void *getData() const;

                void addConnection(Prelude::Connection con);
                void delConnection(Prelude::Connection con);

                void setConnectionAlive(Prelude::Connection &con);
                void setConnectionDead(Prelude::Connection &con);

                void setRequiredPermission(int permission);
                ConnectionPool &operator=(const ConnectionPool &pool);
                operator prelude_connection_pool_t *();
        };
};

#endif /* __PRELUDE_CONNECTION_POOL__ */
