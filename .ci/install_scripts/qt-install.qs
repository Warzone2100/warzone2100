// QT-CI Project
// License: Apache-2.0
// https://github.com/benlau/qtci

function log() {
    var msg = ["QTCI: "].concat([].slice.call(arguments));
    console.log(msg.join(" "));
}

function printObject(object) {
    var lines = [];
    for (var i in object) {
        lines.push([i, object[i]].join(" "));
    }
    log(lines.join(","));
}

var status = {
    widget: null,
    finishedPageVisible: false,
    installationFinished: false
}

function tryFinish() {
    if (status.finishedPageVisible && status.installationFinished) {
        if (status.widget.LaunchQtCreatorCheckBoxForm) {
            // Disable this checkbox for minimal platform
            status.widget.LaunchQtCreatorCheckBoxForm.launchQtCreatorCheckBox.setChecked(false);
        }
        if (status.widget.RunItCheckBox) {
            // LaunchQtCreatorCheckBoxForm may not work for newer versions.
            status.widget.RunItCheckBox.setChecked(false);
        }
        log("Press Finish Button");
        gui.clickButton(buttons.FinishButton);
    }
}

function Controller() {
    installer.installationFinished.connect(function() {
        status.installationFinished = true;
        gui.clickButton(buttons.NextButton);
        tryFinish();
    });
    installer.setMessageBoxAutomaticAnswer("OverwriteTargetDirectory", QMessageBox.Yes);
    installer.setMessageBoxAutomaticAnswer("installationErrorWithRetry", QMessageBox.Ignore);
    installer.setMessageBoxAutomaticAnswer("XcodeError", QMessageBox.Ok);

    // Allow to cancel installation for arguments --list-packages
    installer.setMessageBoxAutomaticAnswer("cancelInstallation", QMessageBox.Yes);
}

Controller.prototype.WelcomePageCallback = function() {
    log("Welcome Page");

    gui.clickButton(buttons.NextButton);

    var widget = gui.currentPageWidget();

    /* 
	   Online installer 3.0.6 
	   - It must disconnect the completeChanged callback after used, otherwise it will click the 'next' button on another pages
	 */
    var callback = function() {
        gui.clickButton(buttons.NextButton);
        widget.completeChanged.disconnect(callback);
    }

    widget.completeChanged.connect(callback);
}

Controller.prototype.CredentialsPageCallback = function() {

    var login = installer.environmentVariable("QT_CI_LOGIN");
    var password = installer.environmentVariable("QT_CI_PASSWORD");

    if (login === "" || password === "") {
        gui.clickButton(buttons.CommitButton);
    }

    var widget = gui.currentPageWidget();

    widget.loginWidget.EmailLineEdit.setText(login);

    widget.loginWidget.PasswordLineEdit.setText(password);

    gui.clickButton(buttons.CommitButton);
}

Controller.prototype.ComponentSelectionPageCallback = function() {
    log("ComponentSelectionPageCallback");

    function list_packages() {
        var components = installer.components();
        log("Available components: " + components.length);
        var packages = ["Packages: "];

        for (var i = 0; i < components.length; i++) {
            packages.push(components[i].name);
        }
        log(packages.join(" "));
    }

    var widget = gui.currentPageWidget();

    var archiveCheckBox = gui.findChild(widget, "Archive");
    var latestCheckBox = gui.findChild(widget, "Latest releases");
    var fetchButton = gui.findChild(widget, "FetchCategoryButton");

    if (archiveCheckBox != null) {
      // check archive
      archiveCheckBox.click();
    }
    if (latestCheckBox != null) {
      // uncheck latest
      latestCheckBox.click();
    }
    if (fetchButton != null) {
      fetchButton.click()
    }

    if (installer.value("QTCI_LIST_PACKAGES", "0") != "0") {
        list_packages();
        gui.clickButton(buttons.CancelButton);
        return;
    }

    log("Select components");

    function trim(str) {
        return str.replace(/^ +/, "").replace(/ *$/, "");
    }

    var packages = trim(installer.value("QTCI_PACKAGES")).split(",");
    if (packages.length > 0 && packages[0] !== "") {
        widget.deselectAll();
        var components = installer.components();
        var allfound = true;
        for (var i in packages) {
            var pkg = trim(packages[i]);
            var found = false;
            for (var j in components) {
                if (components[j].name === pkg) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                allfound = false;
                log("ERROR: Package " + pkg + " not found.");
            } else {
                log("Select " + pkg);
                widget.selectComponent(pkg);
            }
        }
        if (!allfound) {
            list_packages();
            // TODO: figure out how to set non-zero exit status.
            gui.clickButton(buttons.CancelButton);
            return;
        }
    } else {
        log("Use default component list");
    }

    gui.clickButton(buttons.NextButton);
}

Controller.prototype.IntroductionPageCallback = function() {
    log("Introduction Page");
    log("Retrieving meta information from remote repository");

    /* 
	   Online installer 3.0.6 
	   - Don't click buttons.NextButton directly. It will skip the componenet selection.
    */

    if (installer.isOfflineOnly()) {
        gui.clickButton(buttons.NextButton);
    }
}

Controller.prototype.TargetDirectoryPageCallback = function() {
    var output = installer.value("QTCI_OUTPUT");
    log("Set target installation page: " + output);
    var widget = gui.currentPageWidget();

    if (widget != null) {
        widget.TargetDirectoryLineEdit.setText(output);
    }

    gui.clickButton(buttons.NextButton);
}

Controller.prototype.LicenseAgreementPageCallback = function() {
    log("Accept license agreement");
    var widget = gui.currentPageWidget();

    if (widget != null) {
        widget.AcceptLicenseRadioButton.setChecked(true);
    }

    gui.clickButton(buttons.NextButton);
}

Controller.prototype.ReadyForInstallationPageCallback = function() {
    log("Ready to install");

    // Bug? If commit button pressed too quickly finished callback might not show the checkbox to disable running qt creator
    // Behaviour started around 5.10. You don't actually have to press this button at all with those versions, even though gui.isButtonEnabled() returns true.
    gui.clickButton(buttons.CommitButton, 200);
}

Controller.prototype.PerformInstallationPageCallback = function() {
    log("PerformInstallationPageCallback");
    gui.clickButton(buttons.CommitButton);
}

Controller.prototype.FinishedPageCallback = function() {
    log("FinishedPageCallback");

    var widget = gui.currentPageWidget();

    // Bug? Qt 5.9.5 and Qt 5.9.6 installer show finished page before the installation completed
    // Don't press "finishButton" immediately
    status.finishedPageVisible = true;
    status.widget = widget;
    tryFinish();
}

// Telemetry disabled 
Controller.prototype.DynamicTelemetryPluginFormCallback = function()
{
    log("TelemetryPluginFormCallback");
    var page = gui.pageWidgetByObjectName("DynamicTelemetryPluginForm");
    page.statisticGroupBox.disableStatisticRadioButton.setChecked(true);
    gui.clickButton(buttons.NextButton);
}

// On windows installs there is a page about the start menu.
Controller.prototype.StartMenuDirectoryPageCallback = function() {
    log("StartMenuDirectoryPageCallback");
    gui.clickButton(buttons.NextButton);
}

// qt online installer 3.2.1: open source users must now accept the open
// source obligations.
// https://www.qt.io/blog/qt-online-installer-3.2.1-released
Controller.prototype.ObligationsPageCallback = function()
{
    log("ObligationsPageCallback");
    var page = gui.pageWidgetByObjectName("ObligationsPage");
    page.obligationsAgreement.setChecked(true);
    page.completeChanged();
    gui.clickButton(buttons.NextButton);
    // There appears to be a second page we need to click "Next" on...
    gui.clickButton(buttons.NextButton, 500);
}
