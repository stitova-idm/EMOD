/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <map>

using namespace std;

namespace Kernel
{
    typedef int RouteIndex;
    typedef int GroupIndex;

    class TransmissionGroupMembership_t : public map<RouteIndex, GroupIndex>
    {
        // TODO - serialization?
    };
}