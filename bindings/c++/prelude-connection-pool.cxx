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

#include <vector>

#include "prelude.h"
#include "prelude-connection-pool.h"

#include "prelude-connection-pool.hxx"
#include "prelude-client.hxx"
#include "prelude-error.hxx"

using namespace Prelude;


ConnectionPool::~ConnectionPool()
{
        if ( _pool )
                prelude_connection_pool_destroy(_pool);
}


ConnectionPool::ConnectionPool()
{
        _pool = NULL;
}


ConnectionPool::ConnectionPool(prelude_connection_pool_t *pool)
{
        _pool = pool;
}


ConnectionPool::ConnectionPool(const ConnectionPool &con)
{
        _pool = (con._pool) ? prelude_connection_pool_ref(con._pool) : NULL;
}


ConnectionPool::ConnectionPool(ClientProfile &cp, int permission)
{
        int ret;

        ret = prelude_connection_pool_new(&_pool, cp, (prelude_connection_permission_t) permission);
        if ( ret < 0 )
                throw PreludeError(ret);
}



void ConnectionPool::init()
{
        int ret;

        ret = prelude_connection_pool_init(_pool);
        if ( ret < 0 )
                throw PreludeError(ret);
}


std::vector<Prelude::Connection> ConnectionPool::getConnectionList() const
{
        std::vector<Prelude::Connection> clist;
        prelude_connection_t *con;
        prelude_list_t *head, *tmp;

        head = prelude_connection_pool_get_connection_list(_pool);

        prelude_list_for_each(head, tmp) {
                con = (prelude_connection_t *) prelude_linked_object_get_object(tmp);
                clist.push_back(Connection(prelude_connection_ref(con)));
        }

        return clist;
}


void ConnectionPool::addConnection(Connection con)
{
        prelude_connection_pool_add_connection(_pool, prelude_connection_ref(con));
}


void ConnectionPool::delConnection(Connection con)
{
        prelude_connection_pool_del_connection(_pool, con);
}


void ConnectionPool::setConnectionDead(Connection &con)
{
        prelude_connection_pool_set_connection_dead(_pool, con);
}


void ConnectionPool::setConnectionAlive(Connection &con)
{
        prelude_connection_pool_set_connection_alive(_pool, con);
}


void ConnectionPool::setConnectionString(const char *str)
{
        int ret;

        ret = prelude_connection_pool_set_connection_string(_pool, str);
        if ( ret < 0 )
                throw PreludeError(ret);
}


const char *ConnectionPool::getConnectionString() const
{
        return prelude_connection_pool_get_connection_string(_pool);
}


int ConnectionPool::getFlags() const
{
        return prelude_connection_pool_get_flags(_pool);
}


void ConnectionPool::setFlags(int flags)
{
        prelude_connection_pool_set_flags(_pool, (prelude_connection_pool_flags_t) flags);
}


void ConnectionPool::setRequiredPermission(int permission)
{
        prelude_connection_pool_set_required_permission(_pool, (prelude_connection_permission_t) permission);
}


void ConnectionPool::setData(void *data)
{
        prelude_connection_pool_set_data(_pool, data);
}


void *ConnectionPool::getData() const
{
        return prelude_connection_pool_get_data(_pool);
}


ConnectionPool &ConnectionPool::operator=(const ConnectionPool &pool)
{
        if ( this != &pool && _pool != pool._pool ) {
                if ( _pool )
                        prelude_connection_pool_destroy(_pool);

                _pool = (pool._pool) ? prelude_connection_pool_ref(pool._pool) : NULL;
        }

        return *this;
}


ConnectionPool::operator prelude_connection_pool_t *()
{
        return _pool;
}
