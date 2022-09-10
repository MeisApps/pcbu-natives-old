// Test.cpp : Diese Datei enthält die Funktion "main". Hier beginnt und endet die Ausführung des Programms.
//

#include <iostream>
#include <Windows.h>
#include <WinCred.h>

#pragma comment(lib, "credui.lib")
#pragma comment(lib, "Secur32.lib")

int main()
{
    std::cout << "Hello World!\n";

    BOOL save = false;
    DWORD authPackage = 0;
    LPVOID authBuffer;
    ULONG authBufferSize = 0;
    CREDUI_INFO credUiInfo;

    credUiInfo.pszCaptionText = TEXT("VBoxCaption");
    credUiInfo.pszMessageText = TEXT("VBoxMessage");
    credUiInfo.cbSize = sizeof(credUiInfo);
    credUiInfo.hbmBanner = NULL;
    credUiInfo.hwndParent = NULL;

    DWORD rc = CredUIPromptForWindowsCredentialsW(&(credUiInfo), 0, &(authPackage),
        NULL, 0, &authBuffer, &authBufferSize, &(save), 0);
    printf("Test returned %ld\n", rc);
    return rc;
}
