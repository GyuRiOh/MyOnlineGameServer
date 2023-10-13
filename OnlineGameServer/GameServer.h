#pragma once
#include "../NetRoot/NetServer/NetServer.h"
#include "AuthPipe.h"
#include "GamePipe.h"
#include "../NetRoot/NetServer/PipeManager.h"
#include "GameClient.h"
#include "../DBSaveJob.h"
#include "../NetRoot/Common/DBConnector.h"
#include "../NetRoot/Common/LockFreeEnqQueue.h"

namespace server_baby
{
	class GameServer final : public NetRoot
	{
    public:
        explicit GameServer() noexcept;
        ~GameServer() noexcept override;

        void DBThread() noexcept;
        void DBSave(DBSaveJob* job) noexcept;

    private:
        bool OnConnectionRequest(const SOCKADDR_IN* const addr) noexcept override; //Accept 직후. return false시 클라이언트 거부, true시 접속 허용
        void OnClientJoin(NetSessionID NetSessionID) noexcept override; //Accept 후 접속 처리 완료 후 호출.
        void OnClientLeave(NetSessionID NetSessionID) noexcept override; //Release 후 호출
        void OnRecv(NetPacketSet* const packetList) noexcept override;
        void OnSend(NetSessionID NetSessionID, int sendSize) noexcept override; //패킷 송신 완료 후
        void OnWorkerThreadBegin() noexcept override; //워커스레드 GQCS 하단에서 호출
        void OnWorkerThreadEnd() noexcept override; //워커스레드 1루프 종료 후
        void OnError(int errCode, WCHAR*) noexcept {}
        void OnMonitor(const MonitoringInfo* const info) noexcept override;
        void OnStart() noexcept override;
        void OnStop() noexcept override {};


    public:
        GameClient onlineGameClient_;
        std::thread dbThread_;

        HANDLE DBSaveEvent_;
        LockFreeEnqQueue<DBSaveJob*, 9999, 2048> DBQueue_;

        DBConnector* gameDBConnector_;
        DBConnector* logDBConnector_;
        DBConnector* accountDBConnector_;

    private:
        AuthPipe* authPipe_; 
        GamePipe* gamePipe_;
    };


    inline void server_baby::GameServer::OnClientJoin(NetSessionID sessionID) noexcept
    {
        MovePipe(sessionID, authPipe_->GetPipeID().element_.code_);
    }

    inline void server_baby::GameServer::DBSave(DBSaveJob* job) noexcept
    {
        DBQueue_.Enqueue(job);
        SetEvent(DBSaveEvent_);
    }

    void GetDateTime(WCHAR* buf) noexcept;
    
}