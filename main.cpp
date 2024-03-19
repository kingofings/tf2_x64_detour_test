#include <iostream>
#include <dlfcn.h>
#include <unistd.h>
#include <cstring>
#include <funchook.h>
#include <iomanip>
#include <sys/mman.h>

#define CTFPlayer_OnTakeDamage_ADDRESS 0x00000000011e5310
#define MAD_MILK_MULT_ADDRESS 0x0000000000B947D8
#define MAD_MILK_MOVSS_FLOAT_OPERAND_OFFSET 0x009062CC
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

ulong BaseAddress = 0x0;

int Detour_OnTakeDamage(void* pThis, void* pDamageInfo)
{
    std::cout << "CTFPlayer::OnTakeDamage detour called!" << std::endl;

    int* pCritType = reinterpret_cast<int*>(reinterpret_cast<char*>(pDamageInfo) + CTakeDamageInfo_m_eCritType_OFFSET);
    int* pDamageType = reinterpret_cast<int*>(reinterpret_cast<char*>(pDamageInfo) + CTakeDamageInfo_m_bitsDamageType_OFFSET);

    if (!pCritType || !pDamageType)
    {
        std::cout << "CritType, DamageType pointer invalid!" << std::endl;
        return originalOnTakeDamage(pThis, pDamageInfo);
    }

    //Only set if it's not already Critical it breaks for some reason and I don't want to look into it
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

    while (fgets(line, sizeof(line), pFile))
    {
        if (strstr(line, "server_srv.so") != NULL && strstr(line, "r--p") != NULL)
        {
            BaseAddress = strtoul(line, NULL, 16);
            break;
        }
    }

    fclose(pFile);

    *(void**)(&originalOnTakeDamage) = (void*)(BaseAddress + CTFPlayer_OnTakeDamage_ADDRESS);

    std::cout << "CTFPlayer::OnTakeDamage Address: " << &originalOnTakeDamage << std::endl;
}

void SetupDetour()
{
    std::cout << "Trying to create funchook" << std::endl;
    funchook_t* pFunchook = funchook_create();

    int result = funchook_prepare(pFunchook, (void**)&originalOnTakeDamage, (void*)Detour_OnTakeDamage);

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


void SetupPatchMadMilkMult()
{
    std::cout << "Patching Mad Milk Multiplier..." << std::endl;

    unsigned char* pBytes = reinterpret_cast<unsigned char*>(BaseAddress + MAD_MILK_MULT_ADDRESS);

    const unsigned char signature[] = {0xF3, 0x0F, 0x10, 0x05};

    const size_t signatureLength = sizeof(signature) / sizeof(signature[0]);

    bool match = std::memcmp(pBytes, signature, signatureLength) == 0;

    for (size_t i = 0; i < signatureLength; i++)
    {
        std::cout << "Byte " << i << ": "
        << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(pBytes[i]) << std::endl;
    }

    if (!match)
    {
        std::cerr << "Byte Signature does not match for Mad Milk Patch!" << std::endl;
        return;
    }

    //the movss instruction is 8 bytes long
    uintptr_t targetaddress = reinterpret_cast<uintptr_t>(pBytes) + 8 + MAD_MILK_MOVSS_FLOAT_OPERAND_OFFSET;

    float* pValue = reinterpret_cast<float*>(targetaddress);

    std::cout << "Default value: " << *pValue << std::endl << "Allocating new memory..." << std::endl;

    //Yes in this example we will have this dangling around and leaking. We do not care in this scenario
    float* pFloat = reinterpret_cast<float*>(malloc(sizeof(float)));

    *pFloat = 0.1f;

    std::cout << "Writing Bytes..." << std::endl;

    uintptr_t pageSize = sysconf(_SC_PAGESIZE);
    uint8_t* pAlignAddress = (uint8_t*)(reinterpret_cast<uintptr_t>(pBytes) & ~(pageSize - 1));
    uint8_t* pEndAddress = reinterpret_cast<uint8_t*>(pBytes) + 8;
    uintptr_t alignSize = pEndAddress - pAlignAddress;

    /*
     * This here is unstable AF if this patch is applied after players have joined any execution of
     * CTFWeaponBase::ApplyOnHitAttributes while someone has the milk condition will crash.
     * I don't know why yet because I probably don't understand something yet.
     * Will need to research
     */

    /*
     * UPDATE: I now apply the detour after the patch now it's reversed it will crash if you apply it before the
     * player joining
     */

    /*
     * UPDATE 2: Fixed it I forgot to align with the Page beginning and the length incase pBytes is on 2 seperate pages!
     * Before we just took the page size as a length now we calculate the length by getting the start Address of the page
     * then we subtract it from the end address of the movss instruction aka the address of the next instruction
     */
    if (mprotect(pAlignAddress, alignSize, PROT_READ | PROT_WRITE | PROT_EXEC) != 0)
    {
        perror("mprotect");
        return;
    }

    int32_t offset = reinterpret_cast<uintptr_t>(pFloat) - reinterpret_cast<uintptr_t>(pBytes + 8);

    unsigned char newSig[8] = {0xF3, 0x0F, 0x10, 0x05};

    //little endian
    newSig[7] = (offset >> 24) & 0xFF;
    newSig[6] = (offset >> 16) & 0xFF;
    newSig[5] = (offset >> 8) & 0xFF;
    newSig[4] = offset & 0xFF;

    std::memcpy(pBytes, newSig, 8);

    for (size_t i = 0; i < 8; i++)
    {
        std::cout << "Byte " << i << ": "
                  << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(pBytes[i]) << std::endl;
    }

    std::cout << "Wrote new allocated memory with new Value to operand: " << *pFloat << std::endl;

}

__attribute__((constructor))
void Setup()
{
    GetFunctionPointer();
    SetupPatchMadMilkMult();
    SetupDetour();
}