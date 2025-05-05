# src/CMakeLists.txt - Version simplifiée pour Structure A

# Trouver toutes les sources .cpp dans src/ et ses sous-répertoires (core, eval)
file(GLOB_RECURSE ALL_SRC_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "*.cpp"
    "core/*.cpp"
    "eval/*.cpp"
)

# Exclure main.cpp (pour l'exécutable) et les fichiers de test/benchmark
list(FILTER ALL_SRC_FILES EXCLUDE REGEX "main\\.cpp$")
list(FILTER ALL_SRC_FILES EXCLUDE REGEX "(benchmark|test|bench_).*.cpp$")

# Définir la bibliothèque statique gto_core avec toutes les sources trouvées
add_library(gto_core STATIC
    action_abstraction.cpp
    cfr_engine.cpp
    core/bitboard.cpp
    core/cards.cpp
    core/deck.cpp
    eval/hand_evaluator.cpp
    game_state.cpp
)

# Définir les répertoires d'include pour la bibliothèque gto_core
target_include_directories(gto_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/external/2p2
        ${CMAKE_BINARY_DIR}/generated
)

# Spécifier le standard C++ requis
target_compile_features(gto_core PUBLIC cxx_std_20)

# Lier gto_core à ses dépendances (comme spdlog)
target_link_libraries(gto_core PUBLIC spdlog::spdlog)

# --- Exécutable principal ---
# Défini ici car main.cpp est dans src/
add_executable(solver main.cpp)

# Lier l'exécutable à la bibliothèque gto_core et aux autres dépendances
target_link_libraries(solver PRIVATE
    gto_core
    # spdlog::spdlog # Déjà lié via gto_core (PUBLIC)
    # Ajouter ici les dépendances externes nécessaires (ex: tables 2p2)
    # Si la cible 2p2_tables existe (depuis external/2p2/CMakeLists.txt)
    # 2p2_tables
)

# Inclure les headers publics pour l'exécutable solver
target_include_directories(solver PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

# --- Gestion des sous-répertoires ---
# S'assurer qu'ils sont bien inclus pour d'éventuelles cibles spécifiques
# (comme la génération de tables dans eval?), mais SANS qu'ils définissent gto_core/gto_eval
add_subdirectory(core)
add_subdirectory(eval)
