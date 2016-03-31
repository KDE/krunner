#ifndef %{APPNAMEUC}_H
#define %{APPNAMEUC}_H

#include <krunner/abstractrunner.h>

class %{APPNAME} : public Plasma::AbstractRunner
{
    Q_OBJECT

public:
    %{APPNAME}(QObject *parent, const QVariantList &args);
    ~%{APPNAME}();

public: // Plasma::AbstractRunner API
    void match(Plasma::RunnerContext &context) override;
    void run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match) override;
};

#endif
