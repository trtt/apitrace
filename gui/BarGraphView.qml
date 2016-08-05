import QtQuick 2.0
import QtQuick.Controls 1.4
import DataVis 1.0

ScrollView {
    id: scrollview

    function findGraph(x, y) {
        var parent = column
        do {
            var child = parent.childAt(x, y)
            var coord = parent.mapToItem(child, x, y)
            x = coord.x
            y = coord.y
            parent = child
        } while (!child.graph)
        return child.graph
    }

    property real headerWidthMax: 1/4 * width
    property real headerWidth: 0
    property int numGraphs: 10
    property var graphAxis
    property var coarsed
    property var coarse
    property var fine
    property var flickable: flick

    verticalScrollBarPolicy: Qt.ScrollBarAlwaysOn

    Flickable {
        id: flick

        anchors.fill: parent

        clip: true
        contentHeight: column.height


        Component {
            id: columnComponent
            Item {
                id: item

                function setCoarsed() {
                    if (!localCoarsed && graph.needsUpdating) {
                        localCoarsed = true
                        finetimer.restart()
                    }
                }

                property alias graph: graphElement
                property alias header: headerLoader.item
                property bool localCoarsed: true
                property bool inView: {
                    if (parent.parent) {
                        var yTop = parent.parent.mapToItem(flick.contentItem,0,parent.y).y;
                        (yTop + height > flick.contentY) && (yTop < flick.contentY + flick.height)
                    } else false
                }

                Component.onCompleted: {
                    trackingColumn.yChanged.connect(parent.yChanged)
                }

                onInViewChanged: {
                    if (inView) setCoarsed()
                }


                height: 1 * flick.height/numGraphs + 1

                Timer {
                    id: finetimer
                    interval: 300; running: true;
                    onTriggered: {
                        localCoarsed = false;
                    }
                }

                Loader {
                    id: headerLoader
                    sourceComponent:
                        useFullHeader ? fullHeaderComponent : shortHeaderComponent

                    anchors.left: parent.left
                    width: Math.min(headerWidth, headerWidthMax)
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom

                }
                BarGraph {
                    id: graphElement
                    anchors.left: headerLoader.right
                    anchors.right: parent.right
                    height: parent.height

                    bgcolor: (color % 2) ? (useFullHeader ? "#00000000" : "#10000010") :
                        (useFullHeader ? "#10100000" : "#00000000")
                    filtered: useFullHeader ? false : true
                    filter: useFullHeader ? 0 : graphCaption
                    axis: graphAxis
                    data: graphData
                    numElements: Math.max(width * (coarsed ? coarse : (localCoarsed ? coarse : fine)), 0)
                }
                Rectangle {
                    color: "gray"
                    width: parent.width
                    anchors.bottom: parent.bottom
                    height: 1
                }

                Component {
                    id: fullHeaderComponent
                    Item {
                        id: header
                        property real implWidth: caption.implicitWidth +
                        expandSwitch.implicitWidth + vaxis.implicitWidth*4
                        property alias expand: expandSwitch.checked

                        Text {
                            id: caption
                            anchors.left: parent.left
                            width: (parent.width - expandSwitch.implicitWidth)/2
                            anchors.verticalCenter: parent.verticalCenter

                            clip: true
                            text: " " + graphCaption
                        }
                        Rectangle {
                            id: expandSwitch
                            property bool checked: false
                            property bool hovered: false

                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.verticalCenter: parent.verticalCenter
                            width: arrow.implicitHeight + 7
                            height: width

                            radius: 2
                            border.width: 1
                            border.color: "gray"
                            color: hovered ? "lightgray" : "white"

                            Image {
                                id: arrow
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.verticalCenter: parent.verticalCenter
                                source: "resources/arrow.png"
                                rotation: expandSwitch.checked ? 180 : 0
                                transformOrigin: Item.Center
                            }

                            MouseArea {
                                hoverEnabled: true
                                anchors.fill: parent
                                onEntered: parent.hovered = true
                                onExited: parent.hovered = false
                                onClicked: parent.checked = !parent.checked
                            }
                        }
                        VerticalAxis {
                            id: vaxis

                            anchors.right: parent.right
                            width: (parent.width - expandSwitch.implicitWidth)/2
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom

                            clip: true
                            maxY: graphElement.maxY
                        }
                    }
                }

                Component {
                    id: shortHeaderComponent
                    Rectangle {
                        id: header

                        color: "whitesmoke"

                        Text {
                            anchors.left: parent.left
                            width: parent.width/2
                            anchors.verticalCenter: parent.verticalCenter

                            clip: true
                            text: " (" + graphCaption + ")"
                        }
                        VerticalAxis {
                            anchors.right: parent.right
                            width: parent.width/2
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom

                            clip: true
                            maxY: graphElement.maxY
                        }
                    }
                }
            }
        }

        Column {
            id: column

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            Repeater {
                model: graphs
                onItemAdded: headerWidth = Math.max(item.mainGraph.header.implWidth, headerWidth)

                Column {
                    id: mainColumn
                    property alias mainGraph: componentLoader.item

                    anchors.left: parent.left
                    anchors.right: parent.right

                    Loader {
                        id: componentLoader
                        sourceComponent: columnComponent
                        property var graphData: bgdata
                        property var graphCaption: bgcaption
                        property var color: index
                        property bool useFullHeader: true
                        property alias trackingColumn: mainColumn

                        width: parent.width
                    }

                    Component {
                        id: programsComponent
                        Column {
                            Repeater {
                                model: programs
                                Loader {
                                    sourceComponent: columnComponent
                                    property var graphData: bgdata
                                    property var graphCaption: modelData
                                    property var color: index
                                    property bool useFullHeader: false
                                    property var trackingColumn: mainColumn

                                    width: parent.width
                                }
                            }
                        }
                    }

                    Loader {
                        sourceComponent: mainGraph.header.expand ? programsComponent : undefined

                        anchors.left: parent.left
                        anchors.right: parent.right

                        onSourceComponentChanged: {
                            if (item) height = item.childrenRect.height
                            else height = 0
                        }
                    }
                }
            }
        }
    }
}
