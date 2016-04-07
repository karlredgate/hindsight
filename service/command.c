
#include <windows.h>
#include <stdio.h>

#include <tcl.h>

static Tcl_Interp*
create_tcl_interp( int argc, CONST char **argv, Tcl_AppInitProc *appInit ) {
    Tcl_Interp *interp;
    Tcl_FindExecutable( argv[0] );
    interp = Tcl_CreateInterp();
    if ( interp == NULL ) {
        printf( "failed to create interpreter" );
        exit( 1 );
    }
    char *args = Tcl_Merge(argc-1, argv+1);
    Tcl_DString argString;
    Tcl_ExternalToUtfDString(NULL, args, -1, &argString);
    Tcl_SetVar(interp, "argv", Tcl_DStringValue(&argString), TCL_GLOBAL_ONLY);
    Tcl_DStringFree(&argString);
    ckfree(args);
    Tcl_SetVar2Ex(interp, "argc", NULL, Tcl_NewIntObj(argc - 1), TCL_GLOBAL_ONLY);
    Tcl_SetVar2Ex(interp, "argv0", NULL, Tcl_NewStringObj(argv[0], -1), TCL_GLOBAL_ONLY);

    if ( getenv("INIT_VERSION") ) {
        Tcl_SetVar(interp, "tcl_interactive", "0", TCL_GLOBAL_ONLY);
        char *run_level = getenv("RUNLEVEL");
        if ( run_level ) {
            Tcl_SetVar2Ex(interp, "run_level", NULL, Tcl_NewStringObj(run_level, -1), TCL_GLOBAL_ONLY);
        }
    } else {
        Tcl_SetVar(interp, "tcl_interactive", "1", TCL_GLOBAL_ONLY);
    }

    // disable some dangerous commands
    /*
    Tcl_HideCommand( interp, "exit", "hiddenExit" );
    Tcl_GetCommandInfo(interp, "puts", &putsObjCmd);
    Tcl_CreateObjCommand(interp, "puts", PutsSyslog_cmd, (ClientData)0, NULL);
    Tcl_CreateObjCommand(interp, "ERROR", SyslogErr_cmd, (ClientData)0, NULL);
    Tcl_CreateObjCommand(interp, "WARN", SyslogWarn_cmd, (ClientData)0, NULL);

    if ( Channel_Initialize(interp) == false ) {
        printf( "Channel_Initialize failed" );
    }
    */

    appInit( interp );
    return interp;
}

/**
 *  */ 
static int
Svc_Init( Tcl_Interp *interp ) {
    return TCL_OK;
}

long
channel_receive( char *buffer, int length ) {
    gets( buffer );
}

void
channel_send( long receiver, int result, char *data ) {
    printf( "R: %s\n", data );
}

static void
InstallService() {
    SC_HANDLE scm = OpenSCManager( 0, 0, SC_MANAGER_CREATE_SERVICE );

    if ( scm == NULL ) {
	printf( "Failed to install service\n" );
        return;
    }

    CloseServiceHandle( scm );
}

static void
UninstallService() {
    SC_HANDLE scm = OpenSCManager( 0, 0, SC_MANAGER_CREATE_SERVICE );

    if ( scm == NULL ) {
	printf( "Failed to install service\n" );
        return;
    }

    CloseServiceHandle( scm );
}

int
command_thread(int argc, CONST char **argv) {
    Tcl_Interp *interp = create_tcl_interp( argc, argv, Svc_Init );
    char request[1024];
    char response[1024];

    for (;;) {
        long sender = channel_receive( request, sizeof(request) );
        int result = Tcl_EvalEx( interp, request, -1, TCL_EVAL_GLOBAL );
        strncpy( response, Tcl_GetStringResult(interp), sizeof(response) );
        channel_send( sender, result, response );
    }

    return 0;
}

/* vim: set autoindent expandtab sw=4 : */
