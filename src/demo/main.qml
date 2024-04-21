import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Window 2.2
import "./test1.js" as Ts

Window {
    id: root
    visible: true
    width: 640
    height: 480
    title: qsTr("Hello 2 1 World")


    Row{
        Text{
            text: "test"
            Component.onCompleted:{
                Ts.test();
            }
        }

        Hello{
            height: 10
        }

        Rectangle{
            width: 100
            height: 200
            color: "red"
        }

        Button {
            text: "Ok1133"
            onClicked: {
                root.color = Qt.rgba(Math.random(), Math.random(), Math.random(), 1);
            }
        }
    }
}

