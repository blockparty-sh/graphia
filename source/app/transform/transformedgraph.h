#ifndef TRANSFORMEDGRAPH_H
#define TRANSFORMEDGRAPH_H

#include "graphtransform.h"
#include "transformcache.h"

#include "graph/graph.h"
#include "graph/mutablegraph.h"

#include "shared/graph/grapharray.h"
#include "shared/utils/passkey.h"

#include "attributes/attribute.h"

#include <QObject>
#include <QString>
#include <QStringList>

#include <functional>
#include <atomic>
#include <mutex>

class GraphModel;
class ICommand;

class TransformedGraph : public Graph
{
    Q_OBJECT

public:
    explicit TransformedGraph(GraphModel& graphModel, const MutableGraph& source);

    void enableAutoRebuild() { _autoRebuild = true; rebuild(); }
    void cancelRebuild();
    void addTransform(std::unique_ptr<GraphTransform> t) { _transforms.emplace_back(std::move(t)); }
    void clearTransforms() { _transforms.clear(); }
    int numTransforms() const { return static_cast<int>(_transforms.size()); }

    void setCommand(ICommand* command) { _command = command; }

    const std::vector<NodeId>& nodeIds() const override { return _target.nodeIds(); }
    int numNodes() const override { return _target.numNodes(); }
    const INode& nodeById(NodeId nodeId) const override { return _target.nodeById(nodeId); }
    bool containsNodeId(NodeId nodeId) const override { return _target.containsNodeId(nodeId); }
    MultiElementType typeOf(NodeId nodeId) const override { return _target.typeOf(nodeId); }
    ConstNodeIdDistinctSet mergedNodeIdsForNodeId(NodeId nodeId) const override { return _target.mergedNodeIdsForNodeId(nodeId); }
    int multiplicityOf(NodeId nodeId) const override { return _target.multiplicityOf(nodeId); }

    const std::vector<EdgeId>& edgeIds() const override { return _target.edgeIds(); }
    int numEdges() const override { return _target.numEdges(); }
    const IEdge& edgeById(EdgeId edgeId) const override { return _target.edgeById(edgeId); }
    bool containsEdgeId(EdgeId edgeId) const override { return _target.containsEdgeId(edgeId); }
    MultiElementType typeOf(EdgeId edgeId) const override { return _target.typeOf(edgeId); }
    ConstEdgeIdDistinctSet mergedEdgeIdsForEdgeId(EdgeId edgeId) const override { return _target.mergedEdgeIdsForEdgeId(edgeId); }
    int multiplicityOf(EdgeId edgeId) const override { return _target.multiplicityOf(edgeId); }

    EdgeIdDistinctSets edgeIdsForNodeId(NodeId nodeId) const override { return _target.edgeIdsForNodeId(nodeId); }

    std::vector<EdgeId> edgeIdsBetween(NodeId nodeIdA, NodeId nodeIdB) const override { return _target.edgeIdsBetween(nodeIdA, nodeIdB); }

    void setPhase(const QString& phase) const override { _source->setPhase(phase); }
    void clearPhase() const override { _source->clearPhase(); }
    QString phase() const override { return _source->phase(); }

    void setProgress(int progress);

    MutableGraph& mutableGraph() { return _target; }

    void reserve(const Graph& other) override;
    TransformedGraph& operator=(const MutableGraph& other);

    bool update() override;

    // The obscure looking parameters here ensure that only GraphTransform can call these functions
    void resetChangeOccurred(const PassKey<GraphTransform>&) { _graphChangeOccurred = false; }
    bool changeOccurred(const PassKey<GraphTransform>&) const { return _graphChangeOccurred; }

    std::vector<QString> createdAttributeNamesAtTransformIndex(int index) const;

private:
    GraphModel* _graphModel = nullptr;

    const MutableGraph* _source;
    std::vector<std::unique_ptr<GraphTransform>> _transforms;

    // TransformedGraph has the target as a member rather than inheriting
    // from MutableGraph for two reasons:
    //   1. A TransformedGraph shouldn't be mutable
    //   2. The signals the target emits must be intercepted before being
    //      passed on to other parts of the application
    MutableGraph _target;

    TransformCache _cache;

    using CreatedAttributeNamesMap = std::map<int, std::vector<QString>>;
    CreatedAttributeNamesMap _createdAttributeNames;

    bool _graphChangeOccurred = false;
    bool _changeSignalsEmitted = false;
    bool _autoRebuild = false;
    ICommand* _command = nullptr;

    std::atomic_bool _cancelled;

    std::mutex _currentTransformMutex;
    GraphTransform* _currentTransform = nullptr;

    class State
    {
    private:
        enum class Value { Removed, Unchanged, Added };
        Value state = Value::Unchanged;

    public:
        void add()     { state = state == Value::Removed ? Value::Unchanged : Value::Added; }
        void remove()  { state = state == Value::Added ?   Value::Unchanged : Value::Removed; }

        bool added() const   { return state == Value::Added; }
        bool removed() const { return state == Value::Removed; }
    };

    NodeArray<State> _nodesState;
    EdgeArray<State> _edgesState;
    NodeArray<State> _previousNodesState;
    EdgeArray<State> _previousEdgesState;

    void rebuild();

    void setCurrentTransform(GraphTransform* currentTransform);

private slots:
    void onTargetGraphChanged(const Graph* graph);

signals:
    void attributeValuesChanged(QStringList attributeNames);
};

#endif // TRANSFORMEDGRAPH_H
