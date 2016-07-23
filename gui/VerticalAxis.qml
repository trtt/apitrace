import QtQuick 2.0

Item {
id: root

property double maxY: 1
property int numTicks: 3
property var caption

Text {
    anchors.left: parent.left
    anchors.verticalCenter: parent.verticalCenter
    text: caption
}

Column {
    anchors.fill: parent
    Repeater {
        model: numTicks
        Item {
            width: parent.width
            height: root.height / numTicks
            Text {
                anchors.right: tick.left
                text: (maxY * (numTicks - index) / numTicks).toFixed(3) + " "
                font.pixelSize: 7
            }
            Rectangle {
                id: tick
                anchors.top: parent.top
                height: 1
                anchors.right: parent.right
                width: 5
                color: "black"
            }
        }
    }
}

}
