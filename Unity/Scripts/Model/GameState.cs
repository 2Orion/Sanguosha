using System.Collections.Generic;

/// <summary>
/// 游戏核心状态 — 管理阶段、玩家列表、回合追踪、待定动作
/// </summary>
public class GameState
{
    private PhaseType _currentPhase = PhaseType.Prepare;
    private int _currentPlayerIndex = 0;
    private int _turnCount = 0;
    private readonly List<Player> _players = new();
    private CardManager _cardManager;
    private bool _hasPendingAction = false;
    private PendingActionInfo _pendingAction = new();
    private bool _gameOver = false;
    private Player _winner;

    // ---- 阶段管理 ----

    public PhaseType CurrentPhase => _currentPhase;

    public void SetCurrentPhase(PhaseType phase)
    {
        if (_currentPhase != phase)
        {
            _currentPhase = phase;
            PhaseChanged?.Emit(phase);
        }
    }

    // ---- 玩家管理 ----

    public int CurrentPlayerIndex => _currentPlayerIndex;

    public void SetCurrentPlayerIndex(int index)
    {
        if (_currentPlayerIndex != index && index >= 0 && index < _players.Count)
        {
            _currentPlayerIndex = index;
            CurrentPlayerChanged?.Emit(index);
        }
    }

    public Player CurrentPlayer
    {
        get
        {
            if (_currentPlayerIndex >= 0 && _currentPlayerIndex < _players.Count)
                return _players[_currentPlayerIndex];
            return null;
        }
    }

    public Player GetPlayer(int index)
    {
        if (index >= 0 && index < _players.Count)
            return _players[index];
        return null;
    }

    public int PlayerIndex(Player player)
    {
        return _players.IndexOf(player);
    }

    public int PlayerCount => _players.Count;

    public void AddPlayer(Player player)
    {
        if (player != null && !_players.Contains(player))
            _players.Add(player);
    }

    public List<Player> AlivePlayers()
    {
        var alive = new List<Player>();
        foreach (var p in _players)
        {
            if (p != null && p.IsAlive())
                alive.Add(p);
        }
        return alive;
    }

    public List<Player> AllPlayers() => new List<Player>(_players);

    // ---- 回合追踪 ----

    public int TurnCount => _turnCount;
    public void IncrementTurn() { _turnCount++; }

    // ---- 待定动作 ----

    public bool HasPendingAction() => _hasPendingAction;

    public PendingActionInfo PendingAction => _pendingAction;

    public void SetPendingAction(PendingActionInfo info)
    {
        _pendingAction = info;
        _hasPendingAction = true;
        PendingActionCreated?.Emit(info);
    }

    public void ClearPendingAction()
    {
        _hasPendingAction = false;
        _pendingAction = new PendingActionInfo();
        PendingActionCleared?.Emit();
    }

    // ---- 牌堆管理器 ----

    public CardManager CardManager
    {
        get => _cardManager;
        set => _cardManager = value;
    }

    // ---- 游戏结束 ----

    public bool IsGameOver() => _gameOver;
    public Player Winner => _winner;

    public void SetGameOver(Player winnerPlayer)
    {
        _gameOver = true;
        _winner = winnerPlayer;
        GameOver?.Emit(winnerPlayer);
    }

    // ---- 事件通知 ----

    public readonly Signal<PhaseType> PhaseChanged = new();
    public readonly Signal<int> CurrentPlayerChanged = new();
    public readonly Signal<PendingActionInfo> PendingActionCreated = new();
    public readonly Signal PendingActionCleared = new();
    public readonly Signal<Player> GameOver = new();
    public readonly Signal StateRefreshed = new();
}
