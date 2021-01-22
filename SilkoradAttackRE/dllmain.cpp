// dllmain.cpp : Definiuje punkt wejścia dla aplikacji DLL.
#include "pch.h"
#include <vector>
#include <Windows.h>
#include <stdio.h>
#include <algorithm>
#include <fcntl.h>
#include <Psapi.h>
#include <io.h>
#include <iostream>
#include <sstream> 
#include <fstream>
#include <string>



DWORD AttackFuncCall = 0x00791D40;
DWORD MobID = 0x065F3A9E;

DWORD ucAdress = 0x1F718964;

DWORD spawnHookJump = 0x009E0E38;

void Atack(void* monsterID, void* unknowMemory)
{
	void* ad1 = monsterID; void* ad2 = unknowMemory;

	_asm {
		PUSH ad1;
		MOV ECX, ad2;
		CALL AttackFuncCall;
	}
}




VOID Codecave(DWORD destAddress, VOID(*func)(VOID), BYTE nopCount);

// Writes bytes in the current process using an ASM method
VOID WriteBytesASM(DWORD destAddress, LPVOID patch, DWORD numBytes);
DWORD ExtractScoreRetAddr = 0;
DWORD MonsterAdres = 0;


struct MonsterAtackStructure
{
	void* ptr = nullptr;
	void* MonsterID = nullptr;
};

std::vector<MonsterAtackStructure> MonsterList;



VOID Codecave(DWORD destAddress, VOID(*func)(VOID), BYTE nopCount)
{
	// Calculate the code cave for chat interception
	DWORD offset = (PtrToUlong(func) - destAddress) - 5;

	// Buffer of NOPs, static since we limit to 'UCHAR_MAX' NOPs
	BYTE nopPatch[0xFF] = { 0 };

	// Construct the patch to the function call
	BYTE patch[5] = { 0xE8, 0x00, 0x00, 0x00, 0x00 };
	memcpy(patch + 1, &offset, sizeof(DWORD));
	WriteBytesASM(destAddress, patch, 5);

	// We are done if we do not have NOPs
	if (nopCount == 0)
		return;

	// Fill it with nops
	memset(nopPatch, 0x90, nopCount);

	// Make the patch now
	WriteBytesASM(destAddress + 5, nopPatch, nopCount);
}


VOID WriteBytesASM(DWORD destAddress, LPVOID patch, DWORD numBytes)
{
	// Store old protection of the memory page
	DWORD oldProtect = 0;

	// Store the source address
	DWORD srcAddress = PtrToUlong(patch);

	// Make sure page is writeable
	VirtualProtect((void*)(destAddress), numBytes, PAGE_EXECUTE_READWRITE, &oldProtect);

	// Do the patch (oldschool style to avoid memcpy)
	__asm
	{
		nop						// Filler
		nop						// Filler
		nop						// Filler

		mov esi, srcAddress		// Save the address
		mov edi, destAddress	// Save the destination address
		mov ecx, numBytes		// Save the size of the patch
		Start :
		cmp ecx, 0				// Are we done yet?
			jz Exit					// If so, go to end of function

			mov al, [esi]			// Move the byte at the patch into AL
			mov[edi], al			// Move AL into the destination byte
			dec ecx					// 1 less byte to patch
			inc esi					// Next source byte
			inc edi					// Next destination byte
			jmp Start				// Repeat the process
			Exit :
		nop						// Filler
			nop						// Filler
			nop						// Filler
	}

	// Restore old page protection
	VirtualProtect((void*)(destAddress), numBytes, oldProtect, &oldProtect);
}



void ShowMeWhatYouGot()
{


	void* ptr = nullptr;
	memcpy(&ptr, (void*)MonsterAdres, sizeof(void*));


	DWORD BaseAddress = (DWORD)GetModuleHandle(L"sro_client.exe");
	DWORD FinalAddress = (DWORD)(BaseAddress + 0x00D0F80C);

	void* SomeDataPointer = nullptr;
	memcpy(&SomeDataPointer, (void*)FinalAddress, sizeof(void*));
	ptr = ((char*)ptr + 0xf8);

	void* MonsterID;
	memcpy(&MonsterID, (void*)ptr, sizeof(void*));

	MonsterAtackStructure monsterStruc;
	monsterStruc.MonsterID = MonsterID;
	monsterStruc.ptr = ((char*)ptr - 0xf8);
	MonsterList.push_back(monsterStruc);


	std::cout << MonsterAdres << std::endl;
}


__declspec(naked) void CC_MonsterAdress(void)
{
	__asm
	{
		pop ExtractScoreRetAddr

		MOV MonsterAdres, ESI

		PUSHAD
		PUSHFD
	}

	ShowMeWhatYouGot();

	__asm
	{
		POPFD
		POPAD


		PUSH 0
		PUSH EDI
		CALL EDX

		push ExtractScoreRetAddr
		ret
	}
}


BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH: {

		AllocConsole();

		printf("START \n");
		freopen("CONIN$", "r", stdin);
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);

		bool Attacking = false;
		int selectedIndex = -1;
		while (true)
		{
			Codecave(0x009E0D88, CC_MonsterAdress, 0);

			std::vector <MonsterAtackStructure> MonsterListCpy = MonsterList;

			if (MonsterListCpy.size() >= 1) {
				int temp;
			//	printf("1\n");
				if (selectedIndex != -1) {

				memcpy(&temp, MonsterListCpy[selectedIndex].ptr, sizeof(int));
				//printf("\nATACK %04x\n\n", MonsterListCpy[selectedIndex].MonsterID, temp);
					if (temp != 14558916)
					{
						selectedIndex = -1;
					}
				}

				//printf("2\n");
				for (int i = 0; i < MonsterListCpy.size(); i++)
				{
					int temp;
					memcpy(&temp, MonsterListCpy[i].ptr, sizeof(int));
				//	printf("3\n");
					printf("%04x %d\n", MonsterListCpy[i].MonsterID, temp);


					if (temp == 14558916 && Attacking == false && selectedIndex ==-1)
					{
						Attacking = true;
						DWORD BaseAddress = (DWORD)GetModuleHandle(L"sro_client.exe");
						DWORD FinalAddress = (DWORD)(BaseAddress + 0x00D0F80C);

						void* SomeDataPointer = nullptr;
						memcpy(&SomeDataPointer, (void*)FinalAddress, sizeof(void*));
						selectedIndex = i;
						Atack(MonsterListCpy[selectedIndex].MonsterID, SomeDataPointer);

						
						//select(MonsterListCpy[0].MonsterID, SomeDataPointer);
					}
					else
					{
						Attacking = false;
					}
				}
				system("cls");
			}
		}
	}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

