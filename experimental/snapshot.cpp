
#define _WIN32_WINNT 0x0601
#define _UNICODE

#include <stdio.h>
#include <tchar.h>

#include <windows.h>
#include <shlwapi.h>

#include <vss.h>
#include <vswriter.h>
#include <vsbackup.h>
#include <vsserror.h>

#define HILONG(ll) (ll >> 32 & LONG_MAX)
#define LOLONG(ll) ((long)(ll))

void ReleaseInterface (IUnknown* unkn) {
  if (unkn != NULL)
      unkn->Release();
}

// Does this need to be %ls for wchar_t
static void
print_guid( const wchar_t *description, GUID& guid ) {
    wprintf( L"%s {%.8lx-%.4x-%.4x-%.2x%.2x-%.2x%.2x%.2x%.2x%.2x%.2x}\n",
            description, guid.Data1, guid.Data2, guid.Data3,
            guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
            guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7] );
}

static void
parse_args( WCHAR *volume_name, size_t length ) {
    int wargc;
    LPWSTR *wargv = CommandLineToArgvW( GetCommandLineW(), &wargc );

    if ( wargv == NULL ) {
        wprintf(L"CommandLineToArgvW failed\n");
        exit( 1 );
    }

    if ( wargc != 2 ) {
        printf( "Usage: simplesnapshot.exe volume\n" );
        printf( "example: simplesnapshot.exe [d:\\] or [path_to_mounted_folder]\n" );
        exit( 1 );
    }

    wcscpy_s( volume_name, (rsize_t)length, wargv[1] );
    LocalFree( wargv );
}

/** \brief Check if process is elevated
 * A program using VSS must run in elevated mode
 */
static bool
process_is_not_elevated() {
    HANDLE hToken;
    OpenProcessToken( GetCurrentProcess(), TOKEN_READ, &hToken );

    TOKEN_ELEVATION elevation;
    DWORD length;

    GetTokenInformation( hToken, TokenElevation, &elevation, sizeof (elevation), &length );

    return elevation.TokenIsElevated == FALSE;
}

/**
 */
IVssBackupComponents *
VssFactory() {
    IVssBackupComponents *vss = NULL;

    HRESULT result = CreateVssBackupComponents( &vss );

    if ( FAILED(result) ) {
        wprintf( L"CreateVssBackupComponents failed!\n" );
        exit( 1 );
    }

    if ( vss == NULL ) {
        wprintf( L"pBackup is null!\n" );
        exit( 1 );
    }

    return vss;
}

/**
 */
static void
InitializeForBackup( IVssBackupComponents *vss ) {
    wprintf( L"about to InitializeForBackup\n" );
    HRESULT result = vss->InitializeForBackup( NULL );

    if ( FAILED(result) ) {
        wprintf( L"InitializeForBackup failed!\n" );
        exit( 1 );
    }
    wprintf( L"InitializeForBackup succeeded!\n" );
}

/**
 */
static void
SetContext( IVssBackupComponents *vss ) {
    // result = pBackup->SetContext( VSS_CTX_BACKUP | VSS_CTX_CLIENT_ACCESSIBLE_WRITERS | VSS_CTX_APP_ROLLBACK );
    HRESULT result = vss->SetContext( VSS_CTX_CLIENT_ACCESSIBLE_WRITERS );

    switch ( result ) {
    case E_INVALIDARG     : wprintf( L"Invalid ARG\n" ); break;
    case VSS_E_BAD_STATE  : wprintf( L"VSS Bad state\n" ); break;
    case VSS_E_UNEXPECTED : wprintf( L"VSS unexpected\n" ); break;
    }

    if ( FAILED(result) ) {
        wprintf( L"SetContext Error: HRESULT = 0x%08lx\n", result );
        ReleaseInterface( vss );
        exit( -19 );
    }
    wprintf( L"SetContext succeeded!\n" );
}

/**
 */
static void
GatherWriterMetadata( IVssBackupComponents *vss ) {
    IVssAsync *event = NULL;

    // Prompts each writer to send the metadata they have collected
    HRESULT result = vss->GatherWriterMetadata( &event );

    if ( FAILED(result) ) {
        wprintf( L"GatherWriterMetadata failed!\n" );
        exit( 1 );
    }

    wprintf( L"Gathering metadata from writers...\n" );
    result = event->Wait( 1000 ); // msecs

    if ( FAILED(result) ) {
        wprintf( L"Async->Wait failed!\n" );
        exit( 1 );
    }

    event->Release();
}

/**
 */
static void
StartSnapshotSet( IVssBackupComponents *vss, VSS_ID *snapshot ) {
    // Creates a new, empty shadow copy set
    wprintf( L"calling StartSnapshotSet...\n" );
    HRESULT result = vss->StartSnapshotSet( snapshot );

    switch (result) {
    case E_INVALIDARG     : wprintf( L"Invalid ARG\n" ); break;
    case E_OUTOFMEMORY    : wprintf( L"out of memory\n" ); break;
    case VSS_E_BAD_STATE  : wprintf( L"VSS Bad state\n" ); break;
    case VSS_E_UNEXPECTED : wprintf( L"VSS unexpected\n" ); break;
    case VSS_E_UNSUPPORTED_CONTEXT :
        wprintf( L"VSS unsupported context\n" );
        break;
    case VSS_E_SNAPSHOT_SET_IN_PROGRESS :
        wprintf( L"VSS snapshot set in progress\n" );
        break;
    }

    if ( FAILED(result) ) {
        wprintf( L"- Returned HRESULT = 0x%08lx\n", result );
        ReleaseInterface( vss );
        exit( 1 );
    }
}

/**
 */
static void
AddToSnapshotSet( IVssBackupComponents *vss, WCHAR *volume_name, VSS_ID *snapshot ) {
    wprintf( L"AddToSnapshotSet...\n" );
    HRESULT result = vss->AddToSnapshotSet( volume_name, GUID_NULL, snapshot );

    if ( FAILED(result) ) {
        wprintf( L"- Returned HRESULT = 0x%08lx\n", result);
        ReleaseInterface( vss );
        exit( 1 );
    }
}

/**
 */
static void
SetBackupState( IVssBackupComponents *vss ) {
    // Configure the backup operation for Copy with no backup history
    HRESULT result = vss->SetBackupState( false, false, VSS_BT_COPY, false );

    if ( FAILED(result) ) {
        wprintf( L"- Returned HRESULT = 0x%08lx\n", result );
        ReleaseInterface( vss );
        exit( 1 );
    }
}

/**
 */
static void
PrepareForBackup( IVssBackupComponents *vss ) {
    IVssAsync* event = NULL;

    // Make VSS generate a PrepareForBackup event
    HRESULT result = vss->PrepareForBackup( &event );

    if ( FAILED(result) ) {
        wprintf( L"PrepareForBackup failed!\n" );
        exit( 1 );
    }

    wprintf( L"Preparing for backup...\n" );
    result = event->Wait( 1000 );

    event->Release();

    if ( FAILED(result) ) {
        wprintf( L"Prepare->Wait failed!\n" );
        exit( 1 );
    }
}

/**
 */
static void
DoSnapshotSet( IVssBackupComponents *vss ) {
    // Commit all snapshots in this set
    IVssAsync* event = NULL;
    HRESULT result = vss->DoSnapshotSet( &event );

    if ( FAILED(result) ) {
        wprintf( L"DoSnapshotSet failed!\n" );
        exit( 1 );
    }

    wprintf( L"Taking snapshots...\n" );
    result = event->Wait( 10000 );

    if ( FAILED(result) ) {
        wprintf( L"DoSnapshotSet->Wait failed!\n" );
        exit( 1 );
    }

    event->Release();
}

/**
 * typedef struct _VSS_SNAPSHOT_PROP {
 *   VSS_ID             m_SnapshotId;
 *   VSS_ID             m_SnapshotSetId;
 *   LONG               m_lSnapshotsCount;
 *   VSS_PWSZ           m_pwszSnapshotDeviceObject;
 *   VSS_PWSZ           m_pwszOriginalVolumeName;
 *   VSS_PWSZ           m_pwszOriginatingMachine;
 *   VSS_PWSZ           m_pwszServiceMachine;
 *   VSS_PWSZ           m_pwszExposedName;
 *   VSS_PWSZ           m_pwszExposedPath;
 *   VSS_ID             m_ProviderId;
 *   LONG               m_lSnapshotAttributes;
 *   VSS_TIMESTAMP      m_tsCreationTimestamp;
 *   VSS_SNAPSHOT_STATE m_eStatus;
 *   } VSS_SNAPSHOT_PROP, *PVSS_SNAPSHOT_PROP;
 */
static void
PrintSnapshotProperties( IVssBackupComponents *vss, VSS_ID *snapshot ) {
    wprintf( L"Get the snapshot device object from the properties...\n" );

    VSS_SNAPSHOT_PROP properties = {0};
    HRESULT result = vss->GetSnapshotProperties( *snapshot, &properties );

    if ( FAILED(result) ) {
        wprintf( L"GetSnapshotProperties failed!\n" );
        exit( 1 );
    }

    print_guid( L" Snapshot ID    : ", properties.m_SnapshotId );
    print_guid( L" Snapshot Set ID: ", properties.m_SnapshotSetId );
    print_guid( L" Provider ID    : ", properties.m_ProviderId );
    wprintf( L" OriginalVolumeName : %ls\n", properties.m_pwszOriginalVolumeName );

    if (properties.m_pwszExposedName != NULL) {
        wprintf( L" ExposedName : %ls\n", properties.m_pwszExposedName );
    }
    if (properties.m_pwszExposedPath != NULL) {
        wprintf( L" ExposedPath : %ls\n", properties.m_pwszExposedPath );
    }

    if (properties.m_pwszSnapshotDeviceObject != NULL) {
        wprintf( L" DeviceObject : %ls\n", properties.m_pwszSnapshotDeviceObject );
    }

    SYSTEMTIME stUTC, stLocal;
    FILETIME ftCreate;
    // Convert the last-write time to local time.
    ftCreate.dwHighDateTime  =  HILONG(properties.m_tsCreationTimestamp);
    ftCreate.dwLowDateTime   =  LOLONG(properties.m_tsCreationTimestamp);

    FileTimeToSystemTime(&ftCreate, &stUTC);
    SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

    wprintf( L"Created : %02d/%02d/%d  %02d:%02d \n", stLocal.wMonth, stLocal.wDay, stLocal.wYear, stLocal.wHour, stLocal.wMinute );
    wprintf( L"\n" );

    VssFreeSnapshotProperties(&properties);
}

/**
 */
static void
GetSnapshotPrefix( IVssBackupComponents *vss, VSS_ID *snapshot, WCHAR *base, size_t length ) {
    VSS_SNAPSHOT_PROP properties = {0};
    HRESULT result = vss->GetSnapshotProperties( *snapshot, &properties );

    if ( FAILED(result) ) {
        wprintf( L"GetSnapshotProperties failed!\n" );
        exit( 1 );
    }

    if (properties.m_pwszSnapshotDeviceObject == NULL) {
        wprintf( L"snapshot prefix is null!\n" );
        exit( 1 );
    }

    wcscpy_s( base, (rsize_t)length, properties.m_pwszSnapshotDeviceObject );
}

/**
 */
static void
ProcessSnapshot( WCHAR *pattern, size_t length ) {
    wcscat_s( pattern, (rsize_t)length, L"\\*" );
    wprintf( L"processing %sn", pattern );

    WIN32_FIND_DATAW data;

    HANDLE file = FindFirstFileW( pattern, &data );

    for (;;) {
        if ( file == INVALID_HANDLE_VALUE ) {
            break;
        }

        wchar_t file_type = L'F';
        if ( data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
            file_type = L'D';
            // should recurse here
        }

        wprintf( L"%c %s\n", file_type, data.cFileName );
        BOOL result = FindNextFileW( file, &data );

        if ( result == FALSE ) {
            int error = GetLastError();

            switch (error) {
            case ERROR_NO_MORE_FILES:
                wprintf( L"No more files\n" );
                return;
            default:
                wprintf( L"There was an error finding next file\n" );
            }
        }

        // Process file
        // run through policy engine
        // generarte full path
        // hash the file
        // create file in temp dir
        // compress input file - write to temp file
    }

    wprintf( L"Done searching files\n" );
}

/**
 */
int main( int argc, char ** argv ) {
    WCHAR volume_name[MAX_PATH] = {0};
    parse_args( volume_name, sizeof (volume_name) );
    wprintf( L"create snapshot for volume %s :\n", volume_name );

    if ( process_is_not_elevated() ) {
        wprintf( L"this program must run in elevated mode\n" );
        return 1;
    }

    if ( CoInitialize(NULL) != S_OK ) {
        wprintf( L"CoInitialize failed!\n" );
        return 1;
    }

    IVssBackupComponents *vss = VssFactory();
    InitializeForBackup( vss );
    SetContext( vss );
    GatherWriterMetadata( vss );

    VSS_ID snapshot;
    StartSnapshotSet( vss, &snapshot );
    AddToSnapshotSet( vss, volume_name, &snapshot );
    SetBackupState( vss );
    PrepareForBackup( vss );
    DoSnapshotSet( vss );
    PrintSnapshotProperties( vss, &snapshot );

    WCHAR prefix[MAX_PATH] = {0};
    GetSnapshotPrefix( vss, &snapshot, prefix, sizeof (prefix) );

    ProcessSnapshot( prefix, sizeof (prefix) );

    //  free system resources allocated when IVssBackupComponents::GatherWriterMetadata was called.
    vss->FreeWriterMetadata();
    vss->Release();

    return 1;
}

/* vim: set autoindent expandtab sw=4 : */
