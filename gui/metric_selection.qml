import QtQuick 2.0
import QtQuick.Controls 1.4
import QtQml.Models 2.1

Rectangle {
    width: 500
    height: 300

    Rectangle {
        id: rectangle1
        x: 14
        y: 30
        width: 198
        height: 210
        color: "#ffffff"
    }

    Text {
        id: text1
        x: 20
        y: 10
        width: 95
        height: 14
        text: qsTr("Metrics:")
        font.pixelSize: 12
    }

    Button {
        id: button1
        x: 320
        y: 87
        text: qsTr("OK")
    }
   /*TreeViewCustom {
        model: metricSelection
        width: 600
        height: 500
        delegate: Row {spacing:5
            Label {text: model.name}
            CheckBox {checked: model.check
            onCheckedChanged:
                if (checked != model.check) {
                    model.check = checked
                }
            }
            CheckBox {}
            CheckBox {}
        }
    }*/

    TreeView {
    width: 800
    height: 500
    TableViewColumn {
        title: "Name"
        role: "name"
    }
    TableViewColumn {
        title: "frames"
        role: "check"
        horizontalAlignment: Text.AlignHCenter
        delegate: Item {
            CheckBox {anchors.centerIn:parent; checked: model.check
            onCheckedChanged:
                if (checked != model.check) {
                    model.check = checked
                }
            }
        }
    }
    TableViewColumn {
        title: "draw calls"
        role: ""
        horizontalAlignment: Text.AlignHCenter
        delegate: Item{
            CheckBox {anchors.centerIn: parent; checked: false }
        }
    }
    TableViewColumn {
        title: "Description"
        role: "desc"
    }
    model: metricSelection
    }


    /*ListView {
        id: view
        width: 300
        height: 400

        model: DelegateModel {
            model: DelegateModel {model: metricSelection}

            delegate: Rectangle {
                width: 200; height: 25
                Row {
                    Text { text: name }
                    Text { text: index }}

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            view.model.rootIndex = view.model.modelIndex(index)
                        }
                    }
            }
        }
    }*/
}
