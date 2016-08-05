#ifndef GRAPHMODEL_H
#define GRAPHMODEL_H

#include "../graph/graph.h"
#include "../graph/mutablegraph.h"
#include "shared/graph/grapharray.h"

#include "../transform/transformedgraph.h"
#include "../transform/datafield.h"

#include "../ui/graphtransformconfiguration.h"

#include "../layout/nodepositions.h"

#include "shared/plugins/iplugin.h"
#include "shared/graph/igraphmodel.h"

#include <QQuickItem>
#include <QString>
#include <QStringList>
#include <QColor>
#include <QFlags>

#include <memory>
#include <utility>
#include <map>

class SelectionManager;
class SearchManager;

enum VisualFlags
{
    None     = 0x0,
    Selected = 0x1,
    NotFound = 0x2
};

Q_DECLARE_FLAGS(VisualState, VisualFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(VisualState)

struct NodeVisual
{
    float _size = 1.0f;
    QColor _color;
    VisualState _state;
};

using NodeVisuals = NodeArray<NodeVisual>;

struct EdgeVisual
{
    float _size = 1.0f;
    QColor _color;
    VisualState _state;
};

using EdgeVisuals = EdgeArray<EdgeVisual>;

class GraphModel : public QObject, public IGraphModel
{
    Q_OBJECT
public:
    GraphModel(const QString& name, IPlugin* plugin);

private:
    MutableGraph _graph;
    TransformedGraph _transformedGraph;
    NodePositions _nodePositions;
    NodeVisuals _nodeVisuals;
    EdgeVisuals _edgeVisuals;

    NodeArray<QString> _nodeNames;

    QString _name;
    IPlugin* _plugin;

    std::map<QString, DataField> _dataFields;
    std::map<QString, std::pair<DataFieldElementType, std::unique_ptr<GraphTransformFactory>>> _graphTransformFactories;

    void updateVisuals();

public:
    MutableGraph& mutableGraph() { return _graph; }
    const Graph& graph() const { return _transformedGraph; }
    NodePositions& nodePositions() { return _nodePositions; }
    const NodePositions& nodePositions() const { return _nodePositions; }

    const NodeVisuals& nodeVisuals() const { return _nodeVisuals; }
    const EdgeVisuals& edgeVisuals() const { return _edgeVisuals; }

    const NodeArray<QString>& nodeNames() const { return _nodeNames; }

    QString nodeName(NodeId nodeId) const { return _nodeNames[nodeId]; }
    void setNodeName(NodeId nodeId, const QString& name) { _nodeNames[nodeId] = name; }

    const QString& name() const { return _name; }

    bool editable() const { return _plugin->editable(); }
    QString pluginQmlPath() const { return _plugin->qmlPath(); }

    void buildTransforms(const std::vector<GraphTransformConfiguration>& graphTransformConfigurations);

    QStringList availableTransformNames() const;
    QStringList availableDataFields(const QString& transformName) const;
    DataFieldType typeOfDataField(const QString& dataFieldName) const;
    const DataField& dataFieldByName(const QString& name) const;
    QStringList avaliableConditionFnOps(const QString& dataFieldName) const;

    QStringList dataFieldNames(DataFieldElementType elementType) const;

    IDataField& dataField(const QString& name);

public slots:
    void onSelectionChanged(const SelectionManager* selectionManager);
    void onFoundNodeIdsChanged(const SearchManager* searchManager);

signals:
    void visualsWillChange();
    void visualsChanged();
};

#endif // GRAPHMODEL_H
