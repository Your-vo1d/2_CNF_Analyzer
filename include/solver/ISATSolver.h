#ifndef ISATSOLVER_H
#define ISATSOLVER_H

#include <QList>
#include <QMap>
#include <QString>
#include "Connection.h"
#include "TreeElement.h"

class ISATSolver
{
public:
    virtual ~ISATSolver() = default;
    virtual bool solve(const QList<Connection>& connections,
                       const QMap<QString, TreeElement*>& elements) = 0;
};

#endif // ISATSOLVER_H
