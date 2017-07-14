#ifndef IPARSER_H
#define IPARSER_H

#include <functional>

#include "shared/utils/cancellable.h"

class QUrl;
class IMutableGraph;
class IPluginInstance;

class IParser : public Cancellable
{
public:
    virtual ~IParser() = default;

    using ProgressFn = std::function<void(int)>;

    virtual bool parse(const QUrl& url,
                       IMutableGraph& mutableGraph,
                       IPluginInstance& pluginInstance,
                       const ProgressFn& progressReportFn) = 0;
};

#endif // IPARSER_H
