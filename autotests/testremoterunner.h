#pragma once

#include "../src/dbusutils_p.h"
#include <QObject>

class TestRemoteRunner : public QObject
{
    Q_OBJECT
public:
    TestRemoteRunner(const QString &serviceName, bool showLifecycleMethodCalls);

public Q_SLOTS:
    RemoteActions Actions();
    RemoteMatches Match(const QString &searchTerm);
    void Run(const QString &id, const QString &actionId);
    void Teardown();

private:
    bool m_showLifecycleMethodCalls = false;
};
