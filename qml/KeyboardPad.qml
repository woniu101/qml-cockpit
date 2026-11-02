import QtQuick
Rectangle {
    id: keyboard
    property var input: null
    radius: 23; color: Theme.surface; border.width: 1; border.color: Theme.line
    function write(value){if(!input)return;input.forceActiveFocus();if(input.selectionStart!==input.selectionEnd)input.remove(input.selectionStart,input.selectionEnd);input.insert(input.cursorPosition,value)}
    function backspace(){if(!input)return;input.forceActiveFocus();if(input.selectionStart!==input.selectionEnd)input.remove(input.selectionStart,input.selectionEnd);else if(input.cursorPosition>0)input.remove(input.cursorPosition-1,input.cursorPosition)}
    Column { x:16;y:14;spacing:9
        Repeater {model:["QWERTYUIOP","ASDFGHJKL","ZXCVBNM"]
            Row {required property string modelData;spacing:8;x:Math.round((10-modelData.length)*33)
                Repeater {model:modelData.split("");UiButton {required property string modelData;width:58;height:44;text:modelData;textSize:18;focusPolicy:Qt.NoFocus;onClicked:keyboard.write(modelData.toLowerCase())} }
            }
        }
        Row {spacing:10
            UiButton {width:143;height:43;text:"空格";textSize:17;focusPolicy:Qt.NoFocus;onClicked:keyboard.write(" ")}
            UiButton {width:143;height:43;text:"退格";textSize:17;focusPolicy:Qt.NoFocus;onClicked:keyboard.backspace()}
            UiButton {width:143;height:43;text:"清空";textSize:17;focusPolicy:Qt.NoFocus;onClicked:{if(keyboard.input)keyboard.input.clear()} }
            UiButton {width:201;height:43;text:"收起键盘";textSize:17;focusPolicy:Qt.NoFocus;onClicked:keyboard.visible=false}
        }
    }
}
