using System.Collections.Generic;
using System;

/// <summary>
/// 游戏 ViewModel — 中央协调器，管理游戏生命周期、阶段状态机、回合流转
/// </summary>
public class GameViewModel
{
    // ==================== 构造 / 析构 ====================

    public GameViewModel()
    {
        _state = new GameState();
        _cardManager = new CardManager();
        _actionVM = new ActionViewModel();

        _state.CardManager = _cardManager;
        _actionVM.SetGameState(_state);
    }

    /// <summary>断开所有 Model 事件</summary>
    public void Dispose()
    {
        if (_state != null)
        {
            _state.PhaseChanged.Disconnect(_connIds.phaseChangedId);
            _state.CurrentPlayerChanged.Disconnect(_connIds.currentPlayerChangedId);
            _state.PendingActionCreated.Disconnect(_connIds.pendingActionCreatedId);
            _state.PendingActionCleared.Disconnect(_connIds.pendingActionClearedId);
            _state.GameOver.Disconnect(_connIds.gameOverId);
            _state.StateRefreshed.Disconnect(_connIds.stateRefreshedId);
        }
    }

    // ==================== 游戏生命周期 ====================

    /// <summary>开始游戏（characterId: 0=曹操, 1=关羽, 2=张飞, 3=赵云）</summary>
    public void StartGame(int characterId1, int characterId2)
    {
        Character char1 = CreateCharacterById(characterId1);
        Character char2 = CreateCharacterById(characterId2);
        if (char1 == null || char2 == null) return;

        _characters.Add(char1);
        _characters.Add(char2);

        InitGame(char1, char2);
    }

    /// <summary>推进到下个阶段</summary>
    public void AdvancePhase()
    {
        if (_state.IsGameOver()) return;

        PhaseType current = _state.CurrentPhase;

        switch (current)
        {
            case PhaseType.Prepare:
                ExecutePhasePrepare();
                SetNextPhase(PhaseType.Judge);
                break;
            case PhaseType.Judge:
                ExecutePhaseJudge();
                SetNextPhase(PhaseType.Draw);
                break;
            case PhaseType.Draw:
                ExecutePhaseDraw();
                SetNextPhase(PhaseType.Play);
                break;
            case PhaseType.Play:
                SetNextPhase(PhaseType.Discard);
                break;
            case PhaseType.Discard:
                SetNextPhase(PhaseType.End);
                break;
            case PhaseType.End:
                ExecutePhaseEnd();
                break;
        }
    }

    /// <summary>当前玩家结束出牌</summary>
    public void EndPlayPhase()
    {
        if (_state.IsGameOver()) return;
        if (_state.CurrentPhase != PhaseType.Play) return;
        if (_state.HasPendingAction()) return;

        SetNextPhase(PhaseType.Discard);
    }

    // ==================== 状态查询 ====================

    public GameState GameState => _state;
    public Player CurrentPlayer => _state.CurrentPlayer;
    public Player OpponentPlayer
    {
        get
        {
            Player cur = CurrentPlayer;
            if (cur == null) return null;
            foreach (var p in _state.AllPlayers())
            {
                if (p != null && p != cur && p.IsAlive()) return p;
            }
            return null;
        }
    }

    public PhaseType CurrentPhase => _state.CurrentPhase;
    public int TurnCount => _state.TurnCount;
    public bool IsGameOver => _state.IsGameOver();
    public Player Winner => _state.Winner;

    /// <summary>获取阶段的中文名称</summary>
    public static string PhaseName(PhaseType phase)
    {
        return phase switch
        {
            PhaseType.Prepare => "准备阶段",
            PhaseType.Judge => "判定阶段",
            PhaseType.Draw => "摸牌阶段",
            PhaseType.Play => "出牌阶段",
            PhaseType.Discard => "弃牌阶段",
            PhaseType.End => "结束阶段",
            _ => "未知"
        };
    }

    // ==================== 子 ViewModel ====================

    public PlayerViewModel GetPlayerVM(int index)
    {
        if (index >= 0 && index < _playerVMs.Count)
            return _playerVMs[index];
        return null;
    }

    public ActionViewModel ActionVM => _actionVM;

    // ==================== 手牌展示辅助 ====================

    /// <summary>获取当前玩家手牌的 CardViewModel 列表</summary>
    public List<CardViewModel> GetCurrentPlayerCardVMs()
    {
        return GetPlayerCardVMs(CurrentPlayer);
    }

    /// <summary>获取某玩家手牌的 CardViewModel 列表</summary>
    public List<CardViewModel> GetPlayerCardVMs(Player player)
    {
        var result = new List<CardViewModel>();
        if (player == null || _state == null) return result;

        var playable = _actionVM.GetPlayableCards(player);

        foreach (var card in player.HandCards)
        {
            if (card == null) continue;
            var cvm = new CardViewModel(card);

            bool isPlayable = playable.Contains(card);
            cvm.SetPlayable(isPlayable);

            result.Add(cvm);
        }
        return result;
    }

    // ==================== 事件 ====================

    public readonly Signal<PhaseType> PhaseChanged = new();
    public readonly Signal<int> CurrentPlayerChanged = new();
    public readonly Signal<Player> GameOver = new();
    public readonly Signal<string> LogMessage = new();
    public readonly Signal StateChanged = new();

    // ==================== 私有实现 ====================

    private readonly GameState _state;
    private readonly CardManager _cardManager;
    private readonly ActionViewModel _actionVM;
    private readonly List<PlayerViewModel> _playerVMs = new();
    private readonly List<Character> _characters = new();

    // Model 事件连接 ID
    private (int phaseChangedId, int currentPlayerChangedId, int pendingActionCreatedId,
             int pendingActionClearedId, int gameOverId, int stateRefreshedId) _connIds;

    private void InitGame(Character char1, Character char2)
    {
        _cardManager.Initialize();

        var player1 = new Player();
        player1.SetPlayerId(0);
        player1.SetDisplayName("玩家1");
        player1.SetCharacter(char1);

        var player2 = new Player();
        player2.SetPlayerId(1);
        player2.SetDisplayName("玩家2");
        player2.SetCharacter(char2);

        _state.AddPlayer(player1);
        _state.AddPlayer(player2);

        // 发初始手牌
        for (int i = 0; i < GameRule.INITIAL_HAND_COUNT; i++)
        {
            Card c1 = _cardManager.DrawCard();
            Card c2 = _cardManager.DrawCard();
            if (c1 != null) player1.AddHandCard(c1);
            if (c2 != null) player2.AddHandCard(c2);
        }

        _playerVMs.Add(new PlayerViewModel(player1));
        _playerVMs.Add(new PlayerViewModel(player2));

        // 监听 Model 事件
        _connIds.phaseChangedId = _state.PhaseChanged.Connect(p => PhaseChanged.Emit(p));
        _connIds.currentPlayerChangedId = _state.CurrentPlayerChanged.Connect(idx => CurrentPlayerChanged.Emit(idx));
        _connIds.pendingActionCreatedId = _state.PendingActionCreated.Connect(info => OnPendingActionCreated(info));
        _connIds.pendingActionClearedId = _state.PendingActionCleared.Connect(() => StateChanged.Emit());
        _connIds.gameOverId = _state.GameOver.Connect(w => OnGameOver(w));
        _connIds.stateRefreshedId = _state.StateRefreshed.Connect(() => StateChanged.Emit());

        // 监听玩家死亡
        foreach (var p in _state.AllPlayers())
        {
            if (p != null)
                p.Died.Connect(dead => OnPlayerDied(dead));
        }

        // 开始第一个回合
        _state.SetCurrentPhase(PhaseType.Prepare);
        EmitLog($"游戏开始！{player1.DisplayName}（{player1.CharacterName}）vs {player2.DisplayName}（{player2.CharacterName}）");
    }

    private Character CreateCharacterById(int id)
    {
        return id switch
        {
            0 => new CaoCao(),
            1 => new GuanYu(),
            2 => new ZhangFei(),
            3 => new ZhaoYun(),
            _ => null
        };
    }

    // ==================== 阶段执行 ====================

    private void ExecutePhasePrepare()
    {
        Player player = CurrentPlayer;
        if (player == null) return;

        player.ResetTurnState();

        if (player.Character != null &&
            player.Character.TriggerCondition(GameEventType.OnTurnStart, _state, player))
        {
            player.Character.TriggerSkill(_state, player);
            EmitLog($"{player.DisplayName} 触发了技能【{player.Character.SkillName}】");
        }
    }

    private void ExecutePhaseJudge()
    {
        // 简单版：没有延时锦囊，跳过
    }

    private void ExecutePhaseDraw()
    {
        Player player = CurrentPlayer;
        if (player == null) return;

        var cards = _cardManager.DrawCards(GameRule.DRAW_PHASE_COUNT);
        foreach (var card in cards)
        {
            if (card != null) player.AddHandCard(card);
        }

        if (player.Character != null &&
            player.Character.TriggerCondition(GameEventType.OnDrawPhase, _state, player))
        {
            player.Character.TriggerSkill(_state, player);
        }

        EmitLog($"{player.DisplayName} 摸了 {cards.Count} 张牌");
    }

    private void ExecutePhaseEnd()
    {
        Player player = CurrentPlayer;
        if (player == null) return;

        EmitLog($"{player.DisplayName} 结束回合");
        SwitchToNextPlayer();
    }

    // ==================== 辅助 ====================

    private void SetNextPhase(PhaseType phase)
    {
        _state.SetCurrentPhase(phase);
        EmitLog($"→ {PhaseName(phase)}");
        StateChanged.Emit();
    }

    private void SwitchToNextPlayer()
    {
        if (_state.IsGameOver()) return;

        int count = _state.PlayerCount;
        int current = _state.CurrentPlayerIndex;
        for (int i = 1; i <= count; i++)
        {
            int nextIdx = (current + i) % count;
            Player next = _state.GetPlayer(nextIdx);
            if (next != null && next.IsAlive())
            {
                _state.SetCurrentPlayerIndex(nextIdx);
                _state.IncrementTurn();
                SetNextPhase(PhaseType.Prepare);
                EmitLog($"--- 轮到 {next.DisplayName}（{next.CharacterName}）的回合 ---");
                return;
            }
        }
    }

    private void EmitLog(string msg)
    {
        LogMessage.Emit(msg);
    }

    // ==================== Model 事件回调 ====================

    private void OnPendingActionCreated(PendingActionInfo info)
    {
        EmitLog(info.Description);
        StateChanged.Emit();
    }

    private void OnGameOver(Player winner)
    {
        if (winner != null)
            EmitLog($"游戏结束！{winner.DisplayName}（{winner.CharacterName}）获胜！");
        else
            EmitLog("游戏结束！平局！");

        GameOver.Emit(winner);
    }

    private void OnPlayerDied(Player player)
    {
        if (player != null)
            EmitLog($"{player.DisplayName}（{player.CharacterName}）阵亡！");
    }
}
