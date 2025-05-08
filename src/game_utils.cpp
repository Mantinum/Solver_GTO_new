#include "gto/game_utils.hpp"
#include "gto/game_state.h" // Pour l'enum Street, bien que déjà dans game_utils.hpp via forward declaration

namespace gto_solver {

// Implémentation de street_to_string
std::string street_to_string(Street s) {
    switch (s) {
        case Street::PREFLOP:  return "Preflop";
        case Street::FLOP:     return "Flop";
        case Street::TURN:     return "Turn";
        case Street::RIVER:    return "River";
        case Street::SHOWDOWN: return "Showdown";
        default:               return "UnknownStreet"; // Modifié pour être un identifiant valide
    }
}

} // namespace gto_solver 