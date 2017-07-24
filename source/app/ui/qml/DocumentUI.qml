import QtQuick 2.7
import QtQuick.Controls 1.5
import QtQuick.Layouts 1.3
import QtQuick.Window 2.2
import QtQml.Models 2.2

import com.kajeka 1.0

import "Constants.js" as Constants
import "Utils.js" as Utils

import "Transform"
import "Visualisation"

Item
{
    id: root

    property Application application

    property url fileUrl
    property string fileType
    property string pluginName

    property string title: document.title
    property string status: document.status

    property bool idle: document.idle
    property bool canDelete: document.canDelete

    property bool commandInProgress: document.commandInProgress && !commandTimer.running
    property int commandProgress: document.commandProgress
    property string commandVerb: document.commandVerb
    property bool commandIsCancellable: commandInProgress && document.commandIsCancellable

    property int layoutPauseState: document.layoutPauseState

    property bool canUndo : document.canUndo
    property string nextUndoAction: document.nextUndoAction
    property bool canRedo: document.canRedo
    property string nextRedoAction: document.nextRedoAction

    property bool canResetView: document.canResetView
    property bool canEnterOverviewMode: document.canEnterOverviewMode

    property bool hasPluginUI: document.pluginQmlPath
    property bool pluginPoppedOut: false

    property alias pluginMenu0: pluginMenu0
    property alias pluginMenu1: pluginMenu1
    property alias pluginMenu2: pluginMenu2
    property alias pluginMenu3: pluginMenu3
    property alias pluginMenu4: pluginMenu4

    property int foundIndex: document.foundIndex
    property int numNodesFound: document.numNodesFound

    property var selectPreviousFoundAction: find.selectPreviousAction
    property var selectNextFoundAction: find.selectNextAction

    property color contrastingColor:
    {
        return document.contrastingColor;
    }

    Preferences
    {
        id: visuals
        section: "visuals"
        property string backgroundColor
    }

    property bool _darkBackground: { return Qt.colorEqual(contrastingColor, "white"); }
    property bool _brightBackground: { return Qt.colorEqual(contrastingColor, "black"); }

    function hexToRgb(hex)
    {
        hex = hex.replace("#", "");
        var bigint = parseInt(hex, 16);
        var r = ((bigint >> 16) & 255) / 255.0;
        var g = ((bigint >> 8) & 255) / 255.0;
        var b = (bigint & 255) / 255.0;

        return { r: r, b: b, g: g };
    }

    function colorDiff(a, b)
    {
        if(a === undefined || a === null || a.length === 0)
            return 1.0;

        a = hexToRgb(a);

        var ab = 0.299 * a.r + 0.587 * a.g + 0.114 * a.b;
        var bb = 0.299 * b +   0.587 * b +   0.114 * b;

        return ab - bb;
    }

    property var _lesserContrastingColors:
    {
        var colors = [];

        if(_brightBackground)
            colors = [0.0, 0.4, 0.8];

        colors = [1.0, 0.6, 0.3];

        var color1Diff = colorDiff(visuals.backgroundColor, colors[1]);
        var color2Diff = colorDiff(visuals.backgroundColor, colors[2]);

        // If either of the colors are very similar to the background color,
        // move it towards one of the others, depending on whether it's
        // lighter or darker
        if(Math.abs(color1Diff) < 0.15)
        {
            if(color1Diff < 0.0)
                colors[1] = (colors[0] + colors[1]) * 0.5;
            else
                colors[1] = (colors[1] + colors[2]) * 0.5;
        }
        else if(Math.abs(color2Diff) < 0.15)
        {
            if(color2Diff < 0.0)
                colors[2] = (colors[2]) * 0.5;
            else
                colors[2] = (colors[1] + colors[2]) * 0.5;
        }

        return colors;
    }

    property color lessContrastingColor: { return Qt.rgba(_lesserContrastingColors[1],
                                                          _lesserContrastingColors[1],
                                                          _lesserContrastingColors[1], 1.0); }
    property color leastContrastingColor: { return Qt.rgba(_lesserContrastingColors[2],
                                                           _lesserContrastingColors[2],
                                                           _lesserContrastingColors[2], 1.0); }

    function openFile(fileUrl, fileType, pluginName, parameters)
    {
        if(!document.openFile(fileUrl, fileType, pluginName, parameters))
            return false;

        this.fileUrl = fileUrl;
        this.fileType = fileType;
        this.pluginName = pluginName;

        return true;
    }

    function toggleLayout() { document.toggleLayout(); }
    function selectAll() { document.selectAll(); }
    function selectNone() { document.selectNone(); }
    function invertSelection() { document.invertSelection(); }
    function undo() { document.undo(); }
    function redo() { document.redo(); }
    function deleteSelectedNodes() { document.deleteSelectedNodes(); }
    function resetView() { document.resetView(); }
    function switchToOverviewMode() { document.switchToOverviewMode(); }
    function gotoNextComponent() { document.gotoNextComponent(); }
    function gotoPrevComponent() { document.gotoPrevComponent(); }
    function screenshot() { captureScreenshot.open(); }

    function selectAllFound() { document.selectAllFound(); }
    function selectNextFound() { document.selectNextFound(); }
    function selectPrevFound() { document.selectPrevFound(); }
    function find(text) { document.find(text); }

    function cancelCommand() { document.cancelCommand(); }

    function dumpGraph() { document.dumpGraph(); }

    CaptureScreenshot
    {
        id: captureScreenshot;
        graphView: graph
        application: root.application
    }

    SplitView
    {
        id: splitView

        anchors.fill: parent
        orientation: Qt.Vertical

        Item
        {
            id: graphItem

            Layout.fillHeight: true
            Layout.minimumHeight: 100

            Graph
            {
                id: graph
                anchors.fill: parent

                Label
                {
                    id: emptyGraphLabel
                    text: "Empty Graph"
                    font.pixelSize: 48
                    color: root.contrastingColor
                    opacity: 0.0

                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter

                    states: State
                    {
                        name: "showing"
                        when: plugin.loaded && graph.numNodes <= 0
                        PropertyChanges
                        {
                            target: emptyGraphLabel
                            opacity: 1.0
                        }
                    }

                    transitions: Transition
                    {
                        to: "showing"
                        reversible: true
                        NumberAnimation
                        {
                            properties: "opacity"
                            duration: 1000
                            easing.type: Easing.InOutSine
                        }
                    }
                }
            }

            Column
            {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.margins: Constants.margin
                spacing: Constants.spacing

                Find
                {
                    id: find
                    visible: false
                    document: root
                }

                Label
                {
                    visible: toggleFpsMeterAction.checked

                    color: root.contrastingColor

                    horizontalAlignment: Text.AlignLeft
                    text: document.fps.toFixed(1) + qsTr(" fps")
                }
            }

            Transforms
            {
                visible: plugin.loaded

                anchors.right: parent.right
                anchors.top: parent.top

                enabledTextColor: root.contrastingColor
                disabledTextColor: root.lessContrastingColor
                heldColor: root.leastContrastingColor

                document: document
            }

            Column
            {
                spacing: 10

                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: Constants.margin

                Label
                {
                    visible: toggleGraphMetricsAction.checked

                    color: root.contrastingColor

                    anchors.right: parent.right

                    horizontalAlignment: Text.AlignRight
                    text:
                    {
                        // http://stackoverflow.com/questions/9461621
                        function nFormatter(num, digits)
                        {
                            var si =
                                [
                                    { value: 1E9,  symbol: "G" },
                                    { value: 1E6,  symbol: "M" },
                                    { value: 1E3,  symbol: "k" }
                                ], i;

                            for(i = 0; i < si.length; i++)
                            {
                                if(num >= si[i].value)
                                    return (num / si[i].value).toFixed(digits).replace(/\.?0+$/, "") + si[i].symbol;
                            }

                            return num;
                        }

                        var s = "";
                        var numNodes = graph.numNodes;
                        var numEdges = graph.numEdges;
                        var numVisibleNodes = graph.numVisibleNodes;
                        var numVisibleEdges = graph.numVisibleEdges;

                        if(numNodes >= 0)
                        {
                            s += nFormatter(numNodes, 1);
                            if(numVisibleNodes !== numNodes)
                                s += " (" + nFormatter(numVisibleNodes, 1) + ")";
                            s += " nodes";
                        }

                        if(numEdges >= 0)
                        {
                            s += "\n" + nFormatter(numEdges, 1);
                            if(numVisibleEdges !== numEdges)
                                s += " (" + nFormatter(numVisibleEdges, 1) + ")";
                            s += " edges";
                        }

                        if(graph.numComponents >= 0)
                            s += "\n" + nFormatter(graph.numComponents, 1) + " components";

                        return s;
                    }
                }

                LayoutSettings
                {
                    anchors.right: parent.right

                    visible: toggleLayoutSettingsAction.checked

                    document: document
                    textColor: root.contrastingColor
                }
            }

            Visualisations
            {
                visible: plugin.loaded

                anchors.left: parent.left
                anchors.bottom: parent.bottom

                enabledTextColor: root.contrastingColor
                disabledTextColor: root.lessContrastingColor
                heldColor: root.leastContrastingColor

                document: document
            }
        }

        ColumnLayout
        {
            id: pluginContainer
            visible: plugin.loaded && !root.pluginPoppedOut

            Layout.fillWidth: true

            StatusBar
            {
                Layout.fillWidth: true

                RowLayout
                {
                    anchors.fill: parent

                    Label
                    {
                        text: pluginName
                    }

                    Item
                    {
                        id: pluginContainerToolStrip
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                    }

                    ToolButton { action: togglePluginWindowAction }
                }
            }
        }
    }

    Preferences
    {
        id: window
        section: "window"
        property alias pluginX: pluginWindow.x
        property alias pluginY: pluginWindow.y
        property var pluginWidth
        property var pluginHeight
        property var pluginMaximised
        property alias pluginSplitSize: root.pluginSplitSize
        property alias pluginPoppedOut: root.pluginPoppedOut
    }

    // This is only here to get at the default values of its properties
    PluginContent { id: defaultPluginContent }

    Item
    {
        Layout.fillHeight: true
        Layout.fillWidth: true

        id: plugin
        Layout.minimumHeight: plugin.content !== undefined &&
                              plugin.content.minimumHeight !== undefined ?
                              plugin.content.minimumHeight : defaultPluginContent.minimumHeight
        visible: loaded && enabledChildren

        property var model: document.plugin
        property var content
        property bool loaded: false

        onLoadedChanged:
        {
            if(root.pluginPoppedOut)
                popOutPlugin();
            else
                popInPlugin();

            loadComplete();
        }

        // At least one enabled direct child
        property bool enabledChildren:
        {
            for(var i = 0; i < children.length; i++)
            {
                if(children[i].enabled)
                    return true;
            }

            return false;
        }
    }

    signal loadComplete()

    property int pluginX: pluginWindow.x
    property int pluginY: pluginWindow.y
    property int pluginSplitSize:
    {
        if(!pluginPoppedOut)
        {
            return splitView.orientation == Qt.Vertical ?
                        plugin.height : plugin.width;
        }
        else
            return window.pluginSplitSize
    }

    function loadPluginWindowState()
    {
        if(!window.pluginPoppedOut)
            return;

        if(window.pluginWidth !== undefined &&
           window.pluginHeight !== undefined)
        {
            pluginWindow.width = window.pluginWidth;
            pluginWindow.height = window.pluginHeight;
        }

        if(window.pluginMaximised !== undefined)
        {
            pluginWindow.visibility = Utils.castToBool(window.pluginMaximised) ?
                Window.Maximized : Window.Windowed;
        }
    }

    function savePluginWindowState()
    {
        if(!window.pluginPoppedOut)
            return;

        window.pluginMaximised = pluginWindow.maximised;

        if(!pluginWindow.maximised)
        {
            window.pluginWidth = pluginWindow.width;
            window.pluginHeight = pluginWindow.height;
        }
    }

    property bool destructing: false

    ApplicationWindow
    {
        id: pluginWindow
        width: 800
        height: 600
        minimumWidth: 480
        minimumHeight: 480
        title: application && root.pluginName.length > 0 ?
                   root.pluginName + " - " + application.name : "";
        visible: root.visible && root.pluginPoppedOut && plugin.loaded
        property bool maximised: visibility === Window.Maximized

        onClosing:
        {
            if(visible & !destructing)
                popInPlugin();
        }

        menuBar: MenuBar
        {
            Menu { id: pluginMenu0; visible: false }
            Menu { id: pluginMenu1; visible: false }
            Menu { id: pluginMenu2; visible: false }
            Menu { id: pluginMenu3; visible: false }
            Menu { id: pluginMenu4; visible: false }
        }

        toolBar: ToolBar
        {
            id: pluginWindowToolStrip
            visible: plugin.content !== undefined && plugin.content.toolStrip !== undefined
        }

        RowLayout
        {
            id: pluginWindowContent
            anchors.fill: parent
        }
    }

    Component.onCompleted:
    {
        loadPluginWindowState();
    }

    Component.onDestruction:
    {
        savePluginWindowState();

        // Mild hack to get the plugin window to close before the main window
        destructing = true;
        pluginWindow.close();
    }

    function popOutPlugin()
    {
        root.pluginPoppedOut = true;
        plugin.parent = pluginWindowContent;

        if(plugin.content.toolStrip !== undefined)
            plugin.content.toolStrip.parent = pluginWindowToolStrip.contentItem;

        pluginWindow.x = pluginX;
        pluginWindow.y = pluginY;
    }

    function popInPlugin()
    {
        plugin.parent = pluginContainer;

        if(plugin.content.toolStrip !== undefined)
            plugin.content.toolStrip.parent = pluginContainerToolStrip;

        if(splitView.orientation == Qt.Vertical)
            pluginContainer.height = pluginSplitSize;
        else
            pluginContainer.width = pluginSplitSize;

        root.pluginPoppedOut = false;
    }

    function togglePop()
    {
        if(pluginWindow.visible)
            popInPlugin();
        else
            popOutPlugin();
    }

    function createPluginMenu(index, menu)
    {
        if(!plugin.loaded)
            return false;

        // Check the plugin implements createMenu
        if(typeof plugin.content.createMenu === "function")
            return plugin.content.createMenu(index, menu);

        return false;
    }

    property bool findVisible: find.visible
    function showFind()
    {
        find.show();
    }

    Document
    {
        id: document
        application: root.application
        graph: graph

        onPluginQmlPathChanged:
        {
            if(document.pluginQmlPath.length > 0)
            {
                // Destroy anything already there
                while(plugin.children.length > 0)
                    plugin.children[0].destroy();

                var pluginComponent = Qt.createComponent(document.pluginQmlPath);

                if(pluginComponent.status !== Component.Ready)
                {
                    console.log(pluginComponent.errorString());
                    return;
                }

                plugin.content = pluginComponent.createObject(plugin);

                if(plugin.content === null)
                {
                    console.log(document.pluginQmlPath + ": failed to create instance");
                    return;
                }

                plugin.loaded = true;
                pluginLoaded();
            }
        }
    }

    signal pluginLoaded()

    property var comandProgressSamples: []
    property int commandSecondsRemaining

    onCommandProgressChanged:
    {
        // Reset the sample buffer if the command progress is less than the latest sample (i.e. new command)
        if(comandProgressSamples.length > 0 && commandProgress < comandProgressSamples[comandProgressSamples.length - 1].progress)
            comandProgressSamples.length = 0;

        if(commandProgress < 0)
        {
            commandSecondsRemaining = 0;
            return;
        }

        var sample = {progress: commandProgress, seconds: new Date().getTime() / 1000.0};
        comandProgressSamples.push(sample);

        // Only keep this many samples
        while(comandProgressSamples.length > 10)
            comandProgressSamples.shift();

        // Require a few samples before making the calculation
        if(comandProgressSamples.length < 5)
        {
            commandSecondsRemaining = 0;
            return;
        }

        var earliestSample = comandProgressSamples[0];
        var latestSample = comandProgressSamples[comandProgressSamples.length - 1];
        var percentDelta = latestSample.progress - earliestSample.progress;
        var timeDelta = latestSample.seconds - earliestSample.seconds;
        var percentRemaining = 100.0 - currentDocument.commandProgress;

        commandSecondsRemaining = percentRemaining * timeDelta / percentDelta;
    }

    signal commandStarted();
    signal commandComplete();

    Timer
    {
        id: commandTimer
        interval: 200

        onTriggered:
        {
            stop();
            commandStarted();
        }
    }

    Connections
    {
        target: document

        onCommandInProgressChanged:
        {
            if(document.commandInProgress)
                commandTimer.start();
            else
            {
                commandTimer.stop();
                commandComplete();
            }
        }
    }
}
