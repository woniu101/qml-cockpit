import QtQuick
import QtQuick.Layouts

Surface {
    id: card
    default property alias contentData: body.data
    implicitHeight: body.implicitHeight + 36
    radius: 20
    color: Theme.secondary
    ColumnLayout {
        id: body
        x: 18; y: 18
        width: card.width - 36
        spacing: 12
    }
}
