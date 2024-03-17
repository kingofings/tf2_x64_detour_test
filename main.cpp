#include <iostream>
#include <dlfcn.h>
#include <unistd.h>
#include <cstring>
#include <funchook.h>


#define CTFPlayer_OnTakeDamage_ADDRESS 0x00000000011e5310
#define CTakeDamageInfo_m_eCritType_OFFSET 100
#define CTakeDamageInfo_m_bitsDamageType_OFFSET 60
#define DMG_CRITICAL 1048576

enum
{
    CritType_None = 0,
    CritType_MiniCrit,
    CritType_Crit
};

using CTFPlayer_OnTakeDamage = int (*)(void* pThis, void* pDamageInfo);

CTFPlayer_OnTakeDamage originalOnTakeDamage = nullptr;

int Detour_OnTakeDamage(void* pThis, void* pDamageInfo)
{
    std::cout << "CTFPlayer::OnTakeDamage detour called!" << std::endl;

    int* pCritType = reinterpret_cast<int*>(reinterpret_cast<char*>(pDamageInfo) + CTakeDamageInfo_m_eCritType_OFFSET);
    int* pDamageType = reinterpret_cast<int*>(reinterpret_cast<char*>(pDamageInfo) + CTakeDamageInfo_m_bitsDamageType_OFFSET);

    if (!pCritType || !pDamageType)
    {
        std::cout << "CritType, DamageType pointer invalid!" << std::endl;
        return false;
    }

    //Only set if it's not already Critical it breaks for some reason and i don't want to look into it
    if (*pCritType != CritType_Crit && !(*pDamageType & DMG_CRITICAL))
    {
        *pCritType = CritType_Crit;
        *pDamageType += DMG_CRITICAL;
    }

    return originalOnTakeDamage(pThis, pDamageInfo);
}

void GetFunctionPointer()
{
    std::cout << "Getting Function Pointer..." << std::endl;

    pid_t pid = getpid();
    char mapsFile[256];
    snprintf(mapsFile, sizeof(mapsFile), "/proc/%d/maps", pid);

    FILE* pFile = fopen(mapsFile, "r");
    if (!pFile)
    {
        perror("Failed to open maps file");
        return;
    }

    char line[256];
    unsigned long baseAddress = 0;

    while (fgets(line, sizeof(line), pFile))
    {
        if (strstr(line, "server_srv.so") != NULL && strstr(line, "r--p") != NULL)
        {
            baseAddress = strtoul(line, NULL, 16);
            break;
        }
    }

    fclose(pFile);

    *(void**)(&originalOnTakeDamage) = (void*)(baseAddress + CTFPlayer_OnTakeDamage_ADDRESS);

    std::cout << "CTFPlayer::OnTakeDamage Address: " << &originalOnTakeDamage << std::endl;
}

__attribute__((constructor))
void setupDetour()
{
    GetFunctionPointer();

    std::cout << "Trying to create funchook" << std::endl;
    funchook_t* pFunchook = funchook_create();

    int result = funchook_prepare(pFunchook, (void**)&originalOnTakeDamage, (int*)Detour_OnTakeDamage);

    if (result != FUNCHOOK_ERROR_SUCCESS)
    {
        std::cerr << "Funchook prepare failed: " << funchook_error_message(pFunchook);
        return;
    }

    result = funchook_install(pFunchook, 0);

    if (result != FUNCHOOK_ERROR_SUCCESS)
    {
        std::cerr << "Funchook install failed: " << funchook_error_message(pFunchook);
        return;
    }

    std::cout << "Prepared Detour!" << std::endl;
}