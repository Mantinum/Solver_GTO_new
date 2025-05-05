#ifndef GTO_BITBOARD_HPP
#define GTO_BITBOARD_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <iostream> // Pour l'affichage?

// Inclure cards.hpp pour les types Card, Rank, Suit
#include "core/cards.hpp"

namespace gto_solver { // <-- AJOUTÉ namespace

// Type pour les bitboards (masques de bits pour les cartes)
using Bitboard = uint64_t;

// Constantes utiles
constexpr Bitboard EMPTY_BOARD = 0ULL;
constexpr int NUM_CARDS = 52; // Ajout du nombre total de cartes
constexpr Bitboard FULL_DECK = (1ULL << 52) - 1; // 52 bits set

// Fonctions pour manipuler les bitboards
inline void set_card(Bitboard& board, Card c) {
    if (c != INVALID_CARD) {
        board |= (1ULL << c); // Utiliser la valeur Card directement comme index (0-51)
    }
}

inline void clear_card(Bitboard& board, Card c) {
    if (c != INVALID_CARD) {
        board &= ~(1ULL << c);
    }
}

inline bool test_card(Bitboard board, Card c) {
    if (c == INVALID_CARD) return false;
    return (board & (1ULL << c)) != 0;
}

// Compter le nombre de bits à 1 (cartes)
#ifdef __GNUC__ // Ou Clang
inline int count_set_bits(Bitboard board) {
    return __builtin_popcountll(board);
}
#else // Fallback standard (plus lent)
inline int count_set_bits(Bitboard board) {
    int count = 0;
    while (board > 0) {
        board &= (board - 1); // clear the least significant bit set
        count++;
    }
    return count;
}
#endif

// Trouver l'index du bit le moins significatif (LSB)
#ifdef __GNUC__ // Ou Clang
inline int bit_scan_forward(Bitboard board) {
    if (board == 0) return -1;
    return __builtin_ffsll(board) - 1; // ffsll est 1-based
}
#else // Fallback
inline int bit_scan_forward(Bitboard board) {
    if (board == 0) return -1;
    unsigned int index = 0;
    // Technique De Bruijn (rapide)
    static const int DeBruijnLookup[64] = {
        0, 1, 48, 2, 57, 49, 28, 3, 61, 58, 50, 42, 38, 29, 17, 4,
        62, 55, 59, 36, 51, 43, 45, 39, 34, 30, 22, 18, 12, 5, 15, 63,
        47, 56, 27, 60, 41, 37, 16, 54, 35, 44, 33, 21, 11, 14, 46, 26,
        53, 32, 20, 13, 25, 52, 19, 24, 31 
    };
    return DeBruijnLookup[((board & -board) * 0x03f79d71b4cb0a89ULL) >> 58];
}
#endif

// Extraire la carte correspondant au bit le moins significatif et l'enlever
inline Card pop_lsb(Bitboard& board) {
    int lsb_index = bit_scan_forward(board);
    if (lsb_index == -1) {
        return INVALID_CARD; // Retourner une carte invalide si le board est vide
    }
    board &= (board - 1); // Enlever le LSB
    // L'index du bit (0-63) correspond directement à notre Card (0-51)
    return static_cast<Card>(lsb_index);
}

// Fonctions de conversion
std::string board_to_string(Bitboard board); // Affichage lisible
std::vector<Card> board_to_cards(Bitboard board);
Bitboard cards_to_board(const std::vector<Card>& cards);

} // namespace gto_solver <-- AJOUTÉ

#endif // GTO_BITBOARD_HPP 