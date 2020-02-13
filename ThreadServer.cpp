// ThreadServer.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To merge the proxy/stub code into the object DLL, add the file
//      dlldatax.c to the project.  Make sure precompiled headers
//      are turned off for this file, and add _MERGE_PROXYSTUB to the
//      defines for the project.
//
//      If you are not running WinNT4.0 or Win95 with DCOM, then you
//      need to remove the following define from dlldatax.c
//      #define _WIN32_WINNT 0x0400
//
//      Further, if you are running MIDL without /Oicf switch, you also
//      need to remove the following define from dlldatax.c.
//      #define USE_STUBLESS_PROXY
//
//      Modify the custom build rule for ThreadServer.idl by adding the following
//      files to the Outputs.
//          ThreadServer_p.c
//          dlldata.c
//      To build a separate proxy/stub DLL,
//      run nmake -f ThreadServerps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "ThreadServer.h"
#include "dlldatax.h"

#include "ThreadServer_i.c"

#ifdef _MERGE_PROXYSTUB
extern "C" HINSTANCE hProxyDll;
#endif

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()

#define				S_DEBUG				1
static short RUN = 1;

void Log(DWORD, short, int,char *);
short ConsultarStatusThread(long);
short MudarPrioridadeThread(long);
short FinalizarThread(long);
short SuspenderThread(long);
short ResumirThread(long);
short CriaThread(long, short, char, _clsCallerObj *);
DWORD WINAPI ThreadFunc( long & );

/** CtrlThread
  *
  * Tabela para gerenciamento das threads
  *
  **/
static struct CtrlThread
{
	long			thid_e;			// thread identificador externo VB x VC
	HANDLE			thh;			// thread handle
	DWORD			thid_i;			// thread identificador no Windows
	char			thst;			// thread status
	COleDateTime	thhie;			// thread hora inicial de execução
	/* Parametros da DLL */
	short			prioridade;
	char			str[MAX_PATH];
	struct _clsCallerObj	*obj;
}	ctth[65535];

class CThreadServerApp : public CWinApp
{
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CThreadServerApp)
	public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CThreadServerApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CThreadServerApp, CWinApp)
	//{{AFX_MSG_MAP(CThreadServerApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CThreadServerApp theApp;

BOOL CThreadServerApp::InitInstance()
{
#ifdef _MERGE_PROXYSTUB
    hProxyDll = m_hInstance;
#endif
    _Module.Init(ObjectMap, m_hInstance, &LIBID_THREADSERVERLib);
    return CWinApp::InitInstance();
}

int CThreadServerApp::ExitInstance()
{
    _Module.Term();
    return CWinApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
#ifdef _MERGE_PROXYSTUB
    if (PrxDllCanUnloadNow() != S_OK)
        return S_FALSE;
#endif
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    return (AfxDllCanUnloadNow()==S_OK && _Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
#ifdef _MERGE_PROXYSTUB
    if (PrxDllGetClassObject(rclsid, riid, ppv) == S_OK)
        return S_OK;
#endif
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
    HRESULT hRes = PrxDllRegisterServer();
    if (FAILED(hRes))
        return hRes;
#endif
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
    PrxDllUnregisterServer();
#endif
    return _Module.UnregisterServer(TRUE);
}

//////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Log( DWORD err, short local, int obj, char *strerr )
{
	CLogFile *oLog;
	oLog = new CLogFile;
	if( ! oLog )
		return;

	LPVOID	lpMsgBuf;
	FormatMessage
			(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			err,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &lpMsgBuf,
			0,
			NULL
			);

	char msg[_MAX_PATH];
	strcpy(msg,(LPCTSTR)lpMsgBuf);
	int tam = strlen(msg);
	msg[tam-2] = '\0';

	char str[_MAX_PATH];
	sprintf(str, "[%.03d, %s]%s", local, msg,strerr);

	oLog->AppendText(str);

	return;
}

/////////////////////////////////////////////////////////////////////////////

DWORD WINAPI ThreadFunc( long &lpParam )
{
	long i = lpParam;

	CLogFile *oLog;
	oLog = new CLogFile;
	if( ! oLog )
		return -1;

	ctth[i].thhie = COleDateTime::GetCurrentTime();

	char m[_MAX_PATH];

	sprintf(m, "Inicio Thread[%d], parametros %.03d, %s, %u", ctth[i].thid_e, ctth[i].prioridade, ctth[i].str, ctth[i].obj);

	oLog->AppendText(m);

	short k = 0;

	HRESULT hr;
	CLSID IDclsMonta;
	CoInitialize(NULL);

	TRY
	{
		hr = CLSIDFromProgID(OLESTR("CompServer.clsMonta"),&IDclsMonta);
	}
	CATCH_ALL (e)
	{
		char   szCause[1024];
		char strFormatted[2048];
		e->GetErrorMessage(szCause, 1024);
		sprintf(strFormatted, "ThreadFunc[%.06d]", ctth[i].thid_e);
		strcat(strFormatted, szCause);
#ifdef S_DEBUG
		OutputDebugString(strFormatted);
#endif
		Log( GetLastError(), 501, ctth[i].thid_e, strFormatted );
		return -1;
	}
	END_CATCH_ALL

	if(FAILED(hr))
	{
		Log( GetLastError(), 7, ctth[i].thid_e, NULL );
		return -1;
	}

	_clsMonta *t = NULL;

	TRY
	{
		hr = CoCreateInstance(IDclsMonta, NULL, CLSCTX_INPROC_SERVER,__uuidof(_clsMonta),(LPVOID *) &t);
		//hr = CoCreateInstance(_clsMonta, NULL, CLSCTX_INPROC_SERVER,__uuidof(_clsMonta),(LPVOID *) &t);
	}
	CATCH_ALL(e)
	{
		char   szCause[1024];
		char strFormatted[2048];
		e->GetErrorMessage(szCause, 1024);
		sprintf(strFormatted, "ThreadFunc[%.06d]", ctth[i].thid_e);
		strcat(strFormatted, szCause);
#ifdef S_DEBUG
		OutputDebugString(strFormatted);
#endif
		Log( GetLastError(), 501, ctth[i].thid_e, strFormatted );
		return -1;
	}
	END_CATCH_ALL

	if( FAILED(hr) )
	{
		Log( GetLastError(), 8, ctth[i].thid_e, NULL );
		return -1;
	}

	while( ctth[i].thid_e )
	{
		if( ! k )
		{
			sprintf(m, "Thread[%d] executando c/ parms %.03d, %s, %s", ctth[i].thid_e, ctth[i].prioridade, ctth[i].str, ctth[i].obj);
			k++; // = 1
			try
			{
				t->ChamaComponentes(ctth[i].str, ctth[i].thid_e, ctth[i].obj);
			}
			catch(_com_error e)
			{
				Log( GetLastError(), 9, ctth[i].thid_e, NULL );
				return -1;
			}
		}
		Sleep(1000);
	}

	t->Release ();
	CoUninitialize();

	return 0;
}

/////////////////////////////////////////////////////////////////////////////

/**	CControleThread::CriaThread.
  *
  *	@parm t identificador da thread.
  * @parm p permite mudar a prioridade de execução da thread.
  *	@parm c código de pré envio?
  * @parm o object caller?
  *
  **/
short CriaThread(long t, short p, char *c, struct _clsCallerObj * objCaller)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	long i = t;					// código da thread
	ctth[i].prioridade = p;		// prioridade
	ctth[i].obj = objCaller;	// objeto instanciado no VB

	// Thread ja esta em execução ?
	if( ctth[i].thid_e )
	{
		// Registrar chamada invalida
		Log( GetLastError(), 1, ctth[i].thid_e, NULL );
		return S_FALSE;
	}

	ctth[i].thid_e = i;			// id thread VB x VC - Cria
	strcpy( ctth[i].str, c);	// string do parâmetro

	TRY
	{
		ctth[i].thh = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadFunc, &i, 0, &ctth[i].thid_i);
	}
	CATCH_ALL(e)
	{
		char   szCause[1024];
		char strFormatted[2048];
		e->GetErrorMessage(szCause, 1024);
		sprintf(strFormatted, "CriaThread[%.06d]", ctth[i].thid_e);
		strcat(strFormatted, szCause);
#ifdef S_DEBUG
		OutputDebugString(strFormatted);
#endif
		Log( GetLastError(), 501, ctth[i].thid_e, strFormatted );
		return -1;
	}
	END_CATCH_ALL

	Sleep(1000);

	// Falhou na criação da Thread
	DWORD err = GetLastError();
	if( err )
	{
		// Registrar erro na criação da Thread
		Log( err, 2, ctth[i].thid_e, NULL );
		ctth[i].thid_e = 0;
		return S_FALSE;
	}

	// Sucesso !
	Log( err, 0, ctth[i].thid_e, NULL );

	// Prioridade ----------------->
	if( ctth[i].prioridade < -7 || ctth[i].prioridade > 31 )
		ctth[i].prioridade = 1;

	MudarPrioridadeThread(i);
	// Prioridade <-----------------

	/*while(RUN)
	{
		Sleep(1000);
	}*/

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

/** CControleThread::SuspenderThread
  *
  * @parm t identificador da thread.
  *
  **/
short SuspenderThread(long t)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	long i = t;

	if( ! ctth[i].thid_e ) // Thread não é válida
	{
		// Registrar chamada invalida
		Log( GetLastError(), 3, ctth[i].thid_e, NULL );
		return S_FALSE;
	}

	// * //
	DWORD err = 0;
	TRY
	{
		err = SuspendThread( ctth[i].thh );
	}
	CATCH_ALL (e)
	{
		char   szCause[1024];
		char strFormatted[2048];
		e->GetErrorMessage(szCause, 1024);
		sprintf(strFormatted, "SuspendThread[%.06d]", ctth[i].thid_e);
		strcat(strFormatted, szCause);
#ifdef S_DEBUG
		OutputDebugString(strFormatted);
#endif
		Log( GetLastError(), 501, ctth[i].thid_e, strFormatted );
		return -1;
	}
	END_CATCH_ALL

	Log( err, 502, ctth[i].thid_e, NULL );

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

/**	CControleThread::FinalizarThread
  *
  * @parm t identificador da thread.
  *
  **/
short FinalizarThread(long t)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	long i = t;

	CLogFile *oLog;
	oLog = new CLogFile;
	if( ! oLog )
		return NULL;

	if( ! ctth[i].thid_e ) // Thread não é válida
	{
		// Registrar chamada invalida
		Log( GetLastError(), 4, ctth[i].thid_e, NULL );
		return -1;
	}

	TRY
	{
		TerminateThread(ctth[i].thh,0);
	}
	CATCH_ALL(e)
	{
		char   szCause[1024];
		char strFormatted[2048];
		e->GetErrorMessage(szCause, 1024);
		sprintf(strFormatted, "FinalizarThread[%.06d]", ctth[i].thid_e);
		strcat(strFormatted, szCause);
#ifdef S_DEBUG
		OutputDebugString(strFormatted);
#endif
		Log( GetLastError(), 501, ctth[i].thid_e, strFormatted );
		return -1;
	}
	END_CATCH_ALL

	TRY
	{
		CloseHandle(ctth[i].thh);
	}
	CATCH_ALL(e)
	{
		char   szCause[1024];
		char strFormatted[2048];
		e->GetErrorMessage(szCause, 1024);
		sprintf(strFormatted, "FinalizarThread[%.06d]", ctth[i].thid_e);
		strcat(strFormatted, szCause);
#ifdef S_DEBUG
		OutputDebugString(strFormatted);
#endif
		Log( GetLastError(), 501, ctth[i].thid_e, strFormatted );
		return -1;
	}
	END_CATCH_ALL

	char m[_MAX_PATH];

	sprintf(m, "Finalizada Thread[%d]",ctth[i].thid_e);

	TRY
	{
		oLog->AppendText(m);
	}
	CATCH_ALL(e)
	{
		char   szCause[1024];
		char strFormatted[2048];
		e->GetErrorMessage(szCause, 1024);
		sprintf(strFormatted, "FinalizarThread[%.06d]", ctth[i].thid_e);
		strcat(strFormatted, szCause);
#ifdef S_DEBUG
		OutputDebugString(strFormatted);
#endif
		Log( GetLastError(), 501, ctth[i].thid_e, strFormatted );
		return -1;
	}
	END_CATCH_ALL

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

/** CControleThread::MudarPrioridadeThread
  *
  * @parm t identificador da thread.
  *
  **/
short MudarPrioridadeThread(long t)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	long i = t;

	if( ! ctth[i].thid_e ) // Thread não é válida
	{
		// Registrar chamada invalida
		Log( GetLastError(), 500, ctth[i].thid_e, NULL );
		return S_FALSE;
	}

	TRY
	{
		SetThreadPriority(ctth[i].thh, ctth[i].prioridade);
	}
	CATCH_ALL (e)
	{
		char   szCause[1024];
		char strFormatted[2048];
		e->GetErrorMessage(szCause, 1024);
		sprintf(strFormatted, "MudarPrioridadeThread[%.06d]", ctth[i].thid_e);
		strcat(strFormatted, szCause);
#ifdef S_DEBUG
		OutputDebugString(strFormatted);
#endif
		Log( GetLastError(), 501, ctth[i].thid_e, strFormatted );
		return -1;
	}
	END_CATCH_ALL

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

/** CControleThread::ConsultarStatusThread
  *
  * @parm t identificador da thread.
  *
  **/
short ConsultarStatusThread(long t, char *ch)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	long i = t;

	if( ! ctth[t].thid_e ) // Thread não é válida
	{
		// Registrar chamada invalida
		Log( GetLastError(), 6, ctth[i].thid_e, NULL );
		return S_FALSE;
	}
	char strFormatted[2048];
	COleDateTime	tmexec;
	DWORD			thExit = 0;
	char			st[64];
	double			thTotSegs = 0;

	if( GetExitCodeThread(ctth[i].thh, &thExit) )
	{
		switch(thExit)
		{
		case STILL_ACTIVE:	strcpy(st, "ESTA ATIVA");
							break;

		default:			sprintf(st, "Codigo Saida[%d]",thExit);
							break;
		}
	}
	else
	{
		strcpy(st, "Falha obtendo codigo saida!");
	}

	tmexec = COleDateTime::GetCurrentTime();
	COleDateTimeSpan spanElapsed = (tmexec.m_dt - ctth[i].thhie.m_dt );
	thTotSegs = spanElapsed.GetTotalSeconds();

	sprintf(strFormatted,
			"ConsultaThread[%.06d]WID[%.06d]%s Segundos em execucao[%.02d]",
			ctth[i].thid_e, ctth[i].thid_i, st, thTotSegs);

#ifdef S_DEBUG
		OutputDebugString(strFormatted);
#endif

	Log( GetLastError(), 501, ctth[i].thid_e, strFormatted );

	strcpy(ch, strFormatted);

	return 0;
}


/////////////////////////////////////////////////////////////////////////////

/** ResumirThread
  *
  * @parm t identificador da thread.
  *
  **/
short ResumirThread(long t)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	// TODO: Add your implementation code here
	long i = t;

	if( ! ctth[i].thid_e ) // Thread não é válida
	{
		// Registrar chamada invalida
		Log( GetLastError(), 3, ctth[i].thid_e, NULL );
		return S_FALSE;
	}

	// * //
	DWORD err = 0;
	TRY
	{
		err = ResumeThread( ctth[i].thh );
	}
	CATCH_ALL (e)
	{
		char   szCause[1024];
		char strFormatted[2048];
		e->GetErrorMessage(szCause, 1024);
		sprintf(strFormatted, "ResumirThread[%.06d]", ctth[i].thid_e);
		strcat(strFormatted, szCause);
#ifdef S_DEBUG
		OutputDebugString(strFormatted);
#endif
		Log( GetLastError(), 501, ctth[i].thid_e, strFormatted );
		return -1;
	}
	END_CATCH_ALL

	Log( err, 502, ctth[i].thid_e, NULL );

	return S_OK;
}