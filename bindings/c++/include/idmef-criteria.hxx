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

#ifndef _LIBPRELUDE_IDMEF_CRITERIA_HXX
#define _LIBPRELUDE_IDMEF_CRITERIA_HXX

#include <string>
#include "prelude.h"
#include "idmef.hxx"


namespace Prelude {

        class IDMEFCriteria {
            private:
                idmef_criteria_t *_criteria;

            public:
                ~IDMEFCriteria();
                IDMEFCriteria();
                IDMEFCriteria(const IDMEFCriteria &criteria);
                IDMEFCriteria(const char *criteria);
                IDMEFCriteria(const std::string &criteria);
                IDMEFCriteria(idmef_criteria_t *criteria);

                int Match(Prelude::IDMEF *message);
                IDMEFCriteria Clone();
                void ANDCriteria(const IDMEFCriteria &criteria);
                void ORCriteria(const IDMEFCriteria &criteria);
                const std::string ToString() const;

                operator const std::string() const;
                IDMEFCriteria &operator=(const IDMEFCriteria &criteria);
        };
};

#endif
