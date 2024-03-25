//
// Created by kingo on 21/03/2024.
//

#ifndef TF2_DETOUR_LIBRARY_UTILITY_H
#define TF2_DETOUR_LIBRARY_UTILITY_H


#include <cstdlib>

class Utility {
public:
     ulong GetBaseAddress();

private:
    static ulong m_BaseAddress;
};
#endif //TF2_DETOUR_LIBRARY_UTILITY_H
