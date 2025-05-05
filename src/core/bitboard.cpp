#include "core/bitboard.hpp"
#include "core/cards.hpp"
#include <sstream>
#include <algorithm> // for std::sort

namespace gto_solver {

std::string board_to_string(Bitboard board) {
    std::stringstream ss;
    std::vector<Card> cards = board_to_cards(board);
    // Trier les cartes par index pour un affichage cohérent
    std::sort(cards.begin(), cards.end()); 
    
    for (size_t i = 0; i < cards.size(); ++i) {
        ss << to_string(cards[i]); // Concaténer les cartes "As", "Kh", etc.
    }
    // Retourner une chaîne vide si le board est vide (plutôt que "[empty]")
    return ss.str(); 
}

std::vector<Card> board_to_cards(Bitboard board) {
    std::vector<Card> cards;
    cards.reserve(count_set_bits(board)); // Pré-allouer
    while (board != 0) {
        Card c = pop_lsb(board); 
        if (c != INVALID_CARD) { // Vérifier si pop_lsb a réussi
             cards.push_back(c);
        }
    }
    return cards;
}

Bitboard cards_to_board(const std::vector<Card>& cards) {
    Bitboard board = EMPTY_BOARD;
    for (Card c : cards) {
        set_card(board, c);
    }
    return board;
}

// Overload for iterators (if needed later)
// template<CardIterator It>
// Bitboard cards_to_board(It begin, It end) {
//     Bitboard board = EMPTY_BOARD;
//     for (It it = begin; it != end; ++it) {
//         set_card(board, *it);
//     }
//     return board;
// }

} // namespace gto_solver 