/**************************************************************************
 *
 * Copyright 2016 Alexander Trukhin
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **************************************************************************/

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
        } while (child && !child.graph)
        return (child? child.graph : undefined)
    }

    property real headerWidthMax: 1/4 * width
    property real headerWidth: 0
    property int numGraphs: 10
    property var graphData: timelinedata
    property var coarsed
    property var coarse
    property var fine
    property var flickable: flick
    property bool expandable: true
    property bool colored: true
    property var axes: [axisCPU, axisGPU]
    property var axesModel:
        ListModel {
            ListElement {
                caption: "CPU Timeline"
                axisIndex: 0
            }
            ListElement {
                caption: "GPU Timeline"
                axisIndex: 1
            }
        }


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


                height: Math.max(1 * flick.height/numGraphs + 1, 11)

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
                TimelineGraph {
                    id: graphElement
                    anchors.left: headerLoader.right
                    anchors.right: parent.right
                    height: parent.height

                    ignoreUpdates: !inView
                    bgcolor: (color % 2) ? (useFullHeader ? "#00000000" : "#10000010") :
                        (useFullHeader ? "#10100000" : "#00000000")
                    colored: scrollview.colored
                    filtered: useFullHeader ? false : true
                    filter: useFullHeader ? 0 : graphCaption
                    axis: graphAxis
                    data: timelinedata
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
                    Rectangle {
                        id: header
                        property real implWidth: caption.implicitWidth +
                        expandSwitch.implicitWidth
                        property alias expand: expandSwitch.checked

                        color: "white"

                        Text {
                            id: caption
                            anchors.left: parent.left
                            width: parent.width - expandSwitch.implicitWidth
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
                            visible: expandable

                            radius: 2
                            border.width: 1
                            border.color: "gray"
                            color: hovered ? "lightgray" : "white"

                            Image {
                                id: arrow
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.verticalCenter: parent.verticalCenter
                                source: "../resources/arrow.png"
                                rotation: expandSwitch.checked ? 180 : 90
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
                    }
                }

                Component {
                    id: shortHeaderComponent
                    Rectangle {
                        id: header

                        color: "whitesmoke"

                        Text {
                            anchors.left: parent.left
                            width: parent.width
                            anchors.verticalCenter: parent.verticalCenter

                            clip: true
                            text: "pipeline (" + graphCaption + ")"
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
                model: axesModel

                Column {
                    id: mainColumn
                    property alias mainGraph: componentLoader.item

                    anchors.left: parent.left
                    anchors.right: parent.right

                    Loader {
                        id: componentLoader
                        sourceComponent: columnComponent
                        property var graphCaption: caption
                        property var graphAxis: axes[axisIndex]
                        property var color: index
                        property bool useFullHeader: true
                        property alias trackingColumn: mainColumn

                        width: parent.width
                    }

                    Component {
                        id: programsComponent
                        Column {
                            height: childrenRect.height
                            Repeater {
                                model: programs
                                Loader {
                                    sourceComponent: columnComponent
                                    property var graphCaption: modelData
                                    property var graphAxis: axes[axisIndex]
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
                        height: item ? item.childrenRect.height : 0
                    }
                }
            }
        }
    }
}
