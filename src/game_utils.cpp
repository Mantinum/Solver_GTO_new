#include "gto/game_utils.hpp"
#include "gto/game_state.h" // Pour l'enum Street, bien que déjà dans game_utils.hpp via forward declaration
#include <sstream>            // <-- AJOUTÉ
#include <algorithm>          // Pour std::remove

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

std::string vec_to_string(const std::vector<Card>& cards) {
    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < cards.size(); ++i) {
        ss << (cards[i] == INVALID_CARD ? "--" : gto_solver::to_string(cards[i]));
        if (i < cards.size() - 1) {
            ss << " ";
        }
    }
    ss << "]";
    return ss.str();
}

} // namespace gto_solver 