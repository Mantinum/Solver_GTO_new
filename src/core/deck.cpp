#include "core/deck.hpp"
#include "core/bitboard.hpp"
#include <stdexcept> 
#include <random>
#include <algorithm>
#include <numeric>

namespace gto_solver {

Deck::Deck()
    : cards_(NUM_CARDS),
      next_card_index_(0)
{
    std::iota(cards_.begin(), cards_.end(), 0); 
    std::random_device rd;
    rng_.seed(rd());
    // Shuffle initialement
    shuffle(); 
}

void Deck::reset() {
    next_card_index_ = 0;
    shuffle();
}

Card Deck::deal_card() {
    if (next_card_index_ >= cards_.size()) {
        throw std::runtime_error("Deck is empty, cannot deal card.");
    }
    return cards_[next_card_index_++];
}

void Deck::burn_card() {
    if (next_card_index_ >= cards_.size()) {
        // Optionnel: Gérer l'erreur si on essaie de brûler dans un deck vide
        // throw std::runtime_error("Deck empty, cannot burn card.");
        return; 
    }
    next_card_index_++; // Simplement ignorer la prochaine carte
}

void Deck::shuffle() {
    std::shuffle(cards_.begin(), cards_.end(), rng_);
    next_card_index_ = 0;
}

} // namespace gto_solver 