
#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#define SERVICE_NAME  _T("hindsight")

SERVICE_STATUS        g_ServiceStatus = {0};
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;

DWORD WINAPI
ServiceWorkerThread( LPVOID lpParam ) {
    OutputDebugString( _T("HindSight: ServiceWorkerThread: Entry") );

    //  Periodically check if the service has been requested to stop
    while ( WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0 ) {
        Sleep(3000); /* Perform main service function here */
    }

    OutputDebugString( _T("HindSight: ServiceWorkerThread: Exit") );
    return ERROR_SUCCESS;
}

VOID WINAPI
ServiceCtrlHandler( DWORD CtrlCode ) {
    switch (CtrlCode) {
    case SERVICE_CONTROL_STOP :
        OutputDebugString(_T("HindSight: ServiceCtrlHandler: SERVICE_CONTROL_STOP Request"));

        if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
            break;

        /* Perform tasks neccesary to stop the service here */
        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        g_ServiceStatus.dwWin32ExitCode = 0;
        g_ServiceStatus.dwCheckPoint = 4;

        if ( SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE ) {
            OutputDebugString(_T("HindSight: ServiceCtrlHandler: SetServiceStatus returned error"));
        }

        // This will signal the worker thread to start shutting down
        SetEvent(g_ServiceStopEvent);
        break;

     default:
         break;
    }

    OutputDebugString(_T("HindSight: ServiceCtrlHandler: Exit"));
}

static int
set_service_starting( SERVICE_STATUS_HANDLE handle ) {
    SERVICE_STATUS status;

    ZeroMemory( &status, sizeof (status) );
    status.dwServiceType             = SERVICE_WIN32_OWN_PROCESS;
    status.dwControlsAccepted        = 0;
    status.dwCurrentState            = SERVICE_START_PENDING;
    status.dwWin32ExitCode           = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint              = 0;

    if ( SetServiceStatus(handle, &status) == FALSE ) {
        OutputDebugString( _T("Service: ServiceMain: SetServiceStatus returned error") );
        return -1;
    }

    return 0;
}

static int
set_service_running( SERVICE_STATUS_HANDLE handle ) {
    SERVICE_STATUS status;

    ZeroMemory( &status, sizeof (status) );
    status.dwServiceType             = SERVICE_WIN32_OWN_PROCESS;
    status.dwControlsAccepted        = SERVICE_ACCEPT_STOP;
    status.dwCurrentState            = SERVICE_RUNNING;
    status.dwWin32ExitCode           = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint              = 0;

    if ( SetServiceStatus(handle, &status) == FALSE ) {
        OutputDebugString( _T("Service: ServiceMain: SetServiceStatus returned error") );
        return -1;
    }

    return 0;
}

static int
set_service_stopped( SERVICE_STATUS_HANDLE handle ) {
    SERVICE_STATUS status;

    ZeroMemory( &status, sizeof (status) );
    status.dwServiceType             = SERVICE_WIN32_OWN_PROCESS;
    status.dwControlsAccepted        = 0;
    status.dwCurrentState            = SERVICE_STOPPED;
    status.dwWin32ExitCode           = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint              = 3;

    if ( SetServiceStatus(handle, &status) == FALSE ) {
        OutputDebugString( _T("Service: ServiceMain: SetServiceStatus returned error") );
        return -1;
    }

    return 0;
}

// typedef VOID (WINAPI *LPSERVICE_MAIN_FUNCTIONW)(DWORD dwNumServicesArgs,LPWSTR *lpServiceArgVectors);
VOID WINAPI
ServiceMain( DWORD argc, LPTSTR *argv ) {
    g_StatusHandle = RegisterServiceCtrlHandler( SERVICE_NAME, ServiceCtrlHandler );

    /* this writes to C:\Windows\SysWOW64\ */
    FILE *o = fopen( "hindsight.log", "w" );
    if ( o != NULL ) {
        fprintf( o, "ARG: %ld\n", argv[0] );
        fprintf( o, "ARG: %ld\n", argv[1] );
        fclose( o );
    }

    // if ( g_StatusHandle == NULL ) {  // COMPILER WARNING
    if ( g_StatusHandle == (SERVICE_STATUS_HANDLE)0 ) {
        return;
    }

    set_service_starting( g_StatusHandle );

    // Create stop event to wait on later.
    g_ServiceStopEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    if ( g_ServiceStopEvent == NULL ) {
        OutputDebugString(_T("HindSight: ServiceMain: CreateEvent(g_ServiceStopEvent) returned error"));

        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState     = SERVICE_STOPPED;
        g_ServiceStatus.dwWin32ExitCode    = GetLastError();
        g_ServiceStatus.dwCheckPoint       = 1;

        if ( SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE ) {
            OutputDebugString(_T("HindSight: ServiceMain: SetServiceStatus returned error"));
        }

        return;
    }

    set_service_running( g_StatusHandle );

    // Start the thread that will perform the main task of the service
    HANDLE hThread = CreateThread( NULL, 0, ServiceWorkerThread, NULL, 0, NULL );
    // Wait until our worker thread exits effectively signaling that the service needs to stop
    WaitForSingleObject( hThread, INFINITE );

    /* Perform any cleanup tasks */
    OutputDebugString(_T("HindSight: ServiceMain: Performing Cleanup Operations"));

    CloseHandle( g_ServiceStopEvent );

    set_service_stopped( g_StatusHandle );
}

static int
install_service( LPCTSTR service_name ) {
    SC_HANDLE manager_handle;
    SC_HANDLE service_handle;
    TCHAR szPath[MAX_PATH];

    /* if ( !GetModuleFileName( "", szPath, MAX_PATH ) ) { */
    if ( !GetModuleFileName( NULL, szPath, MAX_PATH ) ) {
        printf("Cannot install service (%ld)\n", GetLastError());
        return -1;
    }

    // Get a handle to the SCM database. 
    manager_handle = OpenSCManager( 
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 
 
    if ( manager_handle == NULL ) {
        printf("OpenSCManager failed (%ld)\n", GetLastError());
        return -1;
    }

    // Create the service
    service_handle = CreateService( 
        manager_handle,            // SCM database 
        service_name,               // name of service 
        service_name,               // service name to display 
        SERVICE_ALL_ACCESS,        // desired access 
        SERVICE_WIN32_OWN_PROCESS, // service type 
        SERVICE_DEMAND_START,      // start type 
        SERVICE_ERROR_NORMAL,      // error control type 
        szPath,                    // path to service's binary 
        NULL,                      // no load ordering group 
        NULL,                      // no tag identifier 
        NULL,                      // no dependencies 
        NULL,                      // LocalSystem account 
        NULL                       // no password 
    );
 
    if ( service_handle == NULL ) {
        printf("CreateService failed (%ld)\n", GetLastError()); 
        CloseServiceHandle( manager_handle );
        return -1;
    } else {
        printf("Service installed successfully\n"); 
    }

    CloseServiceHandle( service_handle ); 
    CloseServiceHandle( manager_handle );

    return 0;
}

static int
start_service() {
    printf( "unimplemented\n" );
    return -1;
}

static int
stop_service() {
    printf( "unimplemented\n" );
    return -1;
}

static int
uninstall_service( LPCTSTR service_name ) {
    int result = -1;

    SC_HANDLE manager = OpenSCManager( 0, 0, SC_MANAGER_CONNECT );
    if ( manager == NULL ) {
        printf( "Could not find SCM (%d)\n", GetLastError() );
        return -1; // Error message
    }

    SC_HANDLE service = OpenService( manager, service_name, SERVICE_QUERY_STATUS | DELETE );
    CloseServiceHandle( manager );

    if ( service == NULL ) { // Already uninstalled
        printf( "service already uninstalled (%ld)\n", GetLastError() );
        return -1;
    }

    SERVICE_STATUS status;
    if ( QueryServiceStatus(service, &status) == 0 ) {
        printf( "cannot query service (%ld)\n", GetLastError() );
        result = -1;
        goto finish;
    }

    if ( status.dwCurrentState == SERVICE_STOPPED ) {
        DeleteService( service );
        result = 0;
        goto finish;
    }

    // Should add code to stop it
    printf("service is not stopped (%ld)\n", GetLastError());

finish:
    CloseServiceHandle( service );
    return result;
}

static int
run_service( TCHAR *service_name ) {
    SERVICE_TABLE_ENTRY service_table[] = {
       { service_name, (LPSERVICE_MAIN_FUNCTION) ServiceMain },
       { NULL, NULL }
    };

    if ( StartServiceCtrlDispatcher(service_table) == FALSE ) {
        OutputDebugString( _T("HindSight: Main: StartServiceCtrlDispatcher returned error") );
        return GetLastError();
    }

    return 0;
}

int _tmain( int argc, TCHAR *argv[] ) {
    // LPCTSTR service_name = _T("hindsight");
    TCHAR *service_name = _T("hindsight");

    if ( argc == 1 ) return run_service( service_name );

    if ( lstrcmpi(argv[1], TEXT("install")) == 0 ) {
        return install_service( service_name );
    }

    if ( lstrcmpi(argv[1], TEXT("start")) == 0 ) {
        return start_service( service_name );
    }

    if ( lstrcmpi(argv[1], TEXT("stop")) == 0 ) {
        return stop_service( service_name );
    }

    if ( lstrcmpi(argv[1], TEXT("uninstall")) == 0 ) {
        return uninstall_service( service_name );
    }

    return -1;
}

/* vim: set autoindent expandtab syntax=c : */
