import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Window 2.2
import "./test1.js" as Ts

Window {
    id: root
    visible: true
    width: 640
    height: 480
    title: qsTr("Hello 120     1 World")

    Row{
        Text{
            text: "test"
            Component.onCompleted:{
                Ts.test();
            }
        }

        Go{
            color:"green"
        }

        Hello{
            height: 20
        }

        Rectangle{
            width: 120
            height: 200
            color: "red"
        }

        Button {
            text: "Ok 1 22 sd 1   3"
            onClicked: {
                root.color = Qt.rgba(Math.random(), Math.random(), Math.random(), 1)
            }
        }
    }
}

