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
                prelude_connection_t *GetConnection();

                void Close();
                void Connect(Prelude::ClientProfile &cp, int permission);

                void SetState(int state);
                int GetState();

                void SetData(void *data);
                void *GetData();

                int GetPermission();

                void SetPeerAnalyzerid(uint64_t analyzerid);
                uint64_t GetPeerAnalyzerid();

                const char *GetLocalAddr();
                unsigned int GetLocalPort();

                const char *GetPeerAddr();
                unsigned int GetPeerPort();

                bool IsAlive();

                int GetFd();
                Prelude::IDMEF RecvIDMEF();

                Connection & operator=(const Connection &con);
                operator prelude_connection_t *();
        };
};

#endif
