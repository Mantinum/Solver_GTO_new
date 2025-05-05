#ifndef GTO_CFR_ENGINE_H
#define GTO_CFR_ENGINE_H

// Inclure les dépendances nécessaires (GameState, etc.)
#include "gto/game_state.h"

namespace gto_solver {

class CFREngine {
public:
    CFREngine();
    virtual ~CFREngine();

    // Méthodes publiques placeholder
    void run_iterations(int num_iterations);
    // ... autres méthodes ...

private:
    // Membres privés pour l'état du moteur CFR
    // (ex: structure de l'arbre de jeu, regrets, stratégie moyenne)
};

} // namespace gto_solver

#endif // GTO_CFR_ENGINE_H 