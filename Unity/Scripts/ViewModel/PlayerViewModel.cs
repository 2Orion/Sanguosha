using System.Collections.Generic;

/// <summary>
/// 玩家 ViewModel — 包装 Player 并转发其事件，方便 View 层只监听 ViewModel 而不直接依赖 Model
/// </summary>
public class PlayerViewModel
{
    private readonly Player _player;

    // Model 事件连接 ID（用于析构时断开）
    private readonly Dictionary<string, int> _connIds = new();

    public PlayerViewModel(Player player)
    {
        _player = player;
        ConnectModelEvents();
    }

    /// <summary>断开所有 Model 事件</summary>
    public void Dispose()
    {
        if (_player == null) return;

        _player.HpChanged.Disconnect(_connIds.GetValueOrDefault("hp", 0));
        _player.MaxHpChanged.Disconnect(_connIds.GetValueOrDefault("maxHp", 0));
        _player.Dying.Disconnect(_connIds.GetValueOrDefault("dying", 0));
        _player.Died.Disconnect(_connIds.GetValueOrDefault("died", 0));
        _player.Revived.Disconnect(_connIds.GetValueOrDefault("revived", 0));
        _player.HandCardAdded.Disconnect(_connIds.GetValueOrDefault("cardAdd", 0));
        _player.HandCardRemoved.Disconnect(_connIds.GetValueOrDefault("cardRem", 0));
        _player.HandCardsChanged.Disconnect(_connIds.GetValueOrDefault("cardsCh", 0));
        _player.StateChanged.Disconnect(_connIds.GetValueOrDefault("state", 0));

        _connIds.Clear();
    }

    // ==================== 身份标识 ====================

    public int PlayerId => _player?.PlayerId ?? -1;
    public string DisplayName => _player?.DisplayName ?? "";
    public string CharacterName => _player?.CharacterName ?? "";
    public string SkillName => _player?.Character?.SkillName ?? "";
    public string SkillDescription => _player?.Character?.SkillDescription ?? "";

    // ==================== 体力 ====================

    public int Hp => _player?.Hp ?? 0;
    public int MaxHp => _player?.MaxHp ?? 0;
    public float HpRatio => _player?.HpRatio ?? 0f;
    public bool IsAlive => _player?.IsAlive() ?? false;
    public bool IsDying => _player?.IsDying() ?? false;
    public bool IsFullHp => _player?.IsFullHp() ?? false;

    // ==================== 手牌 ====================

    public int HandCardCount => _player?.HandCardCount ?? 0;
    public List<Card> HandCards => _player?.HandCards ?? new List<Card>();
    public bool HasHandCards => _player?.HasHandCards() ?? false;
    public int HandCardLimit => _player?.HandCardLimit ?? 0;

    // ==================== 回合状态 ====================

    public bool HasUsedKillThisTurn => _player?.HasUsedKillThisTurn() ?? false;
    public bool IsWineEnhanced => _player?.IsWineEnhanced() ?? false;

    // ==================== 装备/判定区（预留）====================

    public int EquipCount => _player?.EquipCards.Count ?? 0;
    public int JudgmentCount => _player?.JudgmentCards.Count ?? 0;

    // ==================== 原始指针访问 ====================

    public Player Player => _player;

    // ==================== 事件（从 Model 转发）====================

    public readonly Signal<int> HpChanged = new();
    public readonly Signal<int> MaxHpChanged = new();
    public readonly Signal<Player> Dying = new();
    public readonly Signal<Player> Died = new();
    public readonly Signal<Player> Revived = new();
    public readonly Signal<Card> HandCardAdded = new();
    public readonly Signal<Card> HandCardRemoved = new();
    public readonly Signal HandCardsChanged = new();
    public readonly Signal StateChanged = new();

    // ==================== 事件转发 ====================

    private void ConnectModelEvents()
    {
        if (_player == null) return;

        _connIds["hp"] = _player.HpChanged.Connect(hp => HpChanged.Emit(hp));
        _connIds["maxHp"] = _player.MaxHpChanged.Connect(mh => MaxHpChanged.Emit(mh));
        _connIds["dying"] = _player.Dying.Connect(p => Dying.Emit(p));
        _connIds["died"] = _player.Died.Connect(p => Died.Emit(p));
        _connIds["revived"] = _player.Revived.Connect(p => Revived.Emit(p));
        _connIds["cardAdd"] = _player.HandCardAdded.Connect(c => HandCardAdded.Emit(c));
        _connIds["cardRem"] = _player.HandCardRemoved.Connect(c => HandCardRemoved.Emit(c));
        _connIds["cardsCh"] = _player.HandCardsChanged.Connect(() => HandCardsChanged.Emit());
        _connIds["state"] = _player.StateChanged.Connect(() => StateChanged.Emit());
    }
}
