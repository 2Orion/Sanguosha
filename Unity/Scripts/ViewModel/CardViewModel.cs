using System.Collections.Generic;

/// <summary>
/// 卡牌 ViewModel — 在 Card 基础上增加 UI 展示状态（是否选中、是否可打出、是否高亮）
/// </summary>
public class CardViewModel
{
    private readonly Card _card;
    private bool _selected = false;
    private bool _playable = false;
    private bool _highlighted = false;

    public CardViewModel(Card card)
    {
        _card = card;
    }

    // ==================== Card 只读属性（透传）====================

    public int Id => _card?.Id ?? -1;
    public CardType CardType => _card?.Type ?? CardType.Kill;
    public CardSuit Suit => _card?.Suit ?? CardSuit.Spade;
    public int Number => _card?.Number ?? 0;
    public string CardName => _card?.CardName ?? "";
    public string Description => _card?.Description ?? "";
    public CardColor Color => _card?.Color ?? CardColor.Black;
    public bool IsBasic => _card?.IsBasic ?? false;
    public bool IsStrategy => _card?.IsStrategy ?? false;

    public string SuitSymbol()
    {
        return _card?.SuitSymbol().ToString() ?? "?";
    }

    public string NumberString()
    {
        return _card?.NumberString() ?? "0";
    }

    // ==================== UI 状态 ====================

    public bool IsSelected => _selected;

    public void SetSelected(bool selected)
    {
        if (_selected != selected)
        {
            _selected = selected;
            SelectedChanged?.Emit(selected);
        }
    }

    public bool IsPlayable => _playable;

    public void SetPlayable(bool playable)
    {
        if (_playable != playable)
        {
            _playable = playable;
            PlayableChanged?.Emit(playable);
        }
    }

    public bool IsHighlighted => _highlighted;

    public void SetHighlighted(bool highlighted)
    {
        if (_highlighted != highlighted)
        {
            _highlighted = highlighted;
            HighlightedChanged?.Emit(highlighted);
        }
    }

    /// <summary>重置所有 UI 状态</summary>
    public void ResetUIState()
    {
        SetSelected(false);
        SetPlayable(false);
        SetHighlighted(false);
    }

    // ==================== 原始指针访问 ====================

    public Card Card => _card;

    // ==================== 事件 ====================

    public readonly Signal<bool> SelectedChanged = new();
    public readonly Signal<bool> PlayableChanged = new();
    public readonly Signal<bool> HighlightedChanged = new();
}
