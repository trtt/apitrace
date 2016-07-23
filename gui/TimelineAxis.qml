import QtQuick 2.0

Item {
id: timeline
property int blockStartingWidth: 150
property double startTime: 0
property double endTime
property int panelHeight: 20
property double scale: width / (endTime - startTime)
property double blockTime: Math.pow(2, Math.floor(Math.log(blockStartingWidth / scale) / Math.LN2))
property double blockWidth: scale * blockTime

function printTime(t) {
    var dur = endTime - startTime;
    if (dur > 1)
        return t.toFixed(2) + "s"
    else if (dur > 1e-3)
        return t.toFixed(5) + "s"
    else
        return t.toFixed(9) + "s"
}

Item {
    id: inner
    x: -Math.round((startTime % blockTime) * timeline.scale)
    y: 0
    Repeater {
        model: Math.ceil(timeline.width / blockStartingWidth) * 2 + 2
        Item {
            property int cindex: timeline.offset > index ? index : index
            property int rindex: -1
            property double time: index * blockTime + startTime - (startTime % blockTime)
            onCindexChanged: rindex = cindex

            width: blockWidth
            height: timeline.height
            y: 0
            x: width * cindex

            Rectangle {
                anchors.top: parent.top
                height: timeline.panelHeight
                anchors.left: parent.left
                anchors.right: parent.right
                color: Math.round(time / blockTime) % 2 ? "whitesmoke" : "azure"
                Text {
                    text: printTime(time)
                    anchors.fill: parent
                    font.pixelSize: 11
                }
            }
            Rectangle {
                color: "gray"
                width: 1
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.right: parent.right
            }
            Row {
                height: timeline.height - timeline.panelHeight*3/4
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                Repeater {
                    model: 4
                    Item {
                        width: blockWidth / 5
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        Rectangle {
                            width: 1
                            color: "lightgray"
                            anchors.top: parent.top
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                        }
                    }
                }
            }
        }
    }
}

}
