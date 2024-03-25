//
// Created by kingo on 21/03/2024.
//

#ifndef TF2_DETOUR_LIBRARY_DETOURS_H
#define TF2_DETOUR_LIBRARY_DETOURS_H

#define CTFPlayer_OnTakeDamage_ADDRESS 0x00000000011e5310
#define CTakeDamageInfo_m_eCritType_OFFSET 100
#define CTakeDamageInfo_m_bitsDamageType_OFFSET 60
#define CTakeDamageInfo_m_hWeapon_OFFSET 44
#define DMG_CRITICAL 1048576

using CTFPlayer_OnTakeDamage = int (*)(void* pCTFPlayerObj, void* pDamageInfo);

void SetupDetours();
void GetFunctionPointers();
int Detour_OnTakeDamage(void* pThis, void* pDamageInfo);


#endif //TF2_DETOUR_LIBRARY_DETOURS_H
