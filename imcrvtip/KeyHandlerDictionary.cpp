﻿
#include "imcrvtip.h"
#include "TextService.h"

void CTextService::_ConnectDic()
{
	DWORD dwMode;

	if(WaitNamedPipeW(mgrpipename, NMPWAIT_USE_DEFAULT_WAIT) == 0)
	{
		return;
	}

	hPipe = CreateFileW(mgrpipename, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, SECURITY_SQOS_PRESENT | SECURITY_EFFECTIVE_ONLY | SECURITY_IDENTIFICATION, NULL);
	if(hPipe == INVALID_HANDLE_VALUE)
	{
		return;
	}

	dwMode = PIPE_READMODE_MESSAGE | PIPE_WAIT;
	SetNamedPipeHandleState(hPipe, &dwMode, NULL, NULL);
}

void CTextService::_DisconnectDic()
{
	if(hPipe != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hPipe);
		hPipe = INVALID_HANDLE_VALUE;
	}
}

void CTextService::_SearchDic(WCHAR command)
{
	DWORD bytesWrite, bytesRead;
	std::wstring s, se, fmt, scd, scr, sad, sar, okurikey;
	std::wregex r;
	std::wsmatch m;

	_StartManager();

	_ConnectDic();

	ZeroMemory(pipebuf, sizeof(pipebuf));

	if(okuriidx != 0)
	{
		okurikey = kana.substr(okuriidx + 1);
		if(okurikey.size() >= 2 &&
			IS_SURROGATE_PAIR(okurikey.c_str()[0], okurikey.c_str()[1]))
		{
			okurikey = okurikey.substr(0, 2);
		}
		else
		{
			okurikey = okurikey.substr(0, 1);
		}
	}

	_snwprintf_s(pipebuf, _TRUNCATE, L"%c\n%s\t%s\t%s\n",
		command, searchkey.c_str(), searchkeyorg.c_str(), okurikey.c_str());

	bytesWrite = (DWORD)((wcslen(pipebuf) + 1) * sizeof(WCHAR));
	if(WriteFile(hPipe, pipebuf, bytesWrite, &bytesWrite, NULL) == FALSE)
	{
		goto exit;
	}

	ZeroMemory(pipebuf, sizeof(pipebuf));

	bytesRead = 0;
	if(ReadFile(hPipe, pipebuf, sizeof(pipebuf), &bytesRead, NULL) == FALSE)
	{
		goto exit;
	}

	if(pipebuf[0] != REP_OK)
	{
		goto exit;
	}

	s.assign(pipebuf);
	r.assign(L"(.*)\t(.*)\t(.*)\t(.*)\n");

	while(std::regex_search(s, m, r))
	{
		se = m.str();
		s = m.suffix();

		fmt.assign(L"$1");
		scd = std::regex_replace(se, r, fmt);
		fmt.assign(L"$2");
		scr = std::regex_replace(se, r, fmt);
		fmt.assign(L"$3");
		sad = std::regex_replace(se, r, fmt);
		fmt.assign(L"$4");
		sar = std::regex_replace(se, r, fmt);

		if(scd.empty())
		{
			scd = scr;
		}
		if(sad.empty())
		{
			sad = sar;
		}

		candidates.push_back(CANDIDATE(CANDIDATEBASE(scd, sad), CANDIDATEBASE(scr, sar)));
	}

exit:
	ZeroMemory(pipebuf, sizeof(pipebuf));

	_DisconnectDic();
}

void CTextService::_ConvertWord(WCHAR command, const std::wstring &key, const std::wstring &candidate, const std::wstring &okuri, std::wstring &conv)
{
	DWORD bytesWrite, bytesRead;

	_StartManager();

	_ConnectDic();

	ZeroMemory(pipebuf, sizeof(pipebuf));

	_snwprintf_s(pipebuf, _TRUNCATE, L"%c\n%s\t%s\t%s\n",
		command, key.c_str(), candidate.c_str(), okuri.c_str());

	bytesWrite = (DWORD)((wcslen(pipebuf) + 1) * sizeof(WCHAR));
	if(WriteFile(hPipe, pipebuf, bytesWrite, &bytesWrite, NULL) == FALSE)
	{
		goto exit;
	}

	ZeroMemory(pipebuf, sizeof(pipebuf));

	bytesRead = 0;
	if(ReadFile(hPipe, pipebuf, sizeof(pipebuf), &bytesRead, NULL) == FALSE)
	{
		goto exit;
	}

	if(pipebuf[0] != REP_OK)
	{
		conv.clear();
		goto exit;
	}

	pipebuf[wcslen(pipebuf) - 1] = L'\0';
	conv.assign(&pipebuf[2]);

exit:
	ZeroMemory(pipebuf, sizeof(pipebuf));

	_DisconnectDic();
}

void CTextService::_AddUserDic(WCHAR command, const std::wstring &key, const std::wstring &candidate, const std::wstring &annotation)
{
	DWORD bytesWrite, bytesRead;
	std::wstring okurikey;

	_ConnectDic();

	ZeroMemory(pipebuf, sizeof(pipebuf));

	if(okuriidx != 0)
	{
		okurikey = kana.substr(okuriidx + 1);
		if(okurikey.size() >= 2 &&
			IS_SURROGATE_PAIR(okurikey.c_str()[0], okurikey.c_str()[1]))
		{
			okurikey = okurikey.substr(0, 2);
		}
		else
		{
			okurikey = okurikey.substr(0, 1);
		}
	}

	_snwprintf_s(pipebuf, _TRUNCATE, L"%c\n%s\t%s\t%s\t%s\n",
		command, key.c_str(), candidate.c_str(), annotation.c_str(), okurikey.c_str());

	bytesWrite = (DWORD)((wcslen(pipebuf) + 1) * sizeof(WCHAR));
	if(WriteFile(hPipe, pipebuf, bytesWrite, &bytesWrite, NULL) == FALSE)
	{
		goto exit;
	}

	ZeroMemory(pipebuf, sizeof(pipebuf));

	bytesRead = 0;
	if(ReadFile(hPipe, pipebuf, sizeof(pipebuf), &bytesRead, NULL) == FALSE)
	{
		goto exit;
	}

exit:
	ZeroMemory(pipebuf, sizeof(pipebuf));

	_DisconnectDic();
}

void CTextService::_DelUserDic(WCHAR command, const std::wstring &key, const std::wstring &candidate)
{
	DWORD bytesWrite, bytesRead;

	_ConnectDic();

	ZeroMemory(pipebuf, sizeof(pipebuf));

	_snwprintf_s(pipebuf, _TRUNCATE, L"%c\n%s\t%s\n",
		command, key.c_str(), candidate.c_str());

	bytesWrite = (DWORD)((wcslen(pipebuf) + 1) * sizeof(WCHAR));
	if(WriteFile(hPipe, pipebuf, bytesWrite, &bytesWrite, NULL) == FALSE)
	{
		goto exit;
	}

	ZeroMemory(pipebuf, sizeof(pipebuf));

	bytesRead = 0;
	if(ReadFile(hPipe, pipebuf, sizeof(pipebuf), &bytesRead, NULL) == FALSE)
	{
		goto exit;
	}

exit:
	ZeroMemory(pipebuf, sizeof(pipebuf));

	_DisconnectDic();
}

void CTextService::_SaveUserDic()
{
	_CommandDic(REQ_USER_SAVE);
}

void CTextService::_CommandDic(WCHAR command)
{
	DWORD bytesWrite, bytesRead;

	_ConnectDic();

	pipebuf[0] = command;
	pipebuf[1] = L'\n';
	pipebuf[2] = L'\0';

	bytesWrite = (DWORD)((wcslen(pipebuf) + 1) * sizeof(WCHAR));
	if(WriteFile(hPipe, pipebuf, bytesWrite, &bytesWrite, NULL) == FALSE)
	{
		goto exit;
	}

	bytesRead = 0;
	if(ReadFile(hPipe, pipebuf, sizeof(pipebuf), &bytesRead, NULL) == FALSE)
	{
		goto exit;
	}

exit:
	ZeroMemory(pipebuf, sizeof(pipebuf));

	_DisconnectDic();
}

void CTextService::_StartManager()
{
	HANDLE hMutex = OpenMutexW(SYNCHRONIZE, FALSE, mgrmutexname);
	if(hMutex != NULL)
	{
		CloseHandle(hMutex);
		return;
	}

	_StartProcess(CORVUSMGREXE);
}

void CTextService::_StartConfigure()
{
	HANDLE hMutex = OpenMutexW(SYNCHRONIZE, FALSE, cnfmutexname);
	if(hMutex != NULL)
	{
		CloseHandle(hMutex);
		return;
	}

	_StartProcess(CORVUSCNFEXE);
}

void CTextService::_StartProcess(LPCWSTR fname)
{
	WCHAR path[MAX_PATH];
	WCHAR drive[_MAX_DRIVE];
	WCHAR dir[_MAX_DIR];
	PROCESS_INFORMATION pi;
	STARTUPINFOW si;

	GetModuleFileNameW(g_hInst, path, _countof(path));
	_wsplitpath_s(path, drive, _countof(drive), dir, _countof(dir), NULL, 0, NULL, 0);
	_snwprintf_s(path, _TRUNCATE, L"%s%s%s", drive, dir, fname);

	ZeroMemory(&pi, sizeof(pi));
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	if(CreateProcessW(path, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
}
