using System;
using System.Collections.Generic;

/// <summary>
/// 牌堆管理器 — 管理抽牌、弃牌、洗牌、全量卡牌生命周期
/// </summary>
public class CardManager
{
    private readonly List<Card> _drawPile = new();
    private readonly List<Card> _discardPile = new();
    private readonly List<Card> _allCards = new();
    private static readonly System.Random _rng = new();

    // ==================== 初始化与洗牌 ====================

    /// <summary>
    /// 创建所有卡牌并洗牌（约 48 张）
    /// 杀×15 闪×10 桃×6 酒×3
    /// 过河拆桥×3 顺手牵羊×3 无中生有×3
    /// 南蛮入侵×2 万箭齐发×2 桃园结义×1
    /// </summary>
    public void Initialize()
    {
        _allCards.Clear();
        _drawPile.Clear();
        _discardPile.Clear();

        // 杀×15
        for (int i = 0; i < 15; i++)
            AddCard(CreateCard(CardType.Kill, RandomSuit(), (i % 13) + 1));

        // 闪×10
        for (int i = 0; i < 10; i++)
            AddCard(CreateCard(CardType.Dodge, RandomSuit(), (i % 13) + 1));

        // 桃×6
        for (int i = 0; i < 6; i++)
            AddCard(CreateCard(CardType.Peach, RandomSuit(), (i % 13) + 1));

        // 酒×3
        for (int i = 0; i < 3; i++)
            AddCard(CreateCard(CardType.Wine, RandomSuit(), (i % 13) + 1));

        // 过河拆桥×3
        for (int i = 0; i < 3; i++)
            AddCard(CreateCard(CardType.Dismantle, RandomSuit(), (i % 13) + 1));

        // 顺手牵羊×3
        for (int i = 0; i < 3; i++)
            AddCard(CreateCard(CardType.Steal, RandomSuit(), (i % 13) + 1));

        // 无中生有×3
        for (int i = 0; i < 3; i++)
            AddCard(CreateCard(CardType.Bountiful, RandomSuit(), (i % 13) + 1));

        // 南蛮入侵×2
        for (int i = 0; i < 2; i++)
            AddCard(CreateCard(CardType.BarbarianInvasion, RandomSuit(), (i % 13) + 1));

        // 万箭齐发×2
        for (int i = 0; i < 2; i++)
            AddCard(CreateCard(CardType.Volley, RandomSuit(), (i % 13) + 1));

        // 桃园结义×1
        AddCard(CreateCard(CardType.PeachGarden, RandomSuit(), 1));

        Shuffle();
    }

    private void AddCard(Card card)
    {
        if (card == null) return;
        _allCards.Add(card);
        _drawPile.Add(card);
    }

    /// <summary>洗牌（Fisher-Yates）</summary>
    public void Shuffle()
    {
        int n = _drawPile.Count;
        for (int i = n - 1; i > 0; i--)
        {
            int j = _rng.Next(i + 1);
            (_drawPile[i], _drawPile[j]) = (_drawPile[j], _drawPile[i]);
        }
    }

    // ==================== 摸牌 ====================

    /// <summary>摸一张（牌堆空时自动回收弃牌堆）</summary>
    public Card DrawCard()
    {
        if (_drawPile.Count == 0)
        {
            ReshuffleDiscardPile();
            if (_drawPile.Count == 0)
            {
                DrawPileEmpty?.Emit();
                return null;
            }
        }

        Card card = _drawPile[_drawPile.Count - 1];
        _drawPile.RemoveAt(_drawPile.Count - 1);
        return card;
    }

    /// <summary>摸多张</summary>
    public List<Card> DrawCards(int count)
    {
        var cards = new List<Card>();
        for (int i = 0; i < count; i++)
        {
            Card card = DrawCard();
            if (card != null)
                cards.Add(card);
            else
                break;
        }
        return cards;
    }

    public int RemainingCount => _drawPile.Count;
    public int TotalCardCount => _allCards.Count;

    // ==================== 弃牌 ====================

    public void Discard(Card card)
    {
        if (card != null && !_discardPile.Contains(card))
        {
            _discardPile.Add(card);
            CardDiscarded?.Emit(card);
        }
    }

    public void DiscardMultiple(List<Card> cards)
    {
        foreach (var card in cards)
            Discard(card);
    }

    public int DiscardPileCount => _discardPile.Count;

    // ==================== 回收 ====================

    public void ReshuffleDiscardPile()
    {
        if (_discardPile.Count == 0) return;

        _drawPile.AddRange(_discardPile);
        _discardPile.Clear();
        Shuffle();

        Reshuffled?.Emit();
    }

    // ==================== 查找 ====================

    public Card FindCardById(int id)
    {
        foreach (var card in _allCards)
        {
            if (card != null && card.Id == id)
                return card;
        }
        return null;
    }

    // ==================== 卡牌工厂 ====================

    private Card CreateCard(CardType type, CardSuit suit, int number)
    {
        return type switch
        {
            CardType.Kill => new KillCard(suit, number),
            CardType.Dodge => new DodgeCard(suit, number),
            CardType.Peach => new PeachCard(suit, number),
            CardType.Wine => new WineCard(suit, number),
            CardType.Dismantle => new DismantleCard(suit, number),
            CardType.Steal => new StealCard(suit, number),
            CardType.Bountiful => new BountifulCard(suit, number),
            CardType.BarbarianInvasion => new BarbarianCard(suit, number),
            CardType.Volley => new VolleyCard(suit, number),
            CardType.PeachGarden => new PeachGardenCard(suit, number),
            _ => null
        };
    }

    private CardSuit RandomSuit()
    {
        return (CardSuit)_rng.Next(4);
    }

    // ==================== 事件通知 ====================

    public readonly Signal DrawPileEmpty = new();
    public readonly Signal Reshuffled = new();
    public readonly Signal<Card> CardDiscarded = new();
}
