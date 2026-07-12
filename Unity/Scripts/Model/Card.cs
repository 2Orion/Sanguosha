using System;
using System.Collections.Generic;

/// <summary>
/// 卡牌基类 — 所有卡牌类型的抽象基类
/// </summary>
public class Card
{
    // ==================== 静态 ====================

    private static int _nextCardId = 1;

    // ==================== 构造 ====================

    public Card(CardType type, CardSuit suit, int number)
    {
        _type = type;
        _suit = suit;
        _number = number;
        _id = _nextCardId++;
        _cardName = CardTypeName(type);
        _description = "";
    }

    // ==================== 只读属性 ====================

    private readonly int _id;
    private readonly CardType _type;
    private readonly CardSuit _suit;
    private readonly int _number;
    protected string _cardName;
    protected string _description;

    /// <summary>全局唯一卡牌 ID</summary>
    public int Id => _id;

    /// <summary>卡牌类型</summary>
    public CardType Type => _type;

    /// <summary>花色</summary>
    public CardSuit Suit => _suit;

    /// <summary>点数（1=A, 2-10, 11=J, 12=Q, 13=K）</summary>
    public int Number => _number;

    /// <summary>卡牌颜色（红/黑）</summary>
    public CardColor Color => IsRed ? CardColor.Red : CardColor.Black;

    /// <summary>卡牌名称（中文）</summary>
    public string CardName => _cardName;

    /// <summary>卡牌说明文字</summary>
    public string Description => _description;

    // ==================== 分类判断 ====================

    public CardCategory Category
    {
        get
        {
            switch (_type)
            {
                case CardType.Kill:
                case CardType.Dodge:
                case CardType.Peach:
                case CardType.Wine:
                    return CardCategory.Basic;
                case CardType.Dismantle:
                case CardType.Steal:
                case CardType.Bountiful:
                case CardType.BarbarianInvasion:
                case CardType.Volley:
                case CardType.PeachGarden:
                    return CardCategory.Strategy;
                default:
                    return CardCategory.Equipment;
            }
        }
    }

    public bool IsBasic => Category == CardCategory.Basic;
    public bool IsStrategy => Category == CardCategory.Strategy;
    public bool IsEquipment => Category == CardCategory.Equipment;
    public bool IsRed => _suit == CardSuit.Heart || _suit == CardSuit.Diamond;
    public bool IsBlack => !IsRed;

    // ==================== 游戏逻辑接口（子类重写）====================

    /// <summary>判断玩家在当前状态下是否能使用此牌</summary>
    public virtual bool CanUse(GameState state, Player user)
    {
        return true;
    }

    /// <summary>判断某玩家是否可以作为此牌的合法目标</summary>
    public virtual bool CanTarget(GameState state, Player user, Player target)
    {
        return true;
    }

    /// <summary>获取所有合法目标列表</summary>
    public virtual List<Player> GetValidTargets(GameState state, Player user)
    {
        // 默认：所有其他存活玩家
        var targets = new List<Player>();
        foreach (var p in state.AlivePlayers())
        {
            if (p != user && CanTarget(state, user, p))
                targets.Add(p);
        }
        return targets;
    }

    /// <summary>执行卡牌效果，返回动作结果</summary>
    public virtual ActionResult Execute(GameState state, Player user, List<Player> targets)
    {
        return ActionResult.Completed;
    }

    // ==================== 工具方法 ====================

    /// <summary>根据 CardType 获取中文名称</summary>
    public static string CardTypeName(CardType type)
    {
        return type switch
        {
            CardType.Kill => "杀",
            CardType.Dodge => "闪",
            CardType.Peach => "桃",
            CardType.Wine => "酒",
            CardType.Dismantle => "过河拆桥",
            CardType.Steal => "顺手牵羊",
            CardType.Bountiful => "无中生有",
            CardType.BarbarianInvasion => "南蛮入侵",
            CardType.Volley => "万箭齐发",
            CardType.PeachGarden => "桃园结义",
            _ => "未知"
        };
    }

    /// <summary>花色符号字符串</summary>
    public string SuitSymbol()
    {
        return _suit switch
        {
            CardSuit.Spade => "♠",
            CardSuit.Heart => "♥",
            CardSuit.Club => "♣",
            CardSuit.Diamond => "♦",
            _ => "?"
        };
    }

    /// <summary>点数显示字符串</summary>
    public string NumberString()
    {
        return _number switch
        {
            1 => "A",
            11 => "J",
            12 => "Q",
            13 => "K",
            _ => _number.ToString()
        };
    }

    // ==================== 子类设置方法 ====================

    protected void SetCardName(string name) { _cardName = name; }
    protected void SetDescription(string desc) { _description = desc; }
}

// ==================== 基本牌 ====================

/// <summary>杀</summary>
public class KillCard : Card
{
    public KillCard(CardSuit suit, int number) : base(CardType.Kill, suit, number) { }

    public override bool CanUse(GameState state, Player user)
    {
        return GameRule.CanPlayKill(state, user);
    }

    public override bool CanTarget(GameState state, Player user, Player target)
    {
        // 杀只能对攻击范围内的其他玩家使用
        if (target == user) return false;
        if (!target.IsAlive()) return false;
        // 简化：距离为 1（相邻玩家）
        int userIdx = state.PlayerIndex(user);
        int targetIdx = state.PlayerIndex(target);
        if (userIdx < 0 || targetIdx < 0) return false;
        int dist = Math.Abs(targetIdx - userIdx);
        return dist == 1 || dist == state.PlayerCount - 1;
    }

    public override List<Player> GetValidTargets(GameState state, Player user)
    {
        var targets = new List<Player>();
        foreach (var p in state.AlivePlayers())
        {
            if (p != user && CanTarget(state, user, p))
                targets.Add(p);
        }
        return targets;
    }

    public override ActionResult Execute(GameState state, Player user, List<Player> targets)
    {
        if (targets == null || targets.Count == 0)
            return ActionResult.Completed;
        GameRule.ExecuteKill(state, user, targets[0]);
        return ActionResult.RequiresDodge;
    }
}

/// <summary>闪</summary>
public class DodgeCard : Card
{
    public DodgeCard(CardSuit suit, int number) : base(CardType.Dodge, suit, number) { }

    /// <summary>闪不能在出牌阶段主动使用</summary>
    public override bool CanUse(GameState state, Player user)
    {
        return false;
    }
}

/// <summary>桃</summary>
public class PeachCard : Card
{
    public PeachCard(CardSuit suit, int number) : base(CardType.Peach, suit, number) { }

    public override bool CanUse(GameState state, Player user)
    {
        // 桃只能在自己受伤或有人濒死时使用
        if (!user.IsFullHp()) return true;
        foreach (var p in state.AlivePlayers())
        {
            if (p.IsDying()) return true;
        }
        return false;
    }

    public override bool CanTarget(GameState state, Player user, Player target)
    {
        // 桃可以对受伤的自己或濒死的任何人使用
        if (target.IsDying()) return true;
        if (target == user && !user.IsFullHp()) return true;
        return false;
    }

    public override List<Player> GetValidTargets(GameState state, Player user)
    {
        var targets = new List<Player>();
        foreach (var p in state.AlivePlayers())
        {
            if (CanTarget(state, user, p))
                targets.Add(p);
        }
        return targets;
    }

    public override ActionResult Execute(GameState state, Player user, List<Player> targets)
    {
        if (targets == null || targets.Count == 0)
            return ActionResult.Completed;
        GameRule.ExecutePeach(state, user, targets[0]);
        return ActionResult.Completed;
    }
}

/// <summary>酒</summary>
public class WineCard : Card
{
    public WineCard(CardSuit suit, int number) : base(CardType.Wine, suit, number) { }

    public override bool CanUse(GameState state, Player user)
    {
        // 酒只能对自己使用，且本回合未使用过酒
        return !user.IsWineEnhanced();
    }

    public override List<Player> GetValidTargets(GameState state, Player user)
    {
        return new List<Player> { user };
    }

    public override ActionResult Execute(GameState state, Player user, List<Player> targets)
    {
        GameRule.ExecuteWine(state, user);
        return ActionResult.Completed;
    }
}

// ==================== 锦囊牌 ====================

/// <summary>锦囊牌基类</summary>
public class StrategyCard : Card
{
    public StrategyCard(CardType type, CardSuit suit, int number)
        : base(type, suit, number) { }

    public override bool CanUse(GameState state, Player user)
    {
        return true;
    }
}

/// <summary>过河拆桥</summary>
public class DismantleCard : StrategyCard
{
    public DismantleCard(CardSuit suit, int number)
        : base(CardType.Dismantle, suit, number) { }

    public override bool CanTarget(GameState state, Player user, Player target)
    {
        if (target == user) return false;
        if (!target.IsAlive()) return false;
        // 必须目标有手牌
        return target.HasHandCards();
    }

    public override List<Player> GetValidTargets(GameState state, Player user)
    {
        var targets = new List<Player>();
        foreach (var p in state.AlivePlayers())
        {
            if (CanTarget(state, user, p))
                targets.Add(p);
        }
        return targets;
    }

    public override ActionResult Execute(GameState state, Player user, List<Player> targets)
    {
        if (targets == null || targets.Count == 0)
            return ActionResult.Completed;
        GameRule.ExecuteDismantle(state, user, targets[0]);
        return ActionResult.Completed;
    }
}

/// <summary>顺手牵羊</summary>
public class StealCard : StrategyCard
{
    public StealCard(CardSuit suit, int number)
        : base(CardType.Steal, suit, number) { }

    public override bool CanTarget(GameState state, Player user, Player target)
    {
        if (target == user) return false;
        if (!target.IsAlive()) return false;
        return target.HasHandCards();
    }

    public override List<Player> GetValidTargets(GameState state, Player user)
    {
        var targets = new List<Player>();
        foreach (var p in state.AlivePlayers())
        {
            if (CanTarget(state, user, p))
                targets.Add(p);
        }
        return targets;
    }

    public override ActionResult Execute(GameState state, Player user, List<Player> targets)
    {
        if (targets == null || targets.Count == 0)
            return ActionResult.Completed;
        GameRule.ExecuteSteal(state, user, targets[0]);
        return ActionResult.Completed;
    }
}

/// <summary>无中生有</summary>
public class BountifulCard : StrategyCard
{
    public BountifulCard(CardSuit suit, int number)
        : base(CardType.Bountiful, suit, number) { }

    public override List<Player> GetValidTargets(GameState state, Player user)
    {
        // 无中生有无目标
        return new List<Player>();
    }

    public override ActionResult Execute(GameState state, Player user, List<Player> targets)
    {
        GameRule.ExecuteBountiful(state, user);
        return ActionResult.Completed;
    }
}

/// <summary>南蛮入侵</summary>
public class BarbarianCard : StrategyCard
{
    public BarbarianCard(CardSuit suit, int number)
        : base(CardType.BarbarianInvasion, suit, number) { }

    public override bool CanTarget(GameState state, Player user, Player target)
    {
        return true; // AOE 无需选择目标
    }

    public override List<Player> GetValidTargets(GameState state, Player user)
    {
        return new List<Player>(); // AOE 无特定目标
    }

    public override ActionResult Execute(GameState state, Player user, List<Player> targets)
    {
        GameRule.ExecuteBarbarianInvasion(state, user);
        return ActionResult.RequiresKill;
    }
}

/// <summary>万箭齐发</summary>
public class VolleyCard : StrategyCard
{
    public VolleyCard(CardSuit suit, int number)
        : base(CardType.Volley, suit, number) { }

    public override bool CanTarget(GameState state, Player user, Player target)
    {
        return true; // AOE 无需选择目标
    }

    public override List<Player> GetValidTargets(GameState state, Player user)
    {
        return new List<Player>(); // AOE 无特定目标
    }

    public override ActionResult Execute(GameState state, Player user, List<Player> targets)
    {
        GameRule.ExecuteVolley(state, user);
        return ActionResult.RequiresDodge;
    }
}

/// <summary>桃园结义</summary>
public class PeachGardenCard : StrategyCard
{
    public PeachGardenCard(CardSuit suit, int number)
        : base(CardType.PeachGarden, suit, number) { }

    public override List<Player> GetValidTargets(GameState state, Player user)
    {
        return new List<Player>(); // 全场效果
    }

    public override ActionResult Execute(GameState state, Player user, List<Player> targets)
    {
        GameRule.ExecutePeachGarden(state);
        return ActionResult.Completed;
    }
}
