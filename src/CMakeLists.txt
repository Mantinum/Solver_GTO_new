# src/CMakeLists.txt - Structure corrigée avec gto_core, gto_eval, gto_solver_lib

# Trouver toutes les sources .cpp dans src/ et ses sous-répertoires (core, eval)
file(GLOB_RECURSE ALL_SRC_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "*.cpp"
    "core/*.cpp"
    "eval/*.cpp"
)

# Exclure main.cpp (pour l'exécutable) et les fichiers de test/benchmark
list(FILTER ALL_SRC_FILES EXCLUDE REGEX "main\\.cpp$")
list(FILTER ALL_SRC_FILES EXCLUDE REGEX "(benchmark|test|bench_).*.cpp$")

# --- gto_core library (Utilitaires de base) ---
add_library(gto_core STATIC
    core/cards.cpp
    core/deck.cpp
    # bitboard.cpp pourrait aussi aller ici si utilisé largement
    core/bitboard.cpp
)

target_include_directories(gto_core PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include> # Accès à include/
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR} # Accès à core/
)

target_compile_features(gto_core PUBLIC cxx_std_20)
target_link_libraries(gto_core PUBLIC spdlog::spdlog)


# --- gto_eval library (Évaluation des mains) ---
add_library(gto_eval STATIC
    eval/hand_evaluator.cpp
)

target_include_directories(gto_eval
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include> # Accès à include/
        ${PROJECT_SOURCE_DIR}/external/2p2 # Pour poker.h
        ${CMAKE_CURRENT_SOURCE_DIR} # Rend src/ (et donc src/core, src/eval) visible publiquement
    # INTERFACE # Commenté pour l'instant
    #    ${CMAKE_CURRENT_SOURCE_DIR} 
)

target_compile_features(gto_eval PUBLIC cxx_std_20)

# gto_eval dépend de gto_core (pour Card) et de pokerlib (pour eval_7hand)
# Assurez-vous que la cible "pokerlib" est définie dans external/2p2/CMakeLists.txt
target_link_libraries(gto_eval PUBLIC
    gto_core
    pokerlib # <-- Cible définie dans external/2p2/CMakeLists.txt
    spdlog::spdlog
)


# --- gto_solver_lib (Logique principale: GameState, ActionAbstraction) ---
add_library(gto_solver_lib STATIC
    game_state.cpp
    action_abstraction.cpp
    information_set.cpp
    cfr_engine.cpp
    game_utils.cpp
)

target_include_directories(gto_solver_lib PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include> # Accès à include/
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR} # Accès aux sources locales (.cpp)
)

target_compile_features(gto_solver_lib PUBLIC cxx_std_20)

# gto_solver_lib dépend de gto_core et gto_eval
target_link_libraries(gto_solver_lib PUBLIC
    gto_core
    gto_eval
    spdlog::spdlog
)


# --- Exécutable principal --- 
add_executable(solver main.cpp)

# Lier l'exécutable à la bibliothèque principale gto_solver_lib
target_link_libraries(solver PRIVATE
    gto_solver_lib
    # Les dépendances transitives (core, eval, spdlog, pokerlib) devraient être liées via gto_solver_lib
)

target_include_directories(solver PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

# --- Gestion des sous-répertoires --- 
# PAS BESOIN de les ajouter ici si les sources sont listées explicitement ci-dessus
# add_subdirectory(core)
# add_subdirectory(eval)
