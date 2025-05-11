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
    const StreetPositionFractionsMap& fractions_by_street_position,
    const StreetPositionBBSizesMap& bb_sizes_by_street_position,
    const StreetPositionExactBetsMap& exact_bets_by_street_position,
    bool allow_all_in)
    : allow_fold_(allow_fold),
      allow_check_call_(allow_check_call),
      fractions_by_street_position_(fractions_by_street_position),
      bb_sizes_by_street_position_(bb_sizes_by_street_position),
      exact_bets_by_street_position_(exact_bets_by_street_position),
      allow_all_in_(allow_all_in)
{
    // Validation des fractions par position
    for (const auto& street_pair : fractions_by_street_position_) {
        const Street street = street_pair.first;
        for (const auto& pos_pair : street_pair.second) {
            const Position pos = pos_pair.first;
            for (double frac : pos_pair.second) {
                if (frac <= 0.0) {
                    spdlog::debug("ActionAbstraction: Pour street {} position {}, raise fraction {} <= 0 sera ignorée.",
                                street_to_string(street),
                                position_to_string(pos),
                                frac);
                }
            }
        }
    }

    // Validation des tailles BB par position
    for (const auto& street_pair : bb_sizes_by_street_position_) {
        const Street street = street_pair.first;
        for (const auto& pos_pair : street_pair.second) {
            const Position pos = pos_pair.first;
            for (double bb_size : pos_pair.second) {
                if (bb_size <= 0.0) {
                    spdlog::debug("ActionAbstraction: Pour street {} position {}, BB size {} <= 0 sera ignoré.",
                                street_to_string(street),
                                position_to_string(pos),
                                bb_size);
                }
            }
        }
    }

    // Validation des mises exactes par position
    for (const auto& street_pair : exact_bets_by_street_position_) {
        const Street street = street_pair.first;
        for (const auto& pos_pair : street_pair.second) {
            const Position pos = pos_pair.first;
            for (int bet : pos_pair.second) {
                if (bet <= 0) {
                    spdlog::debug("ActionAbstraction: Pour street {} position {}, exact bet {} <= 0 sera ignoré.",
                                street_to_string(street),
                                position_to_string(pos),
                                bet);
                }
            }
        }
    }

    // Vérifier si des options de relance sont disponibles
    bool any_raise_option = allow_all_in_;
    if (!any_raise_option) {
        for (const auto& street_pair : fractions_by_street_position_) {
            for (const auto& pos_pair : street_pair.second) {
                if (!pos_pair.second.empty()) {
                    any_raise_option = true;
                    break;
                }
            }
            if (any_raise_option) break;
        }
    }
    if (!any_raise_option) {
        for (const auto& street_pair : bb_sizes_by_street_position_) {
            for (const auto& pos_pair : street_pair.second) {
                if (!pos_pair.second.empty()) {
                    any_raise_option = true;
                    break;
                }
            }
            if (any_raise_option) break;
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
    if (state.is_player_folded(current_player)) {
        spdlog::trace("ActionAbstraction::get_abstract_actions: Pas d'actions pour P{} car déjà foldé.", current_player);
        return actions;
    }

    // 1. Action FOLD (si autorisée)
    if (allow_fold_) {
        add_fold_action(actions, state);
    }

    // 2. Action CHECK/CALL (si autorisée)
    if (allow_check_call_) {
        add_check_call_action(actions, state);
    }

    // 3. Actions RAISE (si autorisées et possibles)
    add_raise_actions(actions, state);

    // Validation finale : au moins une action devrait être possible si le joueur n'est pas all-in ou foldé
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

Position ActionAbstraction::get_effective_position(const GameState& state) const {
    Position pos = state.get_player_position(state.get_current_player());
    if (state.get_num_players() == 2 && pos == Position::SB) {
        return Position::BTN;
    }
    return pos;
}

void ActionAbstraction::add_fold_action(std::vector<Action>& actions, const GameState& state) const {
    if (!allow_fold_) return;

    int current_player = state.get_current_player();
    int player_bet = state.get_current_bets().at(current_player);
    int max_bet = 0;
    for (int bet : state.get_current_bets()) {
        max_bet = std::max(max_bet, bet);
    }

    // On ne peut folder que s'il y a une mise à suivre (max_bet > player_bet)
    if (player_bet < max_bet) {
        actions.push_back({current_player, ActionType::FOLD, 0});
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
    if (amount_to_call <= player_stack) {
        actions.push_back({current_player, ActionType::CALL, max_bet});
        spdlog::trace("ActionAbstraction: CALL/CHECK ajouté pour P{} (montant: {})", current_player, max_bet);
    }
}

void ActionAbstraction::add_raise_actions(std::vector<Action>& actions, const GameState& state) const {
    int current_player = state.get_current_player();
    int player_stack = state.get_player_stack(current_player);
    int player_bet = state.get_current_bets().at(current_player);
    int max_bet = 0;
    for (int bet : state.get_current_bets()) {
        max_bet = std::max(max_bet, bet);
    }

    if (player_stack <= 0) return;

    int min_raise_size = calculate_min_raise_size(state);
    int pot_size = calculate_pot_size(state);
    Position effective_pos = get_effective_position(state);

    spdlog::trace("ActionAbstraction: P{} en position effective {} (street: {})",
                  current_player, 
                  position_to_string(effective_pos),
                  street_to_string(state.get_current_street()));
    spdlog::trace("ActionAbstraction: player_bet={}, max_bet={}, min_raise_size={}, BB={}",
                  player_bet, max_bet, min_raise_size, state.get_big_blind_size());

    std::set<int> raise_targets; // Pour éviter les doublons

    // Tailles de mise en BB
    auto bb_multipliers = get_bb_sizes_for_position(state);
    spdlog::trace("ActionAbstraction: Tailles BB (multiplicateurs) disponibles pour P{}: {}", current_player, bb_multipliers.size());

    // Déterminer si c'est une situation d'open (ou premier bet sur une street)
    // vs. une situation de reraise.
    bool is_open_situation = (state.get_current_street() != Street::PREFLOP && max_bet == 0) || 
                             (max_bet <= state.get_big_blind_size());

    spdlog::trace("ActionAbstraction: is_open_situation pour BB sizes = {} (max_bet={}, street={})",
                  is_open_situation, max_bet, street_to_string(state.get_current_street()));

    for (double bb_mult : bb_multipliers) {
        if (bb_mult <= 0) continue;
        int target = 0;
        int raise_increment_if_applicable = 0;

        if (is_open_situation) { // bb_mult définit la MISE TOTALE CIBLE
            target = static_cast<int>(std::round(state.get_big_blind_size() * bb_mult));
            raise_increment_if_applicable = target - max_bet; 
            // Pour un open, player_bet est la blind postée, max_bet est la BB.
            // L'incrément est par rapport à max_bet.
        } else { // bb_mult définit un INCRÉMENT de relance (X BBs à ajouter à max_bet)
            raise_increment_if_applicable = static_cast<int>(std::round(state.get_big_blind_size() * bb_mult));
            target = max_bet + raise_increment_if_applicable;
        }

        spdlog::trace("ActionAbstraction: bb_mult={}, target={}, raise_increment_if_applicable={}", bb_mult, target, raise_increment_if_applicable);

        int raise_target_for_player = target - player_bet;

        if (raise_target_for_player > 0 && raise_target_for_player <= player_stack) {
            spdlog::trace("ActionAbstraction: Ajout RAISE (BB mult: {}) target: {} pour P{}", bb_mult, target, current_player);
            raise_targets.insert(target);
        } else {
            spdlog::trace("ActionAbstraction: RAISE (BB mult: {}) target: {} pour P{} ignoré (target_for_player={}, stack={})",
                          bb_mult, target, current_player, raise_target_for_player, player_stack);
        }
    }

    // Fractions de pot
    double base_pot_for_fraction_calc = static_cast<double>(state.get_pot_size()); // Pot total actuel (inclut les mises de cette street)
    auto fractions = get_fractions_for_position(state);
    spdlog::trace("ActionAbstraction: Fractions de pot disponibles pour P{}: {}", current_player, fractions.size());

    for (double fraction : fractions) {
        if (fraction <= 0) continue;

        int amount_to_call_for_player = max_bet - player_bet;
        if (amount_to_call_for_player < 0) amount_to_call_for_player = 0; // Si le joueur peut checker ou ouvrir

        // Le "pot" pour un "pot-sized bet/raise" ou une fraction de celui-ci est généralement défini comme:
        // pot_actuel_sur_la_table + montant_que_le_joueur_doit_mettre_pour_caller.
        // Ensuite, le joueur relance de (fraction * ce_nouveau_pot_calculé) EN PLUS de sa mise pour caller.
        double effective_pot_for_raise_calc = base_pot_for_fraction_calc + amount_to_call_for_player;
        
        int raise_increment = static_cast<int>(std::round(effective_pot_for_raise_calc * fraction));
        int target = max_bet + raise_increment;

        spdlog::debug("ActionAbstraction: Fraction {}, amount_to_call_for_player={}, effective_pot_for_raise_calc={}, raise_increment={}, target={}", 
            fraction, amount_to_call_for_player, effective_pot_for_raise_calc, raise_increment, target);

        if (target > max_bet && raise_increment >= min_raise_size && target <= player_bet + player_stack) {
            raise_targets.insert(target);
            spdlog::debug("ActionAbstraction: RAISE {} (fraction {}) ajouté pour P{}", target, fraction, current_player);
        } else {
            spdlog::debug("ActionAbstraction: RAISE {} (fraction {}) REJETÉ pour P{}. Conditions: target({}) > max_bet({}), raise_increment({}) >= min_raise_size({}), target({}) <= player_bet({}) + player_stack({})",
                         target, fraction, current_player, target, max_bet, raise_increment, min_raise_size, target, player_bet, player_stack);
        }
    }

    // Mises exactes : Action.amount = montant total voulu
    auto exact_bet_values = get_exact_bets_for_position(state);
    spdlog::debug("ActionAbstraction: Mises exactes disponibles pour P{}: {}", current_player, exact_bet_values.size());
    for (int exact_target_value : exact_bet_values) {
        if (exact_target_value <= 0) continue;

        int current_target = exact_target_value;
        int increment = current_target - max_bet;

        if (current_target > max_bet) { // Doit être supérieur à la mise actuelle pour être un raise potentiel
            if (increment >= min_raise_size) { // L'incrément est-il suffisant ?
                // Oui, le raise est valide tel quel
                if (current_target <= player_bet + player_stack) { // Le joueur a-t-il assez ?
                    raise_targets.insert(current_target);
                    spdlog::debug("ActionAbstraction: RAISE {} (exact) ajouté pour P{}", current_target, current_player);
                }
            } else { // Non, l'incrément est trop petit (increment < min_raise_size)
                     // Le test s'attend à un "clamping" au min_raise si possible.
                int clamped_target = max_bet + min_raise_size;
                // Assurer que le clamped_target n'est pas un call (si min_raise_size était 0 par erreur)
                // et que le joueur peut le payer.
                if (clamped_target > max_bet && clamped_target <= player_bet + player_stack) {
                    raise_targets.insert(clamped_target);
                    spdlog::debug("ActionAbstraction: EXACT_CLAMPED: Exact bet {} (inc {}) from P{} was too small (min_raise_size {}), CLAMPED and ADDED as target {}.", 
                                 exact_target_value, increment, current_player, min_raise_size, clamped_target);
                } else {
                    spdlog::debug("ActionAbstraction: EXACT_CLAMP_REJECTED: Exact bet {} (inc {}) from P{} was too small (min_raise_size {}). Clamped target {} REJECTED (conds: clamped({}) > max_bet({}), clamped({}) <= p_bet({}) + p_stack({})).",
                                 exact_target_value, increment, current_player, min_raise_size, clamped_target,
                                 clamped_target, max_bet, clamped_target, player_bet, player_stack);
                }
            }
        } else { // current_target <= max_bet
            spdlog::debug("ActionAbstraction: EXACT_REJECTED_NOT_A_RAISE: Exact bet {} from P{} is not > max_bet ({}), not a raise.", 
                         exact_target_value, current_player, max_bet);
        }
    }

    // All-in si autorisé
    if (allow_all_in_ && player_stack > 0) {
        int allin_target = player_bet + player_stack;
        // Un all-in est valide s'il est supérieur à la mise actuelle.
        // Il n'a pas besoin de respecter min_raise_size s'il est le seul montant possible.
        // Cependant, s'il est plus petit qu'un min-raise valide, il ne rouvrira pas les mises pour les joueurs ayant déjà agi.
        // Notre ActionAbstraction ne gère pas la réouverture des mises, donc on l'ajoute si > max_bet.
        if (allin_target > max_bet) {
             // On vérifie aussi si l'all-in est bien un raise (au moins min_raise_size) ou juste un call insuffisant
            // Si (allin_target - max_bet) < min_raise_size ET allin_target < max_bet + min_raise_size,
            // c'est un all-in qui ne constitue pas un "full raise". Il est quand même valide.
            // La condition principale est target > max_bet.
            // La condition (target - max_bet) >= min_raise_size est pour les raises "normaux".
            // Pour un all-in, on permet de miser moins que min_raise_size si c'est tout ce que le joueur a.
            // Il faut juste que ce soit plus que max_bet.
            raise_targets.insert(allin_target);
            spdlog::debug("ActionAbstraction: All-in target {} retenu pour P{}", allin_target, current_player);
        } else {
            spdlog::debug("ActionAbstraction: All-in target {} rejeté pour P{} car non supérieur à max_bet ({})", allin_target, current_player, max_bet);
        }
    }

    // Générer les actions RAISE pour chaque cible unique et triée
    for (int target : raise_targets) {
        actions.push_back({current_player, ActionType::RAISE, target});
        spdlog::debug("ActionAbstraction: Action RAISE {} effectivement ajoutée pour P{}", target, current_player);
    }
}

// Correction de la logique de position pour les tests
std::set<double> ActionAbstraction::get_fractions_for_position(const GameState& state) const {
    Position effective_pos = get_effective_position(state);
    Street street = state.get_current_street();

    auto street_it = fractions_by_street_position_.find(street);
    if (street_it == fractions_by_street_position_.end()) {
        return {};
    }

    auto pos_it = street_it->second.find(effective_pos);
    if (pos_it == street_it->second.end()) {
        return {};
    }

    return pos_it->second;
}

std::set<double> ActionAbstraction::get_bb_sizes_for_position(const GameState& state) const {
    Position effective_pos = get_effective_position(state);
    Street street = state.get_current_street();
    int current_player = state.get_current_player();

    spdlog::debug("ActionAbstraction: get_bb_sizes_for_position: P{} en position effective {} (street: {})", 
                  current_player, position_to_string(effective_pos), street_to_string(street));

    auto street_it = bb_sizes_by_street_position_.find(street);
    if (street_it == bb_sizes_by_street_position_.end()) {
        spdlog::debug("ActionAbstraction: get_bb_sizes_for_position: Aucune taille BB trouvée pour street {}", street_to_string(street));
        return {};
    }

    auto pos_it = street_it->second.find(effective_pos);
    if (pos_it == street_it->second.end()) {
        spdlog::debug("ActionAbstraction: get_bb_sizes_for_position: Aucune taille BB trouvée pour position {}", position_to_string(effective_pos));
        return {};
    }

    spdlog::debug("ActionAbstraction: get_bb_sizes_for_position: Tailles BB trouvées pour P{}: {}", current_player, pos_it->second.size());
    for (double bb_size : pos_it->second) {
        spdlog::debug("ActionAbstraction: get_bb_sizes_for_position: Taille BB {} pour P{}", bb_size, current_player);
    }
    return pos_it->second;
}

std::set<int> ActionAbstraction::get_exact_bets_for_position(const GameState& state) const {
    Position effective_pos = get_effective_position(state);
    Street street = state.get_current_street();

    auto street_it = exact_bets_by_street_position_.find(street);
    if (street_it == exact_bets_by_street_position_.end()) {
        return {};
    }

    auto pos_it = street_it->second.find(effective_pos);
    if (pos_it == street_it->second.end()) {
        return {};
    }

    return pos_it->second;
}

int ActionAbstraction::calculate_min_raise_size(const GameState& state) const {
    int last_raise_amount = state.get_last_raise_size(); // Montant du dernier bet/raise sur la street
    int bb = state.get_big_blind_size();

    if (last_raise_amount <= 0) { // Aucun bet/raise précédent sur cette street (ou c'est le premier bet)
        return bb; // Le premier bet ou raise doit être d'au moins la BB
    } else {
        // Un re-raise doit être d'un montant (ajouté) au moins égal au précédent bet/raise, 
        // et ce montant (ajouté) doit aussi être au moins la BB.
        return std::max(bb, last_raise_amount);
    }
}

int ActionAbstraction::calculate_pot_size(const GameState& state) const {
    int pot = state.get_pot_size();
    // Ajouter les antes au pot pour le calcul des fractions
    pot += state.get_ante() * state.get_num_players();
    return pot;
}

} // namespace gto_solver 