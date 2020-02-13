
ThreadServerps.dll: dlldata.obj ThreadServer_p.obj ThreadServer_i.obj
	link /dll /out:ThreadServerps.dll /def:ThreadServerps.def /entry:DllMain dlldata.obj ThreadServer_p.obj ThreadServer_i.obj \
		mtxih.lib mtx.lib mtxguid.lib \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \
		ole32.lib advapi32.lib 

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		/MD \
		$<

clean:
	@del ThreadServerps.dll
	@del ThreadServerps.lib
	@del ThreadServerps.exp
	@del dlldata.obj
	@del ThreadServer_p.obj
	@del ThreadServer_i.obj
