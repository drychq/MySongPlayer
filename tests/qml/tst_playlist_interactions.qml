import QtQuick
import QtTest
import MySongPlayer

Item {
    id: root

    width: 400
    height: 240

    property int rowClickCount: 0
    property int actionClickCount: 0

    AudioListItem {
        id: audioItem
        width: 320
        showActionButton: true
        onClicked: root.rowClickCount += 1
        onActionClicked: root.actionClickCount += 1
    }

    Component {
        id: popupComponent

        AddSongOptionsPopup {}
    }

    TestCase {
        id: testCase

        name: "PlaylistInteractions"
        when: windowShown
        property var popup: null

        SignalSpy {
            id: localImportSpy
            target: testCase.popup
            signalName: "localImportRequested"
        }

        function init() {
            root.rowClickCount = 0
            root.actionClickCount = 0
        }

        function cleanup() {
            if (popup === null) {
                return
            }

            popup.close()
            popup.destroy()
            popup = null
        }

        function test_audio_list_row_and_action_are_exclusive() {
            const selectionArea = findChild(audioItem, "selectionArea")
            const actionButton = findChild(audioItem, "actionButton")
            verify(selectionArea !== null)
            verify(actionButton !== null)

            mouseClick(selectionArea)
            compare(root.rowClickCount, 1)
            compare(root.actionClickCount, 0)

            mouseClick(actionButton)
            compare(root.rowClickCount, 1)
            compare(root.actionClickCount, 1)
        }

        function test_popup_icon_and_row_each_emit_once() {
            popup = popupComponent.createObject(root)
            verify(popup !== null)
            popup.open()
            tryCompare(popup, "opened", true)
            localImportSpy.clear()

            const selectionArea = findChild(popup.contentItem, "localImportSelectionArea")
            const importButton = findChild(popup.contentItem, "localImportButton")
            verify(selectionArea !== null)
            verify(importButton !== null)

            mouseClick(selectionArea)
            compare(localImportSpy.count, 1)
            mouseClick(importButton)
            compare(localImportSpy.count, 2)
        }
    }
}
