#ifndef GTO_CORE_DECK_HPP
#define GTO_CORE_DECK_HPP

#include "core/cards.hpp" // Pour Card et NUM_CARDS
#include <vector>
#include <random>   // Pour std::mt19937 et std::random_device
#include <algorithm> // Pour std::shuffle
#include <numeric>   // Pour std::iota

namespace gto_solver {

class Deck {
public:
    Deck(); // Constructeur par défaut
    ~Deck() = default;

    Card deal_card();
    void burn_card();
    void shuffle();
    void reset();

private:
    std::vector<Card> cards_;
    size_t            next_card_index_;
    std::mt19937      rng_; // Mersenne Twister random number generator
};

} // namespace gto_solver

#endif // GTO_CORE_DECK_HPP 