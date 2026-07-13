#include "GameState.h"
#include "Player.h"
#include <algorithm>

GameState::GameState()
{
}

PhaseType GameState::currentPhase() const
{
    return m_currentPhase;
}

void GameState::setCurrentPhase(PhaseType phase)
{
    if (m_currentPhase != phase) {
        m_currentPhase = phase;
        phaseChanged.notify(phase);
    }
}

int GameState::currentPlayerIndex() const
{
    return m_currentPlayerIndex;
}

void GameState::setCurrentPlayerIndex(int index)
{
    if (m_currentPlayerIndex != index && index >= 0 && index < static_cast<int>(m_players.size())) {
        m_currentPlayerIndex = index;
        currentPlayerChanged.notify(index);
    }
}

Player* GameState::currentPlayer() const
{
    if (m_currentPlayerIndex >= 0 && m_currentPlayerIndex < static_cast<int>(m_players.size())) {
        return m_players[m_currentPlayerIndex].get();
    }
    return nullptr;
}

Player* GameState::player(int index) const
{
    if (index >= 0 && index < static_cast<int>(m_players.size())) {
        return m_players[index].get();
    }
    return nullptr;
}

int GameState::playerCount() const
{
    return static_cast<int>(m_players.size());
}

void GameState::addPlayer(std::unique_ptr<Player> player)
{
    if (player) {
        // 去重（按原始指针查找）
        Player* raw = player.get();
        auto it = std::find_if(m_players.begin(), m_players.end(),
            [raw](const std::unique_ptr<Player>& p) { return p.get() == raw; });
        if (it == m_players.end()) {
            m_players.push_back(std::move(player));
        }
    }
}

std::vector<Player*> GameState::alivePlayers() const
{
    std::vector<Player*> alive;
    for (const auto& p : m_players) {
        if (p && p->isAlive()) {
            alive.push_back(p.get());
        }
    }
    return alive;
}

std::vector<Player*> GameState::allPlayers() const
{
    std::vector<Player*> result;
    result.reserve(m_players.size());
    for (const auto& p : m_players) {
        result.push_back(p.get());
    }
    return result;
}

int GameState::turnCount() const
{
    return m_turnCount;
}

void GameState::incrementTurn()
{
    m_turnCount++;
}

bool GameState::hasPendingAction() const
{
    return m_hasPendingAction;
}

const PendingActionInfo& GameState::pendingActionInfo() const
{
    return m_pendingAction;
}

void GameState::setPendingAction(const PendingActionInfo& info)
{
    m_pendingAction = info;
    m_hasPendingAction = true;
    pendingActionCreated.notify(info);
}

void GameState::clearPendingAction()
{
    m_hasPendingAction = false;
    m_pendingAction = PendingActionInfo();
    pendingActionCleared.notify();
}

CardManager* GameState::cardManager() const
{
    return m_cardManager;
}

void GameState::setCardManager(CardManager* mgr)
{
    m_cardManager = mgr;
}

bool GameState::isGameOver() const
{
    return m_gameOver;
}

Player* GameState::winner() const
{
    return m_winner;
}

void GameState::setGameOver(Player* winnerPlayer)
{
    m_gameOver = true;
    m_winner = winnerPlayer;
    gameOver.notify(winnerPlayer);
}
