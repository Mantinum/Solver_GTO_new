#include <stdint.h>

// Déclaration externe de la fonction C++ originale (sans extern "C" car on est en C)
// La signature doit correspondre à celle dans pokerlib.cpp
extern short eval_7hand(int *hand);
 
// La fonction wrapper C que nous appellerons depuis C++ via extern "C"
short evaluate_7_cards_C(int *hand) {
    // Appelle directement la fonction de la bibliothèque 2p2
    return eval_7hand(hand);
} 