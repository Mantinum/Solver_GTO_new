#include "eval/hand_ranks_loader.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <sys/mman.h> // Pour mmap
#include <sys/stat.h> // Pour stat
#include <fcntl.h>    // Pour open
#include <unistd.h>   // Pour close
#include <cerrno>     // Pour errno
#include <cstring>    // Pour strerror

namespace gto_solver {

// Déclaration extern des pointeurs (ils sont définis dans hand_evaluator.cpp)
extern unsigned short* hash_values;
extern unsigned short* hash_adjust;

// Taille attendue pour vérification (optionnel)
// constexpr size_t EXPECTED_HAND_RANKS_SIZE_SHORTS = 32487834; // Taille algo HR
// Ou la taille générée par generate_table ? (64975668 shorts?) Vérifier la source des tables.
constexpr size_t EXPECTED_HAND_RANKS_SIZE_SHORTS = 64975668; // Taille du fichier généré

// --- Fonction pour charger HandRanks.dat via mmap --- 

// Structure pour gérer le mapping mémoire et le cleanup
struct MemoryMapping {
    // ... existing code ...
};

std::vector<uint16_t> load_hand_ranks(const char* path) {
    std::vector<uint16_t> ranks;

#ifdef _WIN32
    // --- Memory Mapping Windows --- 
    HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Impossible d'ouvrir le fichier: " + std::string(path));
    }

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize)) {
        CloseHandle(hFile);
        throw std::runtime_error("Impossible d'obtenir la taille du fichier: " + std::string(path));
    }
    if (fileSize.QuadPart == 0) {
        CloseHandle(hFile);
        throw std::runtime_error("Fichier vide: " + std::string(path));
    }
    size_t num_shorts = fileSize.QuadPart / sizeof(uint16_t);

    HANDLE hMapFile = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (hMapFile == NULL) {
        CloseHandle(hFile);
        throw std::runtime_error("Impossible de créer le mapping fichier: " + std::string(path));
    }

    const uint16_t* mapped_data = (const uint16_t*)MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, fileSize.QuadPart);
    if (mapped_data == NULL) {
        CloseHandle(hMapFile);
        CloseHandle(hFile);
        throw std::runtime_error("Impossible de mapper la vue fichier: " + std::string(path));
    }

#else
    // --- Memory Mapping POSIX (Linux/macOS) --- 
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        throw std::runtime_error("Impossible d'ouvrir le fichier: " + std::string(path));
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        close(fd);
        throw std::runtime_error("Impossible d'obtenir la taille du fichier (stat): " + std::string(path));
    }
     if (sb.st_size == 0) {
        close(fd);
        throw std::runtime_error("Fichier vide: " + std::string(path));
    }
    size_t file_size_bytes = sb.st_size;
    size_t num_shorts = file_size_bytes / sizeof(uint16_t);

    const uint16_t* mapped_data = static_cast<const uint16_t*>(mmap(NULL, file_size_bytes, PROT_READ, MAP_PRIVATE, fd, 0));
    if (mapped_data == MAP_FAILED) {
        close(fd);
        throw std::runtime_error("Impossible de mapper la mémoire (mmap): " + std::string(path));
    }
    // Le descripteur de fichier peut être fermé après mmap
    close(fd);

#endif

    // Vérifier la taille si nécessaire
    if (num_shorts != EXPECTED_HAND_RANKS_SIZE_SHORTS) {
         std::cerr << "Warning: Taille de HandRanks.dat (" << num_shorts
                   << " shorts) différente de la taille attendue ("
                   << EXPECTED_HAND_RANKS_SIZE_SHORTS << "). Chemin: " << path << std::endl;
    }

    // Copier les données mappées dans le vecteur
    try {
        ranks.assign(mapped_data, mapped_data + num_shorts);
    } catch (const std::bad_alloc& e) {
        std::cerr << "Erreur d'allocation mémoire lors de la copie des données mappées." << std::endl;
        // Nettoyer le mapping mémoire avant de relancer l'exception
#ifdef _WIN32
        UnmapViewOfFile(mapped_data);
        CloseHandle(hMapFile);
        CloseHandle(hFile);
#else
        munmap((void*)mapped_data, file_size_bytes);
#endif
        throw; // Relancer l'exception bad_alloc
    }

    // Nettoyer le mapping mémoire
#ifdef _WIN32
    UnmapViewOfFile(mapped_data);
    CloseHandle(hMapFile);
    CloseHandle(hFile);
#else
    munmap((void*)mapped_data, file_size_bytes);
#endif

    if (ranks.empty()) { // Vérification supplémentaire après copie
         throw std::runtime_error("Échec de la copie des données mappées depuis HandRanks.dat");
    }
    
    return ranks; // Retourner le vecteur chargé
}

} // namespace gto_solver 