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

#ifndef _LIBPRELUDE_PRELUDE_CONNECTION_HXX
#define _LIBPRELUDE_PRELUDE_CONNECTION_HXX

#include "prelude-connection.h"
#include "prelude-client-profile.hxx"
#include "idmef.hxx"

namespace Prelude {
        class IDMEF;

        class Connection {
            private:
                prelude_connection_t *_con;

            public:
                ~Connection();
                Connection();
                Connection(const char *addr);
                Connection(const Connection &con);
                Connection(prelude_connection_t *con, bool own_data=FALSE);
                prelude_connection_t *getConnection() const;

                void close();
                void connect(Prelude::ClientProfile &cp, int permission);

                void setState(int state);
                int getState() const;

                void setData(void *data);
                void *getData() const;

                int getPermission() const;

                void setPeerAnalyzerid(uint64_t analyzerid);
                uint64_t getPeerAnalyzerid() const;

                const char *getLocalAddr() const;
                unsigned int getLocalPort() const;

                const char *getPeerAddr() const;
                unsigned int getPeerPort() const;

                bool isAlive() const;

                int getFd() const;
                Prelude::IDMEF recvIDMEF();

                Connection & operator=(const Connection &con);
                operator prelude_connection_t *();
        };
};

#endif
