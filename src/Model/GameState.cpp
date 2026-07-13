#include "GameState.h"
#include "Player.h"
#include <algorithm>

GameState::GameState(QObject* parent)
    : QObject(parent)
{
}

PhaseType GameState::currentPhase() const { return m_currentPhase; }

void GameState::setCurrentPhase(PhaseType phase)
{
    if (m_currentPhase != phase) {
        m_currentPhase = phase;
        emit phaseChanged(phase);
    }
}

int GameState::currentPlayerIndex() const { return m_currentPlayerIndex; }

void GameState::setCurrentPlayerIndex(int index)
{
    if (m_currentPlayerIndex != index && index >= 0 && index < static_cast<int>(m_players.size())) {
        m_currentPlayerIndex = index;
        emit currentPlayerChanged(index);
    }
}

Player* GameState::currentPlayer() const
{
    if (m_currentPlayerIndex >= 0 && m_currentPlayerIndex < static_cast<int>(m_players.size())) {
        return m_players[m_currentPlayerIndex];
    }
    return nullptr;
}

Player* GameState::player(int index) const
{
    if (index >= 0 && index < static_cast<int>(m_players.size())) {
        return m_players[index];
    }
    return nullptr;
}

int GameState::playerCount() const { return m_players.size(); }

void GameState::addPlayer(Player* player)
{
    if (player) {
        if (std::find(m_players.begin(), m_players.end(), player) == m_players.end()) {
            m_players.push_back(player);
        }
    }
}

std::vector<Player*> GameState::alivePlayers() const
{
    std::vector<Player*> alive;
    for (Player* p : m_players) {
        if (p && p->isAlive()) alive.push_back(p);
    }
    return alive;
}

std::vector<Player*> GameState::allPlayers() const { return m_players; }

int GameState::turnCount() const { return m_turnCount; }

void GameState::incrementTurn() { m_turnCount++; }

bool GameState::hasPendingAction() const { return m_hasPendingAction; }

const PendingActionInfo& GameState::pendingActionInfo() const { return m_pendingAction; }

void GameState::setPendingAction(const PendingActionInfo& info)
{
    m_pendingAction = info;
    m_hasPendingAction = true;
    emit pendingActionCreated(info);
}

void GameState::clearPendingAction()
{
    m_hasPendingAction = false;
    m_pendingAction = PendingActionInfo();
    emit pendingActionCleared();
}

CardManager* GameState::cardManager() const { return m_cardManager; }

void GameState::setCardManager(CardManager* mgr) { m_cardManager = mgr; }

bool GameState::isGameOver() const { return m_gameOver; }

Player* GameState::winner() const { return m_winner; }

void GameState::setGameOver(Player* winnerPlayer)
{
    m_gameOver = true;
    m_winner = winnerPlayer;
    emit gameOver(winnerPlayer ? winnerPlayer->playerId() : -1);
}
