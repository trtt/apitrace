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
