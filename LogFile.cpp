// LogFile.cpp: implementation of the CLogFile class.
//
//
//////////////////////////////////////////////////////////////////////

/*
'=======================================================================================
'= Arquivo: logfile.cpp
'= Função:  
'= Autor:   Getson Miranda
'= Data:    20/10/2003
'=======================================================================================
*/
#include "stdafx.h"
#include "LogFile.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CLogFile::CLogFile()
{
	idLogFileName();
}

CLogFile::CLogFile(LPCTSTR Filename)
{
	idLogFileName();
}

CLogFile::~CLogFile()
{
}

/*
'=====================================================================
'===		idLogFileName()
'
'Objetivo:	Obtém o caminho para criação e gravação dos 
'			arquivos diários de LOG, no arquivo de 
'			inicialização. Se não encontrar, cria e trabalha 
'			com arquivo de LOG, no diretório do programa 
'			executável do Robo.
'
'=====================================================================
*/
void CLogFile::idLogFileName()
{
	// nome do arquivo do dia
	GetDtHora(1);
	
	// compoe o caminho e o nome do arquivo
	TCHAR ARQ[255];

	::GetModuleFileName(NULL, ARQ, sizeof(ARQ));
	long tam = strlen(ARQ);
	tam -= 12;
	strcpy(&ARQ[tam],DT);
	strcat(ARQ,".LOG"); // acrescenta a extensao .LOG ao arquivo
	
	// abre ou cria arquivo de LOG
	Create(ARQ,"ABERTURA ARQUIVO DE LOG");
}

//////////////////////////////////////////////////////////////////////
// Methods
//////////////////////////////////////////////////////////////////////

/*
'=====================================================================
'===
'
'Objetivo:	Cria o arquivo de LOG do dia, cujo o nome é 
'			gerado no seguinte formato YYYYMMDD.LOG.
'
'Entrada.:	Filename é o caminho parametrizado + o nome do 
'			arquivo de LOG do dia.
'			Text é uma string a ser gravada somente, no 
'			momento em que o arquivo de Log for criado.
'
'Retorno	TRUE = sucesso nas operações internas
'			FALSE = problema durante a execução
'          
'=====================================================================
*/
BOOL CLogFile::Create(LPCTSTR Filename, LPCTSTR Text)
{
   ASSERT( Filename );
   ASSERT( Text );
   m_Filename = Filename;

   ASSERT( !m_Filename.IsEmpty() );

   if( m_Filename.IsEmpty() ) 
	   return TRUE;

   // Write text to (text)file
   CStdioFile f;

   TRY 
   {
	  BOOL res = f.Open( Filename, CFile::modeRead );
	  if( res )
	  {
		  f.Close();
	  }
	  else
	  {
		  res = f.Open( Filename, CFile::modeCreate|CFile::modeWrite|CFile::typeText );
		  if( res ) 
		  {
			  //f.WriteString( Text );
			  //f.WriteString( _T("\n") );
			  f.Close();
			  LogSystemInformation();
		  };
	  }
   }
   CATCH_ALL(e) 
   {
      f.Abort();
#ifdef _DEBUG
      e->ReportError();
#endif
      return FALSE;
   }
	END_CATCH_ALL;
	return TRUE;
};

/*
'=====================================================================
'===
'
'Objetivo:	Grava uma string recebida como parâmetro no final 
'			do arquivo de LOG.
'
'Entrada.:	Text é uma string a ser gravada no final do 
'			arquivo de Log.
'
'Retornos:	TRUE = sucesso no append de texto no arquivo
'			FALSE = o texto não foi gravado
'          
'=====================================================================
*/
BOOL CLogFile::AppendText(LPCTSTR Text, ...)
{
   ASSERT(AfxIsValidString(Text));
   ASSERT(!m_Filename.IsEmpty());
   if( m_Filename.IsEmpty() ) return FALSE;
   
   CStdioFile f;
   CString sText;
   va_list args;   
   va_start(args, Text);   
   sText.FormatV(Text, args);

   TRY 
   {
      BOOL res = f.Open( m_Filename, CFile::modeCreate|CFile::modeNoTruncate|CFile::modeWrite|CFile::typeText );
      if( res ) {
         f.SeekToEnd();
         f.WriteString( sText );
		 f.WriteString( _T("\n") );
         f.Close();
      };
   }
   CATCH_ALL(e) 
   {
      f.Abort();
#ifdef _DEBUG
      e->ReportError();
#endif
      return FALSE;
   }
   END_CATCH_ALL;
   return TRUE;
};

/*
'=====================================================================
'===
'
'Objetivo:	Prepara o arquivo de LOG, para trabalho interno 
'			na classe. Se não encontrar, cria o arquivo na 
'			recepção de um texto para gravação.
'
'Entrada.:	Filename o nome do arquivo que deve estar em 
'			operação, se não existir o arquivo cria.
'
'Retornos:	TRUE = sucesso
'			FALSE = erro na execução
'          
'=====================================================================
*/
BOOL CLogFile::SetFilename(LPCTSTR Filename)
// Sets the log filename.  A new log file will
// be created if the file does not exist.
{
   ASSERT(AfxIsValidString(Filename));
   m_Filename = Filename;
   if( m_Filename.IsEmpty() ) return FALSE;
   return TRUE;
}

/*
'=====================================================================
'===
'
'Objetivo:	Grava um texto padrão de inicialização no 
'			arquivo de LOG, no momento em que ele for criado.
'
'Retornos:	TRUE = sucesso
'			FALSE = erro na execução
'          
'=====================================================================
*/
BOOL CLogFile::LogSystemInformation()
{
   ASSERT(!m_Filename.IsEmpty());

   if( m_Filename.IsEmpty() ) return TRUE;

   // obtem a data e hora local
   SYSTEMTIME time;
   ::GetLocalTime( &time );
   AppendText(_T("Abertura Arquivo - Data: %04d.%02d.%02d Hora: %02d:%02d:%02d"),
              time.wYear,time.wMonth,time.wDay,time.wHour,time.wMinute,time.wSecond);

   // versao do sistema operacional em execucao
   OSVERSIONINFO verinfo;
   verinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   ::GetVersionEx(&verinfo);
   AppendText(_T("Win%s Versao %d.%.2d (build %d) %s\n"), 
      (verinfo.dwPlatformId==VER_PLATFORM_WIN32_NT ? _T("NT") : _T("32")),
      verinfo.dwMajorVersion,
      verinfo.dwMinorVersion,
      verinfo.dwBuildNumber,
      verinfo.szCSDVersion);
  
   // quantidade e tipo do processor(es)
   SYSTEM_INFO sysinfo; 
   LPCTSTR pszProcessor; 
   ::GetSystemInfo(&sysinfo); 

   switch( sysinfo.dwProcessorType ) 
   { 
   case PROCESSOR_INTEL_386: 
   case PROCESSOR_INTEL_486: 
   case PROCESSOR_INTEL_PENTIUM: 
	   pszProcessor = _T("Intel "); 
	   break; 
   case PROCESSOR_MIPS_R4000: 
	   pszProcessor = _T("MIPS R"); 
	   break; 
   case PROCESSOR_ALPHA_21064: 
	   pszProcessor = _T("DEC Alpha "); 
	   break; 
   default: 
	   pszProcessor = _T("Chipset "); 
	   break; 
   }
   
   return AppendText(_T("%s%d, %d Processador(s)\n"), 
      (LPCTSTR)pszProcessor, 
      sysinfo.dwProcessorType, 
      sysinfo.dwNumberOfProcessors);
};

/*
'=====================================================================
'===
'
'Objetivo:	Grava informações internas no final do arquivo de 
'			LOG. Essa informação refere-se a um problema 
'			interno operacional do Robo, o qual pode 
'			ocasionar a parada do programa. Antes de 
'			interromper a sua própria execução por falta de 
'			memória ou recurso de máquina, o robo usa esse 
'			método para faciliar a solução do problema.
'
'Entrada.:	ponto - é a rotina em que ocorreu um problema 
'			interno no robo.
'			msg - é o texto descritivo do erro, podendo ter 
'			texto gerado pelo próprio sistema operacional.
'
'=====================================================================
*/
VOID CLogFile::SetLogInterno(char *ponto,char *msg)
{
	GetDtHora(2);

	AppendText("%s ; %s ; %s ; %s", DT, "CLogFile", ponto, msg);
}

// *************************************************************************************
void CLogFile::GetDtHora(short fmt)
{
	SYSTEMTIME tm;
	int ano, mes, dia, hor, min, seg;

	GetLocalTime(&tm);

	ano = tm.wYear;		// ano yyyy
	mes = tm.wMonth;	// mes mm
	dia = tm.wDay;		// dia dd
	hor = tm.wHour;		// hora hh
	min = tm.wMinute;	// minutos mm
	seg = tm.wSecond;	// segundos ss

	switch(fmt)
	{
		case 1: // yyyymmdd - para nome do arquivo de log do dia
			sprintf(DT,"%04d%02d%02d",ano, mes, dia);
			break;
		case 2: // yyyy/mm/dd hh:mm:ss - para primeira coluna de detalhe do arquivo
			sprintf(DT,"%02d/%02d/%04d %02d:%02d:%02d",
					dia,mes,ano,hor,min,seg);
			break;
		case 3: // yyyymmddhhmmss - para primeira coluna de detalhe do arquivo
			sprintf(DT,"%04d%02d%02d%02d%02d%02d",
					ano,mes,dia,hor,min,seg);
			break;
		default: // dd/mm/yyyy - retorno default
			sprintf(DT,"%02d/%02d/%04", dia, mes, ano);
			break;
	}
}