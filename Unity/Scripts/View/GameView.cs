using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using TMPro;

/// <summary>
/// 游戏主视图 — 管理整个游戏 UI 和协程驱动的游戏循环
/// 挂载到场景中的 GameManager GameObject
/// </summary>
public class GameView : MonoBehaviour
{
    [Header("玩家信息")]
    [SerializeField] private PlayerInfoWidget _player1Info;
    [SerializeField] private PlayerInfoWidget _player2Info;

    [Header("手牌区域")]
    [SerializeField] private Transform _handCardsParent;         // 当前玩家手牌容器
    [SerializeField] private GameObject _cardPrefab;             // 卡牌预制体

    [Header("响应区域")]
    [SerializeField] private GameObject _responsePanel;          // 响应面板
    [SerializeField] private Transform _responseCardsParent;     // 响应卡牌容器
    [SerializeField] private Button _skipButton;                 // 跳过按钮
    [SerializeField] private TextMeshProUGUI _responsePromptText;

    [Header("日志区域")]
    [SerializeField] private TextMeshProUGUI _logText;
    [SerializeField] private ScrollRect _logScroll;
    [SerializeField] private int _maxLogLines = 50;

    [Header("阶段和回合信息")]
    [SerializeField] private TextMeshProUGUI _phaseText;
    [SerializeField] private TextMeshProUGUI _turnText;

    [Header("结束回合按钮")]
    [SerializeField] private Button _endPlayButton;
    [SerializeField] private Button _endDiscardButton;

    [Header("武将选择")]
    [SerializeField] private GameObject _characterSelectPanel;
    [SerializeField] private Button[] _charButtons;          // 0-3: 曹操、关羽、张飞、赵云

    // ==================== 状态 ====================

    private GameViewModel _gvm;
    private CardViewModel _selectedCard;
    private readonly List<CardWidget> _handCardWidgets = new();
    private readonly List<string> _logLines = new();

    // ==================== 生命周期 ====================

    private void Start()
    {
        // 显示武将选择
        ShowCharacterSelect();
    }

    private void ShowCharacterSelect()
    {
        _characterSelectPanel.SetActive(true);
        // 武将选择由 UI 按钮驱动，本示例简化为自动开始
        // 实际项目可以让玩家点击选择，此处为了演示直接开始
        StartCoroutine(AutoStartGame());
    }

    private IEnumerator AutoStartGame()
    {
        yield return new WaitForSeconds(1f);
        // 随机选武将
        int char1 = Random.Range(0, 4);
        int char2 = Random.Range(0, 4);
        _characterSelectPanel.SetActive(false);
        StartGame(char1, char2);
    }

    /// <summary>开始游戏</summary>
    public void StartGame(int charId1, int charId2)
    {
        _gvm = new GameViewModel();

        // 订阅事件
        _gvm.LogMessage.Connect(OnLog);
        _gvm.GameOver.Connect(OnGameOver);
        _gvm.StateChanged.Connect(() => RefreshAll());
        _gvm.PhaseChanged.Connect(OnPhaseChanged);

        _gvm.StartGame(charId1, charId2);

        // 绑定玩家信息
        _player1Info?.Bind(_gvm.GetPlayerVM(0));
        _player2Info?.Bind(_gvm.GetPlayerVM(1));

        // 开始游戏循环
        StartCoroutine(GameLoop());
    }

    // ==================== 游戏循环 ====================

    private IEnumerator GameLoop()
    {
        while (!_gvm.IsGameOver)
        {
            RefreshAll();

            // 优先处理待定动作（响应请求）
            if (_gvm.GameState.HasPendingAction())
            {
                yield return HandlePendingAction();
                continue;
            }

            PhaseType phase = _gvm.CurrentPhase;

            switch (phase)
            {
                case PhaseType.Prepare:
                case PhaseType.Judge:
                case PhaseType.Draw:
                case PhaseType.End:
                    // 自动阶段，直接推进
                    _gvm.AdvancePhase();
                    yield return new WaitForSeconds(0.3f);  // 给动画时间
                    break;

                case PhaseType.Play:
                    yield return PlayPhase();
                    break;

                case PhaseType.Discard:
                    yield return DiscardPhase();
                    break;
            }
        }

        RefreshAll();
    }

    // ==================== 出牌阶段 ====================

    private IEnumerator PlayPhase()
    {
        _endPlayButton?.gameObject.SetActive(true);

        bool playEnded = false;
        System.Action onEndPlay = () => playEnded = true;
        if (_endPlayButton != null)
            _endPlayButton.onClick.AddListener(new UnityEngine.Events.UnityAction(onEndPlay));

        while (!playEnded && !_gvm.IsGameOver &&
               _gvm.CurrentPhase == PhaseType.Play &&
               !_gvm.GameState.HasPendingAction())
        {
            RefreshHandCards();
            yield return null;
        }

        if (_endPlayButton != null)
            _endPlayButton.onClick.RemoveAllListeners();
        _endPlayButton?.gameObject.SetActive(false);

        if (!_gvm.IsGameOver && !_gvm.GameState.HasPendingAction())
            _gvm.AdvancePhase(); // Play → Discard
    }

    /// <summary>UI 按钮回调：结束出牌</summary>
    public void OnEndPlayClicked()
    {
        _gvm?.EndPlayPhase();
    }

    // ==================== 卡牌点击 ====================

    private void OnCardClicked(CardWidget widget)
    {
        if (widget?.ViewModel == null) return;
        if (!widget.ViewModel.IsPlayable) return;

        // 切换选中状态
        if (_selectedCard == widget.ViewModel)
        {
            _selectedCard?.SetSelected(false);
            _selectedCard = null;
            HideResponsePanel();
        }
        else
        {
            _selectedCard?.SetSelected(false);
            _selectedCard = widget.ViewModel;
            _selectedCard.SetSelected(true);

            // 如果有多目标，显示目标选择
            TryPlayCard(_selectedCard);
        }
    }

    private void TryPlayCard(CardViewModel cvm)
    {
        if (_gvm == null) return;

        var targets = _gvm.ActionVM.GetValidTargets(cvm.Card, _gvm.CurrentPlayer);

        if (targets.Count == 0)
        {
            // 无目标卡牌，直接使用
            _gvm.ActionVM.PlayCard(cvm.Card, _gvm.CurrentPlayer, null);
            _selectedCard = null;
        }
        else if (targets.Count == 1)
        {
            // 单一目标，自动选择
            _gvm.ActionVM.PlayCard(cvm.Card, _gvm.CurrentPlayer, new List<Player> { targets[0] });
            _selectedCard = null;
        }
        else
        {
            // 多目标 — 显示目标选择 UI（简化：选第一个）
            _gvm.ActionVM.PlayCard(cvm.Card, _gvm.CurrentPlayer, new List<Player> { targets[0] });
            _selectedCard = null;
        }
    }

    // ==================== 响应处理 ====================

    private IEnumerator HandlePendingAction()
    {
        if (_gvm == null) yield break;

        var info = _gvm.GameState.PendingAction;
        ShowResponsePanel(info);

        bool responded = false;
        while (!responded && _gvm.GameState.HasPendingAction())
        {
            yield return null;
        }

        HideResponsePanel();
        RefreshAll();
    }

    private void ShowResponsePanel(PendingActionInfo info)
    {
        _responsePanel?.SetActive(true);

        if (_responsePromptText != null)
            _responsePromptText.text = info.Description;

        if (_skipButton != null)
        {
            _skipButton.gameObject.SetActive(info.CanSkip);
            _skipButton.onClick.RemoveAllListeners();
            _skipButton.onClick.AddListener(() =>
            {
                _gvm?.ActionVM.SkipResponse(info.Target);
            });
        }

        // 显示可响应的牌
        if (_responseCardsParent != null && _cardPrefab != null)
        {
            // 清理旧卡牌
            foreach (Transform child in _responseCardsParent)
                Destroy(child.gameObject);

            var cards = _gvm.ActionVM.GetResponseCards(info.Target, info.RequiredCardType);
            foreach (var card in cards)
            {
                var go = Instantiate(_cardPrefab, _responseCardsParent);
                var widget = go.GetComponent<CardWidget>();
                if (widget != null)
                {
                    var cvm = new CardViewModel(card);
                    cvm.SetPlayable(true);
                    widget.Bind(cvm, (w) =>
                    {
                        _gvm.ActionVM.RespondCard(card, info.Target);
                    });
                }
            }
        }
    }

    private void HideResponsePanel()
    {
        _responsePanel?.SetActive(false);
    }

    // ==================== 弃牌阶段 ====================

    private IEnumerator DiscardPhase()
    {
        int discardCount = _gvm.ActionVM.GetDiscardCount(_gvm.CurrentPlayer);
        if (discardCount <= 0)
        {
            _gvm.AdvancePhase();
            yield break;
        }

        _endDiscardButton?.gameObject.SetActive(true);

        while (discardCount > 0 && _gvm.CurrentPlayer.HasHandCards() &&
               !_gvm.IsGameOver && _gvm.CurrentPhase == PhaseType.Discard)
        {
            RefreshHandCards();

            // 等待玩家点击卡牌弃置（简化处理）
            yield return null;
        }

        _endDiscardButton?.gameObject.SetActive(false);

        if (!_gvm.IsGameOver)
            _gvm.AdvancePhase(); // Discard → End
    }

    // ==================== UI 刷新 ====================

    private void RefreshAll()
    {
        _player1Info?.Refresh();
        _player2Info?.Refresh();

        if (_turnText != null)
            _turnText.text = $"回合 {_gvm.TurnCount}";

        if (_phaseText != null)
            _phaseText.text = GameViewModel.PhaseName(_gvm.CurrentPhase);

        // 高亮当前回合玩家
        _player1Info?.SetTurnHighlight(_gvm.CurrentPlayer?.PlayerId == 0);
        _player2Info?.SetTurnHighlight(_gvm.CurrentPlayer?.PlayerId == 1);
    }

    private void RefreshHandCards()
    {
        // 清理旧控件
        foreach (var w in _handCardWidgets)
        {
            if (w != null) Destroy(w.gameObject);
        }
        _handCardWidgets.Clear();
        _selectedCard = null;

        if (_handCardsParent == null || _cardPrefab == null) return;

        var cardVMs = _gvm.GetCurrentPlayerCardVMs();
        for (int i = 0; i < cardVMs.Count; i++)
        {
            var cvm = cardVMs[i];
            var go = Instantiate(_cardPrefab, _handCardsParent);
            var widget = go.GetComponent<CardWidget>();
            if (widget != null)
            {
                widget.Bind(cvm, OnCardClicked);
                _handCardWidgets.Add(widget);
            }
        }

        // TODO: 弧形排列手牌（风扇布局）
    }

    // ==================== 日志 ====================

    private void OnLog(string msg)
    {
        _logLines.Add(msg);
        while (_logLines.Count > _maxLogLines)
            _logLines.RemoveAt(0);

        if (_logText != null)
            _logText.text = string.Join("\n", _logLines);

        // 自动滚到底部
        Canvas.ForceUpdateCanvases();
        _logScroll!.verticalNormalizedPosition = 0f;
    }

    // ==================== 阶段变化 ====================

    private void OnPhaseChanged(PhaseType phase)
    {
        if (_phaseText != null)
            _phaseText.text = GameViewModel.PhaseName(phase);

        // 清除选中状态
        _selectedCard?.SetSelected(false);
        _selectedCard = null;
    }

    // ==================== 游戏结束 ====================

    private void OnGameOver(Player winner)
    {
        StopAllCoroutines();

        string result = winner != null
            ? $"{winner.DisplayName}（{winner.CharacterName}）获胜！"
            : "平局！";

        OnLog($"══════════════════════");
        OnLog($"  {result}");
        OnLog($"══════════════════════");
    }

    // ==================== 武将选择 ====================

    public void OnCharacterSelected(int charId)
    {
        // 供 UI 按钮调用
        OnLog($"选择了武将 {charId}");
    }
}
