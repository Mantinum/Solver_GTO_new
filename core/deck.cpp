#include "core/deck.hpp"
#include "core/bitboard.hpp" // Pour NUM_CARDS
#include <stdexcept> // Pour std::runtime_error
#include <random>       // Pour std::mt19937 et std::random_device

namespace gto_solver {

Deck::Deck()
    : cards_(NUM_CARDS),
      next_card_index_(0)
{
    std::iota(cards_.begin(), cards_.end(), 0); // Remplir avec 0..51
    // Initialiser le générateur de nombres aléatoires
    std::random_device rd;
    rng_.seed(rd());
}

void Deck::reset() {
    next_card_index_ = 0;
    // Pas besoin de re-remplir, juste remélanger
    shuffle();
}

Card Deck::deal_card() {
    if (next_card_index_ >= cards_.size()) {
        throw std::runtime_error("Deck is empty, cannot deal card.");
    }
    return cards_[next_card_index_++];
}

// Brûle une carte (avance juste l'index)
void Deck::burn_card() {
    if (next_card_index_ >= cards_.size()) {
        // Idéalement, logguer une erreur ou lancer une exception
        // throw std::runtime_error("Deck is empty, cannot burn card.");
        // Pour l'instant, on ne fait rien pour éviter un crash dans certains scénarios.
        return; 
    }
    next_card_index_++; 
}

void Deck::shuffle() {
    // Remélanger TOUT le paquet, pas juste la partie non distribuée
    std::shuffle(cards_.begin(), cards_.end(), rng_);
    next_card_index_ = 0; // Après mélange, on recommence du début
}

} // namespace gto_solver 