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
    virtual bool triggerCondition(GameEvent event, const GameState* state, const Player* self) const;
    virtual void triggerSkill(GameState* state, Player* self);
    virtual CardType skillTransformCard(const Card* card) const;

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

#endif // CHARACTER_H
