#ifndef GTO_GAME_UTILS_HPP
#define GTO_GAME_UTILS_HPP

#include "gto/game_state.h" // Pour Street (dépendance circulaire si Street est ici? Non, Street est un enum class)
#include <string>

namespace gto_solver {

// Fonction pour convertir l'enum Street en string
std::string street_to_string(Street s);

// Potentiellement d'autres utilitaires liés au jeu ici à l'avenir

} // namespace gto_solver

#endif // GTO_GAME_UTILS_HPP 