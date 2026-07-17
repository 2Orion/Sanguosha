#ifndef CHARACTER_H
#define CHARACTER_H

#include <string>
#include <QObject>
#include "CommonTypes.h"

class GameState;
class Player;
class Card;

class Character : public QObject {
    Q_OBJECT
public:
    Character(const std::string& name, int maxHp,
              const std::string& skillName, const std::string& skillDesc,
              QObject* parent = nullptr);
    ~Character() override = default;

    std::string characterName() const;
    int maxHp() const;
    std::string skillName() const;
    std::string skillDescription() const;

    virtual bool hasSkill() const;
    /// 是否有可通过技能按钮主动发动的技能（如制衡、武圣、流离）
    virtual bool hasActiveSkill() const;
    virtual bool triggerCondition(GameEvent event, const GameState* state, const Player* self) const;
    virtual void triggerSkill(GameState* state, Player* self);
    virtual CardType skillTransformCard(const Card* card) const;

    /// 摸牌阶段额外摸牌数（周瑜英姿）
    virtual int onDrawPhaseBonus() const;
    /// 是否需要两张闪才能抵消杀（吕布无双）
    virtual bool requireExtraDodge() const;
    /// 是否可以转移杀的目标（大乔流离）
    virtual bool canRedirectKill() const;
    /// 转移杀的目标
    virtual Player* getRedirectTarget(GameState* state, Player* self, Player* attacker) const;
    /// 是否可以弃牌摸牌（孙权制衡）
    virtual bool canDiscardAndDraw() const;

signals:
    void skillTriggered(const QString& skillName);

protected:
    std::string m_name;
    int m_maxHp;
    std::string m_skillName;
    std::string m_skillDescription;
};

// 具体武将
class CaoCao : public Character {
    Q_OBJECT
public:
    CaoCao(QObject* parent = nullptr);
    bool hasSkill() const override;
    bool triggerCondition(GameEvent event, const GameState* state, const Player* self) const override;
    void triggerSkill(GameState* state, Player* self) override;
};

class GuanYu : public Character {
    Q_OBJECT
public:
    GuanYu(QObject* parent = nullptr);
    bool hasSkill() const override;
    bool hasActiveSkill() const override;
    CardType skillTransformCard(const Card* card) const override;
};

class ZhangFei : public Character {
    Q_OBJECT
public:
    ZhangFei(QObject* parent = nullptr);
    bool hasSkill() const override;
    bool triggerCondition(GameEvent event, const GameState* state, const Player* self) const override;
    void triggerSkill(GameState* state, Player* self) override;
};

class ZhaoYun : public Character {
    Q_OBJECT
public:
    ZhaoYun(QObject* parent = nullptr);
    bool hasSkill() const override;
    CardType skillTransformCard(const Card* card) const override;
};

// ==================== 新武将 ====================

/// 孙权 - 制衡：出牌阶段可弃任意张牌并摸等量的牌
class SunQuan : public Character {
    Q_OBJECT
public:
    SunQuan(QObject* parent = nullptr);
    bool hasSkill() const override;
    bool hasActiveSkill() const override;
    bool canDiscardAndDraw() const override;
};

/// 周瑜 - 英姿：摸牌阶段多摸一张
class ZhouYu : public Character {
    Q_OBJECT
public:
    ZhouYu(QObject* parent = nullptr);
    bool hasSkill() const override;
    int onDrawPhaseBonus() const override;
};

/// 吕布 - 无双：杀需两张闪才能抵消
class LvBu : public Character {
    Q_OBJECT
public:
    LvBu(QObject* parent = nullptr);
    bool hasSkill() const override;
    bool requireExtraDodge() const override;
};

/// 大乔 - 流离：杀可转移给其他玩家
class DaQiao : public Character {
    Q_OBJECT
public:
    DaQiao(QObject* parent = nullptr);
    bool hasSkill() const override;
    bool hasActiveSkill() const override;
    bool canRedirectKill() const override;
    Player* getRedirectTarget(GameState* state, Player* self, Player* attacker) const override;
};

/// 司马懿 - 反馈：受到伤害后可获得伤害来源一张牌
class SiMaYi : public Character {
    Q_OBJECT
public:
    SiMaYi(QObject* parent = nullptr);
    bool hasSkill() const override;
    bool triggerCondition(GameEvent event, const GameState* state, const Player* self) const override;
    void triggerSkill(GameState* state, Player* self) override;
};

#endif // CHARACTER_H
