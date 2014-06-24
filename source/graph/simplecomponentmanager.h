#ifndef SIMPLECOMPONENTMANAGER_H
#define SIMPLECOMPONENTMANAGER_H

#include "componentmanager.h"
#include "grapharray.h"

#include <map>
#include <queue>

/*
This class is somewhat sub-optimal in that it simply does a depth first
search on the graph resulting in O(n) performance. In the static case this
is perfectly acceptable as it only needs to be performed once, but in the
dynamic case we potentially have many graph changes in a short period of
time meaning that a graph's component configuration may change frequently.

O(log^2 n) performance is achievable using the Holm, de Lichtenberg and
Thorup algorithm explained in the paper "Poly-Logarithmic Deterministic
Fully-Dynamic Algorithms for Connectivity, Minimum Spanning Tree, 2-Edge,
and Biconnectivity". So the plan is to implement this as a replacement
for SimpleComponentManager in future.

Some links:
http://www.mpi-inf.mpg.de/departments/d1/teaching/ss12/AdvancedGraphAlgorithms/Slides08.pdf
http://courses.csail.mit.edu/6.851/spring07/scribe/lec05.pdf
http://www.youtube.com/watch?v=5NEzZPYs04c#t=3076
*/

class SimpleComponentManager : public ComponentManager
{
    Q_OBJECT
private:
    std::vector<ComponentId> _componentIdsList;
    ComponentId _nextComponentId;
    std::queue<ComponentId> _vacatedComponentIdQueue;
    std::map<ComponentId, std::shared_ptr<GraphComponent>> _componentsMap;
    ElementIdSet<ComponentId> _updatesRequired;
    std::map<NodeId, ComponentId> _nodesComponentId;
    std::map<EdgeId, ComponentId> _edgesComponentId;

    ComponentId generateComponentId();
    void releaseComponentId(ComponentId componentId);
    void queueGraphComponentUpdate(const Graph* graph, ComponentId componentId);
    void updateGraphComponent(const Graph* graph, ComponentId componentId);
    void removeGraphComponent(ComponentId componentId);

public:
    SimpleComponentManager(const Graph& graph) :
        ComponentManager(graph),
        _nextComponentId(0)
    {}

private:
    // A more sophisticated implementation may make use of these
    void onNodeAdded(const Graph*, NodeId) {}
    void onNodeWillBeRemoved(const Graph*, NodeId) {}

    void onEdgeAdded(const Graph*, EdgeId) {}
    void onEdgeWillBeRemoved(const Graph*, EdgeId) {}

    void onGraphChanged(const Graph* graph);

private:
    void updateComponents(const Graph* graph);
    int componentArrayCapacity() const { return _nextComponentId; }
    ElementIdSet<ComponentId> assignConnectedElementsComponentId(const Graph* graph, NodeId rootId, ComponentId componentId,
                                                                 std::map<NodeId, ComponentId>& nodesComponentId,
                                                                 std::map<EdgeId, ComponentId>& edgesComponentId);

public:
    const std::vector<ComponentId>& componentIds() const;
    std::shared_ptr<const GraphComponent> componentById(ComponentId componentId);
    ComponentId componentIdOfNode(NodeId nodeId) const;
    ComponentId componentIdOfEdge(EdgeId edgeId) const;
};

#endif // SIMPLECOMPONENTMANAGER_H
