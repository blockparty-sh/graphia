#include "jsongraphparser.h"

#include "shared/utils/container.h"
#include "shared/utils/string.h"
#include "shared/graph/igraphmodel.h"
#include "shared/graph/imutablegraph.h"

#include <QDataStream>
#include <QFile>
#include <QUrl>

bool JsonGraphParser::parse(const QUrl &url, IGraphModel *graphModel)
{
    QFile file(url.toLocalFile());
    QByteArray byteArray;

    if(!file.open(QIODevice::ReadOnly))
        return false;

    auto totalBytes = file.size();

    if(totalBytes == 0)
        return false;

    int bytesRead = 0;
    QDataStream input(&file);

    do
    {
        const int ChunkSize = 2 << 16;
        std::vector<unsigned char> buffer(ChunkSize);

        auto numBytes = input.readRawData(reinterpret_cast<char*>(buffer.data()), ChunkSize);
        byteArray.append(reinterpret_cast<char*>(buffer.data()), numBytes);

        bytesRead += numBytes;

        setProgress((bytesRead * 100) / totalBytes);
    } while(!input.atEnd());

    auto jsonBody = parseJsonFrom(byteArray, this);

    if(cancelled())
        return false;

    if(jsonBody.is_null() || !jsonBody.is_object())
    {
        setFailureReason(QObject::tr("Body is empty, or not an object."));
        return false;
    }

    const json* graph = nullptr;

    if(u::contains(jsonBody, "graphs") && jsonBody["graphs"].is_array() && jsonBody["graphs"].size() > 0)
        graph = &jsonBody["graphs"].at(0);
    else if(u::contains(jsonBody, "graph") && jsonBody["graph"].is_object())
        graph = &jsonBody["graph"];
    else
    {
        setFailureReason(QObject::tr("Body doesn't contain a graph object."));
        return false;
    }

    return parseGraphObject(*graph, graphModel, *this, false, _userNodeData, _userEdgeData);
}

bool JsonGraphParser::parseGraphObject(const json& jsonGraphObject, IGraphModel* graphModel,
                                       IParser& parser, bool useElementIdsLiterally,
                                       UserNodeData* userNodeData, UserEdgeData* userEdgeData)
{
    if(!u::contains(jsonGraphObject, "nodes") || !u::contains(jsonGraphObject, "edges"))
    {
        parser.setFailureReason(QObject::tr("Graph doesn't contain nodes or edges arrays."));
        return false;
    }

    const auto& jsonNodes = jsonGraphObject["nodes"];
    const auto& jsonEdges = jsonGraphObject["edges"];

    uint64_t i = 0;

    graphModel->mutableGraph().setPhase(QObject::tr("Nodes"));

    std::map<std::string, NodeId> stringNodeIdToNodeId;
    for(const auto& jsonNode : jsonNodes)
    {
        if(!u::contains(jsonNode, "id") || !jsonNode["id"].is_string())
        {
            parser.setFailureReason(QObject::tr("Node has no ID."));
            return false;
        }

        auto nodeIdString = jsonNode["id"].get<std::string>();

        NodeId nodeId;

        if(useElementIdsLiterally && u::isNumeric(nodeIdString))
        {
            nodeId = std::stoi(nodeIdString);
            graphModel->mutableGraph().reserveNodeId(nodeId);
            nodeId = graphModel->mutableGraph().addNode(nodeId);
        }
        else
            nodeId = graphModel->mutableGraph().addNode();

        stringNodeIdToNodeId[nodeIdString] = nodeId;

        if(u::contains(jsonNode, "label"))
        {
            auto nodeJsonLabel = jsonNode["label"].get<std::string>();
            graphModel->setNodeName(nodeId, QString::fromStdString(nodeJsonLabel));
        }

        if(u::contains(jsonNode, "metadata") && userNodeData != nullptr)
        {
            auto metadata = jsonNode["metadata"];
            for(auto it = metadata.begin(); it != metadata.end(); ++it)
            {
                auto key = QString::fromStdString(it.key());
                QString value;

                if(it.value().is_string())
                    value = QString::fromStdString(it.value().get<std::string>());
                else if(it.value().is_number_integer())
                    value = QString::number(it.value().get<int>());
                else if(it.value().is_number_float())
                    value = QString::number(it.value().get<double>());

                userNodeData->setValueBy(nodeId, key, value);
            }
        }

        parser.setProgress(static_cast<int>((i++ * 100) / jsonNodes.size()));
    }

    parser.setProgress(-1);

    i = 0;

    graphModel->mutableGraph().setPhase(QObject::tr("Edges"));
    for(const auto& jsonEdge : jsonEdges)
    {
        if(!u::contains(jsonEdge, "source") || !u::contains(jsonEdge, "target"))
        {
            parser.setFailureReason(QObject::tr("Edge has no source or target."));
            return false;
        }

        if(!jsonEdge["source"].is_string() || !jsonEdge["target"].is_string())
            return false;

        auto sourceIdString = jsonEdge["source"].get<std::string>();
        auto targetIdString = jsonEdge["target"].get<std::string>();

        if(!u::contains(stringNodeIdToNodeId, sourceIdString) ||
            !u::contains(stringNodeIdToNodeId, targetIdString))
        {
            return false;
        }

        EdgeId edgeId;
        NodeId sourceId = stringNodeIdToNodeId.at(sourceIdString);
        NodeId targetId = stringNodeIdToNodeId.at(targetIdString);

        if(useElementIdsLiterally && u::contains(jsonEdge, "id") && jsonEdge["id"].is_string())
        {
            edgeId = std::stoi(jsonEdge["id"].get<std::string>());

            graphModel->mutableGraph().reserveEdgeId(edgeId);
            edgeId = graphModel->mutableGraph().addEdge(edgeId, sourceId, targetId);
        }
        else
            edgeId = graphModel->mutableGraph().addEdge(sourceId, targetId);

        if(u::contains(jsonEdge, "metadata") && userEdgeData != nullptr)
        {
            auto metadata = jsonEdge["metadata"];
            for(auto it = metadata.begin(); it != metadata.end(); ++it)
            {
                QString key = QString::fromStdString(it.key());
                QString value;

                if(it.value().is_string())
                    value = QString::fromStdString(it.value().get<std::string>());
                else if(it.value().is_number_integer())
                    value = QString::number(it.value().get<int>());
                else if(it.value().is_number_float())
                    value = QString::number(it.value().get<double>());

                userEdgeData->setValueBy(edgeId, key, value);
            }

        }

        parser.setProgress(static_cast<int>((i++ * 100) / jsonEdges.size()));
    }

    parser.setProgress(-1);
    return true;
}
