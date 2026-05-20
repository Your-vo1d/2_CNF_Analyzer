#ifndef MINISATSOLVER_H
#define MINISATSOLVER_H

#include "ISATSolver.h"

class MinisatSolver : public ISATSolver
{
public:
    bool solve(const QList<Connection>& connections,
               const QMap<QString, TreeElement*>& elements) override;
};

#endif // MINISATSOLVER_H
