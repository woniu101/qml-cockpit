import QtQuick
Rectangle {
    radius: 16
    gradient: Gradient { GradientStop { position: 0; color: "#f3eadb" } GradientStop { position: 1; color: "#b79a70" } }
    clip: true
    Rectangle { x: parent.width * 0.56; y: parent.height * 0.16; width: parent.width * 0.26; height: width; radius: width / 2; color: "#f9f1de" }
    Canvas {
        anchors.fill: parent
        onPaint: {
            const ctx = getContext("2d"); ctx.reset()
            ctx.beginPath(); ctx.roundedRect(0, 0, width, height, 16, 16); ctx.clip()
            ctx.fillStyle = "#8d9a81"; ctx.beginPath(); ctx.moveTo(0, height * 0.74)
            ctx.quadraticCurveTo(width * 0.24, height * 0.42, width * 0.55, height * 0.73)
            ctx.quadraticCurveTo(width * 0.78, height * 0.57, width, height * 0.75)
            ctx.lineTo(width, height); ctx.lineTo(0, height); ctx.fill()
            ctx.fillStyle = "#66745d"; ctx.beginPath(); ctx.moveTo(0, height * 0.9)
            ctx.quadraticCurveTo(width * 0.4, height * 0.67, width, height * 0.89)
            ctx.lineTo(width, height); ctx.lineTo(0, height); ctx.fill()
        }
    }
}
