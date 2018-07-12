#include <iostream>
#include <Windows.h>
using namespace std;


uintptr_t findAddress(HANDLE hProc, uintptr_t ptr, uintptr_t offsets[])
{
    unsigned int numOffsets = 0;
    if (offsets[0] != NULL) numOffsets = sizeof(offsets) / sizeof(offsets[0]);

    uintptr_t addr = ptr;
    for (unsigned int i = 0; i < numOffsets; ++i)
    {
        ReadProcessMemory(hProc, (LPCVOID)addr, &addr, sizeof(addr), 0);
        addr += offsets[i];
    }
    return addr;
}

float distance3D(float pX, float pY, float pZ, float eX, float eY, float eZ) {
    return sqrt(pow(pX - eX, 2.0) + pow(pY - eY, 2.0) + pow(pZ - eZ, 2.0));
}
HWND cleanHWND() {
    HWND hwnd = FindWindowA(NULL, "AssaultCube");
    if (hwnd == NULL) {
        printf("coun't find the window\n");
        Sleep(4000);
        exit(-1);
    }
    return hwnd;
}

int main() {
#pragma region EstablishingMemRW
    HWND hwnd = cleanHWND();
    DWORD procID;
    HANDLE handle;
    GetWindowThreadProcessId(hwnd, &procID);
    handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procID);
    if (procID == NULL)
    {
        printf("can't obtain process\n");
        Sleep(4000);
        exit(-1);
    }

#pragma endregion


#pragma region variables

    uintptr_t playerBaseAdd = 0x509B74;//this will be our reference to everything relating to OUR player
    uintptr_t tarHealth = 0;

    uintptr_t xMoOffs[] = { 0x40 };
    uintptr_t yMoOffs[] = { 0x44 };
    uintptr_t xPosOffs[] = { 0x34 };
    uintptr_t yPosOffs[] = { 0x38 };
    uintptr_t zPosOffs[] = { 0x3c };
    uintptr_t nameOffs[] = { 0x225 };
    uintptr_t teamOffs[] = { 0x032c }; // 0 red 1 blue
    uintptr_t healthOffs[] = { 0xf8 };

    uintptr_t pLocZAdd, pLocXAdd, pLocYAdd, aimXAdd, aimYAdd, myTeamAdd = 0;  // where we are standing
    uintptr_t entHealthAdd, entNameAdd, entTeamAdd, entZAdd, entXAdd, entYAdd, currentEntAdd = 0;  // this will use the same offsets as plyZ, but the baseAddress of who we are targeting
    // will be dependant on who we are targeting

    float angleX, angleY, cloAngleX, cloAngleY = 0.0; // I shouldn't need these, But it may be nice to have them incase I want to write to a local variable without
    // reinitiallizeing it everytime I loop

    //variables for the people we are trying to aim at
    uintptr_t entArrayBaseAdd = 0x0050F4F8; //this is the pointer to the list of pointers that points to everyone entity in the GAME, what an ass sentence
    uintptr_t entSize = 0x4;

    float pLocZ, pLocX, pLocY, entLocX, entLocY, entLocZ, aimX, aimY = 0.0f;  // out coordinates, and where we are aiming
    uintptr_t pTeam, eTeam = 0;
    string tarName = "";

    uintptr_t smoothNum = 30;
#pragma endregion

    //now find the addresses of what we are going to change, lets read them first so we can see what we are getting
    pLocXAdd = findAddress(handle, playerBaseAdd, xPosOffs);
    pLocYAdd = findAddress(handle, playerBaseAdd, yPosOffs);
    pLocZAdd = findAddress(handle, playerBaseAdd, zPosOffs);
    aimXAdd = findAddress(handle, playerBaseAdd, xMoOffs);
    aimYAdd = findAddress(handle, playerBaseAdd, yMoOffs);
    myTeamAdd = findAddress(handle, playerBaseAdd, teamOffs);

    //read from those addresses
    ReadProcessMemory(handle, (PVOID*)pLocXAdd, &pLocX, sizeof(pLocX), 0);
    ReadProcessMemory(handle, (PVOID*)pLocYAdd, &pLocY, sizeof(pLocY), 0);
    ReadProcessMemory(handle, (PVOID*)pLocZAdd, &pLocZ, sizeof(pLocZ), 0);

    uintptr_t fovAllow = 20; //degrees of variance
    uintptr_t distAllow = 20;

#pragma region PlayerDetailsPrinting

    ReadProcessMemory(handle, (PVOID*)entArrayBaseAdd, &currentEntAdd, sizeof(currentEntAdd), 0); //this should change our base address to point to the base pointed too
    uintptr_t currentEntityAddBackup = currentEntAdd;

    char value[16] = { 0 };
    int i = 0;
    float dist = 0.0f;
    float minDist = 99999999.0;
    bool aimToggle, inAimFov, found = false;
    printf("to turn of the cheat press f3\nto aim use left alt or right click\n");
    while (!GetAsyncKeyState(VK_F3) /*aiming*/) {
        dist, i = 0;
        minDist = 99999999.0;
        found = false;
        currentEntAdd = currentEntityAddBackup;
        ReadProcessMemory(handle, (PVOID*)myTeamAdd, &pTeam, sizeof(pTeam), 0);//yes I know I am doing this in a loop, but sometimes a team change is forced
        if (GetAsyncKeyState(VK_RBUTTON) || GetAsyncKeyState(VK_LCONTROL)) {
            while (i < 31) {
                currentEntAdd += entSize;//move to next ent

                //team check
                entTeamAdd = findAddress(handle, currentEntAdd, teamOffs);
                ReadProcessMemory(handle, (PVOID*)entTeamAdd, &eTeam, sizeof(eTeam), 0);
                //health check
                entHealthAdd = findAddress(handle, currentEntAdd, healthOffs);
                ReadProcessMemory(handle, (PVOID*)entHealthAdd, &tarHealth, sizeof(tarHealth), 0);
                i++;


                ReadProcessMemory(handle, (PVOID*)aimXAdd, &aimX, sizeof(aimX), 0);
                ReadProcessMemory(handle, (PVOID*)aimYAdd, &aimY, sizeof(aimY), 0);

                ReadProcessMemory(handle, (PVOID*)pLocXAdd, &pLocX, sizeof(pLocX), 0);
                ReadProcessMemory(handle, (PVOID*)pLocYAdd, &pLocY, sizeof(pLocY), 0);
                ReadProcessMemory(handle, (PVOID*)pLocZAdd, &pLocZ, sizeof(pLocZ), 0);



                //get the offsets for this player
                entXAdd = findAddress(handle, currentEntAdd, xPosOffs);
                entYAdd = findAddress(handle, currentEntAdd, yPosOffs);
                entZAdd = findAddress(handle, currentEntAdd, zPosOffs);
                entNameAdd = findAddress(handle, currentEntAdd, nameOffs);

                //for ent pos
                ReadProcessMemory(handle, (PVOID*)entXAdd, &entLocX, sizeof(entLocX), 0);
                ReadProcessMemory(handle, (PVOID*)entYAdd, &entLocY, sizeof(entLocY), 0);
                ReadProcessMemory(handle, (PVOID*)entZAdd, &entLocZ, sizeof(entLocZ), 0);

                //calculate distance from players crosshair
                dist = distance3D(pLocX, pLocY, pLocZ, entLocX, entLocY, entLocZ);
                angleX = (-(float)atan2(entLocX - pLocX, entLocY - pLocY)) / 3.14159265358979323846 * 180.0f + 180.0f;
                angleY = (atan2(entLocZ - pLocZ, dist)) * 180.0f / 3.14159265358979323846;// somthing is wrong with this when I am not at the same height as other players

                if (!(abs(angleX - aimX) > fovAllow || abs(angleY - aimY) > fovAllow)) {
                    if (eTeam != pTeam && (eTeam == 0 || eTeam == 1) && (tarHealth > 0 && tarHealth <= 100)) {

                        //for ent name
                        ReadProcessMemory(handle, (PVOID*)entNameAdd, &value, 16, 0);

                        if (strcmp(value, "")) {
                            if (dist <= minDist) {
                                minDist = dist;
                                found = true;
                                cloAngleX = angleX;
                                cloAngleY = angleY;
                            }
                        }
                    }
                }
            }
            if (found) {

                entNameAdd = findAddress(handle, playerBaseAdd, xMoOffs); // OFFSET for reading in x angle
                ReadProcessMemory(handle, (PVOID*)entNameAdd, &angleX, sizeof(angleX), 0);
                //printf("angle before aiming %f angle we need to aim for %f\n", angleX, cloAngleX);
                //angleX += cloAngleX / smoothNum;
                //where we are aiming + (where we are trying to aim - where we are aiming)/smooth = newAim
                cloAngleX = angleX + (cloAngleX - angleX) / smoothNum;
                WriteProcessMemory(handle, (PVOID)entNameAdd, &cloAngleX, sizeof(angleX), 0);

                //cloAngleY = (atan2(bonez - plyZ, minDist)) * 180.0f / 3.14159265358979323846;
                entNameAdd = findAddress(handle, playerBaseAdd, yMoOffs);
                ReadProcessMemory(handle, (PVOID*)entNameAdd, &angleY, sizeof(angleY), 0);
                cloAngleY = angleY + (cloAngleY - angleY) / smoothNum;
                WriteProcessMemory(handle, (PVOID)entNameAdd, &cloAngleY, sizeof(angleY), 0);
            }
        }
    }
    return 0;
}