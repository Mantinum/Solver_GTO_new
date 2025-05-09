#include "gto/action_abstraction.h"
#include "gto/game_state.h" // Nécessaire pour accéder à l'état du jeu
#include "spdlog/spdlog.h"
#include "gto/game_utils.hpp" // <-- AJOUT pour street_to_string
#include <algorithm> // Pour std::min/max
#include <cmath>     // Pour std::round
#include <set>       // Pour éviter doublons dans les tailles de raise
#include <stdexcept> // Inclus via header mais bonne pratique de l'inclure si utilisé implicitement

namespace gto_solver {

// Constantes utiles (pourraient être globales ou membres statiques)
// Valeur de la Big Blind, utile pour définir la relance minimale absolue.
// TODO: Rendre cette valeur configurable ou la récupérer depuis GameState/Config.
// static constexpr int BB_SIZE_FOR_MIN_RAISE = 2; // Remplacé par state.get_big_blind_size()

ActionAbstraction::ActionAbstraction(
    bool allow_fold,
    bool allow_check_call,
    const StreetFractionsMap& fractions_by_street,
    const StreetBBSizesMap& bb_sizes_by_street,
    const StreetExactBetsMap& exact_bets_by_street,
    bool allow_all_in)
    : allow_fold_(allow_fold),
      allow_check_call_(allow_check_call),
      fractions_by_street_(fractions_by_street),
      bb_sizes_by_street_(bb_sizes_by_street),
      exact_bets_by_street_(exact_bets_by_street),
      allow_all_in_(allow_all_in)
{
    // Validation pour fractions_by_street_
    for(const auto& pair : fractions_by_street_) {
        const Street street = pair.first;
        const std::set<double>& fractions = pair.second;
        for(double frac : fractions) {
            if (frac <= 0.0) {
                 spdlog::warn("ActionAbstraction: Pour street {}, raise fraction {} <= 0 sera ignorée.", 
                              street_to_string(street), // Assumer street_to_string existe
                              frac);
            }
        }
    }
    // Validation pour bb_sizes_by_street_
    for(const auto& pair : bb_sizes_by_street_) {
        const Street street = pair.first;
        const std::set<double>& bb_sizes = pair.second;
        for(double bb_size_multiplier : bb_sizes) {
            if (bb_size_multiplier <= 0.0) {
                 spdlog::warn("ActionAbstraction: Pour street {}, multiplicateur BB {} <= 0 sera ignoré.", 
                              street_to_string(street), bb_size_multiplier);
            }
        }
    }
    for (const auto& pair : bb_sizes_by_street_) {
        for (double bb_mult : pair.second) {
            if (bb_mult <= 0) {
                spdlog::warn("ActionAbstraction: BB multiplier must be positive: {}", bb_mult);
                // On pourrait lancer une exception ici si on veut être plus strict
            }
        }
    }
    for (const auto& pair : exact_bets_by_street_) {
        for (int exact_bet : pair.second) {
            if (exact_bet <= 0) {
                spdlog::warn("ActionAbstraction: Exact bet amount must be positive: {}", exact_bet);
                // On pourrait lancer une exception ici si on veut être plus strict
            }
        }
    }

    // Vérifier si TOUTES les streets ont des fractions vides ET que all-in est interdit
    bool any_raise_option = allow_all_in_;
    if (!any_raise_option) {
        for(const auto& pair : fractions_by_street_) {
            if (!pair.second.empty()) {
                any_raise_option = true;
                break;
            }
        }
    }
    if (!any_raise_option) {
        for(const auto& pair : bb_sizes_by_street_) {
            if (!pair.second.empty()) { any_raise_option = true; break; }
        }
    }
    if (!any_raise_option) {
         spdlog::warn("ActionAbstraction: Aucune taille de relance spécifiée (ni fractions, ni tailles BB, ni all-in).");
    }
}

std::vector<Action> ActionAbstraction::get_abstract_actions(const GameState& state) const {
    std::vector<Action> actions;
    int current_player = state.get_current_player();

    // Si la partie est terminée ou si le joueur n'est pas valide
    if (current_player < 0 || current_player >= state.get_num_players()) {
        spdlog::trace("ActionAbstraction::get_abstract_actions: Pas d'actions car joueur courant invalide ({}) ou partie terminée.", current_player);
        return actions;
    }

    // Si le joueur est foldé ou n'a plus de stack (et pas de mise devant lui ?) -> il ne peut rien faire
    // Note: Un joueur all-in ne devrait pas être interrogé pour une action. GameState gère ça ?
     if (state.is_player_folded(current_player)) {
         spdlog::trace("ActionAbstraction::get_abstract_actions: Pas d'actions pour P{} car déjà foldé.", current_player);
         return actions;
     }
     // Attention: un joueur avec stack 0 mais une mise devant lui DOIT pouvoir agir si relancé.
     // La logique dans les helpers devrait gérer ça (ex: peut seulement call 0 ou fold).


    // 1. Action FOLD (si autorisée)
    add_fold_action(actions, state);

    // 2. Action CHECK/CALL (si autorisée)
    add_check_call_action(actions, state);

    // 3. Actions RAISE (si autorisées et possibles)
    add_raise_actions(actions, state);

    // Validation finale : au moins une action devrait être possible si le joueur n'est pas all-in ou foldé
    // (au pire, check ou fold)
    if (actions.empty() && state.get_player_stack(current_player) > 0) {
         spdlog::warn("ActionAbstraction::get_abstract_actions: Aucune action générée pour P{} actif avec stack > 0. State:\n{}", current_player, state.toString());
         // On pourrait forcer un check/fold ici comme fallback ?
         // Par sécurité, ajoutons au moins un check/call si possible
         if (allow_check_call_) {
             add_check_call_action(actions, state); // Réessayer au cas où
         }
         if(actions.empty() && allow_fold_) {
             int max_bet = 0; for (int b : state.get_current_bets()) max_bet = std::max(max_bet, b);
             if(state.get_current_bets()[current_player] < max_bet) { // Peut folder
                add_fold_action(actions, state);
             }
         }
         if (actions.empty()) {
             spdlog::error("ActionAbstraction::get_abstract_actions: Toujours aucune action pour P{} actif! Forcing FOLD.", current_player);
             actions.push_back({current_player, ActionType::FOLD, 0}); // Fallback ultime
         }
    }


    spdlog::trace("ActionAbstraction::get_abstract_actions: Actions générées pour P{}: {}", current_player, actions.size());
    return actions;
}

// --- Méthodes Helper Privées ---

void ActionAbstraction::add_fold_action(std::vector<Action>& actions, const GameState& state) const {
    if (!allow_fold_) return;

    int current_player = state.get_current_player();
    int player_bet = state.get_current_bets().at(current_player); // Utiliser .at() pour bounds checking
    int max_bet = 0;
    for (int bet : state.get_current_bets()) {
        max_bet = std::max(max_bet, bet);
    }

    // On ne peut folder que s'il y a une mise à suivre (max_bet > player_bet)
    // Si max_bet == player_bet, l'action légale est CHECK (géré par add_check_call_action)
    if (player_bet < max_bet) {
        actions.push_back({current_player, ActionType::FOLD, 0}); // Montant 0 pour FOLD
        spdlog::trace("ActionAbstraction: FOLD ajouté pour P{}", current_player);
    }
}

void ActionAbstraction::add_check_call_action(std::vector<Action>& actions, const GameState& state) const {
    if (!allow_check_call_) return;

    int current_player = state.get_current_player();
    int player_stack = state.get_player_stack(current_player);
    int player_bet = state.get_current_bets().at(current_player);
    int max_bet = 0;
    for (int bet : state.get_current_bets()) {
        max_bet = std::max(max_bet, bet);
    }
    int amount_to_call = max_bet - player_bet;

    // Vérifier si le joueur PEUT payer le call (même si c'est all-in)
    // Note: amount_to_call peut être 0 (check)
    if (amount_to_call < 0) {
        // Ne devrait jamais arriver si la logique de GameState est correcte
        spdlog::error("ActionAbstraction::add_check_call_action: amount_to_call négatif ({}) pour P{}. State:\n{}", amount_to_call, current_player, state.toString());
        amount_to_call = 0; // Sécurité
    }

    // L'action est possible même si stack == 0, si amount_to_call == 0 (check)
    // Si amount_to_call > 0, il faut avoir du stack pour call (même si c'est pour call all-in)
    if (amount_to_call == 0 || player_stack > 0) {
        // Montant réellement ajouté au pot par cette action
        int call_amount_added = std::min(player_stack, amount_to_call);
        // Montant total misé par le joueur APRES cette action
        int total_bet_after_action = player_bet + call_amount_added;

        actions.push_back({current_player, ActionType::CALL, total_bet_after_action});

        if (amount_to_call == 0) {
            spdlog::trace("ActionAbstraction: CHECK (Call 0 -> total bet {}) ajouté pour P{}", total_bet_after_action, current_player);
        } else {
             if (call_amount_added == player_stack && call_amount_added < amount_to_call) {
                 spdlog::trace("ActionAbstraction: CALL All-in {} (total bet {}) ajouté pour P{}", call_amount_added, total_bet_after_action, current_player);
             } else {
                 spdlog::trace("ActionAbstraction: CALL {} (total bet {}) ajouté pour P{}", call_amount_added, total_bet_after_action, current_player);
             }
        }
    } else {
         spdlog::trace("ActionAbstraction: Cannot CHECK/CALL for P{} (stack={}, amount_to_call={})", current_player, player_stack, amount_to_call);
    }
}

void ActionAbstraction::add_raise_actions(std::vector<Action>& actions, const GameState& state) const {
    int current_player = state.get_current_player();
    int player_stack = state.get_player_stack(current_player);

    // On ne peut pas relancer si on n'a pas de stack
    if (player_stack <= 0) {
        return;
    }

    int player_bet = state.get_current_bets().at(current_player);
    int pot_size = state.get_pot_size(); // Pot *avant* l'action du joueur courant
    int last_raise_size = state.get_last_raise_size(); // Taille de l'incrément de la *dernière* relance
    int max_bet = 0; // Mise la plus haute sur la table actuellement
    for (int bet : state.get_current_bets()) {
        max_bet = std::max(max_bet, bet);
    }
    int amount_to_call = max_bet - player_bet;

    // Stack effectif pour la relance = ce qui reste APRES avoir égalisé la mise max
    int effective_stack_for_raise = player_stack - amount_to_call;
    // On ne peut relancer que si on a quelque chose à ajouter APRÈS avoir call
    if (effective_stack_for_raise <= 0) {
         spdlog::trace("ActionAbstraction: Cannot RAISE for P{} (stack={}, amount_to_call={}), no effective stack.", current_player, player_stack, amount_to_call);
        return;
    }

    // Pot size *après* que le joueur courant ait call (base pour les sizings pot%)
    // Le pot contient déjà les mises précédentes. Il faut ajouter le call du joueur courant.
    int pot_if_player_calls = pot_size + amount_to_call;

    // Calcul du min raise (selon les règles du poker)
    // L'incrément minimum est la taille de la dernière relance, mais au moins 1 BB.
    int current_bb_size = state.get_big_blind_size();
    if (current_bb_size <= 0) { // Sécurité si la BB n'est pas définie ou est invalide
        spdlog::error("ActionAbstraction::add_raise_actions: Big Blind size est <= 0 ({}) depuis GameState. Utilisation fallback à 1.", current_bb_size);
        current_bb_size = 1; // Fallback très conservateur
    }
    int min_raise_increment = std::max(last_raise_size, current_bb_size);

    // Le montant *total* de la mise minimale après relance
    int min_raise_total_bet = max_bet + min_raise_increment;

    // Calcul du max raise (All-in)
    // Le montant total de la mise après all-in
    int max_raise_total_bet = player_bet + player_stack; // Mise courante + tout le stack restant

    // --- Déterminer si c'est une opportunité d'"ouvrir" le pot ---
    bool is_open_opportunity = false;
    if (state.get_current_street() == Street::PREFLOP) {
        // Preflop, c'est une "open opportunity" si la mise max actuelle est juste la BB
        // ET que la "dernière relance" était la BB elle-même (ou moins, e.g. SB limp)
        // Cela couvre le cas UTG qui ouvre, ou SB qui open-raise par-dessus BB.
        if (max_bet == current_bb_size && last_raise_size <= current_bb_size) {
            is_open_opportunity = true;
        }
    } else { // Postflop
        if (max_bet == 0) {
            is_open_opportunity = true;
        }
    }
    if (is_open_opportunity) {
        spdlog::trace("ActionAbstraction: Situation d'ouverture détectée pour P{}", current_player);
    }

    // --- Cas Spécial : Min Raise force All-in ---
    // Si la mise minimale requise est déjà égale ou supérieure à un all-in,
    // la seule "relance" possible est all-in.
    if (min_raise_total_bet >= max_raise_total_bet) {
        if (allow_all_in_) {
            // S'assurer que cet all-in est bien *supérieur* à un simple call.
            // (On ne propose pas "Raise All-in" si ça revient juste à caller)
            if (max_raise_total_bet > max_bet) {
                 actions.push_back({current_player, ActionType::RAISE, max_raise_total_bet});
                 spdlog::trace("ActionAbstraction: RAISE (All-in {}) ajouté pour P{} (seule option car min_raise >= all_in)", max_raise_total_bet, current_player);
            } else {
                 spdlog::trace("ActionAbstraction: All-in ({}) pour P{} est <= max_bet ({}), pas ajouté comme RAISE.", max_raise_total_bet, current_player, max_bet);
            }
        } else {
            spdlog::trace("ActionAbstraction: Min raise ({}) >= All-in ({}) mais All-in non autorisé pour P{}", min_raise_total_bet, max_raise_total_bet, current_player);
        }
        return; // Aucune autre taille de relance n'est possible
    }

    std::set<int> raise_total_bets;
    Street current_street = state.get_current_street();

    // 1. Ajouter les relances basées sur les fractions du pot
    const auto it_frac = fractions_by_street_.find(current_street);
    if (it_frac != fractions_by_street_.end()) {
        const std::set<double>& fracs = it_frac->second;
        if (!fracs.empty()) {
            spdlog::trace("ActionAbstraction: Calcul raises par fraction de pot (pot_if_player_calls={}) pour street {}", pot_if_player_calls, street_to_string(current_street));
            for (double fraction : fracs) {
                if (fraction <= 0) continue;
                int raise_increment_from_pot = static_cast<int>(std::round(fraction * pot_if_player_calls));
                raise_increment_from_pot = std::max(0, raise_increment_from_pot);
                int candidate_total_bet = max_bet + raise_increment_from_pot;
                candidate_total_bet = std::max(min_raise_total_bet, candidate_total_bet);
                candidate_total_bet = std::min(max_raise_total_bet, candidate_total_bet);
                if (candidate_total_bet > max_bet) {
                     raise_total_bets.insert(candidate_total_bet);
                     spdlog::trace("  -> Frac {:.2f}: inc={}, clamped_total_bet={}. Ajouté.", fraction, raise_increment_from_pot, candidate_total_bet);
                }
            }
        }
    }

    // 2. Relances basées sur des multiples de BB
    auto street_bb_sizes_it = bb_sizes_by_street_.find(current_street);
    if (street_bb_sizes_it != bb_sizes_by_street_.end()) {
        const auto& bb_multipliers = street_bb_sizes_it->second;
        for (double bb_mult : bb_multipliers) {
            if (bb_mult <= 0) continue;
            int candidate_total_bet;
            if (is_open_opportunity) {
                // Pour une ouverture, le multiplicateur de BB définit la taille totale de la mise
                candidate_total_bet = static_cast<int>(bb_mult * current_bb_size);
            } else {
                // Pour une sur-relance, le multiplicateur de BB définit la taille de l'incrément de relance
                int raise_increment_bb = static_cast<int>(bb_mult * current_bb_size);
                candidate_total_bet = max_bet + raise_increment_bb;
            }

            candidate_total_bet = std::max(candidate_total_bet, min_raise_total_bet);
            candidate_total_bet = std::min(candidate_total_bet, max_raise_total_bet);

            if (candidate_total_bet > max_bet) {
                raise_total_bets.insert(candidate_total_bet);
                spdlog::trace("  P{}: BB mult {} -> total_bet {} (open? {}, inc {})", current_player, bb_mult, candidate_total_bet, is_open_opportunity, candidate_total_bet - max_bet);
            }
        }
    }
    
    // 3. Mises exactes (NOUVEAU)
    auto street_exact_bets_it = exact_bets_by_street_.find(current_street);
    if (street_exact_bets_it != exact_bets_by_street_.end()) {
        const auto& exact_amounts = street_exact_bets_it->second;
        for (int exact_val : exact_amounts) {
            if (exact_val <= 0) {
                spdlog::trace("  P{}: Exact bet value {} is <=0. Skipping.", current_player, exact_val);
                continue;
            }
            int candidate_total_bet;
            if (is_open_opportunity) {
                // Pour une ouverture, le montant exact définit la taille totale de la mise
                candidate_total_bet = exact_val;
                 spdlog::trace("  P{}: Exact bet (open) specified: {}", current_player, exact_val);
            } else {
                // Pour une sur-relance, le montant exact définit la taille de l'incrément
                candidate_total_bet = max_bet + exact_val;
                spdlog::trace("  P{}: Exact bet (re-raise increment) specified: {} -> total {}", current_player, exact_val, candidate_total_bet);
            }

            // Important: Clamper par min_raise_total_bet seulement si ce n'est pas une ouverture à 0
            // Ou si le min_raise_total_bet est effectivement plus grand que notre mise exacte d'ouverture
            if (!is_open_opportunity || candidate_total_bet < min_raise_total_bet) {
                candidate_total_bet = std::max(candidate_total_bet, min_raise_total_bet);
            }                    
            candidate_total_bet = std::min(candidate_total_bet, max_raise_total_bet);

            if (candidate_total_bet > max_bet) {
                raise_total_bets.insert(candidate_total_bet);
                spdlog::trace("  P{}: Exact bet value {} -> final total_bet {} (open? {})", current_player, exact_val, candidate_total_bet, is_open_opportunity);
            } else {
                spdlog::trace("  P{}: Exact bet value {} (open? {}) -> candidate {} not > max_bet {}. Clamped min_raise_total_bet was {}. Skipping.", 
                              current_player, exact_val, is_open_opportunity, candidate_total_bet, max_bet, min_raise_total_bet);
            }
        }
    }

    // 4. Ajouter l'action All-in (si autorisée)
    if (allow_all_in_) {
        if (max_raise_total_bet > max_bet) {
             spdlog::trace("ActionAbstraction: Ajout de RAISE All-in ({}) pour P{}", max_raise_total_bet, current_player);
             raise_total_bets.insert(max_raise_total_bet);
        } else {
             spdlog::trace("ActionAbstraction: All-in ({}) pour P{} est <= max_bet ({}), pas ajouté comme RAISE.", max_raise_total_bet, current_player, max_bet);
        }
    }

    // --- Finalisation : Ajouter les montants uniques au vecteur d'actions ---
    for (int final_total_bet : raise_total_bets) {
        int raise_increment = final_total_bet - max_bet;
        bool is_all_in_raise = (final_total_bet == max_raise_total_bet);
        if (!is_all_in_raise && raise_increment < min_raise_increment) {
             spdlog::warn("ActionAbstraction: Ignored raise amount {} for P{}: raise_increment {} < min_raise_increment {} (and not all-in)",
                          final_total_bet, current_player, raise_increment, min_raise_increment);
             continue;
        }
        actions.push_back({current_player, ActionType::RAISE, final_total_bet});
        spdlog::trace("ActionAbstraction: RAISE ({}) ajouté pour P{}", final_total_bet, current_player);
    }
}

} // namespace gto_solver 