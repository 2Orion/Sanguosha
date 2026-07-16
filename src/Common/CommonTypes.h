#ifndef CORE_COMMONTYPES_H
#define CORE_COMMONTYPES_H

// ==================== 卡牌枚举 ====================

/// 卡牌花色
enum class CardSuit {
    Spade,   // 黑桃 ♠
    Heart,   // 红桃 ♥
    Club,    // 梅花 ♣
    Diamond  // 方块 ♦
};

/// 卡牌颜色（根据花色派生）
enum class CardColor {
    Red,   // ♥♦
    Black  // ♠♣
};

/// 卡牌大类
enum class CardCategory {
    Basic,     // 基本牌
    Strategy,  // 锦囊牌
    Equipment  // 装备牌
};

/// 装备槽位
enum class EquipSlot {
    Weapon,    // 武器
    Armor,     // 防具
    Mount      // 马（预留）
    // Treasure  // 宝物（预留）
};

/// 卡牌类型
enum class CardType {
    // 基本牌
    Kill,       // 杀
    Dodge,      // 闪
    Peach,      // 桃
    Wine,       // 酒

    // 锦囊牌
    Dismantle,          // 过河拆桥
    Steal,              // 顺手牵羊
    Bountiful,          // 无中生有
    BarbarianInvasion,  // 南蛮入侵
    Volley,             // 万箭齐发
    PeachGarden,        // 桃园结义
    Duel,               // 决斗
    Lightning,          // 闪电
    Nullify,            // 无懈可击
    Borrow,             // 借刀杀人
    Harvest,            // 五谷丰登
    Happy,              // 乐不思蜀
    Famine,             // 兵粮寸断

    // 装备牌 — 武器
    Crossbow,        // 诸葛连弩（♠A）
    QinglongBlade,   // 青龙偃月刀（♠5）
    ZhangbaSnake,    // 丈八蛇矛（♠Q）
    KylinBow,        // 麒麟弓（♥5）
    QinggangSword,   // 青釭剑（♠6）
    IceSword,        // 寒冰剑（♠2）
    DualSword,       // 雌雄双股剑（♠2）

    // 装备牌 — 防具
    EightDiagrams,   // 八卦阵（♠2）
    BenevolentShield // 仁王盾（♠2）
};

// ==================== 游戏流程枚举 ====================

/// 回合阶段
enum class PhaseType {
    Prepare,  // 准备阶段
    Judge,    // 判定阶段
    Draw,     // 摸牌阶段
    Play,     // 出牌阶段
    Discard,  // 弃牌阶段
    End,      // 结束阶段
};

/// 卡牌所在区域
enum class CardArea {
    Hand,      // 手牌区
    Equipment, // 装备区（预留）
    Judgment,  // 判定区（预留）
    Pending    // 处理中
};

/// 游戏事件枚举（用于武将技能触发判断）
enum class GameEvent {
    OnDamage,          // 受到伤害后
    OnDamageCaused,    // 造成伤害后
    OnCardPlayed,      // 使用牌后
    OnCardResponded,   // 打出牌响应后
    OnDrawPhase,       // 摸牌阶段
    OnTurnStart,       // 回合开始
    OnDying,           // 濒死时
};

/// 动作结果（ViewModel 据此判断下一步）
enum class ActionResult {
    Completed,         // 动作已完成，无后续
    RequiresDodge,     // 目标需要出闪
    RequiresKill,      // 目标需要出杀
    RequiresPeach,     // 濒死需要出桃
    RequiresDiscard,   // 需要弃牌
};

#endif // CORE_COMMONTYPES_H
