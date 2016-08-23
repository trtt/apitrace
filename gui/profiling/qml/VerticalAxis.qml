import QtQuick 2.0

Item {
id: root

property double maxY: 1
property int numTicks: 3
property real tickWidth: 5

implicitWidth: column.childrenRect.width

Column {
    id: column
    anchors.right: parent.right
    anchors.top: parent.top
    Repeater {
        model: numTicks
        Item {
            anchors.right: parent.right
            width: text.width + tick.width
            height: root.height / numTicks
            Text {
                id: text
                anchors.right: tick.left
                text: {
                    var n = maxY * (numTicks - index) / numTicks;
                    (n > 1e-3) ? parseFloat(n.toFixed(3)) : n.toExponential(3)
                }
                font.pointSize: 7
            }
            Rectangle {
                id: tick
                anchors.top: parent.top
                height: 1
                anchors.right: parent.right
                width: tickWidth
                color: "black"
            }
        }
    }
}

}
