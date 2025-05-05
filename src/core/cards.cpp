#include "core/cards.hpp"
#include <stdexcept>
#include <algorithm>
#include <iostream> // TODO: Remove later if not needed
#include <vector> // Pour std::vector
#include <map> // Pour la conversion char -> Rank/Suit

namespace gto_solver {

// Helper maps pour la conversion char <-> Rank/Suit
const std::map<char, Rank> CHAR_TO_RANK = {
    {'2', Rank::TWO}, {'3', Rank::THREE}, {'4', Rank::FOUR}, {'5', Rank::FIVE}, 
    {'6', Rank::SIX}, {'7', Rank::SEVEN}, {'8', Rank::EIGHT}, {'9', Rank::NINE}, 
    {'T', Rank::TEN}, {'J', Rank::JACK}, {'Q', Rank::QUEEN}, {'K', Rank::KING}, 
    {'A', Rank::ACE}
};
const std::map<char, Suit> CHAR_TO_SUIT = {
    {'c', Suit::CLUBS}, {'d', Suit::DIAMONDS}, {'h', Suit::HEARTS}, {'s', Suit::SPADES}
};
const std::map<Rank, char> RANK_TO_CHAR = {
    {Rank::TWO, '2'}, {Rank::THREE, '3'}, {Rank::FOUR, '4'}, {Rank::FIVE, '5'}, 
    {Rank::SIX, '6'}, {Rank::SEVEN, '7'}, {Rank::EIGHT, '8'}, {Rank::NINE, '9'}, 
    {Rank::TEN, 'T'}, {Rank::JACK, 'J'}, {Rank::QUEEN, 'Q'}, {Rank::KING, 'K'}, 
    {Rank::ACE, 'A'}
};
const std::map<Suit, char> SUIT_TO_CHAR = {
    {Suit::CLUBS, 'c'}, {Suit::DIAMONDS, 'd'}, {Suit::HEARTS, 'h'}, {Suit::SPADES, 's'}
};


// --- Implémentations des fonctions de conversion --- 

Rank rank_from_char(char r) {
    auto it = CHAR_TO_RANK.find(toupper(r));
    if (it == CHAR_TO_RANK.end()) {
        throw std::invalid_argument("Invalid rank character: " + std::string(1, r));
    }
    return it->second;
}

Suit suit_from_char(char s) {
    auto it = CHAR_TO_SUIT.find(tolower(s));
    if (it == CHAR_TO_SUIT.end()) {
        throw std::invalid_argument("Invalid suit character: " + std::string(1, s));
    }
    return it->second;
}

std::string to_string(Rank r) {
    auto it = RANK_TO_CHAR.find(r);
    if (it == RANK_TO_CHAR.end()) {
        return "?";
    }
    return std::string(1, it->second);
}

std::string to_string(Suit s) {
    auto it = SUIT_TO_CHAR.find(s);
    if (it == SUIT_TO_CHAR.end()) {
        return "?";
    }
    return std::string(1, it->second);
}

std::string to_string(Card c) {
    if (c == INVALID_CARD) return "??";
    return to_string(get_rank(c)) + to_string(get_suit(c));
}

Card card_from_string(const std::string& s) {
    if (s.length() != 2) {
        throw std::invalid_argument("Invalid card string format: '" + s + "'. Expected 'Rs'.");
    }
    try {
        Rank r = rank_from_char(s[0]);
        Suit su = suit_from_char(s[1]);
        return make_card(r, su);
    } catch (const std::invalid_argument& e) {
        // Propage l'erreur avec plus de contexte
        throw std::invalid_argument("Invalid card string '" + s + "': " + e.what());
    }
}

} // namespace gto_solver 