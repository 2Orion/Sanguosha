#ifndef NETWORK_PROTOCOL_H
#define NETWORK_PROTOCOL_H

#include <QString>
#include <QVector>
#include <QtGlobal>
#include "CommonTypes.h"
#include "CardData.h"
#include "PlayerData.h"
#include "PendingActionData.h"

/// 网络协议定义 — 消息类型 + 消息结构体
/// 字段与 GameViewModel/ActionViewModel 的 public slots / signals 参数一一对齐
/// （权威映射见 connection.md）。跨网络只传 Common 值类型，禁止 Model 指针。
namespace Protocol {

/// 协议版本号。Common 值类型（CardData/PlayerData/PendingActionData）字段
/// 增删时必须递增（plan2.0.md §7.2 契约）。
/// v1：CardData 含装备字段 isEquipment/equipSlot/attackRange，
///     PlayerData 含 weaponCard/armorCard 按槽装备区。
constexpr quint16 kProtocolVersion = 1;

/// 默认监听端口
constexpr quint16 kDefaultPort = 9527;

/// 单帧最大长度（防御损坏/恶意的长度前缀导致的内存暴涨）
constexpr quint32 kMaxFrameSize = 1024 * 1024;

/// 消息类型（帧头第 5 字节；显式赋值保证跨版本稳定）
enum class MessageType : quint8 {
    // ---- 握手 / 选将 ----
    Handshake         = 0,   // C→S：请求连接，带协议版本号
    HandshakeAck      = 1,   // S→C：连接确认，含分配的 playerId（-1 = 拒绝）
    SelectCharacter   = 2,   // C→S：选择武将
    GameStarted       = 3,   // S→C：游戏开始，含双方武将 id

    // ---- View 命令的网络化（C→S，对应 VM public slots） ----
    PlayCardRequested    = 10,  // ~ ActionViewModel::onPlayCardRequested(cardId, playerId)
    RespondCardRequested = 11,  // ~ ActionViewModel::onRespondCardRequested(cardId, responderId)
    TargetSelected       = 12,  // ~ ActionViewModel::onTargetSelected(playerIndex)
    DiscardCardRequested = 13,  // ~ GameViewModel::onDiscardCardRequested(cardId, playerId)
    EndPlayRequested     = 14,  // ~ GameViewModel::onEndPlayRequested()
    AdvanceRequested     = 15,  // ~ GameViewModel::onAdvanceRequested()
    SkipRequested        = 16,  // ~ GameViewModel::onSkipRequested()

    // ---- VM 信号的网络化（S→C，广播；HandCardsUpdated 按接收方脱敏） ----
    PhaseChanged            = 30,  // ~ GameViewModel::phaseChanged(PhaseType)
    PlayerDataUpdated       = 31,  // ~ GameViewModel::playerDataUpdated(int, PlayerData)
    HandCardsUpdated        = 32,  // ~ GameViewModel::handCardsUpdated(int, CardList)
    PendingActionCreated    = 33,  // ~ GameViewModel::pendingActionCreated(PendingActionData)
    PendingActionCleared    = 34,  // ~ GameViewModel::pendingActionCleared()
    GameOver                = 35,  // ~ GameViewModel::gameOver(int)
    LogMessage              = 36,  // ~ GameViewModel::logMessage(QString)
    TargetSelectionStarted  = 37,  // ~ ActionViewModel::targetSelectionStarted(QVector<int>)
    TargetSelectionFinished = 38,  // ~ ActionViewModel::targetSelectionFinished()

    // ---- 保活 ----
    Ping = 50,
    Pong = 51,
};

// ==================== 消息结构体（payload） ====================
// 无 payload 的消息（EndPlayRequested/AdvanceRequested/SkipRequested/
// PendingActionCleared/TargetSelectionFinished/Ping/Pong）不需要结构体。

struct HandshakeMsg {                 // C→S
    quint16 protocolVersion = kProtocolVersion;
};

struct HandshakeAckMsg {              // S→C
    quint16 protocolVersion = kProtocolVersion;
    qint32 playerId = -1;             // 分配的 playerId；-1 = 拒绝（超员/版本不符）
    QString reason;                   // 拒绝原因（接受时为空）
};

struct SelectCharacterMsg {           // C→S
    qint32 characterId = -1;
};

struct GameStartedMsg {               // S→C
    qint32 characterId0 = -1;         // playerId 0 的武将
    qint32 characterId1 = -1;         // playerId 1 的武将
};

struct PlayCardMsg {                  // C→S ~ onPlayCardRequested(cardId, playerId)
    qint32 cardId = -1;
    qint32 playerId = -1;
};

struct RespondCardMsg {               // C→S ~ onRespondCardRequested(cardId, responderId)
    qint32 cardId = -1;
    qint32 responderId = -1;
};

struct TargetSelectedMsg {            // C→S ~ onTargetSelected(playerIndex)
    qint32 playerIndex = -1;
};

struct DiscardCardMsg {               // C→S ~ onDiscardCardRequested(cardId, playerId)
    qint32 cardId = -1;
    qint32 playerId = -1;
};

struct PhaseChangedMsg {              // S→C ~ phaseChanged(PhaseType)
    PhaseType phase = PhaseType::Prepare;
};

struct PlayerDataMsg {                // S→C ~ playerDataUpdated(playerId, PlayerData)
    qint32 playerId = -1;
    PlayerData data;
};

struct HandCardsMsg {                 // S→C ~ handCardsUpdated(playerId, CardList)
    qint32 playerId = -1;             // 手牌所有者；发给对手时 cards 已脱敏
    CardList cards;
};

struct PendingActionMsg {             // S→C ~ pendingActionCreated(PendingActionData)
    PendingActionData info;
};

struct GameOverMsg {                  // S→C ~ gameOver(winnerId)
    qint32 winnerId = -1;
};

struct LogMessageMsg {                // S→C ~ logMessage(QString)
    QString message;
};

struct TargetSelectionStartedMsg {    // S→C ~ targetSelectionStarted(QVector<int>)
    QVector<int> targetIds;
};

} // namespace Protocol

#endif // NETWORK_PROTOCOL_H
