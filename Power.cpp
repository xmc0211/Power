// MIT License
//
// Copyright (c) 2025-2026 Power - xmc0211 <xmc0211@qq.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "Power.h"

typedef enum _SHUTDOWN_ACTION {
	ShutdownNoReboot,
	ShutdownReboot,
	ShutdownPowerOff
} SHUTDOWN_ACTION;

typedef NTSTATUS(NTAPI* NtShutdownSystem)(SHUTDOWN_ACTION);
typedef NTSTATUS(NTAPI* NtInitiatePowerAction)(
	POWER_ACTION SystemAction,
	SYSTEM_POWER_STATE MinSystemState,
	ULONG Flags,
	BOOLEAN Asynchronous
	);

DWORD WINAPI AdjustPrivilege(LPCTSTR lpPrivilegeName, BOOL bEnable) {
	HANDLE hToken = NULL;
	TOKEN_PRIVILEGES NewState;
	LUID luidPrivilegeLUID;
	DWORD dwErr = ERROR_SUCCESS;
	HANDLE hProcess = GetCurrentProcess();
	if (hProcess == NULL) {
		dwErr = GetLastError();
		goto EXIT;
	}
	if (!OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
		dwErr = GetLastError();
		goto EXIT;
	}
	if (bEnable == FALSE) {
		if (!AdjustTokenPrivileges(hToken, TRUE, NULL, 0, NULL, NULL)) dwErr = GetLastError();
		goto EXIT;
	}
	LookupPrivilegeValue(NULL, lpPrivilegeName, &luidPrivilegeLUID);
	NewState.PrivilegeCount = 1;
	NewState.Privileges[0].Luid = luidPrivilegeLUID;
	NewState.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if (!AdjustTokenPrivileges(hToken, FALSE, &NewState, 0, NULL, NULL)) dwErr = GetLastError();
EXIT:
	if (hToken) CloseHandle(hToken);
	return dwErr;
}

DWORD PowerPerform(POWEROPTION opt) {
	DWORD dwRes = AdjustPrivilege(SE_SHUTDOWN_NAME, TRUE);
	if (dwRes != ERROR_SUCCESS) return dwRes;
	HMODULE hDll = GetModuleHandle(TEXT("NtDll.dll"));
	if (hDll == NULL) return GetLastError();
	NtShutdownSystem pNtShutdownSystem = (NtShutdownSystem)GetProcAddress(hDll, "NtShutdownSystem");
	NtInitiatePowerAction pNtInitiatePowerAction = (NtInitiatePowerAction)GetProcAddress(hDll, "NtInitiatePowerAction");
	switch (opt) {
	case POWER_SHUTDOWN: { pNtShutdownSystem(ShutdownNoReboot); break; }
	case POWER_RESTART: { pNtShutdownSystem(ShutdownReboot); break; }
	case POWER_HIBERNATE: { pNtInitiatePowerAction(PowerActionHibernate, PowerSystemHibernate, 0, TRUE); break; }
	case POWER_SLEEP: { pNtInitiatePowerAction(PowerActionSleep, PowerSystemSleeping1, 0, TRUE); break; }
	default: { return -1; }
	}
	return ERROR_SUCCESS;
}