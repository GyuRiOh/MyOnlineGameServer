#include "GameServer.h"
#include "../NetRoot/Common/PDHMonitor.h"
#include "../NetRoot/Common/Parser.h"
#include "../CommonProtocol.h"

MyNetwork::GameServer::GameServer() noexcept : 
    authPipe_(nullptr), gamePipe_(nullptr), logDBConnector_(nullptr), accountDBConnector_(nullptr),
DBSaveEvent_(CreateEvent(NULL, FALSE, NULL, NULL)), gameDBConnector_(nullptr)
{
    logDBConnector_ = new DBConnector(
        L"127.0.0.1",
        L"root",
        L"1234",
        L"logdb",
        3306);

    gameDBConnector_ = new DBConnector(
        L"127.0.0.1",
        L"root",
        L"1234",
        L"gamedb",
        3306);

    accountDBConnector_ = new DBConnector(
        L"127.0.0.1",
        L"root",
        L"1234",
        L"accountdb",
        3306);

    authPipe_ = new AuthPipe(this, 60);
    gamePipe_ = new PipeManager<GamePipe>(3, 100, this, 6789, 60, false); 
    
    RegisterPipe(AUTH_PIPE_ID, authPipe_);
    gamePipe_->Start();
}

MyNetwork::GameServer::~GameServer() noexcept 
{
    delete gameDBConnector_;
    delete logDBConnector_;
}

bool MyNetwork::GameServer::OnConnectionRequest(const SOCKADDR_IN* const addr) noexcept
{
    //IP, Port 확인하고 필터링
    return true;
}

void MyNetwork::GameServer::OnClientLeave(NetSessionID NetSessionID) noexcept {}
void MyNetwork::GameServer::OnRecv(NetPacketSet* const packetList) noexcept { DefaultOnRecv(packetList); }
void MyNetwork::GameServer::OnSend(NetSessionID NetSessionID, int sendSize) noexcept {}
void MyNetwork::GameServer::OnWorkerThreadBegin() noexcept {}
void MyNetwork::GameServer::OnWorkerThreadEnd() noexcept {}

void MyNetwork::GameServer::OnMonitor(const MonitoringInfo* const info) noexcept
{
    HardwareMonitor::CpuUsageForProcessor::GetInstance()->UpdateCpuTime();
    ProcessMonitor::CpuUsageForProcess::GetInstance()->UpdateCpuTime();
    ProcessMonitor::MemoryForProcess::GetInstance()->Update();

    time_t timer;
    timer = time(NULL);

    LanPacket* packet = LanPacket::Alloc();

    *packet << static_cast<WORD>(en_PACKET_SS_MONITOR_DATA_UPDATE);
    *packet << static_cast<BYTE>(dfMONITOR_DATA_TYPE_GAME_SERVER_RUN);
    *packet << static_cast<int>(true);
    *packet << static_cast<int>(timer);

    onlineGameClient_.SendPacket(packet);
    packet->Clear();

    *packet << static_cast<WORD>(en_PACKET_SS_MONITOR_DATA_UPDATE);
    *packet << static_cast<BYTE>(dfMONITOR_DATA_TYPE_GAME_SERVER_CPU);
    *packet << static_cast<int>(ProcessMonitor::CpuUsageForProcess::GetInstance()->ProcessTotal());
    *packet << static_cast<int>(timer);

    onlineGameClient_.SendPacket(packet);
    packet->Clear();

    *packet << static_cast<WORD>(en_PACKET_SS_MONITOR_DATA_UPDATE);
    *packet << static_cast<BYTE>(dfMONITOR_DATA_TYPE_GAME_SERVER_MEM);
    *packet << static_cast<int>(ProcessMonitor::MemoryForProcess::GetInstance()->GetPrivateBytes(L"GameEchoServer") / eMEGA_BYTE);
    *packet << static_cast<int>(timer);

    onlineGameClient_.SendPacket(packet);
    packet->Clear();

    *packet << static_cast<WORD>(en_PACKET_SS_MONITOR_DATA_UPDATE);
    *packet << static_cast<BYTE>(dfMONITOR_DATA_TYPE_GAME_SESSION);
    *packet << static_cast<int>(info->sessionCount_);
    *packet << static_cast<int>(timer);

    onlineGameClient_.SendPacket(packet);
    packet->Clear();

    *packet << static_cast<WORD>(en_PACKET_SS_MONITOR_DATA_UPDATE);
    *packet << static_cast<BYTE>(dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS);
    *packet << static_cast<int>(info->acceptTPS_);
    *packet << static_cast<int>(timer);

    onlineGameClient_.SendPacket(packet);
    packet->Clear();

    *packet << static_cast<WORD>(en_PACKET_SS_MONITOR_DATA_UPDATE);
    *packet << static_cast<BYTE>(dfMONITOR_DATA_TYPE_GAME_PACKET_POOL);
    *packet << static_cast<int>(info->packetCount_);
    *packet << static_cast<int>(timer);

    onlineGameClient_.SendPacket(packet);
    packet->Clear();

    *packet << static_cast<WORD>(en_PACKET_SS_MONITOR_DATA_UPDATE);
    *packet << static_cast<BYTE>(dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL);
    *packet << static_cast<int>(HardwareMonitor::CpuUsageForProcessor::GetInstance()->ProcessorTotal());
    *packet << static_cast<int>(timer);

    onlineGameClient_.SendPacket(packet);
    packet->Clear();

    *packet << static_cast<WORD>(en_PACKET_SS_MONITOR_DATA_UPDATE);
    *packet << static_cast<BYTE>(dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY);
    *packet << static_cast<int>(ProcessMonitor::MemoryForProcess::GetInstance()->GetNonPagedByte() / eMEGA_BYTE);
    *packet << static_cast<int>(timer);

    onlineGameClient_.SendPacket(packet);
    packet->Clear();

    *packet << static_cast<WORD>(en_PACKET_SS_MONITOR_DATA_UPDATE);
    *packet << static_cast<BYTE>(dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV);
    *packet << static_cast<int>(ProcessMonitor::MemoryForProcess::GetInstance()->GetTotalRecvBytes() / eKILO_BYTE);
    *packet << static_cast<int>(timer);

    onlineGameClient_.SendPacket(packet);
    packet->Clear();

    *packet << static_cast<WORD>(en_PACKET_SS_MONITOR_DATA_UPDATE);
    *packet << static_cast<BYTE>(dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND);
    *packet << static_cast<int>(ProcessMonitor::MemoryForProcess::GetInstance()->GetTotalSendBytes() / eKILO_BYTE);
    *packet << static_cast<int>(timer);

    onlineGameClient_.SendPacket(packet);
    packet->Clear();

    *packet << static_cast<WORD>(en_PACKET_SS_MONITOR_DATA_UPDATE);
    *packet << static_cast<BYTE>(dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY);
    *packet << static_cast<int>(ProcessMonitor::MemoryForProcess::GetInstance()->GetAvailableBytes());
    *packet << static_cast<int>(timer);

    onlineGameClient_.SendPacket(packet);
    packet->Clear();

    *packet << static_cast<WORD>(en_PACKET_SS_MONITOR_DATA_UPDATE);
    *packet << static_cast<BYTE>(dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS);
    *packet << static_cast<int>(info->recvTPS_);
    *packet << static_cast<int>(timer);

    onlineGameClient_.SendPacket(packet);
    packet->Clear();

    *packet << static_cast<WORD>(en_PACKET_SS_MONITOR_DATA_UPDATE);
    *packet << static_cast<BYTE>(dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS);
    *packet << static_cast<int>(info->sendTPS_);
    *packet << static_cast<int>(timer);

    onlineGameClient_.SendPacket(packet);
    packet->Clear();

    *packet << static_cast<WORD>(en_PACKET_SS_MONITOR_DATA_UPDATE);
    *packet << static_cast<BYTE>(dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL);
    *packet << static_cast<int>(info->recvTPS_ / 10000);
    *packet << static_cast<int>(timer);

    onlineGameClient_.SendPacket(packet);
    packet->Clear();

    *packet << static_cast<WORD>(en_PACKET_SS_MONITOR_DATA_UPDATE);
    *packet << static_cast<BYTE>(dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL);
    *packet << static_cast<int>(info->sendTPS_ / 10000);
    *packet << static_cast<int>(timer);

    onlineGameClient_.SendPacket(packet);
    packet->Clear();

    //*packet << static_cast<WORD>(en_PACKET_SS_MONITOR_DATA_UPDATE);
    //*packet << static_cast<BYTE>(dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER);
    //*packet << static_cast<int>(gamePipe_->GetUserSize());
    //*packet << static_cast<int>(timer);

    //onlineGameClient_.SendPacket(packet);
    //packet->Clear();

    //*packet << static_cast<WORD>(en_PACKET_SS_MONITOR_DATA_UPDATE);
    //*packet << static_cast<BYTE>(dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS);
    //*packet << static_cast<int>(gamePipe_->framePerSec_);
    //*packet << static_cast<int>(timer);

    ////onlineGameClient_.SendPacket(packet);
    //packet->Clear();

    *packet << static_cast<WORD>(en_PACKET_SS_MONITOR_DATA_UPDATE);
    *packet << static_cast<BYTE>(dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER);
    *packet << static_cast<int>(authPipe_->GetUserSize());
    *packet << static_cast<int>(timer);

    onlineGameClient_.SendPacket(packet);
    packet->Clear();

    *packet << static_cast<WORD>(en_PACKET_SS_MONITOR_DATA_UPDATE);
    *packet << static_cast<BYTE>(dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS);
    *packet << static_cast<int>(authPipe_->framePerSec_);
    *packet << static_cast<int>(timer);

    onlineGameClient_.SendPacket(packet);

    LanPacket::Free(packet);

    ProcessMonitor::MemoryForProcess::GetInstance()->ZeroEthernetValue();  
    SystemLogger::GetInstance()->Console(L"GameServer", LEVEL_DEBUG, L"Recv TPS : %d", info->recvTPS_);
    SystemLogger::GetInstance()->Console(L"GameServer", LEVEL_DEBUG, L"Send TPS : %d", info->sendTPS_);
    SystemLogger::GetInstance()->Console(L"GameServer", LEVEL_DEBUG, L"Accept TPS : %d", info->acceptTPS_);

  
}

void MyNetwork::GameServer::OnStart() noexcept
{   
    dbThread_ = std::thread([this] {
        DBThread();
        });

    int relayPort = 0;
    Parser::GetInstance()->GetValue("RelayServerPort", (int*)&relayPort);
    SystemLogger::GetInstance()->Console(L"NetServer", LEVEL_DEBUG, L"Relay Server Port : %d", relayPort);

    char IP[16] = "127.0.0.1";
    onlineGameClient_.Start(IP, relayPort);

}

void MyNetwork::GameServer::DBThread() noexcept
{
    while (isServerRunning())
    {
        WaitForSingleObject(DBSaveEvent_, INFINITE);

        DBSaveJob* job = nullptr;
        while (DBQueue_.Dequeue(&job))
        {
            switch (job->type)
            {
            case eGameDB:
                gameDBConnector_->Query_Save(job->query);
                break;
            case eLogDB:
                logDBConnector_->Query_Save(job->query);
                break;
            default: //있어서는 안 되는 경우
                CrashDump::Crash();
                break;
            }

            SizedMemoryPool::GetInstance()->Free(job);
        }


    }
}

void MyNetwork::GetDateTime(WCHAR* buf) noexcept
{
    time_t timer;
    struct tm t;
    timer = time(NULL);
    localtime_s(&t, &timer);

    swprintf_s(buf, 256, L"%d-%02d-%02d %02d:%02d:%02d",
        t.tm_year + 1900,
        t.tm_mon + 1,
        t.tm_mday,
        t.tm_hour,
        t.tm_min,
        t.tm_sec);
}
