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
import QtQuick.Layouts 1.0

Item {
id: root
property bool coarsed: true
property alias timelineView: timelineView

signal eventDoubleClicked(int id)

function setCoarsed() {
    if (!coarsed) coarsed = true
    finetimer.restart()
}

Binding {
    target: axisCPU
    property: "dispStartTime"
    value: axisCPU.startTime + scroll.position * (axisGPU.endTime-axisCPU.startTime)
}
Binding {
    target: axisCPU
    property: "dispEndTime"
    value: axisCPU.dispStartTime + scroll.size * (axisGPU.endTime-axisCPU.startTime)
}

Binding {
    target: axisGPU
    property: "dispStartTime"
    value: axisCPU.dispStartTime
}

Binding {
    target: axisGPU
    property: "dispEndTime"
    value: axisCPU.dispEndTime
}


focus: true
Keys.onPressed: {
    if (event.key == Qt.Key_Left) {
        x.value -= 1e-7;
    }
    if (event.key == Qt.Key_Right) {
        x.value += 1e-7;
    }
}


TimelineAxis {
    id: axis
    x: barview.flickable.x + Math.min(barview.headerWidth, barview.headerWidthMax)
    width: barview.flickable.width - x
    anchors.top: parent.top
    anchors.bottom: scroll.top
    panelHeight: 20
    startTime: axisCPU.dispStartTime * 1e-9
    endTime: axisCPU.dispEndTime * 1e-9
}

MouseArea {
    property bool direction: true

    anchors.top: axis.top
    height: axis.panelHeight
    anchors.left: axis.left
    anchors.right: axis.right

    hoverEnabled: true

    onPressed: {
        rangeselection.start = axisCPU.dispStartTime + mouse.x * (axisCPU.dispEndTime - axisCPU.dispStartTime) / axis.width
        rangeselection.end = rangeselection.start
    }

    onPositionChanged: {
        if (pressed) {
            var end = axisCPU.dispStartTime + mouse.x * (axisCPU.dispEndTime - axisCPU.dispStartTime) / axis.width

            if ((end > rangeselection.end && !direction) ||
                (end < rangeselection.start && direction))
            {
                rangeselection.end = rangeselection.start
                direction = !direction
            }

            if (direction) rangeselection.end = end
            else rangeselection.start = end
        }
    }
}

Rectangle {
    property double start: 0
    property double end: 0
    id: rangeselection

    anchors.top: sep.bottom
    anchors.bottom: scroll.top
    x: axis.x + axis.width * (start - axisCPU.dispStartTime) / (axisCPU.dispEndTime - axisCPU.dispStartTime)
    width: axis.width * (end - start) / (axisCPU.dispEndTime - axisCPU.dispStartTime)

    color: "#300000FF"
    visible: false
    z: 1

    onEndChanged: {
        if (end == start) visible = false
        else visible = true
    }

    onStartChanged: {
        if (end == start) visible = false
        else visible = true
    }

    Rectangle {
        anchors.left: parent.left
        width: 1
        anchors.top: parent.top
        anchors.topMargin: -15
        anchors.bottom: parent.bottom

        color: "navy"
    }

    Rectangle {
        anchors.right: parent.right
        width: 1
        anchors.top: parent.top
        anchors.topMargin: -15
        anchors.bottom: parent.bottom

        color: "navy"
    }

    MouseArea {
        anchors.fill: parent

        onPressed: {
            if (timer.running) {
                timer.stop()
                zoomIn()
            } else {
                timer.restart()
                mouse.accepted = false
            }
        }

        function zoomIn() {
            scroll.position = rangeselection.start / (axisGPU.endTime-axisCPU.startTime)
            scroll.size = (rangeselection.end - rangeselection.start) / (axisGPU.endTime-axisCPU.startTime)
        }

        Timer {
            id: timer
            interval: 200
        }
    }
}

Rectangle {
    id: sep

    anchors.left: parent.left
    anchors.right: parent.right
    y: axis.panelHeight
    height: 1
    color: "black"
}


SplitView {
    anchors.top: sep.bottom
    anchors.bottom: scroll.top
    anchors.left: root.left
    anchors.right: root.right

    orientation: Qt.Vertical

    TimelineGraphView {
        id: timelineView

        height: root.height/8
        onHeightChanged: setCoarsed()

        numGraphs: height / (root.height/16)
        headerWidth: barview.headerWidth
        coarsed: root.coarsed
        coarse: coarse.value
        fine: fine.value
        expandable: viewType==1
        colored: viewType==1
    }

    BarGraphView {
        id: barview

        Layout.fillHeight: true
        onHeightChanged: setCoarsed()

        numGraphs: 8
        graphAxis: axisGPU
        coarsed: root.coarsed
        coarse: coarse.value
        fine: fine.value
        expandable: viewType==1
    }
}

MouseArea {
    property alias flick: timelineView.flickable
    id: viewTimeline
    x: flick.x + Math.min(timelineView.headerWidth, timelineView.headerWidthMax)
    width: flick.width - x
    anchors.top: sep.bottom
    height: flick.height-2
    property int oldX
    property bool dragged: false
    property int eventUnderCursor: -1
    hoverEnabled: true
    onPressed: {
        oldX = mouse.x
        dragged = true
    }
    onPositionChanged: {
        tooltip_event.visible = false
        tooltip_stats.visible = false
        tooltip.visible = false
        eventUnderCursor = -1
        if (dragged) {
            scroll.position -= (mouse.x-oldX) / view.width * (axisCPU.dispEndTime-axisCPU.dispStartTime)/(axisGPU.endTime-axisCPU.startTime)
            if (scroll.position < 0) scroll.position = 0
            if (scroll.position + scroll.size > 1) scroll.position = 1 - scroll.size
            oldX = mouse.x
        } else {
            vertcursor.x = view.x + mouse.x
            var graph = timelineView.findGraph(mouse.x, flick.contentY + mouse.y)
            if (!graph) return
            statsTimeline.graph = graph
            var axis = statsTimeline.graph.axis
            statsTimeline.start = axisCPU.dispStartTime + mouse.x / view.width * (axisCPU.dispEndTime - axisCPU.dispStartTime)
            statspanel.xtime = statsTimeline.start * 1e-9
            statsTimeline.duration = 1. / view.width * (axisCPU.dispEndTime - axisCPU.dispStartTime)
            statspanel.resolution = statsTimeline.duration * 1e-9
            statsTimeline.collect()

            tooltip.x = view.x + mouse.x + ((mouse.x > view.width/2)?-1.5:0.5)*tooltip.width
            tooltip.y = view.y + mouse.y + ((mouse.y > view.height/2)?-1.5:0.5)*tooltip.height
            if (statsTimeline.numEvents == 0) {
                var id = axis.findEventAtTime(statsTimeline.start)
                var start = axis.eventStartTime(id)
                var duration = axis.eventDurationTime(id)
                if (start <= statsTimeline.start && statsTimeline.start <= start+duration &&
                    !statsTimeline.graph.isEventFiltered(id))
                {
                    if (viewType == 1) {
                        tooltip_event.eventCallString = timelineHelper.name(id)
                        tooltip_event.eventId = timelineHelper.no(id)
                        tooltip_event.eventProgram = statsTimeline.graph.eventFilter(id)
                        tooltip_event.eventFrame = timelineHelper.frame(id)
                    } else {
                        tooltip_event.eventId = id
                    }
                    eventUnderCursor = id
                    tooltip_event.eventStart = start * 1e-9
                    tooltip_event.eventDuration = duration * 1e-9
                    tooltip_event.yAvailable = false
                    tooltip_event.visible = true
                    tooltip.visible = true
                }
            } else {
                tooltip_stats.xtime = statsTimeline.start * 1e-9
                tooltip_stats.xrange = statsTimeline.duration * 1e-9
                tooltip_stats.visible = true
                tooltip_stats.minmax = false
                tooltip.visible = true
            }
        }
    }
    onReleased: {
        dragged = false;
    }
    onWheel: {
        if (wheel.modifiers & Qt.ControlModifier) {
            scroll.position += scroll.size*wheel.angleDelta.y/120/16 * wheel.x / view.width
            if (scroll.position < 0) scroll.position = 0
            scroll.size -= scroll.size*wheel.angleDelta.y/120/16
            if (scroll.size > 1 - scroll.position) scroll.size = 1 - scroll.position
        } else {
            wheel.accepted = false
        }
    }
    onExited: {
        tooltip_event.visible = false
        tooltip_stats.visible = false
        tooltip.visible = false
    }
    onDoubleClicked: {
        if (eventUnderCursor != -1) {
            root.eventDoubleClicked(eventUnderCursor)
        }
    }
}

MouseArea {
    property alias flick: barview.flickable
    id: view
    x: flick.x + Math.min(barview.headerWidth, barview.headerWidthMax)
    width: flick.width - x
    anchors.bottom: scroll.top
    height: flick.height-2
    property int oldX
    property bool dragged: false
    property int eventUnderCursor: -1
    hoverEnabled: true
    onPressed: {
        oldX = mouse.x
        dragged = true
    }
    onPositionChanged: {
        tooltip_event.visible = false
        tooltip_stats.visible = false
        tooltip.visible = false
        eventUnderCursor = -1
        if (dragged) {
            scroll.position -= (mouse.x-oldX) / view.width * (axisCPU.dispEndTime-axisCPU.dispStartTime)/(axisGPU.endTime-axisCPU.startTime)
            if (scroll.position < 0) scroll.position = 0
            if (scroll.position + scroll.size > 1) scroll.position = 1 - scroll.size
            oldX = mouse.x
        } else {
            vertcursor.x = view.x + mouse.x
            var graph = barview.findGraph(mouse.x, flick.contentY + mouse.y)
            if (!graph) return
            statsBar.graph = graph
            statsBar.start = axisCPU.dispStartTime + mouse.x / view.width * (axisCPU.dispEndTime - axisCPU.dispStartTime)
            statspanel.xtime = statsBar.start * 1e-9
            statsBar.duration = 1. / view.width * (axisCPU.dispEndTime - axisCPU.dispStartTime)
            statspanel.resolution = statsBar.duration * 1e-9
            statsBar.collect()

            tooltip.x = view.x + mouse.x + ((mouse.x > view.width/2)?-1.5:0.5)*tooltip.width
            tooltip.y = view.y + mouse.y + ((mouse.y > view.height/2)?-1.5:0.5)*tooltip.height
            if (statsBar.numEvents == 0) {
                var id = axisGPU.findEventAtTime(statsBar.start)
                var start = axisGPU.eventStartTime(id)
                var duration = axisGPU.eventDurationTime(id)
                if (start <= statsBar.start && statsBar.start <= start+duration &&
                    !statsBar.graph.isEventFiltered(id))
                {
                    if (viewType == 1) {
                        tooltip_event.eventCallString = timelineHelper.name(id)
                        tooltip_event.eventId = timelineHelper.no(id)
                        tooltip_event.eventProgram = statsBar.graph.eventFilter(id)
                        tooltip_event.eventFrame = timelineHelper.frame(id)
                    } else {
                        tooltip_event.eventId = id
                    }
                    eventUnderCursor = id
                    tooltip_event.eventStart = start * 1e-9
                    tooltip_event.eventDuration = duration * 1e-9
                    tooltip_event.eventValue = statsBar.graph.data.eventYValue(id)
                    tooltip_event.yAvailable = true
                    tooltip_event.visible = true
                    tooltip.visible = true
                }
            } else {
                tooltip_stats.xtime = statsBar.start * 1e-9
                tooltip_stats.xrange = statsBar.duration * 1e-9
                tooltip_stats.visible = true
                tooltip_stats.minmax = true
                tooltip.visible = true
            }
        }
    }
    onReleased: {
        dragged = false;
    }
    onWheel: {
        if (wheel.modifiers & Qt.ControlModifier) {
            scroll.position += scroll.size*wheel.angleDelta.y/120/16 * wheel.x / view.width
            if (scroll.position < 0) scroll.position = 0
            scroll.size -= scroll.size*wheel.angleDelta.y/120/16
            if (scroll.size > 1 - scroll.position) scroll.size = 1 - scroll.position
        } else if (wheel.modifiers & Qt.ShiftModifier) {
            scroll.position += scroll.size*wheel.angleDelta.y/120/16
            scroll.position = Math.min(Math.max(scroll.position, 0), 1-scroll.size)
        } else {
            wheel.accepted = false
        }
    }
    onExited: {
        tooltip_event.visible = false
        tooltip_stats.visible = false
        tooltip.visible = false
    }
    onDoubleClicked: {
        if (eventUnderCursor != -1) {
            root.eventDoubleClicked(eventUnderCursor)
        }
    }
}



Row {
    id: toprow
    width: parent.width
    spacing: 100
    visible: false // !
    Slider {
        width: parent.width / 3 - 200/3
        id: coarse
        minimumValue: 1
        maximumValue: 10
        value: 5
    }
    Slider {
        width: parent.width / 3 - 200/3
        id: fine
        minimumValue: 1000
        maximumValue: 100000
        value: 1000
    }
}

Timer {
    id: finetimer
    interval: 300; running: true;
    onTriggered: {coarsed = false;}
}

Item {
    id: scroll
    objectName: "scroll"
    anchors.left: view.left
    anchors.right: view.right
    anchors.bottom: statspanel.top
    height: 12
    property double position: 0
    property double size: 1
    onPositionChanged: setCoarsed()
    onSizeChanged: setCoarsed()

    Rectangle {
        anchors.fill: parent
        radius: height/2 - 1
        color: "lightgray"
    }
    Rectangle {
        id: handle
        x: parent.position * (parent.width-2)+1
        y: 1
        width: parent.size * (parent.width-2)
        height: parent.height-2
        radius: height/2 - 1
        color: "gray"
        opacity: 0.7
    }
    MouseArea {
        anchors.fill: parent

        onPressed: {
            scroll.position = Math.min(Math.max(mouse.x / (parent.width-2) - scroll.size / 2,
                0), 1-scroll.size)
        }
    }
    MouseArea {
        property real oldX
        property bool dragged
        hoverEnabled: true
        anchors.fill: handle

        onPressed: {
            var x = mapToItem(parent, mouse.x, 0).x
            oldX = x
            dragged = true
        }

        onPositionChanged: {
            if (dragged) {
                var x = mapToItem(parent, mouse.x, 0).x
                scroll.position += (x - oldX) / (parent.width-2)
                if (scroll.position < 0) scroll.position = 0
                if (scroll.position + scroll.size > 1) scroll.position = 1 - scroll.size
                oldX = x
            }
        }

        onReleased: {
            dragged = false;
        }
    }
}


Item {
    id: statspanel
    property double xtime: 0
    property double resolution: 0
    anchors.bottom: parent.bottom
    height: childrenRect.height
    anchors.left: view.left
    anchors.right: view.right
    Text {
        width: parent.width
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: 10
        text: "time: " + parent.xtime + "s, resolution (per 1 px):  " + parent.resolution + "s"
    }
}

Rectangle {
    id: vertcursor
    anchors.top: viewTimeline.top
    anchors.bottom: view.bottom
    width: 1
    color: "red"
}

Rectangle {
    anchors.top: viewTimeline.top
    anchors.bottom: view.bottom
    x: view.x - 1
    width: 1

    color: "black"
}

Rectangle {
    id: tooltip
    color: "white"
    border.width: 1
    border.color: "black"
    width: (tooltip_event.visible ? tooltip_event_column.width : tooltip_stats_column.width) + 20
    height: (tooltip_event.visible ? tooltip_event_column.height : tooltip_stats_column.height) + 20
    radius: 5
    visible: false

    Item {
        id: tooltip_event
        anchors.fill: parent
        property bool yAvailable: true
        property int eventId: 0
        property int eventProgram: 0
        property int eventFrame: 0
        property double eventStart: 0
        property double eventDuration: 0
        property real eventValue: 0
        property string eventCallString: ""
        visible: false
        Column {
            id: tooltip_event_column
            x: 10
            y: 10
            spacing: 2
            Text {text: tooltip_event.eventCallString; font.bold: true}
            Text {text: (viewType ? "call id: " : "frame id: ") + tooltip_event.eventId}
            Text {text: "program: " + tooltip_event.eventProgram; visible: viewType==1}
            Text {text: "frame: " + tooltip_event.eventFrame; visible: viewType==1}
            Text {text: "start: " + tooltip_event.eventStart + "s"}
            Text {text: "duration: " + tooltip_event.eventDuration + "s"}
            Text {text: "value: " + tooltip_event.eventValue;
                  visible: tooltip_event.yAvailable}
        }
    }

    Item {
        id: tooltip_stats
        anchors.fill: parent
        property double xtime: 0
        property double xrange: 0
        property bool minmax: true
        visible: false
        Column {
            id: tooltip_stats_column
            x: 10
            y: 10
            spacing: 2
            Text {text: "time span: " + tooltip_stats.xrange + "s"}
            Text {text: "num. events: " + (tooltip_stats.minmax ? statsBar.numEvents : statsTimeline.numEvents)}
            Text {text: "max: " + statsBar.max; visible: tooltip_stats.minmax}
            Text {text: "mean: " + statsBar.mean; visible: tooltip_stats.minmax}
            Text {text: "min: " + statsBar.min; visible: tooltip_stats.minmax}
        }
    }
}

}

