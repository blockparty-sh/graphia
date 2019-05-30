#include "nodepositions.h"

#include <cmath>
#include <numeric>

const QVector3D& NodePositions::get(NodeId nodeId) const
{
    return elementFor(nodeId).newest();
}

QVector3D NodePositions::getScaledAndSmoothed(NodeId nodeId) const
{
    return elementFor(nodeId).mean(_smoothing) * _scale;
}

void NodePositions::set(NodeId nodeId, const QVector3D& position)
{
    Q_ASSERT(!std::isnan(position.x()) && !std::isnan(position.y()) && !std::isnan(position.z()));
    elementFor(nodeId).push_back(position);
}

void NodePositions::setExact(NodeId nodeId, const QVector3D& position)
{
    elementFor(nodeId).fill(position);
}

void NodePositions::update(const NodePositions& other)
{
    std::unique_lock<std::recursive_mutex> lock(_mutex);

    _array = other._array;
}

QVector3D NodePositions::centreOfMass(const NodePositions& nodePositions,
                                      const std::vector<NodeId>& nodeIds)
{
    float reciprocal = 1.0f / nodeIds.size();
    QVector3D centreOfMass;

    for(auto nodeId : nodeIds)
        centreOfMass += (nodePositions.get(nodeId) * reciprocal);

    return centreOfMass;
}

QVector3D NodePositions::centreOfMassScaledAndSmoothed(const NodePositions& nodePositions, const std::vector<NodeId>& nodeIds)
{
    float reciprocal = 1.0f / nodeIds.size();

    return std::accumulate(nodeIds.begin(), nodeIds.end(), QVector3D(),
    [&](const auto& centreOfMass, auto nodeId)
    {
        return centreOfMass + (nodePositions.getScaledAndSmoothed(nodeId) * reciprocal);
    });
}

std::vector<QVector3D> NodePositions::positionsVector(const NodePositions& nodePositions, const std::vector<NodeId>& nodeIds)
{
    std::vector<QVector3D> positionsVector;
    positionsVector.reserve(nodeIds.size());

    std::transform(nodeIds.begin(), nodeIds.end(), std::back_inserter(positionsVector),
    [&](auto nodeId)
    {
        return nodePositions.get(nodeId);
    });

    return positionsVector;
}

std::vector<QVector3D> NodePositions::positionsVectorScaled(const NodePositions& nodePositions, const std::vector<NodeId>& nodeIds)
{
    std::vector<QVector3D> positionsVector;
    positionsVector.reserve(nodeIds.size());

    std::transform(nodeIds.begin(), nodeIds.end(), std::back_inserter(positionsVector),
    [&](auto nodeId)
    {
        return nodePositions.getScaledAndSmoothed(nodeId);
    });

    return positionsVector;
}

// http://stackoverflow.com/a/24818473
BoundingSphere NodePositions::boundingSphere(const NodePositions& nodePositions, const std::vector<NodeId>& nodeIds)
{
    QVector3D center = nodePositions.getScaledAndSmoothed(nodeIds.front());
    float radius = 0.0001f;
    QVector3D pos, diff;
    float len, alpha, alphaSq;

    for(int i = 0; i < 2; i++)
    {
        for(auto& nodeId : nodeIds)
        {
            pos = nodePositions.getScaledAndSmoothed(nodeId);
            diff = pos - center;
            len = diff.length();

            if(len > radius)
            {
                alpha = len / radius;
                alphaSq = alpha * alpha;
                radius = 0.5f * (alpha + 1.0f / alpha) * radius;
                center = 0.5f * ((1.0f + 1.0f / alphaSq) * center + (1.0f - 1.0f / alphaSq) * pos);
            }
        }
    }

    for(auto& nodeId : nodeIds)
    {
        pos = nodePositions.getScaledAndSmoothed(nodeId);
        diff = pos - center;
        len = diff.length();
        if(len > radius)
        {
            radius = (radius + len) / 2.0f;
            center = center + ((len - radius) / len * diff);
        }
    }

    return {center, radius};
}
