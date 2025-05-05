#ifndef GTO_ACTION_ABSTRACTION_H
#define GTO_ACTION_ABSTRACTION_H

#include <vector>
// Inclure d'autres dépendances nécessaires, par ex. game_state.h si ActionSpec l'utilise
// #include "gto/game_state.h"

namespace gto_solver {

// Supposer l'existence de ces énumérations (à définir ailleurs ou ici)
enum class ActionType { FOLD, CALL, RAISE };
enum class SizingUnit { NONE, BB, PCT_POT, MULTIPLIER_X, ALL_IN };

// Définition de la structure ActionSpec (basée sur son utilisation dans le .cpp)
struct ActionSpec {
    ActionType type;
    SizingUnit unit;
    double value; // Ou float?
};

// Pré-déclaration si GameState est utilisé seulement par référence/pointeur
class GameState;

class ActionAbstraction {
public:
    ActionAbstraction();
    virtual ~ActionAbstraction();

    // Méthodes publiques (signatures basées sur le .cpp)
    std::vector<ActionSpec> get_possible_action_specs(const GameState& current_state) const;
    int get_action_amount(const ActionSpec& action_spec, const GameState& current_state) const;

private:
    // Membres privés si nécessaire
};

} // namespace gto_solver

#endif // GTO_ACTION_ABSTRACTION_H 