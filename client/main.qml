import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15

Window {
    id: root
    visible: true
    width: 800
    height: 720
    title: "Qt Chat (TCP)"
    color: "#f2f6fc"

    property string currentUser: ""
    property bool connected: false

    ListModel { id: onlineModel }

    Rectangle {
        width: parent.width
        height: 64
        anchors.top: parent.top
        color: "#5b8def"
        radius: 16
        z: 2

        RowLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 16
            Text {
                text: "💬 Qt Chat"
                font.pixelSize: 32
                font.bold: true
                color: "white"
                Layout.alignment: Qt.AlignVCenter
            }
            Rectangle { Layout.fillWidth: true; color: "transparent" }
        }
    }

    ColumnLayout {
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            margins: 16
            topMargin: 80
        }
        spacing: 16

        // Connection and status
        RowLayout {
            spacing: 12
            Layout.alignment: Qt.AlignHCenter

            TextField {
                id: host
                width: 200
                placeholderText: "host"
                text: "localhost"
                font.pixelSize: 18
                height: 40
                background: Rectangle { color: "#e6edfa"; radius: 8 }
            }
            TextField {
                id: port
                width: 80
                placeholderText: "port"
                text: "9000"
                inputMethodHints: Qt.ImhDigitsOnly
                font.pixelSize: 18
                height: 40
                background: Rectangle { color: "#e6edfa"; radius: 8 }
            }
            Button {
                id: connectBtn
                text: connected ? "Disconnect" : "Connect"
                font.pixelSize: 18
                height: 40
                background: Rectangle { color: "#5b8def"; radius: 8 }
                contentItem: Text { text: connectBtn.text; color: "white"; font.pixelSize: 18 }
                onClicked: {
                    if (!connected) {
                        tcpClient.connectToHost(host.text, parseInt(port.text))
                    } else {
                        tcpClient.disconnectFromHost()
                    }
                }
            }
            Label {
                id: statusLabel
                text: (!connected ? "Disconnected"
                    : (currentUser.length > 0 ? "Login" : "Logout"))
                color: (!connected ? "darkred"
                    : (currentUser.length > 0 ? "#158443" : "#fa9500"))
                verticalAlignment: Label.AlignVCenter
                font.bold: true
                font.pixelSize: 18
            }
        }

        // Login and register row
        RowLayout {
            spacing: 12
            Layout.alignment: Qt.AlignHCenter

            TextField {
                id: usernameField
                width: 200
                placeholderText: "username"
                font.pixelSize: 18
                height: 40
                background: Rectangle { color: "#e6edfa"; radius: 8 }
            }
            TextField {
                id: passwordField
                width: 200
                placeholderText: "password"
                echoMode: TextInput.Password
                font.pixelSize: 18
                height: 40
                background: Rectangle { color: "#e6edfa"; radius: 8 }
            }
            Button {
                id: registerOpenBtn
                text: "Register"
                font.pixelSize: 18
                height: 40
                background: Rectangle { color: "#66c6b8"; radius: 8 }
                contentItem: Text { text: "Register"; color: "white"; font.pixelSize: 18 }
                onClicked: registerDialog.open()
            }
            Button {
                id: loginBtn
                text: currentUser.length ? "Logged in" : "Login"
                enabled: !currentUser.length
                font.pixelSize: 18
                height: 40
                background: Rectangle { color: "#158443"; radius: 8 }
                contentItem: Text { text: loginBtn.text; color: "white"; font.pixelSize: 18 }
                onClicked: {
                    tcpClient.sendJson({ type: "login", username: usernameField.text, password: passwordField.text })
                }
            }
            Button {
                id: logoutBtn
                visible: currentUser.length > 0
                text: "Logout"
                font.pixelSize: 18
                height: 40
                background: Rectangle { color: "#FA9500"; radius: 8 }
                contentItem: Text { text: "Logout"; color: "white"; font.pixelSize: 18 }
                onClicked: {
                    tcpClient.sendJson({ type: "logout", username: currentUser });
                    currentUser = "";
                    tcpClient.sendJson({ type: "list_users" });
                }
            }
        }

        // Messages and online count
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Text { text: "Messages in view: " + (listView ? listView.count : 0); font.bold: true; font.pixelSize: 16; color: "#5b8def" }
            Text { text: "Online: " + onlineModel.count; font.pixelSize: 16; color: "#158443" }
        }

        // Main area
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 18

            // Message List
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#ffffff"
                radius: 16
                border.color: "#5b8def"

                ListView {
                    id: listView
                    anchors.fill: parent
                    anchors.margins: 10
                    model: messageModel
                    clip: true
                    spacing: 8
                    highlightFollowsCurrentItem: true

                    delegate: Rectangle {
                        property string body: text
                        visible: !((sender === root.currentUser || sender === "me") && index > 0 && messageModel.get(index-1).sender === "me")
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.margins: 8
                        radius: 10
                        border.color: "#e6edfa"
                        color: (sender === root.currentUser || sender === "me") ? "#e6f7ff" : "#f7f9fd"
                        implicitHeight: contentColumn.implicitHeight + 16

                        Column {
                            id: contentColumn
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 4

                            Row {
                                spacing: 8
                                Text { text: sender; font.bold: true; elide: Text.ElideRight; color: "#5b8def"; font.pixelSize: 16 }
                                Text { text: time; color: "#888888"; font.pointSize: 12 }
                            }
                            Text {
                                id: msgText
                                text: body
                                wrapMode: Text.WordWrap
                                horizontalAlignment: Text.AlignLeft
                                color: "#2E4A62"
                                font.pixelSize: 18
                            }
                        }
                    }

                    onCountChanged: {
                        if (count > 0) positionViewAtEnd()
                    }
                }
            }

            // Online user module
            Rectangle {
                width: 200
                color: "#f9f9ff"
                radius: 14
                border.color: "#66c6b8"
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8

                    Label {
                        text: (connected && currentUser.length == 0)
                            ? "Online Users Number" : "Online Users"
                        font.bold: true
                        font.pixelSize: 18
                        color: "#158443"
                    }

                    ListView {
                        id: usersView
                        model: onlineModel
                        clip: true
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: (connected && currentUser.length > 0)
                        delegate: Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            height: 32
                            radius: 6
                            color: ListView.isCurrentItem ? "#e6f7ff" : "transparent"
                            Text { anchors.verticalCenter: parent.verticalCenter; anchors.left: parent.left; anchors.leftMargin: 6; text: name; font.pixelSize: 16 }
                        }
                    }
                    Item {
                        visible: (connected && currentUser.length == 0)
                        width: parent.width
                        height: 32
                        Row {
                            spacing: 6
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 4
                            Text { text: "Online user count:"; font.pixelSize: 16; color: "#888" }
                            Text { text: onlineModel.count; font.pixelSize: 16; color: "#66c6b8" }
                        }
                    }
                }
            }
        }

        // Input row
        RowLayout {
            id: inputRow
            spacing: 12
            Layout.fillWidth: true
            height: 60

            Rectangle {
                Layout.fillWidth: true
                color: "#ffffff"
                radius: 12
                border.color: "#e6edfa"
                anchors.verticalCenter: parent.verticalCenter
                height: 54

                TextField {
                    id: input
                    placeholderText: "Type a message..."
                    focus: true
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    font.pixelSize: 20
                    height: 54
                    background: null
                    onAccepted: sendBtn.clicked()
                }
            }
            Button {
                id: sendBtn
                text: "Send"
                height: 54
                font.pixelSize: 20
                background: Rectangle { color: "#5b8def"; radius: 12 }
                contentItem: Text { text: "Send"; color: "white"; font.pixelSize: 20 }
                onClicked: {
                    if (input.text.length > 0) {
                        // If logout status, message only display locally with popup, not sent to server
                        if (connected && currentUser.length == 0) {
                            messageModel.addMessage("me", input.text, new Date());
                            notSeenWarningDialog.open();
                            input.text = "";
                            return;
                        }
                        // Only send messages when logged in
                        if (connected && currentUser.length > 0) {
                            tcpClient.sendJson({ type: "message", text: input.text });
                            input.text = "";
                        }
                    }
                }
            }
        }

        Label {
            id: notSeenLabel
            visible: (connected && currentUser.length == 0)
            color: "#e39d26"
            font.bold: true
            font.pixelSize: 16
            text: "Notice: Messages sent when logged out or after logout will NOT be visible to other online users."
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }

    Dialog {
        id: registerOkDialog
        title: "Register"
        modal: true
        standardButtons: Dialog.Ok
        visible: false
        contentItem: Text { text: "Register successful!"; font.bold: true; color: "#158443"; font.pixelSize: 20 }
    }
    Dialog {
        id: alreadyRegisteredDialog
        title: "Register Failed"
        modal: true
        standardButtons: Dialog.Ok
        visible: false
        contentItem: Text { text: "Already registered!"; font.bold: true; color: "#a94442"; font.pixelSize: 20 }
    }
    Dialog {
        id: loginFailDialog
        title: "Login Failed"
        modal: true
        standardButtons: Dialog.Ok
        visible: false
        contentItem: Text { text: "Login is invalid."; font.bold: true; color: "#a94442"; font.pixelSize: 20 }
    }
    Dialog {
        id: loginOkDialog
        title: "Welcome!"
        modal: true
        standardButtons: Dialog.Ok
        visible: false
        contentItem: Text { text: "Login successful!"; font.bold: true; color: "#158443"; font.pixelSize: 20 }
    }
    Dialog {
        id: notSeenWarningDialog
        title: "Notice"
        modal: true
        standardButtons: Dialog.Ok
        visible: false
        // Notice: Messages sent when logged out/disconnected will NOT be visible to other users
        contentItem: Text { text: "Notice: Messages sent when logged out or after logout will NOT be visible to other online users."; font.bold: true; color: "#e39d26"; font.pixelSize: 20 }
    }
    Dialog {
        id: registerDialog
        title: "Register"
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            tcpClient.sendJson({ type: "register", username: regUser.text, password: regPass.text })
        }
        contentItem: ColumnLayout {
            spacing: 10
            TextField { id: regUser; placeholderText: "username"; font.pixelSize: 16; height: 40; background: Rectangle{ color: "#e6edfa"; radius: 8 } }
            TextField { id: regPass; placeholderText: "password"; echoMode: TextInput.Password; font.pixelSize: 16; height: 40; background: Rectangle{ color: "#e6edfa"; radius: 8 } }
        }
    }

    Connections {
        target: tcpClient

        function onConnected() {
            connected = true;
            tcpClient.sendJson({ type: "list_users" });
        }
        function onDisconnected() {
            connected = false;
            onlineModel.clear();
            currentUser = "";
            tcpClient.sendJson({ type: "list_users" });
        }
        function onLoginSucceeded(username) {
            if (username && username.length > 0)
                currentUser = username;
            else
                currentUser = usernameField.text;

            passwordField.text = "";
            tcpClient.sendJson({ type: "list_users" });
            loginOkDialog.open();
        }
        function onLoginFailed(reason) {
            loginFailDialog.open();
        }
        function onRegisterSucceeded() {
            registerDialog.close();
            registerOkDialog.open();
        }
        function onRegisterFailed(reason) {
            if (reason === "already registered"
                || reason === "username_exists"
                || reason.indexOf("exists") > -1
                || reason.indexOf("registered") > -1)
            {
                alreadyRegisteredDialog.open();
            }
        }
        function onOnlineUsersUpdated(users) {
            // Always update onlineModel to ensure online user count is correct
            onlineModel.clear();
            for (var i=0; i<users.length; i++) {
                onlineModel.append({ "name": users[i] });
            }
        }
        function onMessageReceived(from, text, ts) {
            // Cannot receive messages in logout status
            if (connected && currentUser.length == 0) return;
            if (listView.count > 0) listView.positionViewAtEnd();
        }
    }
}