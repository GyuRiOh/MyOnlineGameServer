#include "AuthPipe.h"
#include "AuthPipe_CS_Stub.h"
#include "PipePlayer.h"
#include "../NetRoot/LanServer/LanPacket.h"
#include "../CommonProtocol.h"
#include "../NetRoot/Common/Parser.h"

using namespace MyNetwork;

MyNetwork::AuthPipe::AuthPipe(NetRoot* server, unsigned int framePerSecond, unsigned int threadNum) noexcept
	: NetPipe(server, framePerSecond), framePerSec_(0)
{
	RegisterPipeStub(new AuthPipe_CS_Stub(this));
	proxy_ = new AuthPipe_SC_Proxy(this);
}

MyNetwork::NetUser* MyNetwork::AuthPipe::OnUserJoin(NetSessionID sessionID) noexcept
{
	PipePlayer* player = reinterpret_cast<PipePlayer*>(
		SizedMemoryPool::GetInstance()->Alloc(sizeof(PipePlayer)));

	new (player) PipePlayer(sessionID, GetRoot(), GetPipeID());

    return static_cast<NetUser*>(player);
}

void MyNetwork::AuthPipe::OnUserLeave(NetUser* user) noexcept
{

    PipePlayer* player = static_cast<PipePlayer*>(user);

    onlineMap_.Release(player->GetAccountNumber());
    player->~PipePlayer();
    SizedMemoryPool::GetInstance()->Free(player);
}

void MyNetwork::AuthPipe::OnUserMoveIn(NetUser* user) noexcept
{
    //Player 포인터를 들고 인증스레드로 돌아오는 일은 없음
    CrashDump::Crash();
}

void MyNetwork::AuthPipe::OnUserMoveOut(NetUser* user) noexcept
{
    PipePlayer* player = static_cast<PipePlayer*>(user);

    if (!onlineMap_.Release(player->GetAccountNumber()))
        CrashDump::Crash();

}

void MyNetwork::AuthPipe::OnUserUpdate(NetUser* user) noexcept
{

}

void MyNetwork::AuthPipe::OnMonitor(const LoopInfo* const info) noexcept
{
    wchar_t title[64] = { 0 };
    swprintf_s(title, L"AuthPipe %d", GetCurrentThreadId());
    SystemLogger::GetInstance()->Console(title, LEVEL_DEBUG, L"Player Num : %d, Frame Count : %d", GetUserSize(), info->frameCountPerSecond_);

    framePerSec_ = info->frameCountPerSecond_;
}

void MyNetwork::AuthPipe::OnStart() noexcept
{
    //Redis 사용 세팅
    WORD version = MAKEWORD(2, 2);
    WSADATA data;
    WSAStartup(version, &data);

    redisClient_ = new cpp_redis::client();
    redisClient_->connect();
}

void MyNetwork::AuthPipe::OnStop() noexcept
{
    SystemLogger::GetInstance()->Console(L"PipeManager", LEVEL_DEBUG, L"AuthPipe Stopped");

}

void MyNetwork::AuthPipe::DBSave(BYTE dbType, const WCHAR* stringFormat, ...) noexcept
{
    DBSaveJob* job = reinterpret_cast<DBSaveJob*>(
        SizedMemoryPool::GetInstance()->Alloc(sizeof(DBSaveJob)));
    job->type = dbType;

    va_list va;
    va_start(va, stringFormat);
    HRESULT hResult = StringCchVPrintfW(job->query,
        MAX_QUERY_LEN,
        stringFormat,
        va);
    va_end(va);

    if (FAILED(hResult))
    {
        MyNetwork::SystemLogger::GetInstance()->LogText(
            L"DBSaveJob-Query", MyNetwork::LEVEL_ERROR, L"Query Too Long... : %s", job->query);
        MyNetwork::CrashDump::Crash();
    }

    static_cast<GameServer*>(GetRoot())->DBSave(job);

}

MYSQL_ROW MyNetwork::AuthPipe::GameDBQuery(const WCHAR* stringFormat, ...) noexcept
{
    WCHAR query[1024] = { 0 };

    va_list va;
    va_start(va, stringFormat);
    HRESULT hResult = StringCchVPrintfW(query,
        MAX_QUERY_LEN,
        stringFormat,
        va);
    va_end(va);

    if (FAILED(hResult))
    {
        MyNetwork::SystemLogger::GetInstance()->LogText(
            L"GameDB-Query", MyNetwork::LEVEL_ERROR, L"Query Too Long... : %s", query);
        MyNetwork::CrashDump::Crash();
    }

    GameServer* server = static_cast<GameServer*>(GetRoot());
    server->gameDBConnector_->Query(query);

    return server->gameDBConnector_->FetchRow();

}

MYSQL_ROW MyNetwork::AuthPipe::LogDBQuery(const WCHAR* stringFormat, ...) noexcept
{
    WCHAR query[1024] = { 0 };

    va_list va;
    va_start(va, stringFormat);
    HRESULT hResult = StringCchVPrintfW(query,
        MAX_QUERY_LEN,
        stringFormat,
        va);
    va_end(va);

    if (FAILED(hResult))
    {
        MyNetwork::SystemLogger::GetInstance()->LogText(
            L"LogDB-Query", MyNetwork::LEVEL_ERROR, L"Query Too Long... : %s", query);
        MyNetwork::CrashDump::Crash();
    }

    GameServer* server = static_cast<GameServer*>(GetRoot());
    server->logDBConnector_->Query(query);

    return server->logDBConnector_->FetchRow();
}

MYSQL_ROW MyNetwork::AuthPipe::AccountDBQuery(const WCHAR* stringFormat, ...) noexcept
{
    WCHAR query[1024] = { 0 };

    va_list va;
    va_start(va, stringFormat);
    HRESULT hResult = StringCchVPrintfW(query,
        MAX_QUERY_LEN,
        stringFormat,
        va);
    va_end(va);

    if (FAILED(hResult))
    {
        MyNetwork::SystemLogger::GetInstance()->LogText(
            L"AccountDB-Query", MyNetwork::LEVEL_ERROR, L"Query Too Long... : %s", query);
        MyNetwork::CrashDump::Crash();
    }

    GameServer* server = static_cast<GameServer*>(GetRoot());
    server->accountDBConnector_->Query(query);

    return server->accountDBConnector_->FetchRow();
}

void MyNetwork::AuthPipe::GameDBFreeResult()
{
    GameServer* server = static_cast<GameServer*>(GetRoot());
    server->gameDBConnector_->FreeResult();
}

void MyNetwork::AuthPipe::LogDBFreeResult()
{
    GameServer* server = static_cast<GameServer*>(GetRoot());
    server->logDBConnector_->FreeResult();
}

void MyNetwork::AuthPipe::AccountDBFreeResult()
{
    GameServer* server = static_cast<GameServer*>(GetRoot());
    server->accountDBConnector_->FreeResult();
}

