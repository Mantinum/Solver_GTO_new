#ifndef GTO_ACTION_ABSTRACTION_H
#define GTO_ACTION_ABSTRACTION_H

#include <vector>
#include <string>
#include <set> // Pour stocker les fractions de pot uniques
#include <algorithm> // Pour std::max
#include <cmath> // Pour std::round
#include <stdexcept> // Pour std::logic_error
// Inclure d'autres dépendances nécessaires, par ex. game_state.h si ActionSpec l'utilise
// #include "gto/game_state.h"

namespace gto_solver {

// Forward declaration pour éviter inclusion circulaire si Action a besoin de GameState
class GameState; 

// Types d'action possibles (correspond à ce qui était utilisé implicitement)
enum class ActionType {
    FOLD,
    CALL, // Inclut CHECK si montant à appeler est 0
    RAISE
};

// Structure pour représenter une action concrète (déplacée de game_state.h pour cohérence)
struct Action {
    int player_index = -1;
    ActionType type = ActionType::FOLD; // Valeur par défaut
    int amount = 0; // Pour CALL et RAISE: *Montant total* misé par le joueur *après* l'action

    // Opérateur de comparaison pour utiliser Action dans des sets/maps si besoin
    bool operator<(const Action& other) const {
        if (player_index != other.player_index) return player_index < other.player_index;
        if (type != other.type) return static_cast<int>(type) < static_cast<int>(other.type);
        return amount < other.amount;
    }
     // Opérateur d'égalité pour les tests
    bool operator==(const Action& other) const {
        return player_index == other.player_index &&
               type == other.type &&
               amount == other.amount;
    }
};

// Pré-déclaration si GameState est utilisé seulement par référence/pointeur
// class GameState; <-- Redondant car déjà déclaré plus haut

// Classe pour définir et appliquer une abstraction d'actions
class ActionAbstraction {
public:
    // Constructeur avec un schéma d'abstraction par défaut ou configurable
    ActionAbstraction(
        bool allow_fold = true,
        bool allow_check_call = true,
        const std::set<double>& raise_fractions = {0.33, 0.5, 0.75, 1.0}, // Ex: 33%, 50%, 75%, 100% pot
        bool allow_all_in = true
    );

    // Méthode principale pour obtenir les actions abstraites légales pour un état donné
    std::vector<Action> get_abstract_actions(const GameState& state) const;

    // Méthodes publiques (signatures basées sur le .cpp) <-- SUPPRIMÉ
    /*
    std::vector<ActionSpec> get_possible_action_specs(const GameState& current_state) const;
    int get_action_amount(const ActionSpec& action_spec, const GameState& current_state) const;
    */

private:
    bool allow_fold_;
    bool allow_check_call_;
    std::set<double> raise_pot_fractions_; // Fractions du pot pour les relances
    bool allow_all_in_;

    // Méthodes helper (privées)
    void add_fold_action(std::vector<Action>& actions, const GameState& state) const;
    void add_check_call_action(std::vector<Action>& actions, const GameState& state) const;
    void add_raise_actions(std::vector<Action>& actions, const GameState& state) const;
};

} // namespace gto_solver

#endif // GTO_ACTION_ABSTRACTION_H 