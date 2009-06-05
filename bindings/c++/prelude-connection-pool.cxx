/*****
*
* Copyright (C) 2008 PreludeIDS Technologies. All Rights Reserved.
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
* You should have received a copy of the GNU General Public License
* along with this program; see the file COPYING.  If not, write to
* the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
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



void ConnectionPool::Init()
{
        int ret;

        ret = prelude_connection_pool_init(_pool);
        if ( ret < 0 )
                throw PreludeError(ret);
}


std::vector<Prelude::Connection> ConnectionPool::GetConnectionList()
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


void ConnectionPool::AddConnection(Connection con)
{
        prelude_connection_pool_add_connection(_pool, prelude_connection_ref(con));
}


void ConnectionPool::DelConnection(Connection con)
{
        prelude_connection_pool_del_connection(_pool, con);
}


void ConnectionPool::SetConnectionDead(Connection &con)
{
        prelude_connection_pool_set_connection_dead(_pool, con);
}


void ConnectionPool::SetConnectionAlive(Connection &con)
{
        prelude_connection_pool_set_connection_alive(_pool, con);
}


void ConnectionPool::SetConnectionString(const char *str)
{
        int ret;

        ret = prelude_connection_pool_set_connection_string(_pool, str);
        if ( ret < 0 )
                throw PreludeError(ret);
}


const char *ConnectionPool::GetConnectionString()
{
        return prelude_connection_pool_get_connection_string(_pool);
}


int ConnectionPool::GetFlags()
{
        return prelude_connection_pool_get_flags(_pool);
}


void ConnectionPool::SetFlags(int flags)
{
        prelude_connection_pool_set_flags(_pool, (prelude_connection_pool_flags_t) flags);
}


void ConnectionPool::SetRequiredPermission(int permission)
{
        prelude_connection_pool_set_required_permission(_pool, (prelude_connection_permission_t) permission);
}


void ConnectionPool::SetData(void *data)
{
        prelude_connection_pool_set_data(_pool, data);
}


void *ConnectionPool::GetData()
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
