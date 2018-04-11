#pragma once

#include <QObject>
#include "../src/dbusutils_p.h"

class TestRemoteRunner : public QObject
{
    Q_OBJECT
public:
    TestRemoteRunner(const QString &serviceName);

public Q_SLOTS:
    RemoteActions Actions();
    RemoteMatches Match(const QString &searchTerm);
    void Run(const QString &id, const QString &actionId);
};
