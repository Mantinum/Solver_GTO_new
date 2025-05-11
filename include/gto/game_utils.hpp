#ifndef GTO_GAME_UTILS_HPP
#define GTO_GAME_UTILS_HPP

#include "gto/game_state.h" // Pour Street (dépendance circulaire si Street est ici? Non, Street est un enum class)
#include <string>
#include <vector>
#include "core/cards.hpp" // Pour Card et to_string(Card)

namespace gto_solver {

// Fonction pour convertir l'enum Street en string
std::string street_to_string(Street s);

// Potentiellement d'autres utilitaires liés au jeu ici à l'avenir

std::string hand_history_to_string(const std::vector<Action>& history);
Card card_from_string(const std::string& s);

// Nouvelle fonction utilitaire
std::string vec_to_string(const std::vector<Card>& cards);

// Fonction utilitaire pour convertir une Action en string
std::string action_to_string(const Action& action);

} // namespace gto_solver

#endif // GTO_GAME_UTILS_HPP 