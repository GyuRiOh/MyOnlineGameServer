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
        bool OnConnectionRequest(const SOCKADDR_IN* const addr) noexcept override; //Accept ����. return false�� Ŭ���̾�Ʈ �ź�, true�� ���� ���
        void OnClientJoin(NetSessionID NetSessionID) noexcept override; //Accept �� ���� ó�� �Ϸ� �� ȣ��.
        void OnClientLeave(NetSessionID NetSessionID) noexcept override; //Release �� ȣ��
        void OnRecv(NetPacketSet* const packetList) noexcept override;
        void OnSend(NetSessionID NetSessionID, int sendSize) noexcept override; //��Ŷ �۽� �Ϸ� ��
        void OnWorkerThreadBegin() noexcept override; //��Ŀ������ GQCS �ϴܿ��� ȣ��
        void OnWorkerThreadEnd() noexcept override; //��Ŀ������ 1���� ���� ��
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