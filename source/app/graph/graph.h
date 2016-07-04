#ifndef GRAPH_H
#define GRAPH_H

#include "shared/graph/igraph.h"
#include "shared/graph/elementid.h"
#include "elementiddistinctsetcollection.h"
#include "graphconsistencychecker.h"

#include "../utils/debugpauser.h"

#include <QObject>

#include <vector>
#include <unordered_set>
#include <memory>

class GraphComponent;
class ComponentManager;
class ComponentSplitSet;
class ComponentMergeSet;

class Node : public INode
{
    friend class MutableGraph;

private:
    NodeId _id;
    EdgeIdDistinctSet _inEdgeIds;
    EdgeIdDistinctSet _outEdgeIds;

public:
    int degree() const { return _inEdgeIds.size() + _outEdgeIds.size(); }
    NodeId id() const { return _id; }
};

class Edge : public IEdge
{
    friend class MutableGraph;

private:
    EdgeId _id;
    NodeId _sourceId;
    NodeId _targetId;

public:
    Edge() {}

    Edge(const IEdge& other) :
        _id(other.id()),
        _sourceId(other.sourceId()),
        _targetId(other.targetId())
    {}

    Edge(IEdge&& other) noexcept :
        _id(other.id()),
        _sourceId(other.sourceId()),
        _targetId(other.targetId())
    {}

    Edge& operator=(const IEdge& other)
    {
        if(this != &other)
        {
            _id         = other.id();
            _sourceId   = other.sourceId();
            _targetId   = other.targetId();
        }

        return *this;
    }

    NodeId sourceId() const { return _sourceId; }
    NodeId targetId() const { return _targetId; }
    NodeId oppositeId(NodeId nodeId) const
    {
        if(nodeId == _sourceId)
            return _targetId;
        else if(nodeId == _targetId)
            return _sourceId;

        return NodeId();
    }

    bool isLoop() const { return _sourceId == _targetId; }

    EdgeId id() const { return _id; }
};

class Graph : public QObject, public IGraph
{
    Q_OBJECT

public:
    Graph();
    virtual ~Graph();

    NodeId firstNodeId() const;
    bool containsNodeId(NodeId nodeId) const;

    virtual NodeIdDistinctSetCollection::Type typeOf(NodeId nodeId) const = 0;
    virtual ConstNodeIdDistinctSet mergedNodeIdsForNodeId(NodeId nodeId) const = 0;

    EdgeId firstEdgeId() const;
    bool containsEdgeId(EdgeId edgeId) const;

    virtual EdgeIdDistinctSetCollection::Type typeOf(EdgeId edgeId) const = 0;
    virtual ConstEdgeIdDistinctSet mergedEdgeIdsForEdgeId(EdgeId edgeId) const = 0;

    virtual EdgeIdDistinctSets edgeIdsForNodeId(NodeId nodeId) const = 0;

    template<typename C> EdgeIdSet edgeIdsForNodeIds(const C& nodeIds) const
    {
        EdgeIdSet edgeIds;

        for(auto nodeId : nodeIds)
        {
            for(auto edgeId : edgeIdsForNodeId(nodeId))
                edgeIds.insert(edgeId);
        }

        return edgeIds;
    }

    template<typename C> std::vector<Edge> edgesForNodeIds(const C& nodeIds) const
    {
        auto edgeIds = edgeIdsForNodeIds(nodeIds);
        std::vector<Edge> edges;

        for(auto edgeId : edgeIds)
            edges.emplace_back(edgeById(edgeId));

        return edges;
    }

    virtual void reserve(const Graph& other) = 0;
    virtual void cloneFrom(const Graph& other) = 0;

    void enableComponentManagement();

    virtual const std::vector<ComponentId>& componentIds() const;
    int numComponents() const;
    const GraphComponent* componentById(ComponentId componentId) const;
    ComponentId componentIdOfNode(NodeId nodeId) const;
    ComponentId componentIdOfEdge(EdgeId edgeId) const;
    ComponentId componentIdOfLargestComponent() const;

    template<typename C> ComponentId componentIdOfLargestComponent(const C& componentIds) const
    {
        ComponentId largestComponentId;
        int maxNumNodes = 0;
        for(auto componentId : componentIds)
        {
            auto component = componentById(componentId);
            if(component->numNodes() > maxNumNodes)
            {
                maxNumNodes = component->numNodes();
                largestComponentId = componentId;
            }
        }

        return largestComponentId;
    }

    // Call this to ensure the Graph is in a consistent state
    // Usually it is called automatically and is generally only
    // necessary when accessing the Graph before changes have
    // been completed
    virtual void update() {}

    // Informational messages to indicate progress
    virtual void setPhase(const QString& phase) const;
    virtual void clearPhase() const;
    virtual QString phase() const;

    virtual void setSubPhase(const QString& subPhase) const;
    virtual void clearSubPhase() const;
    virtual QString subPhase() const;

    mutable DebugPauser debugPauser;
    void dumpToQDebug(int detail) const;

private:
    template<typename, typename> friend class NodeArray;
    template<typename, typename> friend class EdgeArray;
    template<typename, typename> friend class ComponentArray;

    NodeId _nextNodeId;
    EdgeId _nextEdgeId;

    mutable std::mutex _nodeArraysMutex;
    mutable std::unordered_set<IGraphArray*> _nodeArrays;
    mutable std::mutex _edgeArraysMutex;
    mutable std::unordered_set<IGraphArray*> _edgeArrays;

    std::unique_ptr<ComponentManager> _componentManager;

    mutable std::recursive_mutex _phaseMutex;
    mutable QString _phase;
    mutable QString _subPhase;
    GraphConsistencyChecker _graphConsistencyChecker;

    void insertNodeArray(IGraphArray* nodeArray) const;
    void eraseNodeArray(IGraphArray* nodeArray) const;

    void insertEdgeArray(IGraphArray* edgeArray) const;
    void eraseEdgeArray(IGraphArray* edgeArray) const;

    int numComponentArrays() const;
    void insertComponentArray(IGraphArray* componentArray) const;
    void eraseComponentArray(IGraphArray* componentArray) const;

    bool isComponentManaged() const { return _componentManager != nullptr; }

protected:
    NodeId nextNodeId() const;
    NodeId largestNodeId() const { return nextNodeId() - 1; }
    virtual void reserveNodeId(NodeId nodeId);
    EdgeId nextEdgeId() const;
    EdgeId largestEdgeId() const { return nextEdgeId() - 1; }
    virtual void reserveEdgeId(EdgeId edgeId);

signals:
    // The signals are listed here in the order in which they are emitted
    void graphWillChange(const Graph*) const;

    void nodeAdded(const Graph*, NodeId) const;
    void nodeRemoved(const Graph*, NodeId) const;
    void edgeAdded(const Graph*, EdgeId) const;
    void edgeRemoved(const Graph*, EdgeId) const;

    void componentsWillMerge(const Graph*, const ComponentMergeSet&) const;
    void componentWillBeRemoved(const Graph*, ComponentId, bool) const;
    void componentAdded(const Graph*, ComponentId, bool) const;
    void componentSplit(const Graph*, const ComponentSplitSet&) const;

    void nodeRemovedFromComponent(const Graph*, NodeId, ComponentId) const;
    void edgeRemovedFromComponent(const Graph*, EdgeId, ComponentId) const;
    void nodeAddedToComponent(const Graph*, NodeId, ComponentId) const;
    void edgeAddedToComponent(const Graph*, EdgeId, ComponentId) const;

    void graphChanged(const Graph*) const;

    void phaseChanged() const;
};

#endif // GRAPH_H
