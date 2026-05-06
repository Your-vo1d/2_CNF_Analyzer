#include "Parser_JSON.h"

bool Parser_JSON::solveCNF(Tree *tree)
{
    if (!tree) return false;

    if (tree->getElementCount() == 0 && tree->getConnectionCount() == 0)
    {
        std::cout << "Tree is empty (all memory freed) - no memory leak" << std::endl;
        return false;
    }

    Minisat::Solver solver;

    size_t maxPos = 0;
    QMap<QString, TreeElement *> elements = tree->getElements();
    for (auto it = elements.constBegin(); it != elements.constEnd(); ++it)
    {
        size_t pos = static_cast<size_t>(it.value()->getPosition());
        if (pos > maxPos) maxPos = pos;
    }
    size_t totalVariables = maxPos + 1;

    if (totalVariables == 0)
    {
        std::cout << "No variables in CNF - no memory leak" << std::endl;
        return false;
    }

    std::vector<Minisat::Var> vars(totalVariables);
    for (size_t i = 0; i < totalVariables; ++i)
        vars[i] = solver.newVar();

    QList<Connection> connections = tree->getConnections();

    if (connections.size() == 0 && elements.size() > 0)
    {
        std::cout << "No connections but elements exist - potential memory leak" << std::endl;
        return true;
    }

    // клозы из рёбер дерева
    for (const Connection &conn : connections)
    {
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

    // используется ли позиция i хоть одним ребром
    auto isUsed = [&](size_t i) -> bool
    {
        for (const Connection &conn : connections)
        {
            if ((i < conn.getPositiveVectorSize() && conn.getPositiveBit(i)) ||
                (i < conn.getNegativeVectorSize() && conn.getNegativeBit(i)))
                return true;
        }
        return false;
    };

    // хотя бы одна переменная истинна
    Minisat::vec<Minisat::Lit> all_pos;
    for (size_t i = 0; i < totalVariables; ++i)
        if (isUsed(i)) all_pos.push(Minisat::mkLit(vars[i], false));
    if (all_pos.size() > 0) solver.addClause(all_pos);

    // хотя бы одна переменная ложна
    Minisat::vec<Minisat::Lit> all_neg;
    for (size_t i = 0; i < totalVariables; ++i)
        if (isUsed(i)) all_neg.push(Minisat::mkLit(vars[i], true));
    if (all_neg.size() > 0) solver.addClause(all_neg);

    bool result = solver.solve();
    if (result)
    {
        std::cout << "SAT - solution found for tree '" << tree->getName().toStdString()
                  << "' - MEMORY LEAK DETECTED" << std::endl;
        for (size_t i = 0; i < totalVariables; ++i)
        {
            if (!isUsed(i)) continue;
            Minisat::lbool value = solver.modelValue(vars[i]);
            QString var_name = "Unknown";
            for (auto it = elements.constBegin(); it != elements.constEnd(); ++it)
            {
                if (it.value()->getPosition() == (int)i)
                {
                    var_name = it.key();
                    break;
                }
            }
            std::cout << var_name.toStdString() << " (x" << i << ") = "
                      << (value == Minisat::lbool((uint8_t)0) ? "true" : "false") << std::endl;
        }
    }
    else
    {
        std::cout << "UNSAT - no solution found for tree '"
                  << tree->getName().toStdString() << "' - NO memory leak" << std::endl;
    }
    return result;
}
