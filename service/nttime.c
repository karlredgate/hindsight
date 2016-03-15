#include <windows.h>
#include <ddk/ntapi.h>

#include <stdio.h>

int WINAPI
WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
    LARGE_INTEGER now;
    ZwQuerySystemTime( &now );
    printf( "hello world (%lu)\n", now );
    return 0;
}
