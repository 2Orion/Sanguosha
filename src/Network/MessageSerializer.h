#ifndef NETWORK_MESSAGESERIALIZER_H
#define NETWORK_MESSAGESERIALIZER_H

#include <QByteArray>
#include <QDataStream>
#include "Protocol.h"

/// Common 值类型的 QDataStream 序列化运算符
/// （协议版本 v1 字段集；字段增删时递增 Protocol::kProtocolVersion 并同步这里）
QDataStream& operator<<(QDataStream& out, const CardData& d);
QDataStream& operator>>(QDataStream& in, CardData& d);
QDataStream& operator<<(QDataStream& out, const PlayerData& d);
QDataStream& operator>>(QDataStream& in, PlayerData& d);
QDataStream& operator<<(QDataStream& out, const PendingActionData& d);
QDataStream& operator>>(QDataStream& in, PendingActionData& d);

/// 消息结构体的序列化运算符
namespace Protocol {
QDataStream& operator<<(QDataStream& out, const HandshakeMsg& m);
QDataStream& operator>>(QDataStream& in, HandshakeMsg& m);
QDataStream& operator<<(QDataStream& out, const HandshakeAckMsg& m);
QDataStream& operator>>(QDataStream& in, HandshakeAckMsg& m);
QDataStream& operator<<(QDataStream& out, const SelectCharacterMsg& m);
QDataStream& operator>>(QDataStream& in, SelectCharacterMsg& m);
QDataStream& operator<<(QDataStream& out, const GameStartedMsg& m);
QDataStream& operator>>(QDataStream& in, GameStartedMsg& m);
QDataStream& operator<<(QDataStream& out, const PlayCardMsg& m);
QDataStream& operator>>(QDataStream& in, PlayCardMsg& m);
QDataStream& operator<<(QDataStream& out, const RespondCardMsg& m);
QDataStream& operator>>(QDataStream& in, RespondCardMsg& m);
QDataStream& operator<<(QDataStream& out, const TargetSelectedMsg& m);
QDataStream& operator>>(QDataStream& in, TargetSelectedMsg& m);
QDataStream& operator<<(QDataStream& out, const DiscardCardMsg& m);
QDataStream& operator>>(QDataStream& in, DiscardCardMsg& m);
QDataStream& operator<<(QDataStream& out, const PhaseChangedMsg& m);
QDataStream& operator>>(QDataStream& in, PhaseChangedMsg& m);
QDataStream& operator<<(QDataStream& out, const PlayerDataMsg& m);
QDataStream& operator>>(QDataStream& in, PlayerDataMsg& m);
QDataStream& operator<<(QDataStream& out, const HandCardsMsg& m);
QDataStream& operator>>(QDataStream& in, HandCardsMsg& m);
QDataStream& operator<<(QDataStream& out, const PendingActionMsg& m);
QDataStream& operator>>(QDataStream& in, PendingActionMsg& m);
QDataStream& operator<<(QDataStream& out, const GameOverMsg& m);
QDataStream& operator>>(QDataStream& in, GameOverMsg& m);
QDataStream& operator<<(QDataStream& out, const LogMessageMsg& m);
QDataStream& operator>>(QDataStream& in, LogMessageMsg& m);
QDataStream& operator<<(QDataStream& out, const TargetSelectionStartedMsg& m);
QDataStream& operator>>(QDataStream& in, TargetSelectionStartedMsg& m);
} // namespace Protocol

/// 帧封装/解码 — 帧格式：
///   [quint32 长度（type + payload 的字节数，big-endian，QDataStream 默认）]
///   [quint8  MessageType]
///   [payload（消息结构体按上面的运算符序列化）]
namespace MessageSerializer {

/// QDataStream 版本固定，保证两端一致（Qt5/Qt6 混连也不会因默认版本不同出错）
constexpr QDataStream::Version kStreamVersion = QDataStream::Qt_5_15;

/// 打包一条带 payload 的消息为完整帧
template <typename Msg>
QByteArray encode(Protocol::MessageType type, const Msg& msg)
{
    QByteArray payload;
    {
        QDataStream out(&payload, QDataStream::WriteOnly);
        out.setVersion(kStreamVersion);
        out << msg;
    }
    QByteArray frame;
    QDataStream out(&frame, QDataStream::WriteOnly);
    out.setVersion(kStreamVersion);
    out << quint32(1 + payload.size()) << quint8(type);
    frame.append(payload);
    return frame;
}

/// 打包一条无 payload 的消息为完整帧
QByteArray encode(Protocol::MessageType type);

/// 从 payload 字节反序列化消息结构体
template <typename Msg>
Msg decodePayload(const QByteArray& payload)
{
    Msg msg;
    QDataStream in(payload);
    in.setVersion(kStreamVersion);
    in >> msg;
    return msg;
}

/// 解出的一帧
struct Frame {
    Protocol::MessageType type;
    QByteArray payload;
};

/// 从接收缓冲区中尽可能多地解出完整帧（处理半包/粘包）。
/// 解出的字节从 buffer 前部移除；不完整的尾部留在 buffer 中等待后续数据。
/// 返回 false 表示流损坏（长度前缀超过 kMaxFrameSize），调用方应断开连接。
bool decodeFrames(QByteArray& buffer, QVector<Frame>& outFrames);

} // namespace MessageSerializer

#endif // NETWORK_MESSAGESERIALIZER_H
