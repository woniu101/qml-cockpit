import QtQuick
Item {
    property url source
    Album { anchors.fill:parent; visible:cover.status!==Image.Ready }
    Image {id:cover;objectName:"trackArtworkImage";anchors.fill:parent;source:parent.source;asynchronous:true;cache:true;fillMode:Image.PreserveAspectFit;sourceSize.width:512;sourceSize.height:512}
}
