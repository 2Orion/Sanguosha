using System.Collections.Generic;

/// <summary>
/// 待定动作信息（当需要玩家响应时填充此结构）
/// </summary>
public class PendingActionInfo
{
    /// <summary>动作来源（使用牌的玩家）</summary>
    public Player Source;

    /// <summary>需要响应的玩家</summary>
    public Player Target;

    /// <summary>触发动作的卡牌</summary>
    public Card SourceCard;

    /// <summary>需要打出的卡牌类型</summary>
    public CardType RequiredCardType;

    /// <summary>界面提示文字（如"请打出一张【闪】"）</summary>
    public string Description;

    /// <summary>是否可以跳过响应</summary>
    public bool CanSkip;

    /// <summary>AOE/链式响应中尚未询问的后续目标</summary>
    public List<Player> RemainingTargets = new();
}
