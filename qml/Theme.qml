pragma Singleton
import QtQuick

QtObject {
    readonly property color background: Vehicle.night ? "#18231f" : "#f1f3ee"
    readonly property color top: Vehicle.night ? "#1c2923" : "#f7f7f2"
    readonly property color bottom: Vehicle.night ? "#25382c" : "#e2e8e0"
    readonly property color ink: Vehicle.night ? "#e5eee5" : "#23332f"
    readonly property color muted: Vehicle.night ? "#a9b8aa" : "#6f7d73"
    readonly property color green: Vehicle.night ? "#acd2ac" : "#557a65"
    readonly property color active: Vehicle.night ? "#38513d" : "#e3ebe2"
    readonly property color surface: Vehicle.night ? "#24342b" : "#ffffff"
    readonly property color secondary: Vehicle.night ? "#304434" : "#f0f3ed"
    readonly property color line: Vehicle.night ? "#3b4d3d" : "#dfe5da"
    readonly property color dock: Vehicle.night ? "#202e25" : "#fafbf8"
}
