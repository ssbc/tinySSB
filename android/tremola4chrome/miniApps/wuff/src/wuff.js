window.miniApps["wuff"] = {
    handleRequest: function(command, args) {
        console.log("Wuff handling request:", command);
        switch (command) {
            case "onBackPressed":
                quitApp();
                break;
            case "incoming_notification":
                console.log("Wuff incoming_notification:", JSON.stringify(args, null, 2));
                handleWuff(args.args);
                break;
        }
        return "Response from Wuff";
    }
};

function initWuff() {
    console.log("Initializing Wuff app...");
}

function handleWuff(raw) {
    const args = typeof raw === "string" ? JSON.parse(raw) : raw;
    console.log("Handling Wuff args:", args);
    console.log("Wuff Type: " + args[0].type);
    if (args[0].type === "Wuff") {
      showWuffPrompt();
    }
  }
function showWuffPrompt() {
    const wuffPrompt = document.getElementById('div:wuff-prompt');
    wuffPrompt.style.opacity = 1;

    setTimeout(() => {
      wuffPrompt.style.opacity = 0;
    }, 1000);
}

function registerWuff() {
    let json = { type: 'Wuff'};
    writeLogEntry(JSON.stringify(json));
}