using System;

/// <summary>
/// 武将基类
/// </summary>
public class Character
{
    protected string _name;
    protected int _maxHp;
    protected string _skillName;
    protected string _skillDescription;

    public Character(string name, int maxHp, string skillName, string skillDesc)
    {
        _name = name;
        _maxHp = maxHp;
        _skillName = skillName;
        _skillDescription = skillDesc;
    }

    // ==================== 只读属性 ====================

    public string CharacterName => _name;
    public int MaxHp => _maxHp;
    public string SkillName => _skillName;
    public string SkillDescription => _skillDescription;

    // ==================== 技能系统（子类重写）====================

    /// <summary>是否拥有技能</summary>
    public virtual bool HasSkill() => true;

    /// <summary>判断技能触发条件是否满足</summary>
    public virtual bool TriggerCondition(GameEventType evt, GameState state, Player self)
    {
        return false;
    }

    /// <summary>执行技能效果</summary>
    public virtual void TriggerSkill(GameState state, Player self)
    {
        SkillTriggered?.Emit(_skillName);
    }

    /// <summary>
    /// 是否可将某张牌转化为另一种牌使用。
    /// 关羽：红牌→杀；赵云：杀↔闪。
    /// 返回可转化成的 CardType，若不能转化则返回 card 的原始类型。
    /// </summary>
    public virtual CardType SkillTransformCard(Card card)
    {
        return card.Type;
    }

    // ==================== 事件通知 ====================

    public readonly Signal<string> SkillTriggered = new();
}

// ==================== 具体武将 ====================

/// <summary>曹操 — 奸雄：受到伤害后摸1张牌</summary>
public class CaoCao : Character
{
    public CaoCao() : base("曹操", 4, "奸雄", "受到伤害后摸1张牌") { }

    public override bool HasSkill() => true;

    public override bool TriggerCondition(GameEventType evt, GameState state, Player self)
    {
        return evt == GameEventType.OnDamage && self.IsAlive();
    }

    public override void TriggerSkill(GameState state, Player self)
    {
        if (state.CardManager == null) return;

        var drawn = state.CardManager.DrawCards(1);
        foreach (var c in drawn)
        {
            if (c != null) self.AddHandCard(c);
        }
        SkillTriggered.Emit(_skillName);
    }
}

/// <summary>关羽 — 武圣：红色牌可当【杀】使用或打出</summary>
public class GuanYu : Character
{
    public GuanYu() : base("关羽", 4, "武圣", "红色牌可当【杀】使用或打出") { }

    public override bool HasSkill() => true;

    public override CardType SkillTransformCard(Card card)
    {
        if (card.IsRed)
            return CardType.Kill;
        return card.Type;
    }
}

/// <summary>张飞 — 咆哮：出牌阶段可使用任意张【杀】</summary>
public class ZhangFei : Character
{
    public ZhangFei() : base("张飞", 4, "咆哮", "出牌阶段可使用任意张【杀】") { }

    public override bool HasSkill() => true;

    public override bool TriggerCondition(GameEventType evt, GameState state, Player self)
    {
        // 咆哮是持续效果，不触发
        return false;
    }
}

/// <summary>赵云 — 龙胆：【杀】可当【闪】，【闪】可当【杀】</summary>
public class ZhaoYun : Character
{
    public ZhaoYun() : base("赵云", 4, "龙胆", "【杀】可当【闪】，【闪】可当【杀】") { }

    public override bool HasSkill() => true;

    public override CardType SkillTransformCard(Card card)
    {
        if (card.Type == CardType.Kill)
            return CardType.Dodge;
        if (card.Type == CardType.Dodge)
            return CardType.Kill;
        return card.Type;
    }
}
