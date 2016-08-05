import QtQuick 2.0
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.0

Item {
id: root
property bool coarsed: true
property alias timelineView: timelineView

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

        height: root.height/3
        onHeightChanged: setCoarsed()

        headerWidth: barview.headerWidth
        coarsed: root.coarsed
        coarse: coarse.value
        fine: fine.value
    }

    BarGraphView {
        id: barview

        Layout.fillHeight: true
        onHeightChanged: setCoarsed()

        graphAxis: axisGPU
        coarsed: root.coarsed
        coarse: coarse.value
        fine: fine.value
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
    hoverEnabled: true
    onPressed: {
        oldX = mouse.x
        dragged = true
    }
    onPositionChanged: {
        if (dragged) {
            scroll.position -= (mouse.x-oldX) / view.width * (axisCPU.dispEndTime-axisCPU.dispStartTime)/(axisGPU.endTime-axisCPU.startTime)
            if (scroll.position < 0) scroll.position = 0
            if (scroll.position + scroll.size > 1) scroll.position = 1 - scroll.size
            oldX = mouse.x
        } else {
            vertcursor.x = view.x + mouse.x
            var start = axisCPU.dispStartTime + mouse.x / view.width * (axisCPU.dispEndTime - axisCPU.dispStartTime)
            statspanel.xtime = start * 1e-9
            var duration = 1. / view.width * (axisCPU.dispEndTime - axisCPU.dispStartTime)
            statspanel.resolution = duration * 1e-9
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
    hoverEnabled: true
    onExited: {
        tooltip_event.visible = false
        tooltip_stats.visible = false
        tooltip.visible = false

    }
    onPressed: {
        oldX = mouse.x
        dragged = true
    }
    onPositionChanged: {
        if (dragged) {
            scroll.position -= (mouse.x-oldX) / view.width * (axisCPU.dispEndTime-axisCPU.dispStartTime)/(axisGPU.endTime-axisCPU.startTime)
            if (scroll.position < 0) scroll.position = 0
            if (scroll.position + scroll.size > 1) scroll.position = 1 - scroll.size
            oldX = mouse.x
        } else {

            vertcursor.x = view.x + mouse.x
            stats.graph = barview.findGraph(mouse.x, flick.contentY + mouse.y)
            stats.start = axisCPU.dispStartTime + mouse.x / view.width * (axisCPU.dispEndTime - axisCPU.dispStartTime)
            statspanel.xtime = stats.start * 1e-9
            stats.duration = 1. / view.width * (axisCPU.dispEndTime - axisCPU.dispStartTime)
            statspanel.resolution = stats.duration * 1e-9
            stats.collect()

            tooltip.x = view.x + mouse.x + ((mouse.x > view.width/2)?-1.5:0.5)*tooltip.width
            tooltip.y = view.y + mouse.y + ((mouse.y > view.height/2)?-1.5:0.5)*tooltip.height
            tooltip_event.visible = false
            tooltip_stats.visible = false
            tooltip.visible = false
            if (stats.numEvents == 0) {
                var id = axisGPU.findEventAtTime(stats.start)
                var start = axisGPU.eventStartTime(id)
                var duration = axisGPU.eventDurationTime(id)
                if (start <= stats.start && stats.start <= start+duration &&
                    !stats.graph.isEventFiltered(id)) {
                    tooltip_event.eventId = id
                    tooltip_event.eventProgram = stats.graph.eventFilter(id)
                    tooltip_event.eventStart = start * 1e-9
                    tooltip_event.eventDuration = duration * 1e-9
                    tooltip_event.eventValue = stats.graph.data.eventYValue(id)
                    tooltip_event.visible = true
                    tooltip.visible = true
                }
            } else {
                tooltip_stats.xtime = stats.start * 1e-9
                tooltip_stats.xrange = stats.duration * 1e-9
                tooltip_stats.visible = true
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
    width: Math.max(tooltip_event_column.width, tooltip_stats_column.width) + 20
    height: tooltip_event_column.height + 20
    radius: 5
    visible: false

    Item {
        id: tooltip_event
        anchors.fill: parent
        property int eventId: 0
        property int eventProgram: 0
        property double eventStart: 0
        property double eventDuration: 0
        property real eventValue: 0
        visible: false
        Column {
            id: tooltip_event_column
            x: 10
            y: 10
            spacing: 2
            Text {text: "id: " + tooltip_event.eventId}
            Text {text: "program: " + tooltip_event.eventProgram}
            Text {text: "start: " + tooltip_event.eventStart + "s"}
            Text {text: "duration: " + tooltip_event.eventDuration + "s"}
            Text {text: "value: " + tooltip_event.eventValue}
        }
    }

    Item {
        id: tooltip_stats
        anchors.fill: parent
        property double xtime: 0
        property double xrange: 0
        visible: false
        Column {
            id: tooltip_stats_column
            x: 10
            y: 10
            spacing: 2
            Text {text: "time span: " + tooltip_stats.xrange + "s"}
            Text {text: "num. events: " + stats.numEvents}
            Text {text: "max: " + stats.max}
            Text {text: "mean: " + stats.mean}
            Text {text: "min: " + stats.min}
        }
    }
}

}

