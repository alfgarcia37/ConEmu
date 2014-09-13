﻿
/*
Copyright (c) 2009-2014 Maximus5
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define HIDE_USE_EXCEPTION_INFO
#include <windows.h>
#include "defines.h"
#include "MAssert.h"
#include "Memory.h"
#include "MStrSafe.h"
#include "RConStartArgs.h"
#include "common.hpp"
#include "WinObjects.h"
#include "CmdLine.h"

// Restricted in ConEmuHk!
#ifdef MCHKHEAP
#undef MCHKHEAP
#define MCHKHEAP PRAGMA_ERROR("Restricted in ConEmuHk")
#endif

#ifdef __GNUC__
#define SecureZeroMemory(p,s) memset(p,0,s)
#endif

#define DefaultSplitValue 500

#ifdef _DEBUG

// На этом - ассерт возникает в NextArg
// L"C:\\Windows\\system32\\cmd.exe /c echo moving \"\"..\\_VCBUILD\\final.ConEmuCD.32W.vc9\\ConEmuCD.pdb\"\" to \"..\\..\\Release\\ConEmu\\ConEmuCD.pdb\""
// L"\"C:\\Program Files\\Microsoft SDKs\\Windows\\v7.0\\bin\\rc.EXE\" /D__GNUC__ /l 0x409 /fo\"\"..\\_VCBUILD\\final.ConEmu.32W.vc9\\obj\\ConEmu.res\"\" /d NDEBUG ConEmu.rc"


void RConStartArgs::RunArgTests()
{
	CmdArg s;
	s.Set(L"Abcdef", 3);
	int nDbg = lstrcmp(s, L"Abc");
	_ASSERTE(nDbg==0);
	s.Set(L"qwerty");
	nDbg = lstrcmp(s, L"qwerty");
	_ASSERTE(nDbg==0);
	s.Empty();
	//s.Set(L""); // !! Set("") must trigger ASSERT !!
	nDbg = s.ms_Arg ? lstrcmp(s, L"") : -2;
	_ASSERTE(nDbg==0);

	struct { LPCWSTR pszWhole; LPCWSTR pszCmp[10]; } lsArgTest[] = {
		{L"\"C:\\ConEmu\\ConEmuC64.exe\"  /PARENTFARPID=1 /C \"C:\\GIT\\cmdw\\ad.cmd CE12.sln & ci -m \"Solution debug build properties\"\"",
			{L"C:\\ConEmu\\ConEmuC64.exe", L"/PARENTFARPID=1", L"/C", L"C:\\GIT\\cmdw\\ad.cmd", L"CE12.sln", L"&", L"ci", L"-m", L"Solution debug build properties"}},
		{L"/C \"C:\\ad.cmd file.txt & ci -m \"Commit message\"\"",
			{L"/C", L"C:\\ad.cmd", L"file.txt", L"&", L"ci", L"-m", L"Commit message"}},
		{L"\"This is test\" Next-arg \t\n \"Third Arg +++++++++++++++++++\" ++", {L"This is test", L"Next-arg", L"Third Arg +++++++++++++++++++"}},
		{L"\"\"cmd\"\"", {L"cmd"}},
		{L"\"\"c:\\Windows\\System32\\cmd.exe\" /?\"", {L"c:\\Windows\\System32\\cmd.exe", L"/?"}},
		// Following example is crazy, but quotation issues may happens
		//{L"First Sec\"\"ond \"Thi\"rd\" \"Fo\"\"rth\"", {L"First", L"Sec\"\"ond", L"Thi\"rd", L"Fo\"\"rth"}},
		{L"First \"Fo\"\"rth\"", {L"First", L"Fo\"\"rth"}},
		// Multiple commands
		{L"set ConEmuReportExe=VIM.EXE & SH.EXE", {L"set", L"ConEmuReportExe=VIM.EXE", L"&", L"SH.EXE"}},
		// Inside escaped arguments
		{L"reg.exe add \"HKCU\\MyCo\" /ve /t REG_EXPAND_SZ /d \"\\\"C:\\ConEmu\\ConEmuPortable.exe\\\" /Dir \\\"%V\\\" /cmd \\\"cmd.exe\\\" \\\"-new_console:nC:cmd.exe\\\" \\\"-cur_console:d:%V\\\"\" /f",
			// Для наглядности:
			// reg.exe add "HKCU\MyCo" /ve /t REG_EXPAND_SZ
			//    /d "\"C:\ConEmu\ConEmuPortable.exe\" /Dir \"%V\" /cmd \"cmd.exe\" \"-new_console:nC:cmd.exe\" \"-cur_console:d:%V\"" /f
			{L"reg.exe", L"add", L"HKCU\\MyCo", L"/ve", L"/t", L"REG_EXPAND_SZ", L"/d",
			 L"\\\"C:\\ConEmu\\ConEmuPortable.exe\\\" /Dir \\\"%V\\\" /cmd \\\"cmd.exe\\\" \\\"-new_console:nC:cmd.exe\\\" \\\"-cur_console:d:%V\\\"",
			 L"/f"}},
		// After 'Inside escaped arguments' regression bug appears
		{L"/dir \"C:\\\" /icon \"cmd.exe\" /single", {L"/dir", L"C:\\", L"/icon", L"cmd.exe", L"/single"}},
		{L"cmd \"one.exe /dir \\\"C:\\\\\" /log\" \"two.exe /dir \\\"C:\\\" /log\" end", {L"cmd", L"one.exe /dir \\\"C:\\\\\" /log", L"two.exe /dir \\\"C:\\\" /log", L"end"}},
		{NULL}
	};
	for (int i = 0; lsArgTest[i].pszWhole; i++)
	{
		s.Empty();
		LPCWSTR pszTestCmd = lsArgTest[i].pszWhole;
		int j = -1;
		while (lsArgTest[i].pszCmp[++j])
		{
			if (NextArg(&pszTestCmd, s) != 0)
			{
				_ASSERTE(FALSE && "Fails on token!");
			}
			else
			{
				nDbg = lstrcmp(s, lsArgTest[i].pszCmp[j]);
				_ASSERTE(nDbg==0);
			}
		}
	}

	bool bTest = true;
	for (size_t i = 0; bTest; i++)
	{
		RConStartArgs arg;
		int nDbg;
		LPCWSTR pszCmp;

		switch (i)
		{
		case 21:
			pszCmp = L"cmd '-new_console' `-new_console` \\\"-new_console\\\"";
			arg.pszSpecialCmd = lstrdup(pszCmp);
			arg.ProcessNewConArg();
			_ASSERTE(0==lstrcmp(arg.pszSpecialCmd, pszCmp) && arg.NewConsole==crb_Undefined);
			break;
		case 20:
			arg.pszSpecialCmd = lstrdup(L"\"c:\\cmd.exe\" \"-new_console\" \"c:\\file.txt\"");
			arg.ProcessNewConArg();
			_ASSERTE(0==lstrcmp(arg.pszSpecialCmd, L"\"c:\\cmd.exe\" \"c:\\file.txt\""));
			break;
		case 19:
			arg.pszSpecialCmd = lstrdup(L"\"c:\\cmd.exe\" -new_console:n \"c:\\file.txt\"");
			arg.ProcessNewConArg();
			_ASSERTE(0==lstrcmp(arg.pszSpecialCmd, L"\"c:\\cmd.exe\" \"c:\\file.txt\""));
			break;
		case 18:
			arg.pszSpecialCmd = lstrdup(L"\"c:\\cmd.exe\" \"-new_console:n\" \"c:\\file.txt\"");
			arg.ProcessNewConArg();
			_ASSERTE(0==lstrcmp(arg.pszSpecialCmd, L"\"c:\\cmd.exe\" \"c:\\file.txt\""));
			break;
		case 17:
			arg.pszSpecialCmd = lstrdup(L"c:\\cmd.exe \"-new_console:n\" \"c:\\file.txt\"");
			arg.ProcessNewConArg();
			_ASSERTE(0==lstrcmp(arg.pszSpecialCmd, L"c:\\cmd.exe \"c:\\file.txt\""));
			break;
		case 16:
			arg.pszSpecialCmd = lstrdup(L"\"c:\\cmd.exe\" \"-new_console:n\" c:\\file.txt");
			arg.ProcessNewConArg();
			_ASSERTE(0==lstrcmp(arg.pszSpecialCmd, L"\"c:\\cmd.exe\" c:\\file.txt"));
			break;
		case 15:
			arg.pszSpecialCmd = lstrdup(L"c:\\file.txt -cur_console");
			arg.ProcessNewConArg();
			_ASSERTE(0==lstrcmp(arg.pszSpecialCmd, L"c:\\file.txt"));
			break;
		case 14:
			arg.pszSpecialCmd = lstrdup(L"\"c:\\file.txt\" -cur_console");
			arg.ProcessNewConArg();
			_ASSERTE(0==lstrcmp(arg.pszSpecialCmd, L"\"c:\\file.txt\""));
			break;
		case 13:
			arg.pszSpecialCmd = lstrdup(L" -cur_console \"c:\\file.txt\"");
			arg.ProcessNewConArg();
			_ASSERTE(0==lstrcmp(arg.pszSpecialCmd, L"\"c:\\file.txt\""));
			break;
		case 12:
			arg.pszSpecialCmd = lstrdup(L"-cur_console \"c:\\file.txt\"");
			arg.ProcessNewConArg();
			_ASSERTE(0==lstrcmp(arg.pszSpecialCmd, L"\"c:\\file.txt\""));
			break;
		case 11:
			arg.pszSpecialCmd = lstrdup(L"-cur_console c:\\file.txt");
			arg.ProcessNewConArg();
			_ASSERTE(0==lstrcmp(arg.pszSpecialCmd, L"c:\\file.txt"));
			break;
		case 10:
			pszCmp = L"reg.exe add \"HKCU\\command\" /ve /t REG_EXPAND_SZ /d \"\\\"C:\\ConEmu\\ConEmuPortable.exe\\\" /Dir \\\"%V\\\" /cmd \\\"cmd.exe\\\" \\\"-new_console:nC:cmd.exe\\\" \\\"-cur_console:d:%V\\\"\" /f";
			arg.pszSpecialCmd = lstrdup(pszCmp);
			arg.ProcessNewConArg();
			_ASSERTE(lstrcmp(arg.pszSpecialCmd, pszCmp)==0 && arg.NewConsole==crb_Undefined);
			break;
		case 9:
			pszCmp = L"\"C:\\Windows\\system32\\cmd.exe\" /C \"\"C:\\Python27\\python.EXE\"\"";
			arg.pszSpecialCmd = lstrdup(pszCmp);
			arg.ProcessNewConArg();
			_ASSERTE(lstrcmp(arg.pszSpecialCmd, pszCmp)==0);
			break;
		case 8:
			arg.pszSpecialCmd = lstrdup(L"cmd --new_console -cur_console:a");
			arg.ProcessNewConArg();
			_ASSERTE(lstrcmp(arg.pszSpecialCmd, L"cmd --new_console")==0 && arg.NewConsole==crb_Undefined && arg.RunAsAdministrator==crb_On);
			break;
		case 7:
			arg.pszSpecialCmd = lstrdup(L"cmd -cur_console:d:\"C:\\My docs\":t:\"My title\" \"-cur_console:C:C:\\my cmd.ico\" -cur_console:P:\"<PowerShell>\":a /k ver");
			arg.ProcessNewConArg();
			pszCmp = L"cmd /k ver";
			_ASSERTE(lstrcmp(arg.pszSpecialCmd, pszCmp)==0);
			_ASSERTE(arg.pszRenameTab && arg.pszPalette && arg.pszIconFile && arg.pszStartupDir && arg.NewConsole==crb_Undefined && lstrcmp(arg.pszRenameTab, L"My title")==0 && lstrcmp(arg.pszPalette, L"<PowerShell>")==0 && lstrcmp(arg.pszStartupDir, L"C:\\My docs")==0 && lstrcmp(arg.pszIconFile, L"C:\\my cmd.ico")==0);
			break;
		case 6:
			arg.pszSpecialCmd = lstrdup(L"cmd -cur_console:b:P:\"^<Power\"\"Shell^>\":t:\"My title\" /k ConEmuC.exe -Guimacro print(\"-new_console:a\")");
			arg.ProcessNewConArg();
			pszCmp = L"cmd /k ConEmuC.exe -Guimacro print(\"-new_console:a\")";
			_ASSERTE(lstrcmp(arg.pszSpecialCmd, pszCmp)==0);
			_ASSERTE(arg.pszRenameTab && arg.pszPalette && arg.BackgroundTab==crb_On && arg.NewConsole==crb_Undefined && arg.RunAsAdministrator==crb_Undefined && lstrcmp(arg.pszRenameTab, L"My title")==0 && lstrcmp(arg.pszPalette, L"<Power\"Shell>")==0);
			break;
		case 5:
			arg.pszSpecialCmd = lstrdup(L"cmd \"-cur_console:t:My title\" /k ver");
			arg.ProcessNewConArg();
			_ASSERTE(lstrcmp(arg.pszSpecialCmd, L"cmd /k ver")==0);
			_ASSERTE(arg.pszRenameTab && lstrcmp(arg.pszRenameTab, L"My title")==0 && arg.NewConsole==crb_Undefined);
			break;
		case 4:
			arg.pszSpecialCmd = lstrdup(L"cmd \"-new_console:P:^<Power\"\"Shell^>\"");
			arg.ProcessNewConArg();
			_ASSERTE(lstrcmp(arg.pszSpecialCmd, L"cmd")==0);
			nDbg = lstrcmp(arg.pszPalette, L"<Power\"Shell>");
			_ASSERTE(nDbg==0 && arg.NewConsole==crb_On);
			break;
		case 3:
			arg.pszSpecialCmd = lstrdup(L"cmd -cur_console:u:Max:");
			arg.ProcessNewConArg();
			_ASSERTE(lstrcmp(arg.pszSpecialCmd, L"cmd")==0);
			nDbg = lstrcmp(arg.pszUserName,L"Max");
			_ASSERTE(nDbg==0 && arg.pszDomain==NULL && !*arg.szUserPassword && arg.ForceUserDialog==crb_Off && arg.NewConsole!=crb_On);
			break;
		case 2:
			arg.pszSpecialCmd = lstrdup(L"cmd -cur_console:u:Max -new_console");
			arg.ProcessNewConArg();
			_ASSERTE(lstrcmp(arg.pszSpecialCmd, L"cmd")==0);
			nDbg = lstrcmp(arg.pszUserName,L"Max");
			_ASSERTE(nDbg==0 && arg.pszDomain==NULL && !*arg.szUserPassword && arg.ForceUserDialog==crb_On && arg.NewConsole==crb_On);
			break;
		case 1:
			arg.pszSpecialCmd = lstrdup(L"cmd -new_console:u -cur_console:h0");
			arg.ProcessNewConArg();
			_ASSERTE(lstrcmp(arg.pszSpecialCmd, L"cmd")==0);
			_ASSERTE(arg.pszUserName==NULL && arg.pszDomain==NULL && arg.ForceUserDialog==crb_On && arg.NewConsole==crb_On && arg.BufHeight==crb_On && arg.nBufHeight==0);
			break;
		case 0:
			arg.pszSpecialCmd = lstrdup(L"cmd \"-new_console:d:C:\\John Doe\\Home\" ");
			arg.ProcessNewConArg();
			_ASSERTE(lstrcmp(arg.pszSpecialCmd, L"cmd ")==0);
			nDbg = lstrcmp(arg.pszStartupDir, L"C:\\John Doe\\Home");
			_ASSERTE(nDbg==0 && arg.NewConsole==crb_On);
			break;
		default:
			bTest = false; // Stop tests
		}
	}

	nDbg = -1;
}
#endif


// If you add some members - don't forget them in RConStartArgs::AssignFrom!
RConStartArgs::RConStartArgs()
{
	Detached = RunAsAdministrator = RunAsRestricted = NewConsole = crb_Undefined;
	ForceUserDialog = BackgroundTab = ForegroungTab = NoDefaultTerm = ForceDosBox = ForceInherit = crb_Undefined;
	eSplit = eSplitNone; nSplitValue = DefaultSplitValue; nSplitPane = 0;
	aRecreate = cra_CreateTab;
	pszSpecialCmd = pszStartupDir = pszUserName = pszDomain = pszRenameTab = NULL;
	pszAddGuiArg = NULL;
	pszIconFile = pszPalette = pszWallpaper = NULL;
	BufHeight = crb_Undefined; nBufHeight = 0; LongOutputDisable = crb_Undefined;
	OverwriteMode = crb_Undefined;
	nPTY = 0;
	InjectsDisable = crb_Undefined;
	ForceNewWindow = crb_Undefined;
	eConfirmation = eConfDefault;
	szUserPassword[0] = 0;
	UseEmptyPassword = crb_Undefined;
	//hLogonToken = NULL;
	#if 0
	hShlwapi = NULL; WcsStrI = NULL;
	#endif
}

#ifndef CONEMU_MINIMAL
bool RConStartArgs::AssignFrom(const struct RConStartArgs* args, bool abConcat /*= false*/)
{
	_ASSERTE(args!=NULL);

	if (args->pszSpecialCmd)
	{
		SafeFree(this->pszSpecialCmd);

		//_ASSERTE(args->bDetached == FALSE); -- Allowed. While duplicating root.
		this->pszSpecialCmd = lstrdup(args->pszSpecialCmd);

		if (!this->pszSpecialCmd)
			return false;
	}

	// Директория запуска. В большинстве случаев совпадает с CurDir в conemu.exe,
	// но может быть задана из консоли, если запуск идет через "-new_console"
	_ASSERTE(this->pszStartupDir==NULL);

	struct CopyValues { wchar_t** ppDst; LPCWSTR pSrc; } values[] =
	{
		{&this->pszStartupDir, args->pszStartupDir},
		{&this->pszRenameTab, args->pszRenameTab},
		{&this->pszIconFile, args->pszIconFile},
		{&this->pszPalette, args->pszPalette},
		{&this->pszWallpaper, args->pszWallpaper},
		{NULL}
	};

	for (CopyValues* p = values; p->ppDst; p++)
	{
		if (abConcat && *p->ppDst && !p->pSrc)
			continue;

		SafeFree(*p->ppDst);
		if (p->pSrc)
		{
			*p->ppDst = lstrdup(p->pSrc);
			if (!*p->ppDst)
				return false;
		}
	}

	if (!abConcat || (args->RunAsRestricted || args->RunAsAdministrator || args->pszUserName))
	{
		this->RunAsRestricted = args->RunAsRestricted;
		this->RunAsAdministrator = args->RunAsAdministrator;
	}
	else
	{
		goto SkipUserName;
	}
	SafeFree(this->pszUserName); //SafeFree(this->pszUserPassword);
	SafeFree(this->pszDomain);
	//SafeFree(this->pszUserProfile);

	//if (this->hLogonToken) { CloseHandle(this->hLogonToken); this->hLogonToken = NULL; }
	if (args->pszUserName)
	{
		this->pszUserName = lstrdup(args->pszUserName);
		if (args->pszDomain)
			this->pszDomain = lstrdup(args->pszDomain);
		lstrcpy(this->szUserPassword, args->szUserPassword);
		this->UseEmptyPassword = args->UseEmptyPassword;
		//this->pszUserProfile = args->pszUserProfile ? lstrdup(args->pszUserProfile) : NULL;

		//SecureZeroMemory(args->szUserPassword, sizeof(args->szUserPassword));

		//this->pszUserPassword = lstrdup(args->pszUserPassword ? args->pszUserPassword : L"");
		//this->hLogonToken = args->hLogonToken; args->hLogonToken = NULL;
		// -- Do NOT fail when password is empty !!!
		if (!this->pszUserName /*|| !*this->szUserPassword*/)
			return false;
	}
SkipUserName:

	if (!abConcat || args->BackgroundTab || args->ForegroungTab)
	{
		this->BackgroundTab = args->BackgroundTab;
		this->ForegroungTab = args->ForegroungTab;
	}
	if (!abConcat || args->NoDefaultTerm)
	{
		this->NoDefaultTerm = args->NoDefaultTerm; _ASSERTE(args->NoDefaultTerm == crb_Undefined);
	}
	if (!abConcat || args->BufHeight)
	{
		this->BufHeight = args->BufHeight;
		this->nBufHeight = args->nBufHeight;
	}
	if (!abConcat || args->eConfirmation)
		this->eConfirmation = args->eConfirmation;
	if (!abConcat || args->ForceUserDialog)
		this->ForceUserDialog = args->ForceUserDialog;
	if (!abConcat || args->InjectsDisable)
		this->InjectsDisable = args->InjectsDisable;
	if (!abConcat || args->ForceNewWindow)
		this->ForceNewWindow = args->ForceNewWindow;
	if (!abConcat || args->LongOutputDisable)
		this->LongOutputDisable = args->LongOutputDisable;
	if (!abConcat || args->OverwriteMode)
		this->OverwriteMode = args->OverwriteMode;
	if (!abConcat || args->nPTY)
		this->nPTY = args->nPTY;

	if (!abConcat)
	{
		this->eSplit = args->eSplit;
		this->nSplitValue = args->nSplitValue;
		this->nSplitPane = args->nSplitPane;
	}

	return true;
}
#endif

RConStartArgs::~RConStartArgs()
{
	SafeFree(pszSpecialCmd); // именно SafeFree
	SafeFree(pszStartupDir); // именно SafeFree
	SafeFree(pszAddGuiArg);
	SafeFree(pszRenameTab);
	SafeFree(pszIconFile);
	SafeFree(pszPalette);
	SafeFree(pszWallpaper);
	SafeFree(pszUserName);
	SafeFree(pszDomain);
	//SafeFree(pszUserProfile);

	//SafeFree(pszUserPassword);
	if (szUserPassword[0]) SecureZeroMemory(szUserPassword, sizeof(szUserPassword));

	//if (hLogonToken) { CloseHandle(hLogonToken); hLogonToken = NULL; }

	#if 0
	if (hShlwapi)
		FreeLibrary(hShlwapi);
	hShlwapi = NULL;
	WcsStrI = NULL;
	#endif
}

#ifndef CONEMU_MINIMAL
wchar_t* RConStartArgs::CreateCommandLine(bool abForTasks /*= false*/) const
{
	wchar_t* pszFull = NULL;
	size_t cchMaxLen =
				 (pszSpecialCmd ? (lstrlen(pszSpecialCmd) + 3) : 0); // только команда
	cchMaxLen += (pszStartupDir ? (lstrlen(pszStartupDir) + 20) : 0); // "-new_console:d:..."
	cchMaxLen += (pszIconFile   ? (lstrlen(pszIconFile) + 20) : 0); // "-new_console:C:..."
	cchMaxLen += (pszWallpaper  ? (lstrlen(pszWallpaper) + 20) : 0); // "-new_console:W:..."
	// Some values may contain 'invalid' symbols (like '<', '>' and so on). They will be escaped. Thats why "len*2".
	cchMaxLen += (pszRenameTab  ? (lstrlen(pszRenameTab)*2 + 20) : 0); // "-new_console:t:..."
	cchMaxLen += (pszPalette    ? (lstrlen(pszPalette)*2 + 20) : 0); // "-new_console:P:..."
	cchMaxLen += 15;
	if (RunAsAdministrator == crb_On) cchMaxLen++; // -new_console:a
	if (RunAsRestricted == crb_On) cchMaxLen++; // -new_console:r
	cchMaxLen += (pszUserName ? (lstrlen(pszUserName) + 32 // "-new_console:u:<user>:<pwd>"
						+ (pszDomain ? lstrlen(pszDomain) : 0)
						+ (szUserPassword ? lstrlen(szUserPassword) : 0)) : 0);
	if (ForceUserDialog == crb_On) cchMaxLen++; // -new_console:u
	if (BackgroundTab == crb_On) cchMaxLen++; // -new_console:b
	if (ForegroungTab == crb_On) cchMaxLen++; // -new_console:f
	if (BufHeight == crb_On) cchMaxLen += 32; // -new_console:h<lines>
	if (LongOutputDisable == crb_On) cchMaxLen++; // -new_console:o
	if (OverwriteMode == crb_On) cchMaxLen++; // -new_console:w
	cchMaxLen += (nPTY ? 15 : 0); // -new_console:e
	if (InjectsDisable == crb_On) cchMaxLen++; // -new_console:i
	if (ForceNewWindow == crb_On) cchMaxLen++; // -new_console:N
	if (eConfirmation) cchMaxLen++; // -new_console:c / -new_console:n
	if (ForceDosBox == crb_On) cchMaxLen++; // -new_console:x
	if (ForceInherit == crb_On) cchMaxLen++; // -new_console:I
	if (eSplit) cchMaxLen += 64; // -new_console:s[<SplitTab>T][<Percents>](H|V)

	pszFull = (wchar_t*)malloc(cchMaxLen*sizeof(*pszFull));
	if (!pszFull)
	{
		_ASSERTE(pszFull!=NULL);
		return NULL;
	}

	if (pszSpecialCmd)
	{
		if ((RunAsAdministrator == crb_On) && abForTasks)
			_wcscpy_c(pszFull, cchMaxLen, L"*");
		else
			*pszFull = 0;						

		// Не окавычиваем. Этим должен озаботиться пользователь
		_wcscat_c(pszFull, cchMaxLen, pszSpecialCmd);

		//131008 - лишние пробелы не нужны
		wchar_t* pS = pszFull + lstrlen(pszFull);
		while ((pS > pszFull) && wcschr(L" \t\r\n", *(pS - 1)))
			*(--pS) = 0;
		//_wcscat_c(pszFull, cchMaxLen, L" ");
	}
	else
	{
		*pszFull = 0;
	}

	wchar_t szAdd[128] = L"";
	if (RunAsAdministrator == crb_On)
		wcscat_c(szAdd, L"a");
	else if (RunAsRestricted == crb_On)
		wcscat_c(szAdd, L"r");

	if ((ForceUserDialog == crb_On) && !(pszUserName && *pszUserName))
		wcscat_c(szAdd, L"u");

	if (BackgroundTab == crb_On)
		wcscat_c(szAdd, L"b");
	else if (ForegroungTab == crb_On)
		wcscat_c(szAdd, L"f");

	if (ForceDosBox == crb_On)
		wcscat_c(szAdd, L"x");

	if (ForceInherit == crb_On)
		wcscat_c(szAdd, L"I");

	if (eConfirmation == eConfAlways)
		wcscat_c(szAdd, L"c");
	else if (eConfirmation == eConfNever)
		wcscat_c(szAdd, L"n");

	if (LongOutputDisable == crb_On)
		wcscat_c(szAdd, L"o");

	if (OverwriteMode == crb_On)
		wcscat_c(szAdd, L"w");

	if (nPTY)
		wcscat_c(szAdd, (nPTY == 1) ? L"p1" : (nPTY == 2) ? L"p2" : L"p0");

	if (InjectsDisable == crb_On)
		wcscat_c(szAdd, L"i");

	if (ForceNewWindow == crb_On)
		wcscat_c(szAdd, L"N");

	if (BufHeight == crb_On)
	{
		if (nBufHeight)
			msprintf(szAdd+lstrlen(szAdd), 16, L"h%u", nBufHeight);
		else
			wcscat_c(szAdd, L"h");
	}

	// -new_console:s[<SplitTab>T][<Percents>](H|V)
	if (eSplit)
	{
		wcscat_c(szAdd, L"s");
		if (nSplitPane)
			msprintf(szAdd+lstrlen(szAdd), 16, L"%uT", nSplitPane);
		if (nSplitValue > 0 && nSplitValue < 1000)
		{
			UINT iPercent = (1000-nSplitValue)/10;
			msprintf(szAdd+lstrlen(szAdd), 16, L"%u", max(1,min(iPercent,99)));
		}
		wcscat_c(szAdd, (eSplit == eSplitHorz) ? L"H" : L"V");
	}

	if (szAdd[0])
	{
		_wcscat_c(pszFull, cchMaxLen, (NewConsole == crb_On) ? L" -new_console:" : L" -cur_console:");
		_wcscat_c(pszFull, cchMaxLen, szAdd);
	}

	struct CopyValues { wchar_t cOpt; bool bEscape; LPCWSTR pVal; } values[] =
	{
		{L'd', false, this->pszStartupDir},
		{L't', true,  this->pszRenameTab},
		{L'C', false, this->pszIconFile},
		{L'P', true,  this->pszPalette},
		{L'W', false, this->pszWallpaper},
		{0}
	};

	wchar_t szCat[32];
	for (CopyValues* p = values; p->cOpt; p++)
	{
		if (p->pVal && *p->pVal)
		{
			bool bQuot = wcspbrk(p->pVal, L" \"") != NULL;

			if (bQuot)
				msprintf(szCat, countof(szCat), (NewConsole == crb_On) ? L" \"-new_console:%c:" : L" \"-cur_console:%c:", p->cOpt);
			else
				msprintf(szCat, countof(szCat), (NewConsole == crb_On) ? L" -new_console:%c:" : L" -cur_console:%c:", p->cOpt);

			_wcscat_c(pszFull, cchMaxLen, szCat);

			if (p->bEscape)
			{
				wchar_t* pD = pszFull + lstrlen(pszFull);
				const wchar_t* pS = p->pVal;
				while (*pS)
				{
					if (wcschr(CmdEscapeNeededChars/* L"<>()&|^\"" */, *pS))
						*(pD++) = (*pS == L'"') ? L'"' : L'^';
					*(pD++) = *(pS++);
				}
				_ASSERTE(pD < (pszFull+cchMaxLen));
				*pD = 0;
			}
			else
			{
				_wcscat_c(pszFull, cchMaxLen, p->pVal);
			}

			if (bQuot)
				_wcscat_c(pszFull, cchMaxLen, L"\"");
		}
	}

	// "-new_console:u:<user>:<pwd>"
	if (pszUserName && *pszUserName)
	{
		_wcscat_c(pszFull, cchMaxLen, (NewConsole == crb_On) ? L" \"-new_console:u:" : L" \"-cur_console:u:");
		if (pszDomain && *pszDomain)
		{
			_wcscat_c(pszFull, cchMaxLen, pszDomain);
			_wcscat_c(pszFull, cchMaxLen, L"\\");
		}
		_wcscat_c(pszFull, cchMaxLen, pszUserName);
		if (*szUserPassword || (ForceUserDialog != crb_On))
		{
			_wcscat_c(pszFull, cchMaxLen, L":");
		}
		if (*szUserPassword)
		{
			_wcscat_c(pszFull, cchMaxLen, szUserPassword);
		}
		_wcscat_c(pszFull, cchMaxLen, L"\"");
	}

	return pszFull;
}
#endif

void RConStartArgs::AppendServerArgs(wchar_t* rsServerCmdLine, INT_PTR cchMax)
{
	if (eConfirmation == RConStartArgs::eConfAlways)
		_wcscat_c(rsServerCmdLine, cchMax, L" /CONFIRM");
	else if (eConfirmation == RConStartArgs::eConfNever)
		_wcscat_c(rsServerCmdLine, cchMax, L" /NOCONFIRM");

	if (InjectsDisable == crb_On)
		_wcscat_c(rsServerCmdLine, cchMax, L" /NOINJECT");
}

#ifndef CONEMU_MINIMAL
bool RConStartArgs::CheckUserToken(HWND hPwd)
{
	//SafeFree(pszUserProfile);
	UseEmptyPassword = crb_Undefined;

	//if (hLogonToken) { CloseHandle(hLogonToken); hLogonToken = NULL; }
	if (!pszUserName || !*pszUserName)
		return FALSE;

	//wchar_t szPwd[MAX_PATH]; szPwd[0] = 0;
	//szUserPassword[0] = 0;

	if (!GetWindowText(hPwd, szUserPassword, MAX_PATH-1))
	{
		szUserPassword[0] = 0;
		UseEmptyPassword = crb_On;
	}
	else
	{
		UseEmptyPassword = crb_Off;
	}

	SafeFree(pszDomain);
	wchar_t* pszSlash = wcschr(pszUserName, L'\\');
	if (pszSlash)
	{
		pszDomain = pszUserName;
		*pszSlash = 0;
		pszUserName = lstrdup(pszSlash+1);
	}

	HANDLE hLogonToken = CheckUserToken();
	bool bIsValid = (hLogonToken != NULL);
	// Token itself is not needed now
	SafeCloseHandle(hLogonToken);

	return bIsValid;
}

HANDLE RConStartArgs::CheckUserToken()
{
	HANDLE hLogonToken = NULL;
	// Empty password? Really? Security hole? Are you sure?
	// aka: code 1327 (ERROR_ACCOUNT_RESTRICTION)
	// gpedit.msc - Конфигурация компьютера - Конфигурация Windows - Локальные политики - Параметры безопасности - Учетные записи
	// Ограничить использование пустых паролей только для консольного входа -> "Отключить". 
	LPWSTR pszPassword = (UseEmptyPassword == crb_On) ? NULL : szUserPassword;
	DWORD nFlags = LOGON32_LOGON_INTERACTIVE;
	BOOL lbRc = LogonUser(pszUserName, pszDomain, pszPassword, nFlags, LOGON32_PROVIDER_DEFAULT, &hLogonToken);

	if (!lbRc || !hLogonToken)
	{
		return NULL;
	}

	return hLogonToken;
}
#endif

// Returns ">0" - when changes was made
//  0 - no changes
// -1 - error
// bForceCurConsole==true, если разбор параметров идет 
//   при запуске Tasks из GUI
int RConStartArgs::ProcessNewConArg(bool bForceCurConsole /*= false*/)
{
	NewConsole = crb_Undefined;

	if (!pszSpecialCmd || !*pszSpecialCmd)
	{
		_ASSERTE(pszSpecialCmd && *pszSpecialCmd);
		return -1;
	}

	int nChanges = 0;

	// 140219 - Остановить обработку, если встретим любой из: ConEmu[.exe], ConEmu64[.exe], ConEmuC[.exe], ConEmuC64[.exe]
	LPCWSTR pszStopAt = NULL;
	{
		LPCWSTR pszTemp = pszSpecialCmd;
		LPCWSTR pszSave = pszSpecialCmd;
		LPCWSTR pszName;
		CmdArg szExe;
		LPCWSTR pszWords[] = {L"ConEmu", L"ConEmu.exe", L"ConEmu64", L"ConEmu64.exe", L"ConEmuC", L"ConEmuC.exe", L"ConEmuC64", L"ConEmuC64.exe", L"ConEmuPortable.exe", L"ConEmuPortable", NULL};
		while (!pszStopAt && (0 == NextArg(&pszTemp, szExe)))
		{
			pszName = PointToName(szExe);
			for (size_t i = 0; pszWords[i]; i++)
			{
				if (lstrcmpi(pszName, pszWords[i]) == 0)
				{
					pszStopAt = pszSave;
					break;
				}
			}
			pszSave = pszTemp;
		}
	}

	#if 0
	// 140219 - Остановить обработку, если встретим любой из: ConEmu[.exe], ConEmu64[.exe], ConEmuC[.exe], ConEmuC64[.exe]
	if (!hShlwapi)
	{
		hShlwapi = LoadLibrary(L"Shlwapi.dll");
		WcsStrI = hShlwapi ? (StrStrI_t)GetProcAddress(hShlwapi, "StrStrIW") : NULL;
	}
	#endif


	// 111211 - здесь может быть передан "-new_console:..."
	LPCWSTR pszNewCon = L"-new_console";
	// 120108 - или "-cur_console:..." для уточнения параметров запуска команд (из фара например)
	LPCWSTR pszCurCon = L"-cur_console";
	int nNewConLen = lstrlen(pszNewCon);
	_ASSERTE(lstrlen(pszCurCon)==nNewConLen);

	wchar_t* pszFrom = pszSpecialCmd;

	bool bStop = false;
	while (!bStop)
	{
		wchar_t* pszSwitch = wcschr(pszFrom, L'-');
		if (!pszSwitch)
			break;
		// Pre-validation
		if (((pszSwitch[1] != L'n') && (pszSwitch[1] != L'c')) // -new_... or -cur_...
			|| (((pszSwitch != /* > */ pszFrom) // If it is started from pszFrom - no need to check previous symbols
				&& (*(pszSwitch-1) != L'"') || (((pszSwitch-2) >= pszFrom) && (*(pszSwitch-2) == L'\\'))) // Found: \"-new...
				&& (*(pszSwitch-1) != L' '))) // Prev symbol was space
		{
			// НЕ наш аргумент
			pszSwitch = wcschr(pszSwitch+1, L' ');
			if (!pszSwitch)
				break;
			pszFrom = pszSwitch;
			continue;
		}

		wchar_t* pszFindNew = NULL;
		wchar_t* pszFind = NULL;
		wchar_t szTest[12]; lstrcpyn(szTest, pszSwitch+1, countof(szTest));

		if (lstrcmp(szTest, L"new_console") == 0)
			pszFindNew = pszFind = pszSwitch;
		else if (lstrcmp(szTest, L"cur_console") == 0)
			pszFind = pszSwitch;
		else
		{
			// НЕ наш аргумент
			pszSwitch = wcschr(pszSwitch+1, L' ');
			if (!pszSwitch)
				break;
			pszFrom = pszSwitch;
			continue;
		}

		if (!pszFind)
			break;
		if (pszStopAt && (pszFind >= pszStopAt))
			break;

		// Проверка валидности
		_ASSERTE(pszFind >= pszSpecialCmd);
		if ((pszFind[nNewConLen] != L' ') && (pszFind[nNewConLen] != L':')
			&& (pszFind[nNewConLen] != L'"') && (pszFind[nNewConLen] != 0))
		{
			// НЕ наш аргумент
			pszFrom = pszFind+nNewConLen;
		}
		else
		{
			if (pszFindNew)
				NewConsole = crb_On;

			// -- не будем пока, мешает. например, при запуске задач
			//// По умолчанию, принудительно включить "Press Enter or Esc to close console"
			//if (!bForceCurConsole)
			//	eConfirmation = eConfAlways;

			bool lbQuot = (*(pszFind-1) == L'"');
			bool lbWasQuot = lbQuot;
			const wchar_t* pszEnd = pszFind+nNewConLen;
			//wchar_t szNewConArg[MAX_PATH+1];
			if (lbQuot)
				pszFind--;

			if (*pszEnd == L'"')
			{
				pszEnd++;
			}
			else if (*pszEnd != L':')
			{
				// Конец
				_ASSERTE(*pszEnd == L' ' || *pszEnd == 0);
			}
			else
			{
				if (*pszEnd == L':')
				{
					pszEnd++;
				}
				else
				{
					_ASSERTE(*pszEnd == L':');
				}

				// Найти конец аргумента
				const wchar_t* pszArgEnd = pszEnd;
				bool lbLocalQuot = false;
				while (*pszArgEnd)
				{
					switch (*pszArgEnd)
					{
					case L'^':
						pszArgEnd++; // Skip control char, goto escaped char
						break;
					case L'"':
						if (*(pszArgEnd+1) == L'"')
						{
							pszArgEnd += 2; // Skip qoubled qouble quote
							continue;
						}
						if (!lbQuot)
						{
							if (!lbLocalQuot && (*(pszArgEnd-1) == L':'))
							{
								lbLocalQuot = true;
								pszArgEnd++;
								continue;
							}
							if (lbLocalQuot)
							{
								if (*(pszArgEnd+1) != L':')
									goto EndFound;
								lbLocalQuot = false;
								pszArgEnd += 2;
								continue;
							}
						}
						goto EndFound;
					case L' ':
						if (!lbQuot && !lbLocalQuot)
							goto EndFound;
						break;
					case 0:
						goto EndFound;
					}

					pszArgEnd++;
				}
				EndFound:

				// Обработка доп.параметров -new_console:xxx
				bool lbReady = false;
				while (!lbReady && *pszEnd)
				{
					_ASSERTE(pszEnd <= pszArgEnd);
					wchar_t cOpt = *(pszEnd++);

					switch (cOpt)
					{
					//case L'-':
					//	bStop = true; // следующие "-new_console" - не трогать!
					//	break;
					case L'"':
						_ASSERTE(pszEnd > pszArgEnd);
						lbReady = true;
						break;
					case L' ':
					case 0:
						lbReady = true;
						break;

					case L':':
						// Just skip ':'. Delimiter between switches: -new_console:c:b:a
						// Revert stored value to lbQuot. We need to "cut" last double quote in the first two cases
						// cmd -cur_console:d:"C:\users":t:"My title" "-cur_console:C:C:\cmd.ico" -cur_console:P:"<PowerShell>":a /k ver
						lbWasQuot = lbQuot;
						break;

					case L'b':
						// b - background, не активировать таб
						BackgroundTab = crb_On; ForegroungTab = crb_Off;
						break;
					case L'f':
						// f - foreground, активировать таб (аналог ">" в Tasks)
						ForegroungTab = crb_On; BackgroundTab = crb_Off;
						break;

					case L'z':
						// z - don't use "Default terminal" feature
						NoDefaultTerm = crb_On;
						break;

					case L'a':
						// a - RunAs shell verb (as admin on Vista+, login/password in WinXP-)
						RunAsAdministrator = crb_On;
						break;

					case L'r':
						// r - run as restricted user
						RunAsRestricted = crb_On;
						break;

					case L'o':
						// o - disable "Long output" for next command (Far Manager)
						LongOutputDisable = crb_On;
						break;

					case L'w':
						// e - enable "Overwrite" mode in console prompt
						OverwriteMode = crb_On;
						break;

					case L'p':
						if (isDigit(*pszEnd))
						{
							switch (*(pszEnd++))
							{
								case L'0':
									nPTY = 0; // don't change
									break;
								case L'1':
									nPTY = 1; // enable PTY mode
									break;
								case L'2':
									nPTY = 2; // disable PTY mode (switch to plain $CONIN, $CONOUT, $CONERR)
									break;
								default:
									nPTY = 1;
							}
						}
						else
						{
							nPTY = 1; // enable PTY mode
						}
						break;

					case L'i':
						// i - don't inject ConEmuHk into the starting application
						InjectsDisable = crb_On;
						break;

					case L'N':
						// N - Force new ConEmu window with Default terminal
						ForceNewWindow = crb_On;
						break;

					case L'h':
						// "h0" - отключить буфер, "h9999" - включить буфер в 9999 строк
						{
							BufHeight = crb_On;
							if (isDigit(*pszEnd))
							{
								wchar_t* pszDigits = NULL;
								nBufHeight = wcstoul(pszEnd, &pszDigits, 10);
								if (pszDigits)
									pszEnd = pszDigits;
							}
							else
							{
								nBufHeight = 0;
							}
						} // L'h':
						break;

					case L'n':
						// n - отключить "Press Enter or Esc to close console"
						eConfirmation = eConfNever;
						break;

					case L'c':
						// c - принудительно включить "Press Enter or Esc to close console"
						eConfirmation = eConfAlways;
						break;

					case L'x':
						// x - Force using dosbox for .bat files
						ForceDosBox = crb_On;
						break;

					case L'I':
						// I - tell GuiMacro to execute new command inheriting active process state. This is only usage ATM.
						ForceInherit = crb_On;
						break;

					// "Long" code blocks below: 'd', 'u', 's' and so on (in future)

					case L's':
						// s[<SplitTab>T][<Percents>](H|V)
						// Пример: "s3T30H" - разбить 3-ий таб. будет создан новый Pane справа, шириной 30% от 3-го таба.
						{
							UINT nTab = 0 /*active*/, nValue = /*пополам*/DefaultSplitValue/10;
							bool bDisableSplit = false;
							while (*pszEnd)
							{
								if (isDigit(*pszEnd))
								{
									wchar_t* pszDigits = NULL;
									UINT n = wcstoul(pszEnd, &pszDigits, 10);
									if (!pszDigits)
										break;
									pszEnd = pszDigits;
									if (*pszDigits == L'T')
									{
                                    	nTab = n;
                                	}
                                    else if ((*pszDigits == L'H') || (*pszDigits == L'V'))
                                    {
                                    	nValue = n;
                                    	eSplit = (*pszDigits == L'H') ? eSplitHorz : eSplitVert;
                                    }
                                    else
                                    {
                                    	break;
                                    }
                                    pszEnd++;
								}
								else if (*pszEnd == L'T')
								{
									nTab = 0;
									pszEnd++;
								}
								else if ((*pszEnd == L'H') || (*pszEnd == L'V'))
								{
	                            	nValue = DefaultSplitValue/10;
	                            	eSplit = (*pszEnd == L'H') ? eSplitHorz : eSplitVert;
	                            	pszEnd++;
								}
								else if (*pszEnd == L'N')
								{
									bDisableSplit = true;
									pszEnd++;
									break;
								}
								else
								{
									break;
								}
							}

							if (bDisableSplit)
							{
								eSplit = eSplitNone; nSplitValue = DefaultSplitValue; nSplitPane = 0;
							}
							else
							{
								if (!eSplit)
									eSplit = eSplitHorz;
								// Для удобства, пользователь задает размер НОВОЙ части
								nSplitValue = 1000-max(1,min(nValue*10,999)); // проценты
								_ASSERTE(nSplitValue>=1 && nSplitValue<1000);
								nSplitPane = nTab;
							}
						} // L's'
						break;



					// Following options (except of single 'u') must be placed on the end of "-new_console:..."
					// If one needs more that one option - use several "-new_console:..." switches

					case L'd':
						// d:<StartupDir>. MUST be last option
					case L't':
						// t:<TabName>. MUST be last option
					case L'u':
						// u - ConEmu choose user dialog (may be specified in the middle, if it is without ':' - user or pwd)
						// u:<user> - ConEmu choose user dialog with prefilled user field. MUST be last option
						// u:<user>:<pwd> - specify user/pwd in args. MUST be last option
					case L'C':
						// C:<IconFile>. MUST be last option
					case L'P':
						// P:<Palette>. MUST be last option
					case L'W':
						// W:<Wallpaper>. MUST be last option
						{
							if (cOpt == L'u')
							{
								// Show choose user dialog (may be specified in the middle, if it is without ':' - user or pwd)
								SafeFree(pszUserName);
								SafeFree(pszDomain);
								if (szUserPassword[0]) SecureZeroMemory(szUserPassword, sizeof(szUserPassword));
							}


							if (*pszEnd == L':')
							{
								pszEnd++;
							}
							else
							{
								if (cOpt == L'u')
								{
									ForceUserDialog = crb_On;
									break;
								}
							}

							const wchar_t* pszTab = pszEnd;
							// we need to find end of argument
							pszEnd = pszArgEnd;
							// temp buffer
							wchar_t* lpszTemp = NULL;

							wchar_t** pptr = NULL;
							switch (cOpt)
							{
							case L'd': pptr = &pszStartupDir; break;
							case L't': pptr = &pszRenameTab; break;
							case L'u': pptr = &lpszTemp; break;
							case L'C': pptr = &pszIconFile; break;
							case L'P': pptr = &pszPalette; break;
							case L'W': pptr = &pszWallpaper; break;
							}

							if (pszEnd > pszTab)
							{
								size_t cchLen = pszEnd - pszTab;
								SafeFree(*pptr);
								*pptr = (wchar_t*)malloc((cchLen+1)*sizeof(**pptr));
								if (*pptr)
								{
									// We need to process escape sequences ("^>" -> ">", "^&" -> "&", etc.)
									//wmemmove(*pptr, pszTab, cchLen);
									wchar_t* pD = *pptr;
									const wchar_t* pS = pszTab;

									if (lbQuot)
									{
										lbLocalQuot = false;
									}
									else if (*pS == L'"' && *(pS+1) != L'"')
									{
										// Remember, that last processed switch was local-quoted
										lbWasQuot = true;
										// This item is local quoted. Example: -new_console:t:"My title"
										lbLocalQuot = true;
										pS++;
									}

									// There is enough room allocated
									while (pS < pszEnd)
									{
										if ((*pS == L'^') && ((pS + 1) < pszEnd))
										{
											pS++; // Skip control char, goto escaped char
										}
										else if (*pS == L'"')
										{
											if (((pS + 1) < pszEnd) && (*(pS+1) == L'"'))
											{
												pS++; // Skip qoubled qouble quote
											}
											else if (lbLocalQuot)
											{
												pszEnd = (pS+1);
												_ASSERTE(*pszEnd==L':' || *pszEnd==L' ' || *pszEnd==0);
												break; // End of local quoted argument: -new_console:d:"C:\User\super user":t:"My title"
											}
										}

										*(pD++) = *(pS++);
									}
									// Terminate with '\0'
									_ASSERTE(pD <= ((*pptr)+cchLen));
									*pD = 0;
									// Issue 1711: Supposing there can't be ending quotes
									INT_PTR iLen = (pD - *pptr);
									while (((iLen--) > 0) && (*(--pD) == L'"'))
										*pD = 0;
								}
								// Additional processing
								switch (cOpt)
								{
								case L'd':
									// Например, "%USERPROFILE%"
									// TODO("А надо ли разворачивать их тут? Наверное при запуске под другим юзером некорректно? Хотя... все равно до переменных не доберемся");
									if (wcschr(pszStartupDir, L'%'))
									{
										wchar_t* pszExpand = NULL;
										if (((pszExpand = ExpandEnvStr(pszStartupDir)) != NULL))
										{
											SafeFree(pszStartupDir);
											pszStartupDir = pszExpand;
										}
									}
									break;
								case L'u':
									if (lpszTemp)
									{
										// Split in form:
										// [Domain\]UserName[:Password]
										wchar_t* pszPwd = wcschr(lpszTemp, L':');
										if (pszPwd)
										{
											// Password was specified, dialog prompt is not required
											ForceUserDialog = crb_Off;
											*pszPwd = 0;
											int nPwdLen = lstrlen(pszPwd+1);
											lstrcpyn(szUserPassword, pszPwd+1, countof(szUserPassword));
											if (nPwdLen > 0)
												SecureZeroMemory(pszPwd+1, nPwdLen);
											UseEmptyPassword = (nPwdLen == 0) ? crb_On : crb_Off;
										}
										else
										{
											// Password was NOT specified, dialog prompt IS required
											ForceUserDialog = crb_On;
											UseEmptyPassword = crb_Off;
										}
										wchar_t* pszSlash = wcschr(lpszTemp, L'\\');
										if (pszSlash)
										{
											*pszSlash = 0;
											pszDomain = lstrdup(lpszTemp);
											pszUserName = lstrdup(pszSlash+1);
										}
										else
										{
											pszUserName = lstrdup(lpszTemp);
										}
									}
									break;
								}
							}
							SafeFree(lpszTemp);
						} // L't':
						break;

					}
				}
			}

			if (pszEnd > pszFind)
			{
				// pszEnd должен указывать на конец -new_console[:...] / -cur_console[:...]
				// и включать обрамляющую кавычку, если он окавычен
				if (lbWasQuot)
				{
					if (*pszEnd == L'"' && *(pszEnd-1) != L'"')
						pszEnd++;
				}
				else
				{
					while (*(pszEnd-1) == L'"')
						pszEnd--;
				}

				// Откусить лишние пробелы, которые стоят ПЕРЕД -new_console[:...] / -cur_console[:...]
				while (((pszFind - 1) >= pszSpecialCmd)
					&& (*(pszFind-1) == L' ')
					&& (((pszFind - 1) == pszSpecialCmd) || (*(pszFind-2) == L' ') || (/**pszEnd == L'"' ||*/ *pszEnd == 0 || *pszEnd == L' ')))
				{
					pszFind--;
				}
				// Откусить лишние пробелы ПОСЛЕ -new_console[:...] / -cur_console[:...] если он стоит в НАЧАЛЕ строки!
				if (pszFind == pszSpecialCmd)
				{
					while (*pszEnd == L' ')
						pszEnd++;
				}

				// Здесь нужно подвинуть pszStopAt
				if (pszStopAt)
					pszStopAt -= (pszEnd - pszFind);

				// Удалить из строки запуска обработанный ключ
				wmemmove(pszFind, pszEnd, (lstrlen(pszEnd)+1));
				nChanges++;
			}
			else
			{
				_ASSERTE(pszEnd > pszFind);
				*pszFind = 0;
				nChanges++;
			}
		} // if ((((pszFind == pszFrom) ...
	} // while (!bStop)

	return nChanges;
} // int RConStartArgs::ProcessNewConArg(bool bForceCurConsole /*= false*/)
