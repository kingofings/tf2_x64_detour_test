//
// Created by kingo on 21/03/2024.
//

#include <iostream>
#include "detours.h"
#include "Utility.h"
#include <funchook.h>
#include "Enums.h"
#include "CTFPlayer.h"

CTFPlayer_OnTakeDamage fCTFPlayer_OnTakeDamage = nullptr;

void SetupDetours()
{
    GetFunctionPointers();

    std::cout << "[detours.cpp] Trying to create funchook" << std::endl;
    funchook_t* pFunchook = funchook_create();


    int result = funchook_prepare(pFunchook,(void**)&fCTFPlayer_OnTakeDamage, (void*)Detour_OnTakeDamage);

    if (result != FUNCHOOK_ERROR_SUCCESS)
    {
        std::cerr << "[detours.cpp] Funchook prepare failed: " << funchook_error_message(pFunchook);
        return;
    }

    result = funchook_install(pFunchook, 0);

    if (result != FUNCHOOK_ERROR_SUCCESS)
    {
        std::cerr << "[detours.cpp] Funchook install failed: " << funchook_error_message(pFunchook);
        return;
    }

    std::cout << "[detours.cpp] Prepared Detour!" << std::endl;
}

void GetFunctionPointers()
{
    std::cout << "[detours.cpp] Getting Function Pointers..." << std::endl;

    Utility util;

    ulong baseAddress = util.GetBaseAddress();

    if (baseAddress == 0)
    {
        std::cerr << "[detours.cpp ERROR] Unable to get Base Address!" << std::endl;
        return;
    }

    *(void**)(&fCTFPlayer_OnTakeDamage) = (void*)(baseAddress + CTFPlayer_OnTakeDamage_ADDRESS);

    std::cout << "[detours.cpp] CTFPlayer::OnTakeDamage Address: " << &fCTFPlayer_OnTakeDamage << std::endl;
}

int Detour_OnTakeDamage(void* pThis, void* pDamageInfo)
{
    std::cout << "[detours.cpp] CTFPlayer::OnTakeDamage detour called!" << std::endl;

    CTFPlayer* pPlayer = reinterpret_cast<CTFPlayer*>(pThis);


    std::cout << "[detours.cpp] Blast Jump state pThis: " << pPlayer->m_iBlastJumpState << std::endl;

    int* pCritType = reinterpret_cast<int*>(reinterpret_cast<char*>(pDamageInfo) + CTakeDamageInfo_m_eCritType_OFFSET);
    int* pDamageType = reinterpret_cast<int*>(reinterpret_cast<char*>(pDamageInfo) + CTakeDamageInfo_m_bitsDamageType_OFFSET);

    Only set if it's not already Critical it breaks for some reason and I don't want to look into it
    if (*pCritType != CritType_Crit && !(*pDamageType & DMG_CRITICAL))
    {
        *pCritType = CritType_Crit;
        *pDamageType += DMG_CRITICAL;
    }

    return fCTFPlayer_OnTakeDamage(pThis, pDamageInfo);
}
