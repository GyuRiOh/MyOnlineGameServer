#include "GamePipe.h"
#include "GamePipe_CS_Stub.h"
#include "PipePlayer.h"
#include "../NetRoot/LanServer/LanPacket.h"
#include "../CommonProtocol.h"
#include "../NetRoot/Common/Parser.h"
#include "../TileAround.h"
#include "../OnlineGameServer/GameServer.h"
#include <fstream>
#include <cassert>

constexpr int MONSTER_GEN_MAX = 200;
constexpr int MONSTER_DAMAGE = 10;
constexpr int MONSTER_ATTACK_PROBABILITY = 3;

server_baby::GamePipe::GamePipe(NetRoot* server, unsigned int framePerSecond, unsigned int threadNum) noexcept
	: NetPipe(server, framePerSecond), framePerSec_(0), clientIDStamp_(1)
{
    RegisterPipeStub(new GamePipe_CS_Stub(this));
	proxy_ = new GamePipe_SC_Proxy(this);

    InitializeMap();
}

server_baby::GamePipe::~GamePipe() noexcept
{
    delete proxy_;
}

server_baby::NetUser* server_baby::GamePipe::OnUserJoin(NetSessionID sessionID) noexcept
{
    //절대 호출 X
    CrashDump::Crash();
    return nullptr;
}

void server_baby::GamePipe::OnUserLeave(NetUser* user) noexcept
{
    //네트워크 스레드가 알려준
    //스레드에서 떠나는 유저
    PipePlayer* player = static_cast<PipePlayer*>(user);

    //로그아웃 로그 저장
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

    //캐릭터 정보 DB에 저장
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

    //나 삭제요~ 패킷을 주위 9개 섹터에 전송
    NetSessionIDSet* idSet = NetSessionIDSet::Alloc();
    GetSessionIDSet_AroundSector_WithoutID(player->GetSessionID(), player->GetCurSector(), idSet);
    proxy_->ResRemoveObject(player->GetClientID(), idSet);

    //섹터맵에서 삭제
    sectorMap_.RemoveFrom(player->GetClientID(), player->GetCurSector());
 
    //소멸자 호출
    player->~PipePlayer();

    //플레이어 풀로 되돌리기
    SizedMemoryPool::GetInstance()->Free(player);

}

void server_baby::GamePipe::OnUserMoveIn(NetUser* user) noexcept
{
    PipePlayer* player = static_cast<PipePlayer*>(user);

    //로그인 로그 남기기
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

    //인증 스레드에서 넘어온 플레이어
    //게임 시작 (패킷을 전송)
    StartGame(static_cast<PipePlayer*>(user));
}

void server_baby::GamePipe::OnUserMoveOut(NetUser* user) noexcept
{
    //절대 호출 X
    CrashDump::Crash();
}

void server_baby::GamePipe::OnUpdate() noexcept
{
    //죽은 몬스터가 있는 경우 삭제하고, 
    //크리스탈 생성하기
    RemoveMonsterAndCreateCrystal();

    //ObjectManager의 Update 호출하기
    objectManager_.Update();

    //몬스터 신규 생성
    GenerateNewMonster();

    //Monster의 경우 움직이고, 위치패킷 쏴주기
    MoveMonster(); 

    //Monster가 임의로 플레이어를 공격
    AttackPlayer();

}

void server_baby::GamePipe::OnUserUpdate(NetUser* user) noexcept
{
    PipePlayer* player = static_cast<PipePlayer*>(user);
    ULONGLONG nowTime = GetTickCount64();

    //마지막 DB저장 시각이 3분 전이면
    //현재의 데이터를 저장
    if (player->GetLastDBSaveTime() < nowTime - 30000)
    {
        //캐릭터 정보 DB에 저장
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
    } 

    //플레이어가 죽었으면 나가기
    if (player->isDead())
        return;

    //플레이어의 HP가 0 이하이나, 아직 죽음 처리가 되지 않음
    if (player->isFatal())
    {
        //크리스탈 1 차감
        player->SubCrystal(1);

        //죽은 상태로 바꾸기
        player->Kill();

        //플레이어 HP정보 바꾸기
        player->ResetInGameData();

        //캐릭터 정보 DB에 저장
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

        //죽음 로그 남기기
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

        //죽음 패킷 전송
        NetSessionIDSet* idSet = NetSessionIDSet::Alloc();
        GetSessionIDSet_AroundSector(player->GetCurSector(), idSet);
        proxy_->ResPlayerDie(player->GetClientID(), 1, idSet);

        return;
    }
}


void server_baby::GamePipe::OnMonitor(const LoopInfo* const info) noexcept 
{
    wchar_t title[64] = { 0 };
    swprintf_s(title, L"GamePipe %d", GetCurrentThreadId());
    SystemLogger::GetInstance()->Console(title, LEVEL_DEBUG, L"Player Num : %d, Frame Count : %d", GetUserSize(), info->frameCountPerSecond_);
   
    framePerSec_ = info->frameCountPerSecond_;
}

void server_baby::GamePipe::OnStart() noexcept
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

void server_baby::GamePipe::OnStop() noexcept
{
    SystemLogger::GetInstance()->Console(L"PipeManager", LEVEL_DEBUG, L"GamePipe Stopped");

}

void server_baby::GamePipe::GetSessionIDSet_AroundSector(SectorPos sector, NetSessionIDSet* const set) noexcept
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

void server_baby::GamePipe::GetSessionIDSet_AroundSector_WithoutID(NetSessionID exceptionID, SectorPos sector, NetSessionIDSet* const set) noexcept
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

void server_baby::GamePipe::UpdateSector(PipePlayer* player) noexcept
{

    if (!player->isSectorChanged())
        return;

    //주변 섹터 다른 캐릭터 생성 패킷 -> 나에게 보냄
    CreateOthreCharacterInSectorAroundForSpecificPlayer(player);

    //섹터 맵 업데이트
    sectorMap_.Update(
        player->GetClientID(),
        player->GetOldSector(),
        player->GetCurSector(),
        player
    );

    //예전 섹터에서 내 캐릭터 삭제 -> 섹터 9개 보냄
    RemoveMyCharacterFromOldSector(player);

    //예전 섹터에서 다른 오브젝트 삭제 -> 나에게 보냄
    RemoveObjectFromOldSectorForSpecificPlayer(player);

    //예전 섹터에서 다른 캐릭터 삭제 -> 나에게 보냄
    RemoveOtherCharacterFromOldSectorForSpecificPlayer(player);

    //주변 섹터 다른 오브젝트 생성 -> 나에게 보냄
    CreateObjectInSectorAroundForSpecificPlayer(player);

    //섹터 단위로 접속중인 사용자에게 
    //내 캐릭터 생성 패킷을 뿌림 -> 섹터 9개 보냄
    CreateMyCharacterInSectorAround(player);
}

void server_baby::GamePipe::UpdateSector(BaseObject* object) noexcept
{
    if (!object->isSectorChanged())
        return;

    //오브젝트 섹터 맵 업데이트
    objectManager_.UpdateSectorMap(object);

    //이전 섹터 주변 플레이어들에게 오브젝트 삭제 패킷 전송
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
    
    //지금 섹터 플레이어들에게 오브젝트 생성 패킷 전송
    sectorMap_.ForeachComplementSectorAround_BasedFrom(
        object->GetCurSector(),
        object->GetOldSector(),
        [object, this](PipePlayer* player)
        {
            if (object->GetType() == eMONSTER_TYPE)
            {
                Monster* monster = static_cast<Monster*>(object);

                //monster 정보 보내기
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

                //Crystal 정보 보내기
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

void server_baby::GamePipe::StartGame(PipePlayer* player) noexcept
{
    //ClientID 부여
    player->InitializeInGameData(GetClientIDStamp());

    //GameDB에 죽음 표식 없애기
    DBSave(player, eGameDB, L"UPDATE gamedb.character SET die = 0 WHERE accountno = %d",
        player->GetAccountNumber());

    //내 캐릭터 생성 패킷
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

    //섹터맵 업데이트 및
    //지정된 섹터에 업데이트 패킷 날리기
    UpdateSector(player);
}

void server_baby::GamePipe::RestartGame(PipePlayer* player) noexcept
{

    //주변 섹터에 삭제 패킷 전송
    NetSessionIDSet* idSet = NetSessionIDSet::Alloc();
    GetSessionIDSet_AroundSector_WithoutID(player->GetSessionID(), player->GetCurSector(), idSet);
    proxy_->ResRemoveObject(player->GetClientID(), idSet);

    //재시작 로그 남기기
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

    //플레이어 부활 및 정보 리셋팅
    player->Resurrect();

    //GameDB에 플레이어 정보 업데이트
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


    //섹터 맵 업데이트
    sectorMap_.Update(
        player->GetClientID(),
        player->GetOldSector(),
        player->GetCurSector(),
        player
    );

    //Res보내기
    proxy_->ResPlayerRestart(player->GetSessionID());

    //내 캐릭터 생성 패킷
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

    //주변 섹터 다른 캐릭터 생성 패킷 -> 나에게 보냄
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

    //주변 섹터 다른 오브젝트 생성 -> 나에게 보냄
    objectManager_.ForeachSectorAround(
        player->GetCurSector(),
        [player, this](BaseObject* obj) {
            if (obj->GetType() == eMONSTER_TYPE)
            {
                Monster* monster = static_cast<Monster*>(obj);

                //monster 정보 보내기
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

                //Crystal 정보 보내기
                proxy_->ResCreateCrystal(
                    crystal->GetClientID(),
                    crystal->GetCrystalType(),
                    crystal->GetClientPosX(),
                    crystal->GetClientPosY(),
                    player->GetSessionID());
            }

        });

    //섹터 단위로 접속중인 사용자에게 
    //내 캐릭터 생성 패킷을 뿌림 -> 섹터 9개 보냄
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

void server_baby::GamePipe::CreateObjectInSectorAroundForSpecificPlayer(PipePlayer* player) noexcept
{ 
    //주변 섹터 다른 오브젝트 생성
    objectManager_.ForeachComplementSectorAround_BasedFrom(
        player->GetCurSector(),
        player->GetOldSector(),
        [player, this](BaseObject* obj) {
        if (obj->GetType() == eMONSTER_TYPE)
        {
            Monster* monster = static_cast<Monster*>(obj);

            //monster 정보 보내기
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

                //Crystal 정보 보내기
                proxy_->ResCreateCrystal(
                    crystal->GetClientID(),
                    crystal->GetCrystalType(),
                    crystal->GetClientPosX(),
                    crystal->GetClientPosY(),
                    player->GetSessionID());
        }
    
        });

}

void server_baby::GamePipe::CreateOthreCharacterInSectorAroundForSpecificPlayer(PipePlayer* player) noexcept
{
    //주변 섹터 다른 캐릭터 생성
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

void server_baby::GamePipe::CreateMyCharacterInSectorAround(PipePlayer* player) noexcept
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

void server_baby::GamePipe::RemoveMyCharacterFromOldSector(PipePlayer* player) noexcept
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

void server_baby::GamePipe::RemoveObjectFromOldSectorForSpecificPlayer(PipePlayer* player) noexcept
{

    objectManager_.ForeachComplementSectorAround_BasedFrom(
        player->GetOldSector(),
        player->GetCurSector(),
        [player, this](BaseObject* obj)
        {
            proxy_->ResRemoveObject(obj->GetClientID(), player->GetSessionID());
        });

}

void server_baby::GamePipe::RemoveOtherCharacterFromOldSectorForSpecificPlayer(PipePlayer* player) noexcept
{ 
    //주변 섹터 다른 캐릭터 삭제
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


void server_baby::GamePipe::AttackPlayer() noexcept
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
            //몬스터의 공격 패킷을 주위 9개 섹터에 전송
            NetSessionIDSet* idSet = NetSessionIDSet::Alloc();
            GetSessionIDSet_AroundSector(player->GetCurSector(), idSet);
            proxy_->ResMonsterAttack(player->GetClientID(), idSet);

            //데미지 패킷을 주위 9개 섹터에 전송
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

void server_baby::GamePipe::DBSave(PipePlayer* player, BYTE dbType, const WCHAR* stringFormat, ...) noexcept
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
        server_baby::SystemLogger::GetInstance()->LogText(
            L"DBSaveJob-Query", server_baby::LEVEL_ERROR, L"Query Too Long... : %s", job->query);
        server_baby::CrashDump::Crash();
    }

    static_cast<GameServer*>(GetRoot())->DBSave(job);

}

void server_baby::GamePipe::GenerateNewMonster() noexcept
{
    //몬스터 50마리 이하면 50마리까지 신규 생성
    //100마리 이상이면 신규생성하지 않음
    while (objectManager_.GetMonsterMapSize() < MONSTER_GEN_MAX)
    {
        Monster* monster = objectManager_.CreateMonster(GetClientIDStamp());

        //몬스터 주변 플레이어에게 몬스터 생성패킷 날리기
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

void server_baby::GamePipe::MoveMonster() noexcept
{
    std::vector<Monster*> movedMonster;

    objectManager_.ForeachAll([&movedMonster, this](BaseObject* obj)
    {
        if (obj->GetType() != eMONSTER_TYPE)
            return;

        Monster* monster = static_cast<Monster*>(obj);

        //이동 가능 범위 추리기
        TileAround tileAround;
        TileAround::GetTileAround(monster->GetTileX(), monster->GetTileY(), tileAround);

        //해당 범위에서 벽에 해당하는 타일 제외
        TilePos destPos = tileAround.around_[rand() % tileAround.count_];
        while (map_[destPos.tileY][destPos.tileX] == 0)
        {
            destPos = tileAround.around_[rand() % tileAround.count_];
        }

        //타일 및 섹터 업데이트
        monster->UpdateTileAndSector(destPos);

        //섹터가 바뀌었으면 패킷 보내기 위헤
        //움직인 몬스터 벡터에 삽입
        if (monster->isSectorChanged())
            movedMonster.push_back(monster);
        else
        {
            //몬스터 이동 패킷 보내기
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

        //몬스터 이동 패킷 보내기
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

}

void server_baby::GamePipe::InitializeMap() noexcept
{
    //비트맵 파일 헤더
    BITMAPFILEHEADER bf;

    //비트맵 이미지 헤더
    BITMAPINFOHEADER bi;

    ifstream fin;
    //map.bmp는 24비트 형식 비트맵
    fin.open("map.bmp", std::ios::binary);
    assert(fin.is_open());

    //헤더 정보 먼저 읽기
    fin.read(reinterpret_cast<char*>(&bf), sizeof(bf));
    fin.read(reinterpret_cast<char*>(&bi), sizeof(bi));

    //픽셀 데이터를 읽기 위한 메모리 할당
    UCHAR inputData[400 * 200 * 3];
    fin.read(reinterpret_cast<char*>(&inputData), bi.biWidth * bi.biHeight* 3);

    //색상값을 읽어 처리하는 코드
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
            map_[Y][X] = 0; //흰색이라면 0 넣기
        else
            map_[Y][X] = 1; //blue값이 있다면 1 넣기. 움직임 가능

        X++;
        if (X == MAP_TILE_X_MAX)
        {
            Y++;
            X = 0;
        }
    }

    ////제대로 bmp파일을 읽었는지 확인하기
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

void server_baby::GamePipe::RemoveMonsterAndCreateCrystal() noexcept
{

    std::vector<Crystal*> createdCrystal;

    //죽은 몬스터가 있으면 죽었다 패킷 쏘기
    objectManager_.ForeachAll([&createdCrystal, this](BaseObject* object)
        {
            if (object->GetType() != eMONSTER_TYPE)
                return;

            Monster* monster = static_cast<Monster*>(object);
            if (!monster->isDead())
                return;

            monster->ReserveDestroy();

            //섹터 단위로 접속중인 사용자에게 몬스터 삭제 패킷을 뿌림
            NetSessionIDSet* idSet = NetSessionIDSet::Alloc();
            GetSessionIDSet_AroundSector(monster->GetCurSector(), idSet);
            proxy_->ResMonsterDie(monster->GetClientID(), idSet);

            //크리스탈 생성
            Crystal* crystal = objectManager_.CreateCrystal(
                monster->GetClientPosX(),
                monster->GetClientPosY(),
                ((rand() % 3) + 1),
                GetClientIDStamp()
            );

            //생성된 크리스탈 리스트에 넣음
            createdCrystal.push_back(crystal);

        });

    //크리스탈 생성 패킷 날리기
    //크리스탈 1개당 현재 섹터 주변의 모든 플레이어들에게 생성 패킷 날리기
    for (auto crystal : createdCrystal)
    {
        //섹터 단위로 접속중인 사용자에게 크리스탈 생성 패킷을 뿌림
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


MYSQL_ROW server_baby::GamePipe::GameDBQuery(const WCHAR* stringFormat, ...) noexcept
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
        server_baby::SystemLogger::GetInstance()->LogText(
            L"GameDB-Query", server_baby::LEVEL_ERROR, L"Query Too Long... : %s", query);
        server_baby::CrashDump::Crash();
    }

    server_baby::GameServer* server = static_cast<GameServer*>(GetRoot());
    server->gameDBConnector_->Query(query);

    return server->gameDBConnector_->FetchRow();

}

MYSQL_ROW server_baby::GamePipe::LogDBQuery(const WCHAR* stringFormat, ...) noexcept
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
        server_baby::SystemLogger::GetInstance()->LogText(
            L"LogDB-Query", server_baby::LEVEL_ERROR, L"Query Too Long... : %s", query);
        server_baby::CrashDump::Crash();
    }

    server_baby::GameServer* server = static_cast<GameServer*>(GetRoot());
    server->logDBConnector_->Query(query);

    return server->logDBConnector_->FetchRow();
}

void server_baby::GamePipe::GameDBFreeResult()
{
    GameServer* server = static_cast<GameServer*>(GetRoot());
    server->gameDBConnector_->FreeResult();
}

void server_baby::GamePipe::LogDBFreeResult()
{
    GameServer* server = static_cast<GameServer*>(GetRoot());
    server->logDBConnector_->FreeResult();
}
