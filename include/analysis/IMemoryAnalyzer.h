#ifndef IMEMORYANALYZER_H
#define IMEMORYANALYZER_H

#include <QString>

class IMemoryAnalyzer
{
public:
    virtual ~IMemoryAnalyzer() = default;

    virtual bool analyze(const QString& filePath) = 0;
    virtual bool hasError() const = 0;
    virtual bool hasMemoryLeak() const = 0;
    virtual void printElements() const = 0;
};

#endif // IMEMORYANALYZER_H
