#include "../include/gto/action_abstraction.h"
#include "../include/gto/game_state.h"
#include "spdlog/spdlog.h"
#include <vector>
#include <cmath>     
#include <algorithm> 
#include <stdexcept>

const double BIG_BLIND_SIZE = 2.0; // Décommenté - Placeholder à déplacer !

namespace gto_solver {

// Constructeur (placeholder)
ActionAbstraction::ActionAbstraction() {
    // Initialisation si nécessaire
}

// Destructeur (placeholder)
ActionAbstraction::~ActionAbstraction() {
    // Nettoyage si nécessaire
}

// Méthode pour obtenir les actions possibles (placeholder - signature basée sur le test)
std::vector<ActionSpec> ActionAbstraction::get_possible_action_specs(const GameState& current_state) const {
    // Implémentation placeholder - DOIT être remplacée par la vraie logique
    spdlog::warn("ActionAbstraction::get_possible_action_specs - Placeholder implementation");
    std::vector<ActionSpec> specs;
    // Exemple: Ajouter un check/fold
    specs.push_back({ActionType::FOLD, SizingUnit::NONE, 0.0});
    specs.push_back({ActionType::CALL, SizingUnit::NONE, 0.0}); // Renommé depuis CHECK_CALL
    // Ajouter un exemple de raise
    specs.push_back({ActionType::RAISE, SizingUnit::MULTIPLIER_X, 3.0});
    return specs;
}

// Méthode pour calculer le montant de l'action (avec le patch appliqué)
int ActionAbstraction::get_action_amount(const ActionSpec& action_spec, const GameState& current_state) const {
    // Récupérer les infos nécessaires depuis GameState
    int current_player_index = current_state.get_current_player();
    int player_stack = current_state.get_player_stack(current_player_index);
    int current_bet = current_state.get_current_bets()[current_player_index];
    int max_bet = 0;
    for(int bet : current_state.get_current_bets()) {
        max_bet = std::max(max_bet, bet);
    }
    int amount_to_call = max_bet - current_bet;

    switch (action_spec.type) {
        case ActionType::FOLD:
            return 0; // Fold n'a pas de montant

        case ActionType::CALL: // Renommé depuis CHECK_CALL
            // Si c'est un check (amount_to_call == 0), retourne 0?
            // Non, l'action CALL représente toujours le montant *supplémentaire* à mettre
            // Si amount_to_call == 0, c'est un check, on mise 0 de plus.
            // Si amount_to_call > 0, c'est un call, on mise amount_to_call de plus (jusqu'à all-in).
            return std::min(player_stack, amount_to_call);

        case ActionType::RAISE: {
            int raise_base_amount = current_bet + amount_to_call; // Montant total après simple call (== max_bet)
            int target_total_bet = -1;

            if (action_spec.unit == SizingUnit::BB) {
                target_total_bet = static_cast<int>(std::round(action_spec.value * BIG_BLIND_SIZE));
            } else if (action_spec.unit == SizingUnit::PCT_POT) {
                int pot_after_call = current_state.get_pot_size();
                // Note: get_pot_size() inclut-il déjà current_bets? A vérifier.
                // Supposons que non, et qu'il faut ajouter les mises du tour courant.
                // Le calcul exact dépend de la définition de get_pot_size()
                // Ce calcul suppose que get_pot_size() est le pot *avant* les mises du tour courant
                int current_round_bets = 0;
                 for(int bet : current_state.get_current_bets()) {
                    current_round_bets += bet;
                }
                // Pot si tout le monde call la mise actuelle (max_bet)
                int pot_if_called = current_state.get_pot_size() + (current_state.get_num_active_players() * max_bet - current_round_bets);
                // Le calcul original de l'utilisateur était différent, revoyons-le:
                // int pot_after_call = current_state.get_pot_size();
                // for (int bet : current_state.get_current_bets()) pot_after_call += bet;
                // pot_after_call += amount_to_call; // Pot si ce joueur call
                // Utilisons la logique de l'utilisateur:
                int user_pot_after_call = current_state.get_pot_size() + current_round_bets + amount_to_call;

                target_total_bet = raise_base_amount
                                 + static_cast<int>(std::round(user_pot_after_call * (action_spec.value / 100.0)));
            } else if (action_spec.unit == SizingUnit::MULTIPLIER_X) { // <-- Patch appliqué ici
                target_total_bet = static_cast<int>(std::round(raise_base_amount * action_spec.value));
            } else if (action_spec.unit == SizingUnit::ALL_IN) {
                 target_total_bet = player_stack + current_bet; // Le joueur mise tout son stack
            } else {
                spdlog::error("Unsupported unit for RAISE action: {}", static_cast<int>(action_spec.unit));
                return -1; // Erreur
            }

            // Appliquer la relance minimale
            int min_raise_amount = std::max((int)BIG_BLIND_SIZE, current_state.get_last_raise_size());
            int min_raise_total = max_bet + min_raise_amount; // Mise totale minimale pour une relance valide
            target_total_bet = std::max(target_total_bet, min_raise_total);

            // Ne pas dépasser le tapis (le montant retourné est la mise totale)
            int final_bet_amount = std::min(player_stack + current_bet, target_total_bet);

            // Gérer le cas où le raise minimum force un all-in inférieur au min-raise normal
            if (final_bet_amount == player_stack + current_bet && final_bet_amount < min_raise_total) {
                // C'est un all-in, c'est valide même si < min_raise_total
            } else {
                final_bet_amount = std::max(final_bet_amount, min_raise_total);
            }
            
            // Assurer qu'on ne mise pas moins que le call
            final_bet_amount = std::max(final_bet_amount, max_bet); 

            return final_bet_amount;
        }

        default:
            spdlog::error("Unsupported action type");
            return -1;
    }
}

// Ajoutez ici d'autres méthodes de ActionAbstraction si nécessaire

} // namespace gto_solver 