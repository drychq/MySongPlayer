import QtQuick

Image {
    id: root

        signal
    clicked

    opacity: buttonTapHandler.hovered ? 0.75 : 1
    mipmap: true
    fillMode: Image.PreserveAspectFit

    HoverHandler {
        id: buttonHoverHandler
    }

    TapHandler {
        id: buttonTapHandler

        onTapped: {
            root.clicked()
        }
    }
}
