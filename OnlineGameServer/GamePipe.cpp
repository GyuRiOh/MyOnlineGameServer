#include "GamePipe.h"
#include "GamePipe_CS_Stub.h"
#include "PipePlayer.h"
#include "../NetRoot/LanServer/LanPacket.h"
#include "../CommonProtocol.h"
#include "../NetRoot/Common/Parser.h"
#include "../TileAround.h"
#include "../OnlineGameServer/GameServer.h"
#include "../Monster_FSM_State.h"
#include <fstream>
#include <cassert>

constexpr int MONSTER_GEN_MAX = 200;
constexpr int MONSTER_DAMAGE = 10;
constexpr int MONSTER_ATTACK_PROBABILITY = 3;

std::vector<vector<BYTE>> map_(MAP_TILE_Y_MAX, vector<BYTE>(MAP_TILE_X_MAX, 0));

MyNetwork::GamePipe::GamePipe(NetRoot* server, unsigned int framePerSecond, unsigned int threadNum) noexcept
	: NetPipe(server, framePerSecond), framePerSec_(0), clientIDStamp_(1)
{
    RegisterPipeStub(new GamePipe_CS_Stub(this));
	proxy_ = new GamePipe_SC_Proxy(this);

    InitializeMap();
}

MyNetwork::GamePipe::~GamePipe() noexcept
{
    delete proxy_;
}

MyNetwork::NetUser* MyNetwork::GamePipe::OnUserJoin(NetSessionID sessionID) noexcept
{
    //���� ȣ�� X
    CrashDump::Crash();
    return nullptr;
}

void MyNetwork::GamePipe::OnUserLeave(NetUser* user) noexcept
{
    //��Ʈ��ũ �����尡 �˷���
    //�����忡�� ������ ����
    PipePlayer* player = static_cast<PipePlayer*>(user);

    //�α׾ƿ� �α� ����
    WCHAR logTime[256] = { 0 };
    GetDateTime(logTime);

    DBSave(player, eLogDB,
        L"INSERT INTO gamelog values(NULL, '%s', %d, 'GameServer', %d, %d, %d, %d, %d, %d, NULL)",
        logTime,
        player->GetAccountNumber(),
        1,
        12,
        player->GetTileX(),
        player->GetTileY(),
        player->GetCrystal(),
        player->GetHP());

    //ĳ���� ���� DB�� ����
    DBSave(player, eGameDB, L"UPDATE gamedb.character SET posx = %f, posy = %f, tilex = %d, tiley = %d, rotation = %d WHERE accountno = %d",
        player->GetClientPosX(),
        player->GetClientPosY(),
        player->GetTileX(),
        player->GetTileY(),
        player->GetRotation(),
        player->GetAccountNumber());

    //�� ������~ ��Ŷ�� ���� 9�� ���Ϳ� ����
    NetSessionIDSet* idSet = NetSessionIDSet::Alloc();
    GetSessionIDSet_AroundSector_WithoutID(player->GetSessionID(), player->GetCurSector(), idSet);
    proxy_->ResRemoveObject(player->GetClientID(), idSet);

    //���͸ʿ��� ����
    sectorMap_.RemoveFrom(player->GetClientID(), player->GetCurSector());
 
    //�Ҹ��� ȣ��
    player->~PipePlayer();

    //�÷��̾� Ǯ�� �ǵ�����
    SizedMemoryPool::GetInstance()->Free(player);

}

void MyNetwork::GamePipe::OnUserMoveIn(NetUser* user) noexcept
{
    PipePlayer* player = static_cast<PipePlayer*>(user);

    //�α��� �α� �����
    WCHAR logTime[256] = { 0 };
    GetDateTime(logTime);

    char IP[30] = { 0 };
    WCHAR wIP[30] = { 0 };
    size_t size = 0;
    player->GetIP(IP);
    mbstowcs_s(&size, wIP, IP, 30);

    DBSave(player, 
        eLogDB,
        L"INSERT INTO gamelog values(NULL, '%s', %d, 'GameServer', %d, %d, %d, %d, %d, %d, '%s:%d')",
        logTime,
        player->GetAccountNumber(),
        1,
        11,
        player->GetTileX(),
        player->GetTileY(),
        player->GetCrystal(),
        player->GetHP(),
        wIP,
        player->GetPort());

    //���� �����忡�� �Ѿ�� �÷��̾�
    //���� ���� (��Ŷ�� ����)
    StartGame(static_cast<PipePlayer*>(user));
}

void MyNetwork::GamePipe::OnUserMoveOut(NetUser* user) noexcept
{
    //���� ȣ�� X
    CrashDump::Crash();
}

void MyNetwork::GamePipe::OnUpdate() noexcept
{
    //���� ���Ͱ� �ִ� ��� �����ϰ�, 
    //ũ����Ż �����ϱ�
    RemoveMonsterAndCreateCrystal();

    //ObjectManager�� Update ȣ���ϱ�
    objectManager_.Update();

    //���� �ű� ����
    GenerateNewMonster();

    //Monster�� ��� �����̰�, ��ġ��Ŷ ���ֱ�
    MoveMonster(); 

    //Monster�� ���Ƿ� �÷��̾ ����
    AttackPlayer();

}

void MyNetwork::GamePipe::OnUserUpdate(NetUser* user) noexcept
{
    PipePlayer* player = static_cast<PipePlayer*>(user);
    ULONGLONG nowTime = GetTickCount64();

    //�÷��̾ �׾����� ������
    if (player->isDead())
        return;

    //�÷��̾��� HP�� 0 �����̳�, ���� ���� ó���� ���� ����
    if (player->isFatal())
    {
        //ũ����Ż 1 ����
        player->SubCrystal(1);

        //���� ���·� �ٲٱ�
        player->KillSelf();

        //�÷��̾� HP���� �ٲٱ�
        player->ResetInGameData();

        //ĳ���� ���� DB�� ����
        DBSave(player, eGameDB, L"UPDATE gamedb.character SET posx = %f, posy = %f, tilex = %d, tiley = %d, rotation = %d, crystal = %d, hp = %d, exp = %d, die = %d WHERE accountno = %d",
            player->GetClientPosX(),
            player->GetClientPosY(),
            player->GetTileX(),
            player->GetTileY(),
            player->GetRotation(),
            player->GetCrystal(),
            player->GetHP(),
            player->GetExp(),
            player->isDead(),
            player->GetAccountNumber());

        //���� �α� �����
        WCHAR logTime[256] = { 0 };
        GetDateTime(logTime);

        DBSave(player, eLogDB,
            L"INSERT INTO gamelog values(NULL, '%s', %d, 'GameServer', %d, %d, %d, %d, %d, NULL, NULL)",
            logTime,
            player->GetAccountNumber(),
            3,
            31,
            player->GetTileX(),
            player->GetTileY(),
            player->GetCrystal());

        //���� ��Ŷ ����
        NetSessionIDSet* idSet = NetSessionIDSet::Alloc();
        GetSessionIDSet_AroundSector(player->GetCurSector(), idSet);
        proxy_->ResPlayerDie(player->GetClientID(), 1, idSet);

        return;
    }
}


void MyNetwork::GamePipe::OnMonitor(const LoopInfo* const info) noexcept 
{
    wchar_t title[64] = { 0 };
    swprintf_s(title, L"GamePipe %d", GetCurrentThreadId());
    SystemLogger::GetInstance()->Console(title, LEVEL_DEBUG, L"Player Num : %d, Frame Count : %d", GetUserSize(), info->frameCountPerSecond_);
   
    framePerSec_ = info->frameCountPerSecond_;
}

void MyNetwork::GamePipe::OnStart() noexcept
{
    HANDLE thread = GetCurrentThread();
    DWORD dwThreadPri = GetThreadPriority(thread);
    SystemLogger::GetInstance()->Console(L"Priority", LEVEL_DEBUG, L"Logic Thread - 0x%x", dwThreadPri);
    SetThreadPriority(thread, THREAD_PRIORITY_ABOVE_NORMAL);

    dwThreadPri = GetThreadPriority(thread);
    SystemLogger::GetInstance()->Console(L"Priority", LEVEL_DEBUG, L"Logic Thread - 0x%x", dwThreadPri);

    CloseHandle(thread);
    
    SystemLogger::GetInstance()->Console(L"PipeManager", LEVEL_DEBUG, L"GamePipe Started");

}

void MyNetwork::GamePipe::OnStop() noexcept
{
    SystemLogger::GetInstance()->Console(L"PipeManager", LEVEL_DEBUG, L"GamePipe Stopped");

}

void MyNetwork::GamePipe::GetSessionIDSet_AroundSector(SectorPos sector, NetSessionIDSet* const set) noexcept
{
    SectorAround sectorAround;
    sectorMap_.GetSectorAround(sector, sectorAround);
    for (int iCnt = 0; iCnt < sectorAround.count_; iCnt++)
    {
        auto& sectorSet = sectorMap_.sectorMap_[sectorAround.around_[iCnt].yPos_][sectorAround.around_[iCnt].xPos_];
        for (auto setIter = sectorSet.begin(); setIter != sectorSet.end(); ++setIter)
        {
            PipePlayer* player = (*setIter).second;
            set->Enqueue(player->GetSessionID());
        }
    }

}

void MyNetwork::GamePipe::GetSessionIDSet_AroundSector_WithoutID(NetSessionID exceptionID, SectorPos sector, NetSessionIDSet* const set) noexcept
{
    SectorAround sectorAround;
    sectorMap_.GetSectorAround(sector, sectorAround);
    for (int iCnt = 0; iCnt < sectorAround.count_; iCnt++)
    {
        auto& sectorSet = sectorMap_.sectorMap_[sectorAround.around_[iCnt].yPos_][sectorAround.around_[iCnt].xPos_];
        for (auto setIter = sectorSet.begin(); setIter != sectorSet.end(); ++setIter)
        {
            PipePlayer* player = (*setIter).second;
            if(player->GetSessionID().element_.unique_ != exceptionID.element_.unique_)
               set->Enqueue(player->GetSessionID());
        }
    }
}

void MyNetwork::GamePipe::UpdateSector(PipePlayer* player) noexcept
{

    if (!player->isSectorChanged())
        return;

    //�ֺ� ���� �ٸ� ĳ���� ���� ��Ŷ -> ������ ����
    CreateOthreCharacterInSectorAroundForSpecificPlayer(player);

    //���� �� ������Ʈ
    sectorMap_.Update(
        player->GetClientID(),
        player->GetOldSector(),
        player->GetCurSector(),
        player
    );

    //���� ���Ϳ��� �� ĳ���� ���� -> ���� 9�� ����
    RemoveMyCharacterFromOldSector(player);

    //���� ���Ϳ��� �ٸ� ������Ʈ ���� -> ������ ����
    RemoveObjectFromOldSectorForSpecificPlayer(player);

    //���� ���Ϳ��� �ٸ� ĳ���� ���� -> ������ ����
    RemoveOtherCharacterFromOldSectorForSpecificPlayer(player);

    //�ֺ� ���� �ٸ� ������Ʈ ���� -> ������ ����
    CreateObjectInSectorAroundForSpecificPlayer(player);

    //���� ������ �������� ����ڿ��� 
    //�� ĳ���� ���� ��Ŷ�� �Ѹ� -> ���� 9�� ����
    CreateMyCharacterInSectorAround(player);
}

void MyNetwork::GamePipe::UpdateSector(BaseObject* object) noexcept
{
    if (!object->isSectorChanged())
        return;

    //������Ʈ ���� �� ������Ʈ
    objectManager_.UpdateSectorMap(object);

    //���� ���� �ֺ� �÷��̾�鿡�� ������Ʈ ���� ��Ŷ ����
    sectorMap_.ForeachComplementSectorAround_BasedFrom(
        object->GetOldSector(),
        object->GetCurSector(),
        [object, this](PipePlayer* player)
        {
            proxy_->ResRemoveObject(
                object->GetClientID(),
                player->GetSessionID()
            );
        }

    );
    
    //���� ���� �÷��̾�鿡�� ������Ʈ ���� ��Ŷ ����
    sectorMap_.ForeachComplementSectorAround_BasedFrom(
        object->GetCurSector(),
        object->GetOldSector(),
        [object, this](PipePlayer* player)
        {
            if (object->GetType() == eMONSTER_TYPE)
            {
                Monster* monster = static_cast<Monster*>(object);

                //monster ���� ������
                proxy_->ResCreateMonsterCharacter(
                    monster->GetClientID(),
                    monster->GetClientPosX(),
                    monster->GetClientPosY(),
                    monster->GetRotation(),
                    0,
                    player->GetSessionID());
            }
            else if (object->GetType() == eCRYSTAL_TYPE)
            {
                Crystal* crystal = static_cast<Crystal*>(object);

                //Crystal ���� ������
                proxy_->ResCreateCrystal(
                    crystal->GetClientID(),
                    crystal->GetCrystalType(),
                    crystal->GetClientPosX(),
                    crystal->GetClientPosY(),
                    player->GetSessionID());
            }

        }

    );
}

void MyNetwork::GamePipe::StartGame(PipePlayer* player) noexcept
{
    //ClientID �ο�
    player->InitializeInGameData(GetClientIDStamp());

    //GameDB�� ���� ǥ�� ���ֱ�
    DBSave(player, eGameDB, L"UPDATE gamedb.character SET die = 0 WHERE accountno = %d",
        player->GetAccountNumber());

    //�� ĳ���� ���� ��Ŷ
    proxy_->ResCreateMyCharacter(
        player->GetClientID(),
        player->GetCharacterType(),
        player->GetNickName(),
        player->GetClientPosX(),
        player->GetClientPosY(),
        player->GetRotation(),
        player->GetCrystal(),
        player->GetHP(),
        player->GetExp(),
        player->GetLevel(),
        player->GetSessionID());

    //���͸� ������Ʈ ��
    //������ ���Ϳ� ������Ʈ ��Ŷ ������
    UpdateSector(player);
}

void MyNetwork::GamePipe::RestartGame(PipePlayer* player) noexcept
{

    //�ֺ� ���Ϳ� ���� ��Ŷ ����
    NetSessionIDSet* idSet = NetSessionIDSet::Alloc();
    GetSessionIDSet_AroundSector_WithoutID(player->GetSessionID(), player->GetCurSector(), idSet);
    proxy_->ResRemoveObject(player->GetClientID(), idSet);

    //����� �α� �����
    WCHAR logTime[256] = { 0 };
    GetDateTime(logTime);

    DBSave(player, eLogDB,
        L"INSERT INTO gamelog values (NULL, '%s', %d, 'GameServer', %d, %d, %d, %d, %d, NULL, NULL)",
        logTime,
        player->GetAccountNumber(),
        3,
        33,
        player->GetTileX(),
        player->GetTileY());

    //�÷��̾� ��Ȱ �� ���� ������
    player->Resurrect();

    //GameDB�� �÷��̾� ���� ������Ʈ
    DBSave(player, eGameDB, L"UPDATE gamedb.character SET posx = %f, posy = %f, tilex = %d, tiley = %d, crystal = %d, hp = %d, exp = %d, die = %d WHERE accountno = %d",
        player->GetClientPosX(),
        player->GetClientPosY(),
        player->GetTileX(),
        player->GetTileY(),
        player->GetCrystal(),
        player->GetHP(),
        player->GetExp(),
        player->isDead(),
        player->GetAccountNumber());


    //���� �� ������Ʈ
    sectorMap_.Update(
        player->GetClientID(),
        player->GetOldSector(),
        player->GetCurSector(),
        player
    );

    //Res������
    proxy_->ResPlayerRestart(player->GetSessionID());

    //�� ĳ���� ���� ��Ŷ
    proxy_->ResCreateMyCharacter(
        player->GetClientID(),
        player->GetCharacterType(),
        player->GetNickName(),
        player->GetClientPosX(),
        player->GetClientPosY(),
        player->GetRotation(),
        player->GetCrystal(),
        player->GetHP(),
        player->GetExp(),
        player->GetLevel(),
        player->GetSessionID());

    //�ֺ� ���� �ٸ� ĳ���� ���� ��Ŷ -> ������ ����
    sectorMap_.ForeachSectorAround(
        player->GetCurSector(),
        [player, this](PipePlayer* other) {

            if (player == other)
            return;

        proxy_->ResCreateOtherCharacter(
            other->GetClientID(),
            other->GetCharacterType(),
            other->GetNickName(),
            other->GetClientPosX(),
            other->GetClientPosY(),
            other->GetRotation(),
            other->GetLevel(),
            1,
            other->isSitting(),
            other->isDead(),
            player->GetSessionID());
        });

    //�ֺ� ���� �ٸ� ������Ʈ ���� -> ������ ����
    objectManager_.ForeachSectorAround(
        player->GetCurSector(),
        [player, this](BaseObject* obj) {
            if (obj->GetType() == eMONSTER_TYPE)
            {
                Monster* monster = static_cast<Monster*>(obj);

                //monster ���� ������
                proxy_->ResCreateMonsterCharacter(
                    monster->GetClientID(),
                    monster->GetClientPosX(),
                    monster->GetClientPosY(),
                    monster->GetRotation(),
                    0,
                    player->GetSessionID());
            }
            else if (obj->GetType() == eCRYSTAL_TYPE)
            {
                Crystal* crystal = static_cast<Crystal*>(obj);

                //Crystal ���� ������
                proxy_->ResCreateCrystal(
                    crystal->GetClientID(),
                    crystal->GetCrystalType(),
                    crystal->GetClientPosX(),
                    crystal->GetClientPosY(),
                    player->GetSessionID());
            }

        });

    //���� ������ �������� ����ڿ��� 
    //�� ĳ���� ���� ��Ŷ�� �Ѹ� -> ���� 9�� ����
    sectorMap_.ForeachSectorAround(
        player->GetCurSector(),
        [player, this](PipePlayer* other)
        {
            if (player == other)
            return;

            proxy_->ResCreateOtherCharacter(
                player->GetClientID(),
                player->GetCharacterType(),
                player->GetNickName(),
                player->GetClientPosX(),
                player->GetClientPosY(),
                player->GetRotation(),
                player->GetLevel(),
                1,
                player->isSitting(),
                player->isDead(),
                other->GetSessionID());
        }
    );
}

void MyNetwork::GamePipe::CreateObjectInSectorAroundForSpecificPlayer(PipePlayer* player) noexcept
{ 
    //�ֺ� ���� �ٸ� ������Ʈ ����
    objectManager_.ForeachComplementSectorAround_BasedFrom(
        player->GetCurSector(),
        player->GetOldSector(),
        [player, this](BaseObject* obj) {
        if (obj->GetType() == eMONSTER_TYPE)
        {
            Monster* monster = static_cast<Monster*>(obj);

            //monster ���� ������
            proxy_->ResCreateMonsterCharacter(
                monster->GetClientID(),
                monster->GetClientPosX(),
                monster->GetClientPosY(),
                monster->GetRotation(),
                0,
                player->GetSessionID());
        }
        else if (obj->GetType() == eCRYSTAL_TYPE)
        {
             Crystal* crystal = static_cast<Crystal*>(obj);

                //Crystal ���� ������
                proxy_->ResCreateCrystal(
                    crystal->GetClientID(),
                    crystal->GetCrystalType(),
                    crystal->GetClientPosX(),
                    crystal->GetClientPosY(),
                    player->GetSessionID());
        }
    
        });

}

void MyNetwork::GamePipe::CreateOthreCharacterInSectorAroundForSpecificPlayer(PipePlayer* player) noexcept
{
    //�ֺ� ���� �ٸ� ĳ���� ����
    sectorMap_.ForeachComplementSectorAround_BasedFrom(
        player->GetCurSector(),
        player->GetOldSector(),
        [player, this](PipePlayer* other) {

            if (player == other)
                return;

        proxy_->ResCreateOtherCharacter(
            other->GetClientID(),
            other->GetCharacterType(),
            other->GetNickName(),
            other->GetClientPosX(),
            other->GetClientPosY(),
            other->GetRotation(),
            other->GetLevel(),
            1, 
            other->isSitting(),
            other->isDead(),
            player->GetSessionID());
        });

}

void MyNetwork::GamePipe::CreateMyCharacterInSectorAround(PipePlayer* player) noexcept
{
    sectorMap_.ForeachComplementSectorAround_BasedFrom(
        player->GetCurSector(),
        player->GetOldSector(),
        [player, this](PipePlayer* other)
        {
            if (player == other)
                return;

            proxy_->ResCreateOtherCharacter(
                player->GetClientID(),
                player->GetCharacterType(),
                player->GetNickName(),
                player->GetClientPosX(),
                player->GetClientPosY(),
                player->GetRotation(),
                player->GetLevel(),
                1,
                player->isSitting(),
                player->isDead(),
                other->GetSessionID());
        }
    );
}

void MyNetwork::GamePipe::RemoveMyCharacterFromOldSector(PipePlayer* player) noexcept
{
    sectorMap_.ForeachComplementSectorAround_BasedFrom(
        player->GetOldSector(), 
        player->GetCurSector(),
        [player, this](PipePlayer* other)
        {
            proxy_->ResRemoveObject(
                player->GetClientID(),
                other->GetSessionID());
        });
}

void MyNetwork::GamePipe::RemoveObjectFromOldSectorForSpecificPlayer(PipePlayer* player) noexcept
{

    objectManager_.ForeachComplementSectorAround_BasedFrom(
        player->GetOldSector(),
        player->GetCurSector(),
        [player, this](BaseObject* obj)
        {
            proxy_->ResRemoveObject(obj->GetClientID(), player->GetSessionID());
        });

}

void MyNetwork::GamePipe::RemoveOtherCharacterFromOldSectorForSpecificPlayer(PipePlayer* player) noexcept
{ 
    //�ֺ� ���� �ٸ� ĳ���� ����
    sectorMap_.ForeachComplementSectorAround_BasedFrom(
        player->GetOldSector(),
        player->GetCurSector(),
        [player, this](PipePlayer* other) {

            proxy_->ResRemoveObject(
                other->GetClientID(),
                player->GetSessionID());
        }
    );
}


void MyNetwork::GamePipe::AttackPlayer() noexcept
{
    
    objectManager_.ForeachAll([this](BaseObject* obj)
        {
        if (obj->GetType() != eMONSTER_TYPE)
                return;

        int random = rand() % 1000;
        if (random > MONSTER_ATTACK_PROBABILITY)
                return;

        std::vector<PipePlayer*> damagedPlayer;

        sectorMap_.ForeachSector(
            obj->GetCurSector(),
            [&damagedPlayer, this](PipePlayer* player)
            {
                if (player->isDead())
                    return;

            player->Damaged(MONSTER_DAMAGE);          
            damagedPlayer.push_back(player);
        });

        for (auto player : damagedPlayer)
        {
            //������ ���� ��Ŷ�� ���� 9�� ���Ϳ� ����
            NetSessionIDSet* idSet = NetSessionIDSet::Alloc();
            GetSessionIDSet_AroundSector(player->GetCurSector(), idSet);
            proxy_->ResMonsterAttack(player->GetClientID(), idSet);

            //������ ��Ŷ�� ���� 9�� ���Ϳ� ����
            NetSessionIDSet* damageidSet = NetSessionIDSet::Alloc();
            GetSessionIDSet_AroundSector(player->GetCurSector(), damageidSet);
            proxy_->ResDamage
               (obj->GetClientID(),
                player->GetClientID(),
                MONSTER_DAMAGE,
                damageidSet);

        }
        });

}

void MyNetwork::GamePipe::DBSave(PipePlayer* player, BYTE dbType, const WCHAR* stringFormat, ...) noexcept
{
    if (dbType == eGameDB)
        player->UpdateLastDBSaveTime();

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

void MyNetwork::GamePipe::GenerateNewMonster() noexcept
{
    //���� 50���� ���ϸ� 50�������� �ű� ����
    //100���� �̻��̸� �űԻ������� ����
    while (objectManager_.GetMonsterMapSize() < MONSTER_GEN_MAX)
    {
        Monster* monster = objectManager_.CreateMonster(GetClientIDStamp());

        //���� �ֺ� �÷��̾�� ���� ������Ŷ ������
        sectorMap_.ForeachSectorAround(
            monster->GetCurSector(),
            [this, monster](PipePlayer* player)
            {
                proxy_->ResCreateMonsterCharacter(
                    monster->GetClientID(),
                    monster->GetClientPosX(),
                    monster->GetClientPosY(),
                    monster->GetRotation(),
                    0,
                    player->GetSessionID());
            }
        );

    }
}

void MyNetwork::GamePipe::MoveMonster() noexcept
{
    std::vector<Monster*> movedMonster;

    objectManager_.ForeachAll([&movedMonster, this](BaseObject* obj)
    {
        if (obj->GetType() != eMONSTER_TYPE)
            return;
        
        Monster* monster = static_cast<Monster*>(obj);
        monster->context_->Move();

        //���Ͱ� �ٲ������ ��Ŷ ������ ����
        //������ ���� ���Ϳ� ����
        if (monster->isSectorChanged())
            movedMonster.push_back(monster);
        else
        {
            //���� �̵� ��Ŷ ������
            sectorMap_.ForeachSectorAround(monster->GetCurSector(),
                [this, monster](PipePlayer*& player)
                {
                    proxy_->ResMoveMonster(
                        monster->GetClientID(),
                        monster->GetClientPosX(),
                        monster->GetClientPosY(),
                        monster->GetRotation(),
                        player->GetSessionID());
                });
        }


    });

    for (auto& monster : movedMonster)
    {
        UpdateSector(monster);

        //���� �̵� ��Ŷ ������
        sectorMap_.ForeachSectorAround(monster->GetCurSector(),
            [this, monster](PipePlayer* player)
            {
                proxy_->ResMoveMonster(
                    monster->GetClientID(),
                    monster->GetClientPosX(),
                    monster->GetClientPosY(),
                    monster->GetRotation(),
                    player->GetSessionID());
            });
    }    

}

MyNetwork::PipePlayer* MyNetwork::GamePipe::GetNearestPlayer(TilePos monsterPos)
{
    PipePlayer* res = nullptr;
    SectorPos monsterSector;
    monsterSector.SetSector(monsterPos.tileX, monsterPos.tileY);

    sectorMap_.ForeachSectorAround(monsterSector, [this, &res](PipePlayer* player)
        {
            if(!res)
                res = player;
        });

    return res;
}

void MyNetwork::GamePipe::InitializeMap() noexcept
{


    //��Ʈ�� ���� ���, �̹��� ���
    BITMAPFILEHEADER bf;
    BITMAPINFOHEADER bi;

    ifstream fin;
    //map.bmp�� 24��Ʈ ���� ��Ʈ��
    fin.open("map.bmp", std::ios::binary);
    assert(fin.is_open());

    //��� ���� ���� �б�
    fin.read(reinterpret_cast<char*>(&bf), sizeof(bf));
    fin.read(reinterpret_cast<char*>(&bi), sizeof(bi));

    //�ȼ� �����͸� �б� ���� �޸� �Ҵ� �� �޸� �б�
    UCHAR inputData[400 * 200 * 3];
    fin.read(reinterpret_cast<char*>(&inputData), 
        bi.biWidth * bi.biHeight* 3);

    //������ �о� ó���ϴ� �ڵ�
    bool red, green, blue = 0;
    int X = 0;
    int Y = 0;
    for (int pixel = 0; pixel< bi.biWidth * bi.biHeight; ++pixel)
    {
        int pixelIdx = (pixel * 3);

        blue = inputData[pixelIdx];
        green = inputData[pixelIdx + 1];
        red = inputData[pixelIdx + 2];
        
        if (blue)
            map_[Y][X] = 0; //����̶�� 0 �ֱ�
        else
            map_[Y][X] = 1; //blue���� �ִٸ� 1 �ֱ�. ������ ����

        X++;
        if (X == MAP_TILE_X_MAX)
        {
            Y++;
            X = 0;
        }
    }

    ////����� bmp������ �о����� Ȯ���ϱ�
    //printf("\n");

    //for (int i = MAP_TILE_Y_MAX - 1; i >= 0; i--)
    //{
    //    for (int j = 0; j < (MAP_TILE_X_MAX / 4); j++)
    //    {
    //        printf("%d", map_[i][j]);
    //    }
    //    printf("$");
    //}

}

void MyNetwork::GamePipe::RemoveMonsterAndCreateCrystal() noexcept
{

    std::vector<Crystal*> createdCrystal;

    //���� ���Ͱ� ������ �׾��� ��Ŷ ���
    objectManager_.ForeachAll([&createdCrystal, this](BaseObject* object)
        {
            if (object->GetType() != eMONSTER_TYPE)
                return;

            Monster* monster = static_cast<Monster*>(object);

            if (!monster->isZeroHP())
                return;

            monster->DestroySelf();

            //���� ������ �������� ����ڿ��� ���� ���� ��Ŷ�� �Ѹ�
            NetSessionIDSet* idSet = NetSessionIDSet::Alloc();
            GetSessionIDSet_AroundSector(monster->GetCurSector(), idSet);
            proxy_->ResMonsterDie(monster->GetClientID(), idSet);

            //ũ����Ż ����
            Crystal* crystal = objectManager_.CreateCrystal(
                monster->GetClientPosX(),
                monster->GetClientPosY(),
                ((rand() % 3) + 1),
                GetClientIDStamp()
            );

            //������ ũ����Ż ����Ʈ�� ����
            createdCrystal.push_back(crystal);

        });

    //ũ����Ż ���� ��Ŷ ������
    //ũ����Ż 1���� ���� ���� �ֺ��� ��� �÷��̾�鿡�� ���� ��Ŷ ������
    for (auto crystal : createdCrystal)
    {
        //���� ������ �������� ����ڿ��� ũ����Ż ���� ��Ŷ�� �Ѹ�
        NetSessionIDSet* idSet = NetSessionIDSet::Alloc();
        GetSessionIDSet_AroundSector(crystal->GetCurSector(), idSet);
        proxy_->ResCreateCrystal(
            crystal->GetClientID(),
            crystal->GetCrystalType(),
            crystal->GetClientPosX(),
            crystal->GetClientPosY(),
            idSet);

    }

}
