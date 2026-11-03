import QtQuick
import QtQuick.Controls as Controls
Controls.ComboBox {
    id:control
    leftPadding:14;rightPadding:34
    font.pixelSize:16
    background:Rectangle {radius:13;color:control.down?Theme.active:Theme.secondary;border.width:control.activeFocus?1:0;border.color:Theme.green}
    contentItem:Label {text:control.displayText;textFormat:Text.PlainText;font:control.font;color:Theme.ink;verticalAlignment:Text.AlignVCenter;elide:Text.ElideRight}
    indicator:Icon {x:control.width-28;y:(control.height-height)/2;width:17;height:17;name:"chevron-right";rotation:90;tint:Theme.muted}
    delegate:Controls.ItemDelegate {objectName:control.objectName+"Choice"+index;width:control.width;text:control.textRole?modelData[control.textRole]:modelData;highlighted:control.highlightedIndex===index;font:control.font;contentItem:Label{text:parent.text;textFormat:Text.PlainText;color:Theme.ink;font:control.font;elide:Text.ElideRight;verticalAlignment:Text.AlignVCenter}background:Rectangle{radius:8;color:parent.highlighted?Theme.active:Theme.surface}}
    popup:Controls.Popup {y:control.height+5;width:control.width;implicitHeight:Math.min(310,contentItem.implicitHeight+12);padding:6;background:Rectangle{radius:15;color:Theme.surface;border.color:Theme.line}contentItem:ListView{clip:true;implicitHeight:contentHeight;model:control.popup.visible?control.delegateModel:null;currentIndex:control.highlightedIndex;Controls.ScrollBar.vertical:Controls.ScrollBar{}}}
}
