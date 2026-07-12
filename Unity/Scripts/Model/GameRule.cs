using System.Collections.Generic;
using System;

/// <summary>
/// 游戏规则 — 静态方法集合，处理卡牌执行、伤害、濒死、AOE响应等核心逻辑
/// </summary>
public static class GameRule
{
    // ==================== 常量 ====================

    public const int INITIAL_HAND_COUNT = 4;
    public const int DRAW_PHASE_COUNT = 2;

    // ==================== 规则判断 ====================

    public static bool CanPlayKill(GameState state, Player player)
    {
        if (state == null || player == null) return false;

        // 张飞咆哮：不受杀次数限制
        if (player.Character != null && player.Character is ZhangFei)
            return true;

        return !player.HasUsedKillThisTurn();
    }

    public static bool CanPlayCard(GameState state, Player player, Card card)
    {
        if (state == null || player == null || card == null) return false;
        return card.CanUse(state, player);
    }

    public static int HandLimit(Player player)
    {
        return player?.HandCardLimit ?? 0;
    }

    public static bool HasDodgeToRespond(Player player)
    {
        if (player == null) return false;

        foreach (var card in player.HandCards)
        {
            if (card == null) continue;
            if (card.Type == CardType.Dodge) return true;

            if (player.Character != null)
            {
                CardType transformed = player.Character.SkillTransformCard(card);
                if (transformed == CardType.Dodge) return true;
            }
        }
        return false;
    }

    public static bool HasKillToRespond(Player player)
    {
        if (player == null) return false;

        foreach (var card in player.HandCards)
        {
            if (card == null) continue;
            if (card.Type == CardType.Kill) return true;

            if (player.Character != null)
            {
                CardType transformed = player.Character.SkillTransformCard(card);
                if (transformed == CardType.Kill) return true;
            }
        }
        return false;
    }

    public static bool HasPeachToSave(Player player, Player dyingPlayer)
    {
        if (player == null || dyingPlayer == null) return false;

        foreach (var card in player.HandCards)
        {
            if (card != null && (card.Type == CardType.Peach || card.Type == CardType.Wine))
                return true;
        }
        return false;
    }

    // ==================== 卡牌执行 ====================

    public static void ExecuteKill(GameState state, Player user, Player target)
    {
        if (state == null || user == null || target == null) return;

        user.SetUsedKillThisTurn(true);

        var info = new PendingActionInfo
        {
            Source = user,
            Target = target,
            SourceCard = null,
            RequiredCardType = CardType.Dodge,
            Description = $"{user.DisplayName} 对 {target.DisplayName} 使用了【杀】，请打出【闪】",
            CanSkip = false
        };

        state.SetPendingAction(info);
    }

    public static void HandleKillResponse(GameState state, Player responder, Card dodgeCard)
    {
        if (state == null || responder == null) return;

        var info = state.PendingAction;
        Player attacker = info.Source;

        if (dodgeCard != null)
        {
            responder.RemoveHandCard(dodgeCard);
            state.CardManager?.Discard(dodgeCard);
            state.ClearPendingAction();
        }
        else
        {
            int damageValue = 1;
            if (attacker != null && attacker.IsWineEnhanced())
            {
                damageValue = 2;
                attacker.SetWineEnhanced(false);
            }

            DealDamage(state, responder, damageValue, attacker);
            state.ClearPendingAction();
        }
    }

    public static void ExecutePeach(GameState state, Player user, Player target)
    {
        if (state == null || target == null) return;
        target.Heal(1);
    }

    public static void ExecuteWine(GameState state, Player user)
    {
        if (state == null || user == null) return;
        user.SetWineEnhanced(true);
    }

    // ==================== 锦囊执行 ====================

    public static void ExecuteDismantle(GameState state, Player user, Player target)
    {
        if (state == null || user == null || target == null) return;

        Card card = target.GetRandomHandCard();
        if (card == null && target.hasEquipCards())
        {
            var equips = target.EquipCards;
            if (equips.Count > 0) card = equips[0];
        }

        if (card != null)
        {
            target.RemoveHandCard(card);
            target.RemoveEquipCard(card);
            state.CardManager?.Discard(card);
        }
    }

    public static void ExecuteSteal(GameState state, Player user, Player target)
    {
        if (state == null || user == null || target == null) return;

        Card card = target.GetRandomHandCard();
        if (card == null && target.hasEquipCards())
        {
            var equips = target.EquipCards;
            if (equips.Count > 0) card = equips[0];
        }

        if (card != null)
        {
            target.RemoveHandCard(card);
            target.RemoveEquipCard(card);
            user.AddHandCard(card);
        }
    }

    public static void ExecuteBountiful(GameState state, Player user)
    {
        if (state == null || user == null || state.CardManager == null) return;

        var cards = state.CardManager.DrawCards(2);
        foreach (var card in cards)
        {
            if (card != null) user.AddHandCard(card);
        }
    }

    public static void ExecuteBarbarianInvasion(GameState state, Player user)
    {
        if (state == null || user == null) return;

        var targets = new List<Player>();
        foreach (var p in state.AlivePlayers())
        {
            if (p != user) targets.Add(p);
        }

        if (targets.Count == 0) return;

        Player firstTarget = targets[0];

        var info = new PendingActionInfo
        {
            Source = user,
            Target = firstTarget,
            SourceCard = null,
            RequiredCardType = CardType.Kill,
            Description = $"{user.DisplayName} 使用了【南蛮入侵】，{firstTarget.DisplayName} 需打出【杀】",
            CanSkip = true
        };
        info.RemainingTargets.AddRange(targets.GetRange(1, targets.Count - 1));

        state.SetPendingAction(info);
    }

    public static void ExecuteVolley(GameState state, Player user)
    {
        if (state == null || user == null) return;

        var targets = new List<Player>();
        foreach (var p in state.AlivePlayers())
        {
            if (p != user) targets.Add(p);
        }

        if (targets.Count == 0) return;

        Player firstTarget = targets[0];

        var info = new PendingActionInfo
        {
            Source = user,
            Target = firstTarget,
            SourceCard = null,
            RequiredCardType = CardType.Dodge,
            Description = $"{user.DisplayName} 使用了【万箭齐发】，{firstTarget.DisplayName} 需打出【闪】",
            CanSkip = true
        };
        info.RemainingTargets.AddRange(targets.GetRange(1, targets.Count - 1));

        state.SetPendingAction(info);
    }

    public static void ExecutePeachGarden(GameState state)
    {
        if (state == null) return;

        foreach (var p in state.AlivePlayers())
        {
            if (p != null && !p.IsFullHp())
                p.Heal(1);
        }
    }

    // ==================== AOE 响应处理 ====================

    public static void HandleAoeKillResponse(GameState state, Player responder, Card killCard)
    {
        if (state == null || responder == null) return;

        var info = state.PendingAction; // 复制引用（C# class 是引用类型）

        if (killCard != null)
        {
            responder.RemoveHandCard(killCard);
            state.CardManager?.Discard(killCard);
        }
        else
        {
            DealDamage(state, responder, 1, info.Source);
        }

        state.ClearPendingAction();

        // 构建剩余目标列表
        var remaining = new List<Player>();
        foreach (var p in info.RemainingTargets)
        {
            if (p != null && p.IsAlive()) remaining.Add(p);
        }

        if (remaining.Count > 0)
        {
            Player nextTarget = remaining[0];
            var nextInfo = new PendingActionInfo
            {
                Source = info.Source,
                Target = nextTarget,
                SourceCard = null,
                RequiredCardType = CardType.Kill,
                Description = $"{info.Source.DisplayName} 使用了【南蛮入侵】，{nextTarget.DisplayName} 需打出【杀】",
                CanSkip = true
            };
            nextInfo.RemainingTargets.AddRange(remaining.GetRange(1, remaining.Count - 1));

            state.SetPendingAction(nextInfo);
        }
    }

    public static void HandleAoeDodgeResponse(GameState state, Player responder, Card dodgeCard)
    {
        if (state == null || responder == null) return;

        var info = state.PendingAction;

        if (dodgeCard != null)
        {
            responder.RemoveHandCard(dodgeCard);
            state.CardManager?.Discard(dodgeCard);
        }
        else
        {
            DealDamage(state, responder, 1, info.Source);
        }

        state.ClearPendingAction();

        var remaining = new List<Player>();
        foreach (var p in info.RemainingTargets)
        {
            if (p != null && p.IsAlive()) remaining.Add(p);
        }

        if (remaining.Count > 0)
        {
            Player nextTarget = remaining[0];
            var nextInfo = new PendingActionInfo
            {
                Source = info.Source,
                Target = nextTarget,
                SourceCard = null,
                RequiredCardType = CardType.Dodge,
                Description = $"{info.Source.DisplayName} 使用了【万箭齐发】，{nextTarget.DisplayName} 需打出【闪】",
                CanSkip = true
            };
            nextInfo.RemainingTargets.AddRange(remaining.GetRange(1, remaining.Count - 1));

            state.SetPendingAction(nextInfo);
        }
    }

    public static void HandleAoeSkipResponse(GameState state, Player responder)
    {
        if (state == null || responder == null) return;

        var info = state.PendingAction;

        if (info.RequiredCardType == CardType.Kill)
            HandleAoeKillResponse(state, responder, null);
        else if (info.RequiredCardType == CardType.Dodge)
            HandleAoeDodgeResponse(state, responder, null);
    }

    // ==================== 伤害与濒死 ====================

    public static void DealDamage(GameState state, Player target, int value, Player source)
    {
        if (state == null || target == null || value <= 0) return;

        target.Damage(value);

        // 触发受伤技能（曹操奸雄）
        if (target.Character != null && source != null)
        {
            if (target.Character.TriggerCondition(GameEventType.OnDamage, state, target))
            {
                target.Character.TriggerSkill(state, target);
            }
        }

        // 检查濒死
        if (target.Hp <= 0 && !target.IsDying())
        {
            target.SetDying(true);
            StartDyingProcess(state, target);
        }
    }

    public static void StartDyingProcess(GameState state, Player dyingPlayer)
    {
        if (state == null || dyingPlayer == null) return;

        var saviors = state.AlivePlayers();
        if (saviors.Count == 0) return;

        Player firstSavior = saviors[0];

        var info = new PendingActionInfo
        {
            Source = firstSavior,
            Target = dyingPlayer,
            SourceCard = null,
            RequiredCardType = CardType.Peach,
            Description = $"{dyingPlayer.DisplayName} 濒死，{firstSavior.DisplayName} 可以使用【桃】或【酒】",
            CanSkip = true
        };

        state.SetPendingAction(info);
    }

    public static bool HandleDyingPeach(GameState state, Player dyingPlayer, Player peachUser, Card peachCard)
    {
        if (state == null || dyingPlayer == null || peachUser == null || peachCard == null)
            return false;

        peachUser.RemoveHandCard(peachCard);
        state.CardManager?.Discard(peachCard);

        dyingPlayer.Heal(1);

        if (dyingPlayer.Hp > 0)
        {
            dyingPlayer.SetDying(false);
            state.ClearPendingAction();
            return true;
        }

        return false;
    }

    public static void SkipDyingResponse(GameState state, Player dyingPlayer)
    {
        if (state == null || dyingPlayer == null) return;

        var info = state.PendingAction;
        Player currentSavior = info.Source;

        var allPlayers = state.AlivePlayers();
        int currentIndex = allPlayers.IndexOf(currentSavior);

        if (currentIndex >= 0 && currentIndex + 1 < allPlayers.Count)
        {
            Player nextSavior = allPlayers[currentIndex + 1];

            var nextInfo = new PendingActionInfo
            {
                Source = nextSavior,
                Target = dyingPlayer,
                SourceCard = null,
                RequiredCardType = CardType.Peach,
                Description = $"{dyingPlayer.DisplayName} 濒死，{nextSavior.DisplayName} 可以使用【桃】或【酒】",
                CanSkip = true
            };

            state.SetPendingAction(nextInfo);
        }
        else
        {
            CheckDeath(state, dyingPlayer);
        }
    }

    public static void CheckDeath(GameState state, Player player)
    {
        if (state == null || player == null) return;

        if (player.Hp <= 0)
        {
            player.SetDying(false);
            player.Died?.Emit(player);
            state.ClearPendingAction();
            CheckGameOver(state);
        }
    }

    public static void CheckGameOver(GameState state)
    {
        if (state == null) return;

        var alive = state.AlivePlayers();

        if (alive.Count == 1)
            state.SetGameOver(alive[0]);
        else if (alive.Count == 0)
            state.SetGameOver(null);
    }

    // ==================== 弃牌阶段 ====================

    public static int GetDiscardCount(Player player)
    {
        if (player == null) return 0;

        int handCount = player.HandCardCount;
        int limit = HandLimit(player);

        return Math.Max(0, handCount - limit);
    }
}
