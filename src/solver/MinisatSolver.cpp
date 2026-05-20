#include "MinisatSolver.h"
#include <minisat/core/Solver.h>
#include <iostream>
#include <vector>

bool MinisatSolver::solve(const QList<Connection>& connections,
                           const QMap<QString, TreeElement*>& elements)
{
    if (elements.isEmpty() && connections.isEmpty()) {
        std::cout << "Tree is empty (all memory freed) - no memory leak" << std::endl;
        return false;
    }

    Minisat::Solver solver;

    size_t maxPos = 0;
    for (auto it = elements.constBegin(); it != elements.constEnd(); ++it) {
        size_t pos = static_cast<size_t>(it.value()->getPosition());
        if (pos > maxPos) maxPos = pos;
    }
    size_t totalVariables = maxPos + 1;

    if (totalVariables == 0) {
        std::cout << "No variables in CNF - no memory leak" << std::endl;
        return false;
    }

    std::vector<Minisat::Var> vars(totalVariables);
    for (size_t i = 0; i < totalVariables; ++i)
        vars[i] = solver.newVar();

    if (connections.isEmpty() && !elements.isEmpty()) {
        std::cout << "No connections but elements exist - potential memory leak" << std::endl;
        return true;
    }

    for (const Connection& conn : connections) {
        Minisat::vec<Minisat::Lit> clause;
        for (size_t i = 0; i < conn.getPositiveVectorSize(); ++i)
            if (conn.getPositiveBit(i) && i < totalVariables)
                clause.push(Minisat::mkLit(vars[i], false));
        for (size_t i = 0; i < conn.getNegativeVectorSize(); ++i)
            if (conn.getNegativeBit(i) && i < totalVariables)
                clause.push(Minisat::mkLit(vars[i], true));
        if (clause.size() > 0)
            solver.addClause(clause);
    }

    auto isUsed = [&](size_t i) -> bool {
        for (const Connection& conn : connections) {
            if ((i < conn.getPositiveVectorSize() && conn.getPositiveBit(i)) ||
                (i < conn.getNegativeVectorSize() && conn.getNegativeBit(i)))
                return true;
        }
        return false;
    };

    Minisat::vec<Minisat::Lit> allPos;
    for (size_t i = 0; i < totalVariables; ++i)
        if (isUsed(i)) allPos.push(Minisat::mkLit(vars[i], false));
    if (allPos.size() > 0) solver.addClause(allPos);

    Minisat::vec<Minisat::Lit> allNeg;
    for (size_t i = 0; i < totalVariables; ++i)
        if (isUsed(i)) allNeg.push(Minisat::mkLit(vars[i], true));
    if (allNeg.size() > 0) solver.addClause(allNeg);

    bool result = solver.solve();
    if (result) {
        std::cout << "SAT - solution found - MEMORY LEAK DETECTED" << std::endl;
        for (size_t i = 0; i < totalVariables; ++i) {
            if (!isUsed(i)) continue;
            Minisat::lbool value = solver.modelValue(vars[i]);
            QString varName = "Unknown";
            for (auto it = elements.constBegin(); it != elements.constEnd(); ++it) {
                if (it.value()->getPosition() == (int)i) {
                    varName = it.key();
                    break;
                }
            }
            std::cout << varName.toStdString() << " (x" << i << ") = "
                      << (value == Minisat::lbool((uint8_t)0) ? "true" : "false") << std::endl;
        }
    } else {
        std::cout << "UNSAT - no solution found - NO memory leak" << std::endl;
    }
    return result;
}
