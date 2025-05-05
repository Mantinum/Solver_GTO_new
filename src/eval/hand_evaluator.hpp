#ifndef GTO_HAND_EVALUATOR_HPP
#define GTO_HAND_EVALUATOR_HPP

#include <vector>
#include <cstdint>
#include <string>
#include <array>

// Inclure les types nécessaires
#include "core/cards.hpp"
#include "core/bitboard.hpp"

// Déclaration de la fonction C de pokerlib pour l'évaluation 7 cartes
extern "C" short eval_7hand(int* hand);

namespace gto_solver { // <-- AJOUTÉ namespace

// Définition simple pour HandRank (peut être une structure plus tard)
using HandRank = uint16_t;

// Constante pour un rang invalide
constexpr HandRank INVALID_HAND_RANK = 0;

// --- Interface de l'évaluateur --- 

/**
 * @brief Évalue une main de 7 cartes fournie comme un Bitboard.
 * @param seven_card_mask Bitboard contenant 7 bits à 1.
 * @return Le HandRank de la meilleure main de 5 cartes parmi les 7.
 */
HandRank evaluate_hand_7_card(Bitboard seven_card_mask);

/**
 * @brief Évalue une main de 7 cartes (2 cartes privées + 5 cartes de board).
 * @param c1 Première carte privée.
 * @param c2 Deuxième carte privée.
 * @param board Vecteur contenant exactement 5 cartes communes.
 * @return Le HandRank de la meilleure main de 5 cartes.
 */
HandRank evaluate_hand_7_card(Card c1, Card c2, const std::vector<Card>& board); // Attend 5 cartes de board

/**
 * @brief Évalue une main de 7 cartes (directement fournie).
 * @param cards Array contenant exactement 7 cartes.
 * @return Le HandRank de la meilleure main de 5 cartes.
 */
HandRank evaluate_hand_7_card(const std::array<Card, 7>& cards);

// Fonctions utilitaires pour interpréter le HandRank (optionnel)
std::string hand_rank_to_string(HandRank rank);
std::string hand_type_to_string(HandRank rank);

} // namespace gto_solver <-- AJOUTÉ

#endif // GTO_HAND_EVALUATOR_HPP 