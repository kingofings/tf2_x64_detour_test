#include <iostream>
#include <unistd.h>
#include <cstring>
#include <iomanip>
#include <sys/mman.h>
#include "Utility.h"
#include "detours.h"

#define MAD_MILK_MULT_ADDRESS 0x0000000000B947D8
#define MAD_MILK_MOVSS_FLOAT_OPERAND_OFFSET 0x009062CC


void SetupPatchMadMilkMult()
{
    std::cout << "Patching Mad Milk Multiplier..." << std::endl;

    Utility util;

    unsigned char* pBytes = reinterpret_cast<unsigned char*>(util.GetBaseAddress() + MAD_MILK_MULT_ADDRESS);

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
    SetupPatchMadMilkMult();
    SetupDetours();
}