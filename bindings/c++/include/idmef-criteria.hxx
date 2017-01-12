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

#ifndef _LIBPRELUDE_IDMEF_CRITERIA_HXX
#define _LIBPRELUDE_IDMEF_CRITERIA_HXX

#include <string>
#include "prelude.h"
#include "idmef.hxx"

namespace Prelude {
        class IDMEFCriterion {
            public:
                enum IDMEFCriterionOperatorEnum {
                        OPERATOR_NOT     = IDMEF_CRITERION_OPERATOR_NOT,
                        OPERATOR_NOCASE  = IDMEF_CRITERION_OPERATOR_NOCASE,

                        OPERATOR_EQUAL   = IDMEF_CRITERION_OPERATOR_EQUAL,
                        OPERATOR_EQUAL_NOCASE = IDMEF_CRITERION_OPERATOR_EQUAL_NOCASE,
                        OPERATOR_NOT_EQUAL = IDMEF_CRITERION_OPERATOR_NOT_EQUAL,
                        OPERATOR_NOT_EQUAL_NOCASE = IDMEF_CRITERION_OPERATOR_NOT_EQUAL_NOCASE,

                        OPERATOR_LESSER = IDMEF_CRITERION_OPERATOR_LESSER,
                        OPERATOR_LESSER_OR_EQUAL = IDMEF_CRITERION_OPERATOR_LESSER_OR_EQUAL,

                        OPERATOR_GREATER = IDMEF_CRITERION_OPERATOR_GREATER,
                        OPERATOR_GREATER_OR_EQUAL = IDMEF_CRITERION_OPERATOR_GREATER_OR_EQUAL,

                        OPERATOR_SUBSTR = IDMEF_CRITERION_OPERATOR_SUBSTR,
                        OPERATOR_SUBSTR_NOCASE = IDMEF_CRITERION_OPERATOR_SUBSTR_NOCASE,
                        OPERATOR_NOT_SUBSTR = IDMEF_CRITERION_OPERATOR_NOT_SUBSTR,
                        OPERATOR_NOT_SUBSTR_NOCASE = IDMEF_CRITERION_OPERATOR_NOT_SUBSTR_NOCASE,

                        OPERATOR_REGEX = IDMEF_CRITERION_OPERATOR_REGEX,
                        OPERATOR_REGEX_NOCASE = IDMEF_CRITERION_OPERATOR_REGEX_NOCASE,
                        OPERATOR_NOT_REGEX = IDMEF_CRITERION_OPERATOR_NOT_REGEX,
                        OPERATOR_NOT_REGEX_NOCASE = IDMEF_CRITERION_OPERATOR_NOT_REGEX_NOCASE,

                        OPERATOR_NULL = IDMEF_CRITERION_OPERATOR_NULL,
                        OPERATOR_NOT_NULL = IDMEF_CRITERION_OPERATOR_NOT_NULL
                };
        };

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

                int match(Prelude::IDMEF *message) const;
                IDMEFCriteria clone() const;
                void andCriteria(const IDMEFCriteria &criteria);
                void orCriteria(const IDMEFCriteria &criteria);
                const std::string toString() const;

                operator const std::string() const;
                operator idmef_criteria_t *() const;
                IDMEFCriteria &operator=(const IDMEFCriteria &criteria);
        };
};

#endif
