//
// Created by kingo on 21/03/2024.
//

#include <iostream>
#include <unistd.h>
#include <cstring>
#include "Utility.h"

ulong Utility::m_BaseAddress = 0;

ulong Utility::GetBaseAddress()
{
    if (m_BaseAddress > 0) return m_BaseAddress;

    std::cout << "Getting Base Pointer..." << std::endl;

    pid_t pid = getpid();
    char mapsFile[256];
    snprintf(mapsFile, sizeof(mapsFile), "/proc/%d/maps", pid);

    FILE* pFile = fopen(mapsFile, "r");
    if (!pFile)
    {
        perror("Failed to open maps file");
        return 0;
    }

    char line[256];

    while (fgets(line, sizeof(line), pFile))
    {
        if (strstr(line, "server_srv.so") != NULL && strstr(line, "r--p") != NULL)
        {
            m_BaseAddress = strtoul(line, NULL, 16);
            break;
        }
    }
    fclose(pFile);

    return m_BaseAddress;
}
