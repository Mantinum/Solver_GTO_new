#include "gto/game_utils.hpp"
#include "gto/game_state.h" // Pour l'enum Street, bien que déjà dans game_utils.hpp via forward declaration
#include <sstream>            // <-- AJOUTÉ
#include <algorithm>          // Pour std::remove
#include "core/cards.hpp" // Pour to_string(Card)
#include <iterator>  // Pour std::ostream_iterator

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

std::string action_to_string(const Action& action) {
    std::string type_str;
    switch (action.type) {
        case ActionType::FOLD: type_str = "FOLD"; break;
        case ActionType::CALL: type_str = "CALL"; break;
        case ActionType::RAISE: type_str = "RAISE"; break;
        default: type_str = "UNKNOWN_ACTION_TYPE"; break; // Plus descriptif
    }
    // Pour CALL et RAISE, on affiche aussi le montant.
    // Pour FOLD, le montant est généralement 0 et non pertinent pour la description.
    if (action.type == ActionType::CALL || action.type == ActionType::RAISE) {
        return type_str + " " + std::to_string(action.amount);
    } else {
        return type_str;
    }
}

} // namespace gto_solver 