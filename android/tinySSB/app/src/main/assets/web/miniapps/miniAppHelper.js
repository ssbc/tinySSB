function getContactsList() {
    // Get the contacts list from the native code
    backend("getContactsList");
}

function writeLogEntry(entry) {
    // Write a log entry to the native code, use currentMiniAppID to identify the app
    // entry is a JSONString
    backend("customApp:writeEntry " + currentMiniAppID + " " + entry);
}

function readLogEntries(numberOfEntries) {
    // Read the log entries from the native code associated with the currentMiniAppID
    backend("customApp:readEntries " + currentMiniAppID + " " + numberOfEntries);
}

function quitApp() {
    // Quit the app
    setScenario('miniapps');
}

function launchContactsMenu(heading, subheading) {
    closeOverlay()
    fill_members(true);
    prev_scenario = 'customApp';
    setScenario("members");

    document.getElementById("div:textarea").style.display = 'none';
    document.getElementById("div:confirm-members").style.display = 'flex';
    document.getElementById("tremolaTitle").style.display = 'none';
    var c = document.getElementById("conversationTitle");
    c.style.display = null;
    c.innerHTML = "<font size=+1><strong>" + heading + "</strong></font><br>" + subheading;
    document.getElementById('plus').style.display = 'none';
}