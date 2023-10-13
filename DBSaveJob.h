#pragma once
#include <Windows.h>
#include "NetRoot/Common/SystemLogger.h"

constexpr int MAX_QUERY_LEN = 1024;
enum DBType
{
	eGameDB = 1,
	eLogDB
};

struct DBSaveJob
{
	WCHAR query[MAX_QUERY_LEN] = { 0 };
	BYTE type = eGameDB;
};

