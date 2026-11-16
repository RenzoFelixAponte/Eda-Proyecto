// Pruebas de correctitud (O3): secuencias aleatorias comparadas contra std::set,
// mas casos borde y verificacion de invariantes.
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "domain/avl/AvlTree.hpp"
#include "domain/skiplist/SkipList.hpp"

using eda::domain::Key;
using eda::domain::OrderedSet;
using Factory = std::function<std::unique_ptr<OrderedSet>()>;

namespace {

int gFailures = 0;

void check(bool condition, const std::string& message) {
    if (!condition) {
        ++gFailures;
        std::cerr << "  FALLO: " << message << '\n';
    }
}

void testEdgeCases(const Factory& make) {
    auto s = make();
    check(!s->search(5), "busqueda en vacio");
    check(!s->remove(5), "eliminar en vacio");
    check(s->insert(5), "insertar 5");
    check(!s->insert(5), "duplicado rechazado");
    check(s->size() == 1, "size tras duplicado");
    check(s->search(5), "buscar 5");
    check(s->remove(5), "eliminar 5");
    check(!s->search(5) && s->size() == 0, "vacio tras eliminar");
    check(s->checkInvariants(), "invariantes en vacio");
}

void testOrderedInputs(const Factory& make, int n) {
    for (bool ascending : {true, false}) {
        auto s = make();
        for (int i = 0; i < n; ++i) s->insert(ascending ? i : n - i);
        check(s->checkInvariants(), "invariantes con entrada ordenada");
        for (int i = 0; i < n; i += 2) s->remove(ascending ? i : n - i);
        check(s->checkInvariants(), "invariantes tras borrar la mitad");
        check(s->size() == static_cast<std::size_t>(n / 2), "size tras borrar la mitad");
    }
}

void testRandomAgainstStdSet(const Factory& make, std::uint64_t seed, int ops, Key range) {
    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<Key> keyDist(-range, range);
    std::uniform_int_distribution<int> opDist(0, 2);
    std::set<Key> reference;
    auto s = make();

    for (int i = 0; i < ops; ++i) {
        const Key k = keyDist(rng);
        switch (opDist(rng)) {
            case 0: check(s->insert(k) == reference.insert(k).second, "insert vs std::set"); break;
            case 1: check(s->search(k) == (reference.count(k) > 0), "search vs std::set"); break;
            case 2: check(s->remove(k) == (reference.erase(k) > 0), "remove vs std::set"); break;
        }
        if (i % 1000 == 0) check(s->checkInvariants(), "invariantes periodicos");
    }
    check(s->size() == reference.size(), "size final vs std::set");
    check(s->checkInvariants(), "invariantes finales");
    for (Key k : reference) check(s->search(k), "todas las claves presentes");
}

void testAvlHeightBound() {
    eda::domain::AvlTree tree;
    const int n = 100000;
    for (int i = 0; i < n; ++i) tree.insert(i);
    // h <= 1.44 log2(n + 2)
    check(tree.height() <= static_cast<std::size_t>(1.44 * std::log2(n + 2.0)), "cota de altura AVL");
}

}  // namespace

int main() {
    const std::vector<std::pair<std::string, Factory>> structures{
        {"SkipList p=1/2", [] { return std::make_unique<eda::domain::SkipList>(0.5, 20, 1); }},
        {"SkipList p=1/4", [] { return std::make_unique<eda::domain::SkipList>(0.25, 20, 2); }},
        {"AVL", [] { return std::make_unique<eda::domain::AvlTree>(); }},
    };

    for (const auto& [name, make] : structures) {
        std::cout << "== " << name << '\n';
        testEdgeCases(make);
        testOrderedInputs(make, 5000);
        for (std::uint64_t seed = 1; seed <= 5; ++seed) {
            testRandomAgainstStdSet(make, seed, 50000, 500);     // muchas colisiones
            testRandomAgainstStdSet(make, seed, 50000, 1000000); // pocas colisiones
        }
    }
    testAvlHeightBound();

    std::cout << (gFailures == 0 ? "TODAS LAS PRUEBAS PASARON\n" : "HAY FALLOS\n");
    return gFailures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
