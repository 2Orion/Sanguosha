using System.Collections.Generic;

/// <summary>
/// 操作 ViewModel — 管理出牌、响应、目标选择等逻辑
/// </summary>
public class ActionViewModel
{
    private GameState _state;

    public void SetGameState(GameState state) => _state = state;
    public GameState GameState => _state;

    // ==================== 出牌阶段 ====================

    /// <summary>获取当前玩家在出牌阶段可打出的手牌</summary>
    public List<Card> GetPlayableCards(Player player)
    {
        var result = new List<Card>();
        if (_state == null || player == null) return result;

        foreach (var card in player.HandCards)
        {
            if (card == null) continue;
            if (!card.CanUse(_state, player)) continue;
            if (!GameRule.CanPlayCard(_state, player, card)) continue;
            result.Add(card);
        }
        return result;
    }

    /// <summary>获取某张牌对指定玩家的合法目标列表</summary>
    public List<Player> GetValidTargets(Card card, Player user)
    {
        if (_state == null || card == null || user == null) return new List<Player>();
        return card.GetValidTargets(_state, user);
    }

    /// <summary>出牌（由当前玩家主动使用）</summary>
    public ActionResult PlayCard(Card card, Player user, List<Player> targets)
    {
        if (_state == null || card == null || user == null)
            return ActionResult.Completed;

        ActionResult result = card.Execute(_state, user, targets);

        // 记录日志
        string targetStr = "";
        if (targets != null)
        {
            for (int i = 0; i < targets.Count; i++)
            {
                if (i > 0) targetStr += "、";
                targetStr += targets[i].DisplayName;
            }
        }

        EmitLog($"{user.DisplayName} 使用了【{card.CardName}】{(string.IsNullOrEmpty(targetStr) ? "" : " → " + targetStr)}");

        ActionCompleted?.Emit();
        return result;
    }

    // ==================== 响应阶段 ====================

    /// <summary>获取响应某类请求时可打出的牌（含武将技能转化）</summary>
    public List<Card> GetResponseCards(Player player, CardType requiredType)
    {
        var result = new List<Card>();
        if (player == null) return result;

        foreach (var card in player.HandCards)
        {
            if (card == null) continue;

            // 直接匹配
            if (card.Type == requiredType)
            {
                result.Add(card);
                continue;
            }

            // 武将技能转化
            if (player.Character != null)
            {
                CardType transformed = player.Character.SkillTransformCard(card);
                if (transformed == requiredType)
                    result.Add(card);
            }
        }
        return result;
    }

    /// <summary>是否可以跳过当前待定动作</summary>
    public bool CanSkipPendingAction()
    {
        if (_state == null) return false;
        if (!_state.HasPendingAction()) return false;
        return _state.PendingAction.CanSkip;
    }

    /// <summary>响应出牌（如出闪响应杀、出杀响应南蛮、出桃救人）</summary>
    public void RespondCard(Card card, Player responder)
    {
        if (_state == null || card == null || responder == null) return;
        if (!_state.HasPendingAction()) return;

        var info = _state.PendingAction;
        string cardName = card.CardName;

        switch (info.RequiredCardType)
        {
            case CardType.Dodge:
                if (info.CanSkip)
                {
                    GameRule.HandleAoeDodgeResponse(_state, responder, card);
                    EmitLog($"{responder.DisplayName} 打出【{cardName}】响应万箭齐发");
                }
                else
                {
                    GameRule.HandleKillResponse(_state, responder, card);
                    EmitLog($"{responder.DisplayName} 打出【{cardName}】响应杀");
                }
                break;

            case CardType.Kill:
                GameRule.HandleAoeKillResponse(_state, responder, card);
                EmitLog($"{responder.DisplayName} 打出【{cardName}】响应南蛮入侵");
                break;

            case CardType.Peach:
                {
                    Player dyingPlayer = info.Target;
                    bool saved = GameRule.HandleDyingPeach(_state, dyingPlayer, responder, card);
                    EmitLog($"{responder.DisplayName} 对 {dyingPlayer.DisplayName} 使用【{cardName}】{(saved ? "，救回！" : "，仍需继续救援")}");
                }
                break;
        }

        ActionCompleted?.Emit();
    }

    /// <summary>跳过响应</summary>
    public void SkipResponse(Player responder)
    {
        if (_state == null || responder == null) return;
        if (!_state.HasPendingAction()) return;

        var info = _state.PendingAction;
        if (!info.CanSkip) return;

        switch (info.RequiredCardType)
        {
            case CardType.Dodge:
            case CardType.Kill:
                GameRule.HandleAoeSkipResponse(_state, responder);
                EmitLog($"{responder.DisplayName} 放弃了响应");
                break;

            case CardType.Peach:
                GameRule.SkipDyingResponse(_state, info.Target);
                EmitLog($"{responder.DisplayName} 放弃救助 {info.Target.DisplayName}");
                break;
        }

        ActionCompleted?.Emit();
    }

    // ==================== 弃牌阶段 ====================

    public void DiscardCard(Card card, Player player)
    {
        if (_state == null || card == null || player == null) return;

        player.RemoveHandCard(card);
        _state.CardManager?.Discard(card);

        EmitLog($"{player.DisplayName} 弃置了【{card.CardName}】");
    }

    /// <summary>获取需要弃牌的数量（0 则不需要）</summary>
    public int GetDiscardCount(Player player)
    {
        return GameRule.GetDiscardCount(player);
    }

    // ==================== 事件 ====================

    public readonly Signal<string> LogMessage = new();
    public readonly Signal ActionCompleted = new();

    private void EmitLog(string msg)
    {
        LogMessage.Emit(msg);
    }
}
