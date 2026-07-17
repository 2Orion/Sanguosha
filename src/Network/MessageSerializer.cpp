#include "MessageSerializer.h"

// ==================== 枚举读写辅助（显式 qint32，避免枚举底层类型差异） ====================

namespace {

template <typename Enum>
void writeEnum(QDataStream& out, Enum v)
{
    out << qint32(v);
}

template <typename Enum>
void readEnum(QDataStream& in, Enum& v)
{
    qint32 raw = 0;
    in >> raw;
    v = static_cast<Enum>(raw);
}

} // namespace

// ==================== Common 值类型 ====================

QDataStream& operator<<(QDataStream& out, const CardData& d)
{
    out << qint32(d.cardId);
    writeEnum(out, d.cardType);
    writeEnum(out, d.suit);
    out << qint32(d.number) << d.cardName << d.description;
    writeEnum(out, d.color);
    out << d.isBasic << d.isStrategy << d.suitSymbol << d.numberString
        << d.isSelected << d.isPlayable << d.isHighlighted << qint32(d.ownerId)
        << d.isEquipment << qint32(d.equipSlot) << qint32(d.attackRange);
    return out;
}

QDataStream& operator>>(QDataStream& in, CardData& d)
{
    qint32 cardId = 0, number = 0, ownerId = 0, attackRange = 0;
    in >> cardId;
    readEnum(in, d.cardType);
    readEnum(in, d.suit);
    in >> number >> d.cardName >> d.description;
    readEnum(in, d.color);
    qint32 equipSlot = 0;
    in >> d.isBasic >> d.isStrategy >> d.suitSymbol >> d.numberString
       >> d.isSelected >> d.isPlayable >> d.isHighlighted >> ownerId
       >> d.isEquipment >> equipSlot >> attackRange;
    d.equipSlot = equipSlot;
    d.cardId = cardId;
    d.number = number;
    d.ownerId = ownerId;
    d.attackRange = attackRange;
    return in;
}

QDataStream& operator<<(QDataStream& out, const PlayerData& d)
{
    out << qint32(d.playerId) << d.displayName << d.characterName
        << d.skillName << d.skillDescription
        << qint32(d.hp) << qint32(d.maxHp) << d.isAlive << d.isDying
        << qint32(d.handCardCount) << qint32(d.handCardLimit) << d.isCurrentPlayer
        << d.canUseActiveSkill << qint32(d.attackRange) << d.equipCards
        << d.judgmentCards;
    return out;
}

QDataStream& operator>>(QDataStream& in, PlayerData& d)
{
    qint32 playerId = 0, hp = 0, maxHp = 0, handCardCount = 0, handCardLimit = 0, attackRange = 0;
    in >> playerId >> d.displayName >> d.characterName
       >> d.skillName >> d.skillDescription
       >> hp >> maxHp >> d.isAlive >> d.isDying
       >> handCardCount >> handCardLimit >> d.isCurrentPlayer
       >> d.canUseActiveSkill >> attackRange >> d.equipCards
       >> d.judgmentCards;
    d.attackRange = attackRange;
    d.playerId = playerId;
    d.hp = hp;
    d.maxHp = maxHp;
    d.handCardCount = handCardCount;
    d.handCardLimit = handCardLimit;
    return in;
}

QDataStream& operator<<(QDataStream& out, const PendingActionData& d)
{
    out << qint32(d.sourceId) << qint32(d.targetId) << qint32(d.sourceCardId);
    writeEnum(out, d.requiredCardType);
    out << d.description << d.canSkip << d.isSkillChoice << d.remainingTargetIds;
    return out;
}

QDataStream& operator>>(QDataStream& in, PendingActionData& d)
{
    qint32 sourceId = 0, targetId = 0, sourceCardId = 0;
    in >> sourceId >> targetId >> sourceCardId;
    readEnum(in, d.requiredCardType);
    in >> d.description >> d.canSkip >> d.isSkillChoice >> d.remainingTargetIds;
    d.sourceId = sourceId;
    d.targetId = targetId;
    d.sourceCardId = sourceCardId;
    return in;
}

// ==================== 消息结构体 ====================

namespace Protocol {

QDataStream& operator<<(QDataStream& out, const HandshakeMsg& m)
{
    out << m.protocolVersion;
    return out;
}

QDataStream& operator>>(QDataStream& in, HandshakeMsg& m)
{
    in >> m.protocolVersion;
    return in;
}

QDataStream& operator<<(QDataStream& out, const HandshakeAckMsg& m)
{
    out << m.protocolVersion << m.playerId << m.reason;
    return out;
}

QDataStream& operator>>(QDataStream& in, HandshakeAckMsg& m)
{
    in >> m.protocolVersion >> m.playerId >> m.reason;
    return in;
}

QDataStream& operator<<(QDataStream& out, const SelectCharacterMsg& m)
{
    out << m.characterId;
    return out;
}

QDataStream& operator>>(QDataStream& in, SelectCharacterMsg& m)
{
    in >> m.characterId;
    return in;
}

QDataStream& operator<<(QDataStream& out, const GameStartedMsg& m)
{
    out << m.characterId0 << m.characterId1;
    return out;
}

QDataStream& operator>>(QDataStream& in, GameStartedMsg& m)
{
    in >> m.characterId0 >> m.characterId1;
    return in;
}

QDataStream& operator<<(QDataStream& out, const PlayCardMsg& m)
{
    out << m.cardId << m.playerId;
    return out;
}

QDataStream& operator>>(QDataStream& in, PlayCardMsg& m)
{
    in >> m.cardId >> m.playerId;
    return in;
}

QDataStream& operator<<(QDataStream& out, const RespondCardMsg& m)
{
    out << m.cardId << m.responderId;
    return out;
}

QDataStream& operator>>(QDataStream& in, RespondCardMsg& m)
{
    in >> m.cardId >> m.responderId;
    return in;
}

QDataStream& operator<<(QDataStream& out, const TargetSelectedMsg& m)
{
    out << m.playerIndex;
    return out;
}

QDataStream& operator>>(QDataStream& in, TargetSelectedMsg& m)
{
    in >> m.playerIndex;
    return in;
}

QDataStream& operator<<(QDataStream& out, const DiscardCardMsg& m)
{
    out << m.cardId << m.playerId;
    return out;
}

QDataStream& operator>>(QDataStream& in, DiscardCardMsg& m)
{
    in >> m.cardId >> m.playerId;
    return in;
}

QDataStream& operator<<(QDataStream& out, const SkillRequestedMsg& m)
{
    out << m.cardIds << m.playerId;
    return out;
}

QDataStream& operator>>(QDataStream& in, SkillRequestedMsg& m)
{
    in >> m.cardIds >> m.playerId;
    return in;
}

QDataStream& operator<<(QDataStream& out, const PhaseChangedMsg& m)
{
    writeEnum(out, m.phase);
    return out;
}

QDataStream& operator>>(QDataStream& in, PhaseChangedMsg& m)
{
    readEnum(in, m.phase);
    return in;
}

QDataStream& operator<<(QDataStream& out, const PlayerDataMsg& m)
{
    out << m.playerId << m.data;
    return out;
}

QDataStream& operator>>(QDataStream& in, PlayerDataMsg& m)
{
    in >> m.playerId >> m.data;
    return in;
}

QDataStream& operator<<(QDataStream& out, const HandCardsMsg& m)
{
    out << m.playerId << m.cards;
    return out;
}

QDataStream& operator>>(QDataStream& in, HandCardsMsg& m)
{
    in >> m.playerId >> m.cards;
    return in;
}

QDataStream& operator<<(QDataStream& out, const PendingActionMsg& m)
{
    out << m.info;
    return out;
}

QDataStream& operator>>(QDataStream& in, PendingActionMsg& m)
{
    in >> m.info;
    return in;
}

QDataStream& operator<<(QDataStream& out, const GameOverMsg& m)
{
    out << m.winnerId;
    return out;
}

QDataStream& operator>>(QDataStream& in, GameOverMsg& m)
{
    in >> m.winnerId;
    return in;
}

QDataStream& operator<<(QDataStream& out, const LogMessageMsg& m)
{
    out << m.message;
    return out;
}

QDataStream& operator>>(QDataStream& in, LogMessageMsg& m)
{
    in >> m.message;
    return in;
}

QDataStream& operator<<(QDataStream& out, const TargetSelectionStartedMsg& m)
{
    out << m.targetIds;
    return out;
}

QDataStream& operator>>(QDataStream& in, TargetSelectionStartedMsg& m)
{
    in >> m.targetIds;
    return in;
}

QDataStream& operator<<(QDataStream& out, const JudgmentPerformedMsg& m)
{
    out << m.judgeCard << m.resultText << m.effective;
    return out;
}

QDataStream& operator>>(QDataStream& in, JudgmentPerformedMsg& m)
{
    in >> m.judgeCard >> m.resultText >> m.effective;
    return in;
}

} // namespace Protocol

// ==================== 帧封装/解码 ====================

namespace MessageSerializer {

QByteArray wrapFrame(Protocol::MessageType type, const QByteArray& payload)
{
    QByteArray frame;
    QDataStream out(&frame, QDataStream::WriteOnly);
    out.setVersion(kStreamVersion);
    out << quint32(1 + payload.size()) << quint8(type);
    frame.append(payload);
    return frame;
}

QByteArray encode(Protocol::MessageType type)
{
    return wrapFrame(type);
}

bool decodeFrames(QByteArray& buffer, QVector<Frame>& outFrames)
{
    for (;;) {
        if (buffer.size() < 4)
            return true;  // 长度前缀不完整，等待更多数据

        quint32 frameLen = 0;
        {
            QDataStream in(buffer);
            in.setVersion(kStreamVersion);
            in >> frameLen;
        }
        if (frameLen < 1 || frameLen > Protocol::kMaxFrameSize)
            return false;  // 流损坏，调用方应断开

        if (quint32(buffer.size()) < 4 + frameLen)
            return true;  // 帧体不完整，等待更多数据

        Frame frame;
        frame.type = static_cast<Protocol::MessageType>(quint8(buffer.at(4)));
        frame.payload = buffer.mid(5, int(frameLen) - 1);
        outFrames.append(frame);
        buffer.remove(0, 4 + int(frameLen));
    }
}

} // namespace MessageSerializer
