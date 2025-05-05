#ifndef GTO_CARDS_HPP
#define GTO_CARDS_HPP

#include <cstdint>
#include <string>
#include <stdexcept> // Pour std::invalid_argument

namespace gto_solver {

// Type alias pour une carte (par exemple, représentation 8 bits)
// Pourrait être une classe/struct plus tard
using Card = uint8_t;

// Constante pour une carte invalide/inconnue
constexpr Card INVALID_CARD = 52; // Utiliser 52 comme index invalide
// constexpr int  NUM_CARDS    = 52; // Supprimé, déjà dans bitboard.hpp

// Enum pour les couleurs (suits)
enum class Suit : uint8_t { CLUBS = 0, DIAMONDS = 1, HEARTS = 2, SPADES = 3 };
// Enum pour les rangs (ranks)
enum class Rank : uint8_t {
    TWO = 0, THREE = 1, FOUR = 2, FIVE = 3, SIX = 4, SEVEN = 5, EIGHT = 6, 
    NINE = 7, TEN = 8, JACK = 9, QUEEN = 10, KING = 11, ACE = 12
};

// Fonctions pour créer/décomposer une carte
// Format: index 0-51 = suit * 13 + rank
constexpr Card make_card(Rank r, Suit s) {
    // Conversion en index 0-51
    return static_cast<uint8_t>(s) * 13 + static_cast<uint8_t>(r);
}

constexpr Rank get_rank(Card c) {
    if (c >= INVALID_CARD) return static_cast<Rank>(13); // Valeur invalide hors enum?
    return static_cast<Rank>(c % 13);
}

constexpr Suit get_suit(Card c) {
    if (c >= INVALID_CARD) return static_cast<Suit>(4); // Valeur invalide hors enum?
    return static_cast<Suit>(c / 13);
}

// Fonctions de conversion string <-> Card/Rank/Suit
std::string to_string(Suit s);
std::string to_string(Rank r);
std::string to_string(Card c);

Card card_from_string(const std::string& s);
Rank rank_from_char(char r);
Suit suit_from_char(char s);

} // namespace gto_solver

#endif // GTO_CARDS_HPP 