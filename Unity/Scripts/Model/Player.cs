using System;
using System.Collections.Generic;

/// <summary>
/// 玩家 — 管理体力、手牌、装备区、判定区、回合状态
/// </summary>
public class Player
{
    private static readonly System.Random _rng = new System.Random();
    // ==================== 身份标识 ====================

    private int _playerId = -1;
    private string _displayName = "";

    public int PlayerId => _playerId;
    public void SetPlayerId(int id) { _playerId = id; }

    public string DisplayName => _displayName;
    public void SetDisplayName(string name)
    {
        _displayName = name;
        StateChanged?.Emit();
    }

    // ==================== 武将管理 ====================

    private Character _character;

    public void SetCharacter(Character c)
    {
        _character = c;
        if (c != null)
        {
            _hp = c.MaxHp;
            MaxHpChanged?.Emit(_hp);
        }
        CharacterChanged?.Emit(c);
    }

    public Character Character => _character;
    public string CharacterName => _character?.CharacterName ?? "";
    public bool HasCharacter() => _character != null;

    // ==================== 体力管理 ====================

    private int _hp = 0;
    private bool _dying = false;

    public int Hp => _hp;

    public void SetHp(int value)
    {
        int old = _hp;
        _hp = Math.Max(0, Math.Min(value, MaxHp));
        if (old != _hp)
            HpChanged?.Emit(_hp);
        StateChanged?.Emit();
    }

    public int MaxHp => _character?.MaxHp ?? 0;

    public void Damage(int value)
    {
        SetHp(_hp - value);
    }

    public void Heal(int value)
    {
        SetHp(_hp + value);
    }

    public bool IsAlive() => _hp > 0;

    public bool IsDying() => _dying;

    public void SetDying(bool dying)
    {
        if (_dying != dying)
        {
            _dying = dying;
            if (dying)
                Dying?.Emit(this);
            else
                Revived?.Emit(this);
        }
    }

    public bool IsFullHp() => _hp >= MaxHp;

    public float HpRatio => MaxHp > 0 ? (float)_hp / MaxHp : 0f;

    public int HandCardLimit => _hp; // = 当前体力值

    // ==================== 手牌管理 ====================

    private readonly List<Card> _handCards = new();

    public List<Card> HandCards => _handCards;
    public int HandCardCount => _handCards.Count;

    public bool HasCard(Card card) => _handCards.Contains(card);

    public void AddHandCard(Card card)
    {
        if (card == null) return;
        _handCards.Add(card);
        HandCardAdded?.Emit(card);
        HandCardsChanged?.Emit();
    }

    public void RemoveHandCard(Card card)
    {
        if (card == null) return;
        if (_handCards.Remove(card))
        {
            HandCardRemoved?.Emit(card);
            HandCardsChanged?.Emit();
        }
    }

    public bool HasHandCards() => _handCards.Count > 0;

    public Card GetRandomHandCard()
    {
        if (_handCards.Count == 0) return null;
        int idx = _rng.Next(_handCards.Count);
        return _handCards[idx];
    }

    // ==================== 装备区（预留）====================

    private readonly List<Card> _equipCards = new();

    public List<Card> EquipCards => _equipCards;
    public bool hasEquipCards() => _equipCards.Count > 0;

    public void AddEquipCard(Card card)
    {
        if (card != null) _equipCards.Add(card);
    }

    public void RemoveEquipCard(Card card)
    {
        if (card != null) _equipCards.Remove(card);
    }

    // ==================== 判定区（预留）====================

    private readonly List<Card> _judgmentCards = new();

    public List<Card> JudgmentCards => _judgmentCards;
    public bool HasJudgmentCards() => _judgmentCards.Count > 0;

    public void AddJudgmentCard(Card card)
    {
        if (card != null) _judgmentCards.Add(card);
    }

    public void RemoveJudgmentCard(Card card)
    {
        if (card != null) _judgmentCards.Remove(card);
    }

    // ==================== 回合状态 ====================

    private bool _usedKillThisTurn = false;
    private bool _wineEnhanced = false;

    public void ResetTurnState()
    {
        _usedKillThisTurn = false;
        _wineEnhanced = false;
    }

    public bool HasUsedKillThisTurn() => _usedKillThisTurn;
    public void SetUsedKillThisTurn(bool used) => _usedKillThisTurn = used;

    public bool IsWineEnhanced() => _wineEnhanced;
    public void SetWineEnhanced(bool enhanced) => _wineEnhanced = enhanced;

    // ==================== 工具 ====================

    /// <summary>获取所有可被选中的牌（手牌 + 装备区）</summary>
    public List<Card> AllSelectableCards()
    {
        var all = new List<Card>();
        all.AddRange(_handCards);
        all.AddRange(_equipCards);
        return all;
    }

    // ==================== 事件通知 ====================

    public readonly Signal<int> HpChanged = new();
    public readonly Signal<int> MaxHpChanged = new();
    public readonly Signal<Player> Dying = new();
    public readonly Signal<Player> Died = new();
    public readonly Signal<Player> Revived = new();
    public readonly Signal<Card> HandCardAdded = new();
    public readonly Signal<Card> HandCardRemoved = new();
    public readonly Signal HandCardsChanged = new();
    public readonly Signal<Character> CharacterChanged = new();
    public readonly Signal StateChanged = new();
}
