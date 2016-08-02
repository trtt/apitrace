import QtQuick 2.0
import QtQuick.Controls 1.4
import DataVis 1.0

Item {
id: root
property bool coarsed: true

function setCoarsed() {
    if (!coarsed) coarsed = true;
    finetimer.restart();
}

function findGraph(x, y) {
    var index = column.childAt(x, y)
    var coord = column.mapToItem(index, x, y)
    var oldIndex = index
    index = index.childAt(coord.x, coord.y)
    if (index.graph) return index.graph
    coord = oldIndex.mapToItem(index, coord.x, coord.y)
    return index.item.childAt(coord.x, coord.y).graph
}

Binding {
    target: baraxis
    property: "dispStartTime"
    /*value: baraxis.startTime + x.value*(baraxis.endTime-baraxis.startTime) - Math.min(Math.exp(-zoom.value) * (baraxis.endTime-baraxis.startTime), x.value*(baraxis.endTime-baraxis.startTime))*/
    value: baraxis.startTime + scroll.position * (baraxis.endTime-baraxis.startTime)
}
Binding {
    target: baraxis
    property: "dispEndTime"
    /*value: baraxis.startTime + x.value*(baraxis.endTime-baraxis.startTime) + Math.min(Math.exp(-zoom.value) * (baraxis.endTime-baraxis.startTime), (1-x.value)*(baraxis.endTime-baraxis.startTime))*/
    value: baraxis.dispStartTime + scroll.size * (baraxis.endTime-baraxis.startTime)
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
    x: flick.x + flick.vaxisWidth
    width: parent.width - x
    anchors.top: parent.top
    anchors.bottom: scrollview.bottom
    panelHeight: 20
    startTime: baraxis.dispStartTime * 1e-9
    endTime: baraxis.dispEndTime * 1e-9
}
Rectangle {
    anchors.left: parent.left
    anchors.right: parent.right
    y: axis.panelHeight
    height: 1
    color: "black"
}

Component {
    id: programComponent

    Column {
        width: flick.width
    Repeater {
        model: programs
        Item {
            width: parent.width
            height: 1. * flick.height/10 + ((index == programs.length-1)?3:1)
            property var graph: graphElement
            Item {
                id: vaxisheader
                anchors.left: parent.left
                width: flick.vaxisWidth - 100
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                Text {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    text: " (" + modelData + ")"
                }
            }
            VerticalAxis {
                id: vaxis
                maxY: graphElement.maxY
                anchors.left: vaxisheader.right
                width: 100
                anchors.top: parent.top
                anchors.bottom: parent.bottom
            }
            BarGraph {
                id: graphElement
                anchors.left: vaxis.right
                anchors.right: parent.right
                height: parent.height

                bgcolor: index % 2 == 1 ? Qt.rgba(0,0,0,0) : Qt.tint("transparent", "#10FF0000")
                filter: modelData
                axis: baraxis
                data: graphData
                numElements: Math.max(width * (coarsed ? coarse.value : fine.value), 0)
            }
            Rectangle {
                color: "gray"
                width: parent.width
                anchors.bottom: parent.bottom
                height: (index == programs.length-1) ? 3:1
            }
        }
    }
    }
}


ScrollView {
    id: scrollview
    y: axis.panelHeight
    height: root.height - y - statspanel.height - scroll.height
    anchors.left: root.left
    anchors.right: root.right
Flickable {
    id: flick
    anchors.fill: parent
    property int vaxisWidth: 240
    clip: true
    contentHeight: column.height
    onContentYChanged: {
        var first = findGraph(vaxisWidth+10, contentY+1)
        var last = findGraph(vaxisWidth+10, contentY+height-1)
        first.update()
        var coord = first.parent.mapToItem(column, first.x, first.y)
        for (var i=1; i < 10; i++) {
            findGraph(coord.x+10, coord.y+20 + i * (3 + flick.height/10)).update()
        }
        last.update()
    }
    Column {
        id: column
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        Repeater {
            model: graphs
            Column {
                property bool programFiltered: programSwitch.checked
                anchors.left: parent.left
                anchors.right: parent.right
                Item {
                    width: parent.width
                    height: 1. * flick.height/10 + 1
                    property var graph: graphElement
                    Rectangle {
                        id: vaxisheader
                        anchors.left: parent.left
                        width: flick.vaxisWidth - 100
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        color: "oldlace"
                        Text {
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            text: " " + bgcaption
                        }
                        Text {
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 1
                            anchors.right: programSwitch.left
                            font.pixelSize: 9
                            text: "expanded: "
                        }
                        Switch {
                            id: programSwitch
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 1
                            anchors.right: parent.right
                            anchors.rightMargin: 1
                            checked: false
                        }
                    }
                    VerticalAxis {
                        id: vaxis
                        maxY: graphElement.maxY
                        anchors.left: vaxisheader.right
                        width: 100
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                    }
                    BarGraph {
                        id: graphElement
                        anchors.left: vaxis.right
                        anchors.right: parent.right
                        height: parent.height

                        bgcolor: index % 2 == 1 ? Qt.rgba(0,0,0,0) : Qt.tint("transparent", "#10FF0000")
                        filtered: false
                        axis: baraxis
                        data: bgdata
                        numElements: Math.max(width * (coarsed ? coarse.value : fine.value), 0)
                    }
                    Rectangle {
                        color: "gray"
                        width: parent.width
                        anchors.bottom: parent.bottom
                        height: 1
                    }
                }

                Loader {
                    sourceComponent: programFiltered?programComponent:undefined
                    property var graphData: bgdata
                    onSourceComponentChanged: {
                        setCoarsed()
                        if (item) height = item.childrenRect.height
                        else height = 0
                    }
                }

            }

        }
    }
}
}

MouseArea {
    id: view
    x: flick.x + flick.vaxisWidth
    width: flick.width - x
    anchors.top: scrollview.top
    anchors.bottom: scrollview.bottom
    property int oldX
    property bool dragged: false
    hoverEnabled: true
    onPressed: {
        oldX = mouse.x
        dragged = true
    }
    onPositionChanged: {
        if (dragged) {
            scroll.position -= (mouse.x-oldX) / view.width * (baraxis.dispEndTime-baraxis.dispStartTime)/(baraxis.endTime-baraxis.startTime)
            if (scroll.position < 0) scroll.position = 0
            if (scroll.position + scroll.size > 1) scroll.position = 1 - scroll.size
            oldX = mouse.x
            if (!coarsed) setCoarsed()
        } else {

            vertcursor.x = view.x + mouse.x
            stats.graph = findGraph(mouse.x, mouse.y)
            stats.start = baraxis.dispStartTime + mouse.x / view.width * (baraxis.dispEndTime - baraxis.dispStartTime)
            statspanel.xtime = stats.start * 1e-9
            stats.duration = 1. / view.width * (baraxis.dispEndTime - baraxis.dispStartTime)
            statspanel.resolution = stats.duration * 1e-9
            stats.collect()

            tooltip.x = view.x + mouse.x + ((mouse.x > view.width/2)?-1.5:0.5)*tooltip.width
            tooltip.y = view.y + mouse.y + ((mouse.y > view.height/2)?-1.5:0.5)*tooltip.height
            tooltip_event.visible = false
            tooltip_stats.visible = false
            tooltip.visible = false
            if (stats.numEvents == 0) {
                var id = baraxis.findEventAtTime(stats.start)
                var start = baraxis.eventStartTime(id)
                var duration = baraxis.eventDurationTime(id)
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
    anchors.bottom: scrollview.bottom
    width: parent.width
    spacing: 100
    visible: false // !
    Slider {
        width: parent.width / 3 - 200/3
        id: coarse
        minimumValue: 1
        maximumValue: 10
        value: 3
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
    anchors.top: view.bottom
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
    anchors.top: scroll.bottom
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
    anchors.top: view.top
    anchors.bottom: view.bottom
    width: 1
    color: "red"
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

