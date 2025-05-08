#include "gto/information_set.h"
#include "core/cards.hpp"      // Pour Card et to_string(Card)
#include "gto/game_utils.hpp"  // Pour street_to_string
#include <algorithm> // Pour std::max, std::transform
#include <numeric>   // Pour std::accumulate, std::iota
#include <iomanip>   // Pour std::setprecision, std::fixed
#include <sstream>

namespace gto_solver {

void InformationSet::initialize(size_t num_actions) {
    if (num_actions == 0) {
        // Peut arriver pour un nœud terminal ou si aucune action n'est légale (rare)
        // spdlog ici si on en ajoute un logger
        return;
    }
    cumulative_regrets.assign(num_actions, 0.0);
    cumulative_strategy.assign(num_actions, 0.0);
    visit_count = 0;
}

std::vector<double> InformationSet::get_current_strategy() const {
    std::vector<double> strategy(cumulative_regrets.size());
    double sum_positive_regrets = 0.0;

    for (double regret : cumulative_regrets) {
        sum_positive_regrets += std::max(0.0, regret);
    }

    if (sum_positive_regrets > 0.0) {
        for (size_t i = 0; i < cumulative_regrets.size(); ++i) {
            strategy[i] = std::max(0.0, cumulative_regrets[i]) / sum_positive_regrets;
        }
    } else {
        // Si tous les regrets sont nuls ou négatifs, jouer uniformément.
        // Cela arrive souvent au début ou si une action domine largement les autres.
        double uniform_prob = (cumulative_regrets.empty()) ? 0.0 : (1.0 / cumulative_regrets.size());
        std::fill(strategy.begin(), strategy.end(), uniform_prob);
    }
    return strategy;
}

// node_value est la EV de l'état courant (infoset) si on suit la stratégie actuelle.
void InformationSet::update_regrets(const std::vector<double>& action_values, double node_value) {
    if (action_values.size() != cumulative_regrets.size()) {
        // Gérer l'erreur : tailles incohérentes. Peut-être lancer une exception.
        // Ou logguer une erreur sévère.
        // Pour l'instant, on suppose que les tailles correspondent.
        // Exemple: throw std::logic_error("Mismatch action_values and cumulative_regrets size");
        return; 
    }

    for (size_t i = 0; i < action_values.size(); ++i) {
        cumulative_regrets[i] += (action_values[i] - node_value);
    }
}

void InformationSet::update_strategy_sum(const std::vector<double>& current_strategy_profile) {
    if (current_strategy_profile.size() != cumulative_strategy.size()) {
        // Gérer l'erreur
        return;
    }
    for (size_t i = 0; i < current_strategy_profile.size(); ++i) {
        cumulative_strategy[i] += current_strategy_profile[i];
    }
    visit_count++; // On pourrait aussi passer player_reach_prob et l'ajouter ici.
}

// Génération de la clé d'infoset
// Format: "[P<player_idx>]<HoleCards>|<BoardCards>|<Street>|<ActionHistory>"
std::string InformationSet::generate_key(
    int player_index,
    const std::vector<Card>& hole_cards, 
    const std::array<Card, 5>& board, 
    int board_cards_dealt,
    Street current_street,
    const std::vector<Action>& action_history
) {
    std::stringstream ss;
    ss << "P" << player_index << ";";

    // Hole Cards (triées pour la canonicité)
    std::vector<Card> sorted_hole_cards = hole_cards;
    std::sort(sorted_hole_cards.begin(), sorted_hole_cards.end()); // Trier par index de carte
    for (size_t i = 0; i < sorted_hole_cards.size(); ++i) {
        ss << gto_solver::to_string(sorted_hole_cards[i]) << (i == sorted_hole_cards.size() - 1 ? "" : "-");
    }
    ss << "|";

    // Board Cards (triées pour la canonicité si board_cards_dealt > 0)
    if (board_cards_dealt > 0) {
        std::vector<Card> dealt_board_cards;
        for (int i = 0; i < board_cards_dealt; ++i) {
            if (board[i] != INVALID_CARD) {
                dealt_board_cards.push_back(board[i]);
            }
        }
        std::sort(dealt_board_cards.begin(), dealt_board_cards.end());
        for (size_t i = 0; i < dealt_board_cards.size(); ++i) {
            ss << gto_solver::to_string(dealt_board_cards[i]) << (i == dealt_board_cards.size() - 1 ? "" : "-");
        }
    }
    ss << "|";

    // Street
    ss << street_to_string(current_street) << "|";

    // Action History
    // Chaque action: "<P_idx><Type><Amt>,"
    // Exemple: P0RAISE10,P1CALL10,
    // Pour CALL, Amt est le montant *total* misé par le joueur après son call.
    // Pour RAISE, Amt est le montant *total* misé par le joueur après sa relance.
    // Pour FOLD, Amt est 0.
    for (const auto& action : action_history) {
        ss << "A" << action.player_index;
        switch (action.type) {
            case ActionType::FOLD: ss << "F"; break;
            case ActionType::CALL: ss << "C"; break;
            case ActionType::RAISE: ss << "R"; break;
        }
        ss << action.amount << ",";
    }

    return ss.str();
}


} // namespace gto_solver 