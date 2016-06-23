#include "gmlfileparser.h"
#include "shared/graph/imutablegraph.h"
#include "shared/plugins/basegenericplugin.h"

#ifdef _MSC_VER
#pragma warning(disable:4503) // AXE makes a lot of these
#endif

#include "thirdparty/axe/include/axe.h"

#include <QTime>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QDebug>

#include <fstream>

template<class It> bool parseGml(IMutableGraph &graph,
                                 BaseGenericPluginInstance* genericPluginInstance,
                                 const IParser::ProgressFn& progress,
                                 It begin, It end)
{
    int percentage = 0;
    int size = std::distance(begin, end);

    // Progress Capture event (Fired on gml_value rule match)
    auto captureCount = axe::e_ref([&percentage, &size, &begin, progress](It, It i2)
    {
        int lengthLeft = (std::distance(begin, i2)) * 100 / size;
        int newPercentage = lengthLeft;
        if(percentage < newPercentage)
        {
            percentage = newPercentage;
            progress(percentage);
        }
    });

    // General GML structure rules
    axe::r_rule<It> keyValue;
    double d;
    auto whitespace = *axe::r_any(" \t\n\r");

    // If this is declared and initialised on the same line, the move constructor for
    // axe::r_rule is called which (for some reason) infinitely recurses, whereas using
    // operator= doesn't
    axe::r_rule<It> quotedString;
    quotedString = '"' & *("\\\"" | (axe::r_any() - '"')) & '"';

    auto key = (+axe::r_alnum());
    auto value = axe::r_double(d) | quotedString;

    auto keyValueList = whitespace & key & whitespace & '[' & whitespace &
            axe::r_many(keyValue - axe::r_char(']'), axe::r_any(" \t\n\r"), 0) & whitespace & ']';
    auto keyValuePair = (whitespace & key & whitespace & value);

    // Node State
    int id = -1;
    std::string label;
    std::map<int, NodeId> nodeIndexMap;

    // Node Capture Events
    auto captureNodeId = axe::e_ref([&id](It i1, It i2)
    {
        id = std::stoi(std::string(i1, i2));
    });

    auto captureLabel = axe::e_ref([&label](It i1, It i2)
    {
        label = std::string(i1, i2);
    });

    auto captureNode = axe::e_ref([&nodeIndexMap, &id, &graph, &label,
                                  &genericPluginInstance](It, It)
    {
        if(id >= 0)
        {
            nodeIndexMap[id] = graph.addNode();
            genericPluginInstance->setNodeName(nodeIndexMap[id],
                                               QString::fromStdString(label));
            label.clear();
            id = -1;
        }
    });

    // Node Rules
    axe::r_rule<It> nodeKeyValuePair;
    auto nodeKeyValueList = (whitespace & "node" & whitespace & '[' & whitespace &
                           axe::r_many(nodeKeyValuePair - axe::r_char(']'), axe::r_any(" \t\n\r"), 0) & whitespace
                           & ']') >> captureNode;
    auto nodeId = whitespace & "id" & whitespace & value >> captureNodeId;
    auto labelElement = whitespace & "label" & whitespace & value >> captureLabel;
    nodeKeyValuePair = nodeId | labelElement | keyValuePair | keyValueList;

    // Edge State
    int source, target;
    bool isSourceSet = false, isTargetSet = false;

    // Edge Capture Events
    auto captureEdgeSource = axe::e_ref([&source, &isSourceSet](It i1, It i2)
    {
        source = std::stoi(std::string(i1,i2));
        isSourceSet = true;
    });

    auto captureEdgeTarget = axe::e_ref([&target, &isTargetSet](It i1, It i2)
    {
        target = std::stoi(std::string(i1,i2));
        isTargetSet = true;
    });

    auto captureEdge = axe::e_ref([&source, &target, &nodeIndexMap, &graph, &isSourceSet, &isTargetSet](It, It)
    {
        // Check if Target and Source values are set
        if(isTargetSet && isSourceSet)
        {
            NodeId sourceId = nodeIndexMap[source];
            NodeId targetId = nodeIndexMap[target];
            graph.addEdge(sourceId, targetId);
            isTargetSet = false;
            isSourceSet = false;
        }
    });

    // Edge Rules
    axe::r_rule<It> edgeKeyValuePair;
    auto edgeKeyValueList =  (whitespace & "edge" & whitespace & '[' & whitespace &
                            axe::r_many(edgeKeyValuePair - axe::r_char(']'), axe::r_any(" \t\n\r"), 0) & whitespace &
                            ']' ) >> captureEdge;
    auto edgeSource = (whitespace & "source" & whitespace & value >> captureEdgeSource);
    auto edgeTarget = (whitespace & "target" & whitespace & value >> captureEdgeTarget);
    edgeKeyValuePair = edgeSource | edgeTarget | keyValuePair | keyValueList;

    // Full GML file rule
    auto file = +(keyValue & axe::r_any(" \t\n\r"));
    bool succeeded = true;

    // Failure capture event
    auto onFail = [&succeeded, &begin](It, It)
    {
        succeeded = false;
    };

    // All GML keyValue options
    keyValue = ((edgeKeyValueList | nodeKeyValueList | keyValueList | keyValuePair | +axe::r_any(" \t\n\r") |
              axe::r_end() | axe::r_fail(onFail)) >> captureCount);

    // Perform file rule against begin & end iterators
    (file >> axe::e_ref([progress](It, It)
    {
        progress(100);
    }) & axe::r_end())(begin, end);

    return succeeded;
}

GmlFileParser::GmlFileParser(BaseGenericPluginInstance* genericPluginInstance) :
    _genericPluginInstance(genericPluginInstance)
{}

bool GmlFileParser::parse(const QUrl& url, IMutableGraph& graph, const ProgressFn& progress)
{
    QString localFile = url.toLocalFile();
    std::ifstream stream(localFile.toStdString());
    stream.unsetf(std::ios::skipws);

    std::istreambuf_iterator<char> startIt(stream.rdbuf());

    if(!QFileInfo(localFile).exists())
        return false;

    std::vector<char> vec(startIt, std::istreambuf_iterator<char>());

    return parseGml(graph, _genericPluginInstance,  progress, vec.begin(), vec.end());;
}
