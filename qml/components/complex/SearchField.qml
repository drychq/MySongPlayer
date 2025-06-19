// Written by HanQin Chen (cqnuchq@outlook.com) 2025-06-19
import QtQuick
import QtQuick.Controls
import SongPlayer

Rectangle {
  id: root

  property alias text: searchInput.text
  signal accepted(string value)

  color: AppStyles.backgroundColor
  border.color: searchInput.activeFocus ? AppStyles.primaryColorLight : AppStyles.transparentColor
  border.width: AppStyles.standardBorderWidth

  opacity: enabled ? 1 : 0.6

  TextField {
    id: searchInput

    anchors.fill: parent

    font: AppStyles.bodyFont
    color: AppStyles.textPrimary
    

    background: null

 // Search icon size
    leftPadding: 30
    verticalAlignment: TextInput.AlignVCenter

    Image {
      anchors {
        left: parent.left
        leftMargin: 5
        verticalCenter: parent.verticalCenter
      }

      width: AppStyles.searchIconSize
      height: AppStyles.searchIconSize

      mipmap: true
      source: AppStyles.searchIcon
    }

    onAccepted: {
      root.accepted(text)
    }
  }
}
