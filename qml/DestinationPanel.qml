import QtQuick
import QtQuick.Controls
import "." as App
Item {
    id: destination
    required property var shell
    property alias input: destinationInput
    signal keyboardRequested()
    App.Label {x:30;y:27;text:"选择目的地";font.pixelSize:28}
    TextField {
        id: destinationInput; objectName:"destinationInput";x:30;y:94;width:433;height:61
        placeholderText:"输入目的地，或选择下方常用位置";color:Theme.ink;font.pixelSize:20;selectByMouse:true
        background:Rectangle{radius:17;color:Theme.secondary}
        onAccepted: destination.shell.navigateTo(text)
    }
    UiButton {objectName:"searchKeyboard";x:475;y:94;width:75;height:61;text:"键盘";textSize:17;onClicked:destination.keyboardRequested()}
    Repeater {
        model:["回家","公司","滨江艺术中心","云杉公园"]
        UiButton {
            required property string modelData; required property int index
            objectName:"searchDestination"+index;x:30;y:179+index*74;width:520;height:61
            background:Rectangle{radius:16;color:Theme.secondary}
            contentItem:Item {
                Icon {x:7;y:7;name:index===0?"home":index===1?"briefcase":"pin";tint:Theme.green}
                App.Label {x:50;y:4;text:modelData;font.pixelSize:21}
                App.Label {x:302;y:8;width:175;horizontalAlignment:Text.AlignRight;text:Vehicle.routePreview(modelData).km+" km · "+Vehicle.routePreview(modelData).minutes+" 分钟";font.pixelSize:17;color:Theme.muted}
            }
            onClicked:destination.shell.navigateTo(modelData)
        }
    }
    UiButton {objectName:"startSearchNavigation";x:30;y:504;width:520;height:58;text:"规划路线";primary:true;enabled:destinationInput.text.trim()!=="";onClicked:destination.shell.navigateTo(destinationInput.text)}
    App.Label {x:30;y:576;text:"路线为本地演示数据，规划路线不会改变挡位。";font.pixelSize:14;color:Theme.muted}
}
