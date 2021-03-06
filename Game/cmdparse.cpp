

#include <boost/asio.hpp>
#include <iostream>

#include "postgresqlstruct.h"
#include "cmdtypes.h"
#include "./../server.h"

#include "userposition.hpp"
#include "log.h"
#include "./../NetWork/tcpconnection.h"
#include "./../PlayerLogin/playerlogin.h"
#include "./../GameTask/gametask.h"
#include "./../TeamFriend/teamfriend.h"
#include "./../GameAttack/gameattack.h"
#include "./../GameInterchange/gameinterchange.h"
#include "./../OperationGoods/operationgoods.h"
#include "./../GameChat/gamechat.h"
#include "cmdparse.h"

void CommandParse(TCPConnection::Pointer conn , void *reg)
{
//     tcp::socket *sk =  &(conn->socket());
    char *buf = (char*)reg;
    STR_PackHead *head = (STR_PackHead*)buf;
    hf_uint16 flag = head->Flag;
    hf_uint16 len = head->Len;

    Server *srv = Server::GetInstance();
    PlayerLogin* t_playerLogin = srv->GetPlayerLogin();
    GameTask* t_task = srv->GetGameTask();
    TeamFriend* t_teamFriend = srv->GetTeamFriend();
    GameAttack* t_gameAttack = srv->GetGameAttack();
    GameInterchange* t_gameInterchange = srv->GetGameInterchange();
    OperationGoods* t_operationGoods = srv->GetOperationGoods();
    GameChat* t_gameChat = srv->GetGameChat();

    SessionMgr::SessionPointer smap =  SessionMgr::Instance()->GetSession();
    Session* sess = &(*smap)[conn];
    hf_uint32 hp = sess->m_roleInfo.HP;
    if(hp == 0) //玩家死亡
    {
        if(!(flag == FLAG_PlayerRevive || flag == FLAG_PlayerPosition))
            return;
    }

    switch( flag )
    {
    case FLAG_PlayerRevive:          //玩家复活   test
    {
        STR_PlayerRelive* reg = (STR_PlayerRelive*)(buf + sizeof(STR_PackHead));
        t_playerLogin->PlayerRelive(conn, reg->mode);
//        srv->RunTask(boost::bind(&PlayerLogin::PlayerRelive, t_playerLogin, conn, reg->mode));
        break;
    }

    case FLAG_PlayerOffline:  //玩家下线，给其他玩家发送下线通知
    {
        Logger::GetLogger()->Info("User Position recv");

        void *buf2 = srv->malloc();
        STR_PackPlayerOffline *pos =  new (buf2)STR_PackPlayerOffline ();
        memcpy(pos,buf,sizeof(STR_PackHead) + len);
        t_playerLogin->PlayerOffline(conn, pos);
//        srv->RunTask(boost::bind(&PlayerLogin::PlayerOffline, t_playerLogin, conn, pos));
        break;
    }
    case FLAG_PlayerPosition:   //玩家位置变动(test)
    {
//        Logger::GetLogger()->Debug("Player position changed");
//        STR_PackPlayerPosition* reg = (STR_PackPlayerPosition*)srv->malloc();
//        memcpy(reg, buf, len + sizeof(STR_PackHead));
        UserPosition::PlayerMove(conn, (STR_PackPlayerPosition*)reg);
//        srv->RunTask(boost::bind(UserPosition::PlayerMove, conn, reg));
        break;
    }

    case FLAG_PlayerDirect:
    {
        hf_float direct = 0/* *(hf_float*)buf*/;
        memcpy(&direct, buf + sizeof(STR_PackHead), 4);
//        printf("%f\n", direct);
        UserPosition::PlayerDirectChange(conn, direct);
//        srv->RunTask(boost::bind(UserPosition::PlayerDirectChange, conn, direct));
        break;
    }

    case FLAG_PlayerAction:
    {
        hf_uint8 action = 0;
        memcpy(&action, (buf + sizeof(STR_PackHead)), 1);
//        printf("playerAction:%d\n", action);
        UserPosition::PlayerActionChange(conn, action);
//        srv->RunTask(boost::bind(UserPosition::PlayerActionChange, conn, action));
        break;
    }
    case FLAG_PlayerMove:    //玩家移动
    {
        Logger::GetLogger()->Debug("Player move");
        STR_PlayerMove* t_move = (STR_PlayerMove*)srv->malloc();
        memcpy(t_move, buf + sizeof(STR_PackHead), len);
        UserPosition::PlayerPositionMove(conn, t_move);
//        srv->RunTask(boost::bind(UserPosition::PlayerPositionMove, conn, t_move));
        break;
    }
    case FLAG_UserAskTask: //玩家请求接受任务数据
    {
        hf_uint32 taskID = ((STR_PackUserAskTask*)buf)->TaskID;
        t_task->AskTask(conn, taskID);
//        srv->RunTask(boost::bind(&GameTask::AskTask, t_task, conn, taskID));
        break;
    }
    case FLAG_QuitTask:   //玩家请求放弃任务
    {
        hf_uint32 taskID = ((STR_PackQuitTask*)buf)->TaskID;
        t_task->QuitTask(conn, taskID);
//        srv->RunTask(boost::bind(&GameTask::QuitTask, t_task, conn, taskID));
        break;
    }
    case FLAG_AskFinishTask:  //玩家请求完成任务
    {
        STR_FinishTask* finishTask = (STR_FinishTask*)srv->malloc();
        memcpy(finishTask, buf + sizeof(STR_PackHead), len);
        t_task->AskFinishTask(conn, finishTask);
//        srv->RunTask(boost::bind(&GameTask::AskFinishTask, t_task, conn, finishTask));
        break;
    }
    case FLAG_TaskExeDlg:
    {
        STR_AskTaskExeDlg* askTaskExeDlg = (STR_AskTaskExeDlg*)srv->malloc();
        memcpy(askTaskExeDlg, buf + sizeof(STR_PackHead), len);
        t_task->AskTaskExeDialog(conn, askTaskExeDlg);
//        srv->RunTask(boost::bind(&GameTask::AskTaskExeDialog, t_task, conn, askTaskExeDlg));
        break;
    }
    case FLAG_TaskExeDlgFinish:
    {
        STR_AskTaskExeDlg* askTaskExeDlg = (STR_AskTaskExeDlg*)srv->malloc();
        memcpy(askTaskExeDlg, buf + sizeof(STR_PackHead), len);
        t_task->TaskExeDialogFinish(conn, askTaskExeDlg);
//        srv->RunTask(boost::bind(&GameTask::TaskExeDialogFinish, t_task, conn, askTaskExeDlg));
        break;
    }
    case FLAG_AskTaskData:    //请求任务数据包
    {               
        AskTaskData* t_ask = (AskTaskData*)(buf + sizeof(STR_PackHead));
        switch(t_ask->Flag)
        {
        case FLAG_TaskStartDlg:
        {
            t_task->StartTaskDlg(conn, t_ask->TaskID);
//            srv->RunTask(boost::bind(&GameTask::StartTaskDlg, t_task, conn, t_ask->TaskID));
            break;
        }
        case FLAG_TaskFinishDlg:
        {
            t_task->FinishTaskDlg(conn, t_ask->TaskID);
//            srv->RunTask(boost::bind(&GameTask::FinishTaskDlg, t_task, conn, t_ask->TaskID));
            break;
        }
        case FLAG_TaskDescription:
        {
            t_task->TaskDescription(conn, t_ask->TaskID);
//            srv->RunTask(boost::bind(&GameTask::TaskDescription, t_task, conn, t_ask->TaskID));
            break;
        }
        case FLAG_TaskAim:
        {
            t_task->TaskAim(conn, t_ask->TaskID);
//            srv->RunTask(boost::bind(&GameTask::TaskAim, t_task, conn, t_ask->TaskID));
            break;
        }
        case FLAG_TaskReward:
        {
            t_task->TaskReward(conn, t_ask->TaskID);
//            srv->RunTask(boost::bind(&GameTask::TaskReward, t_task, conn, t_ask->TaskID));
            break;
        }
        default:
            break;
        }
        break;
    }
    case FLAG_AddFriend:     //添加好友
    {
        STR_PackAddFriend* t_add =(STR_PackAddFriend*)srv->malloc();
        memcpy(t_add, buf, len + sizeof(STR_PackHead));
        t_teamFriend->addFriend(conn, t_add);
//        srv->RunTask(boost::bind(&TeamFriend::addFriend, t_teamFriend, conn, t_add));
        break;

    }
    case FLAG_DeleteFriend:   //删除好友
    {     
        hf_uint32 friendID = ((STR_DeleteFriend*)(buf + sizeof(STR_PackHead)))->RoleID;
        t_teamFriend->deleteFriend(conn, friendID);
//        srv->RunTask(boost::bind(&TeamFriend::deleteFriend, t_teamFriend, conn, friendID));
        break;
    }
    case FLAG_AddFriendReturn:  //添加好友客户端返回
    {
        STR_PackAddFriendReturn* t_AddFriend =(STR_PackAddFriendReturn*)srv->malloc();
        memcpy(t_AddFriend, buf, len + sizeof(STR_PackHead));
        t_teamFriend->ReciveAddFriend(conn, t_AddFriend);
//        srv->RunTask(boost::bind(&TeamFriend::ReciveAddFriend, t_teamFriend, conn, t_AddFriend));
        break;
    }
    case FLAG_UserAttackAim:  //攻击目标
    {
        STR_PackUserAttackAim* t_attack =(STR_PackUserAttackAim*)srv->malloc();
        memcpy(t_attack, buf, sizeof(STR_PackUserAttackAim));
        t_gameAttack->AttackAim(conn, t_attack);
//        srv->RunTask(boost::bind(&GameAttack::AttackAim, t_gameAttack, conn, t_attack));
        break;
    }
    case FLAG_UserAttackPoint:  //攻击点
    {
        STR_PackUserAttackPoint* t_attack =(STR_PackUserAttackPoint*)srv->malloc();
        memcpy(t_attack, buf+sizeof(STR_PackHead), sizeof(STR_PackUserAttackPoint));
        t_gameAttack->AttackPoint(conn, t_attack);
//        srv->RunTask(boost::bind(&GameAttack::AttackPoint, t_gameAttack, conn, t_attack));
        break;
    }
    case FLAG_PickGoods:      //捡物品
    {
        STR_PickGoods* t_pick =(STR_PickGoods*)srv->malloc();
        memcpy(t_pick, buf+sizeof(STR_PackHead), len);
        t_operationGoods->PickUpGoods(conn, t_pick);
//        srv->RunTask(boost::bind(&OperationGoods::PickUpGoods, t_operationGoods, conn, t_pick));
        break;
    }
    case FLAG_RemoveGoods:  //丢弃物品
    {
        STR_RemoveBagGoods* t_remove = (STR_RemoveBagGoods*)srv->malloc();
        memcpy(t_remove, buf+sizeof(STR_PackHead), len);
        t_operationGoods->RemoveBagGoods(conn, t_remove);
//        srv->RunTask(boost::bind(&OperationGoods::RemoveBagGoods, t_operationGoods, conn, t_remove));
        break;
    }
    case FLAG_MoveGoods:  //移动或分割物品
    {
        STR_MoveBagGoods* t_move = (STR_MoveBagGoods*)srv->malloc();
        memcpy(t_move, buf+sizeof(STR_PackHead), len);
        t_operationGoods->MoveBagGoods(conn, t_move);
//        srv->RunTask(boost::bind(&OperationGoods::MoveBagGoods, t_operationGoods, conn, t_move));
        break;
    }
    case FLAG_BuyGoods:    //购买物品
    {
        STR_BuyGoods* t_buy = (STR_BuyGoods*)srv->malloc();
        memcpy(t_buy, buf + sizeof(STR_PackHead), len);
        t_operationGoods->BuyGoods(conn, t_buy);
//        srv->RunTask(boost::bind(&OperationGoods::BuyGoods, t_operationGoods, conn, t_buy));
        break;
    }
    case FLAG_SellGoods:   //出售物品
    {
        STR_SellGoods* t_sell = (STR_SellGoods*)srv->malloc();
        memcpy(t_sell, buf + sizeof(STR_PackHead), len);
        t_operationGoods->SellGoods(conn, t_sell);
//        srv->RunTask(boost::bind(&OperationGoods::SellGoods, t_operationGoods, conn, t_sell));
        break;
    }
    case FLAG_ArrangeBagGoods:   //整理背包物品
    {
        t_operationGoods->ArrangeBagGoods(conn);
//        srv->RunTask(boost::bind(&OperationGoods::ArrangeBagGoods, t_operationGoods, conn));
        break;
    }

    case FLAG_WearBodyEqu: //穿装备
    {
        STR_WearEqu* t_wearEqu = (STR_WearEqu*)(buf + sizeof(STR_PackHead));
        t_operationGoods->WearBodyEqu(conn, t_wearEqu->equid, t_wearEqu->pos);
//        srv->RunTask(boost::bind(&OperationGoods::WearBodyEqu, t_operationGoods, conn, t_wearEqu->equid, t_wearEqu->pos));
        break;
    }

    case FLAG_TakeOffEqu:  //脱装备
    {
        hf_uint32 equid = *(hf_uint32*)(buf + sizeof(STR_PackHead));
        t_operationGoods->TakeOffBodyEqu(conn, equid);
//        srv->RunTask(boost::bind(&OperationGoods::TakeOffBodyEqu, t_operationGoods, conn, equid));
        break;
    }

    case FLAG_UseBagGoods:
    {
        STR_UseBagGoods* bagGoods = (STR_UseBagGoods*)(buf + sizeof(STR_PackHead));
        t_operationGoods->UseBagGoods(conn, bagGoods->goodsid, bagGoods->pos);
//        srv->RunTask(boost::bind(&OperationGoods::UseBagGoods, t_operationGoods, conn, bagGoods->goodsid, bagGoods->pos));
        break;
    }

    case FLAG_Chat:   //聊天 test
    {
        STR_PackRecvChat* t_chat = (STR_PackRecvChat*)srv->malloc();
        memcpy(t_chat, buf, len + sizeof(STR_PackHead));
        t_gameChat->Chat(conn, t_chat);
//        srv->RunTask(boost::bind(&GameChat::Chat, t_gameChat, conn, t_chat));
        break;
    }
    case 1000:
    {
        conn->Write_all(buf, sizeof(STR_PackHead) + len);
        break;
    }
//    case FLAG_TradeOper:
//    {
//        STR_PackRequestOper* t_oper = (STR_PackRequestOper*)srv->malloc();
//        memcpy(t_oper, buf, len + sizeof(STR_PackHead));

//        break;
//    }
//    case FLAG_TradeOperResult:
//    {

//        break;
//    }
//    case FLAG_TradeOperGoods:
//    {

//        break;
//    }
//    case FLAG_TradeOperMoney:
//    {

//        break;
//    }
    case FLAG_OperRequest:
    {
        operationRequest* t_operReq = (operationRequest*)srv->malloc();
        memcpy(t_operReq,buf,sizeof(STR_PackHead)+len);
        switch(t_operReq->operType)
        {
            case FLAG_WantToChange:
            {
                t_gameInterchange->operRequest(conn, t_operReq);
//                srv->RunTask(boost::bind(&GameInterchange::operRequest, t_gameInterchange, conn,t_operReq));
                break;
            }
            default:
                break;
        }
        break;
    }

    case FLAG_OperResult:
    {
        operationRequestResult* t_operReq = (operationRequestResult*)srv->malloc();
        memcpy(t_operReq,buf, len + sizeof(STR_PackHead));
        switch(t_operReq->operType)
        {
            case FLAG_WantToChange:
            {
                t_gameInterchange->operResponse(conn, t_operReq);
//                srv->RunTask(boost::bind(&GameInterchange::operResponse, t_gameInterchange, conn,t_operReq));
                break;
            }
            default:
                break;
        }
        break;
    }
    case FLAG_InterchangeMoneyCount:
    {
        interchangeMoney* t_Money = (interchangeMoney*)srv->malloc();
        memcpy(t_Money,buf,sizeof(STR_PackHead) + len);
        t_gameInterchange->operMoneyChanges(conn, t_Money);
//        srv->RunTask(boost::bind(&GameInterchange::operMoneyChanges, t_gameInterchange, conn, t_Money));
        break;
    }
    case FLAG_InterchangeGoods:
    {
        interchangeOperGoods* t_OperPro = (interchangeOperGoods*)srv->malloc();
        memcpy(t_OperPro,buf,sizeof(STR_PackHead) + len);
        t_gameInterchange->operChanges(conn, t_OperPro);
//        srv->RunTask(boost::bind(&GameInterchange::operChanges, t_gameInterchange, conn, t_OperPro));
        break;
    }
    case  FLAG_InterchangeOperPro:
    {
        interchangeOperPro* t_OperPro = (interchangeOperPro*)srv->malloc();
        memcpy(t_OperPro,buf,sizeof(STR_PackHead) + len);
        switch(t_OperPro->operType)
        {
            case Interchange_Change:  //交易
            {
                t_gameInterchange->operProCheckChange(conn, t_OperPro);
//                srv->RunTask(boost::bind(&GameInterchange::operProCheckChange, t_gameInterchange, conn, t_OperPro));
                break;
            }
            case Interchange_Lock:  //锁定
            {
                t_gameInterchange->operProLock(conn, t_OperPro);
//                srv->RunTask(boost::bind(&GameInterchange::operProLock, t_gameInterchange, conn, t_OperPro));
                break;
            }
            case Interchange_Unlock:  //取消锁定
            {
                t_gameInterchange->operProUnlock(conn, t_OperPro);
//                srv->RunTask(boost::bind(&GameInterchange::operProUnlock, t_gameInterchange, conn, t_OperPro));
                break;
            }
            case Interchange_CancelChange:  //取消交易
            {
                t_gameInterchange->operProCancelChange(conn, t_OperPro);
//                srv->RunTask(boost::bind(&GameInterchange::operProCancelChange, t_gameInterchange, conn, t_OperPro));
                break;
            }
            default:
                break;
        }
        break;
    }

    default:
    {
        Logger::GetLogger()->Info("Unkown pack");
        Logger::GetLogger()->Error("flag:%d, len:%d\n", head->Flag, head->Len);
        break;
    }
    }
}

void CommandParseLogin(TCPConnection::Pointer conn, void* reg)
{
    hf_char *buf = (hf_char*)reg;
    STR_PackHead *head = (STR_PackHead*)buf;
    hf_uint16 flag = head->Flag;
    hf_uint16 len = head->Len;

    Server *srv = Server::GetInstance();
    PlayerLogin* t_playerLogin = srv->GetPlayerLogin();
    switch( flag )
    {
    //注册玩家帐号
    case FLAG_PlayerRegisterUserId:
    {
        STR_PlayerRegisterUserId* reg = (STR_PlayerRegisterUserId*)srv->malloc();
        memcpy(reg, buf+sizeof(STR_PackHead), len);
        t_playerLogin->RegisterUserID(conn, reg);
//        srv->RunTask(boost::bind(&PlayerLogin::RegisterUserID, t_playerLogin, conn, reg));
        break;
    }
        //注册玩家角色
    case FLAG_PlayerRegisterRole:
    {
        STR_PlayerRegisterRole* reg =(STR_PlayerRegisterRole*)srv->malloc();
        memcpy(reg, buf+sizeof(STR_PackHead), len);
        t_playerLogin->RegisterRole(conn, reg);
//        srv->RunTask(boost::bind(&PlayerLogin::RegisterRole, t_playerLogin, conn, reg));
        break;
    }
        //删除角色
    case FLAG_UserDeleteRole:
    {
        STR_PlayerRole* reg = (STR_PlayerRole*)(buf + sizeof(STR_PackHead));
        t_playerLogin->DeleteRole(conn, reg->Role);
//        srv->RunTask(boost::bind(&PlayerLogin::DeleteRole, t_playerLogin, conn, reg->Role));
        break;
    }
        //登陆帐号
    case FLAG_PlayerLoginUserId:
    {
        STR_PlayerLoginUserId* reg = (STR_PlayerLoginUserId*)srv->malloc();
        memcpy(reg, buf+sizeof(STR_PackHead), len);
        t_playerLogin->LoginUserId(conn, reg);
//        srv->RunTask(boost::bind(&PlayerLogin::LoginUserId, t_playerLogin, conn, reg));
        break;
    }
        //登陆角色
    case FLAG_PlayerLoginRole:
    {
        STR_PlayerRole* reg = (STR_PlayerRole*)(buf + sizeof(STR_PackHead));
        t_playerLogin->LoginRole(conn, reg->Role);
//        srv->RunTask(boost::bind(&PlayerLogin::LoginRole, t_playerLogin, conn, reg->Role));
        break;
    }
    default:
    {
        Logger::GetLogger()->Info("Unkown pack");
        Logger::GetLogger()->Error("flag:%d, len:%d\n", head->Flag, head->Len);
        break;
    }
    }
}
