/**
 * mini_apps.js
 *
 * This file handles the dynamic loading and initialization of mini applications and chat extensions
 * within the main application. It retrieves the manifest files for each mini app, processes the
 * manifest data, and creates corresponding UI elements such as buttons. These buttons allow users
 * to launch the mini apps  and chat extensions from the mini app menu.
 *
 */

"use strict";

/**
 * Handles the paths to manifest files.
 *
 * This function gets the paths to the manifest files of every app from the backend as input and
 * forwards each path to the backend to retrieve the data of each manifest file.
 *
 * @param {string} manifestPathsJson - JSON string containing an array of paths to manifest files.
 */
function handleManifestPaths(manifestPathsJson) {

    const manifestPaths = JSON.parse(manifestPathsJson);
    const listElement = document.getElementById('lst:miniapps');

    manifestPaths.forEach(path => {
        backend("getManifestData " + path);
        //fetchManifestFile(path);
    });

}

/**
 * Handles the content of a manifest file.
 *
 * This function gets the data of a manifest file, creates a button containing that data and appends
 * that button to the list that contains all the buttons that initiate each mini app.
 *
 * @param {string} content - JSON string containing the manifest data.
 */
function handleManifestContent(content) {

    setTimeout(() => {
        const manifest = JSON.parse(content);
        // Process the manifest data (e.g., create buttons)
        const listElement = document.getElementById('lst:miniapps');
        const miniAppButton = createMiniAppButton(manifest);
        listElement.appendChild(miniAppButton);
        console.log(`Added button after delay: ${miniAppButton.id}`);
    }, 100);

}

/**
 * Creates a button to initiate a Mini App.
 *
 * This function gets the manifest data as input and uses it to create the button that initiates
 * the mini app in the mini app menu. If the mini app is also a chat extension, a button will
 * be added in the attach-menu.
 *
 * @param {Object} manifest - The manifest data of the mini app.
 * @param {string} manifest.id - The ID of the mini app.
 * @param {string} manifest.icon - The path to the icon of the mini app.
 * @param {string} manifest.name - The name of the mini app.
 * @param {string} manifest.description - The description of the mini app.
 * @param {string} manifest.init - The initialization function as a string.
 * @param {string} [manifest.extension] - Indicates if the app is also a chat extension.
 * @returns {HTMLElement} The created button element.
 */
function createMiniAppButton(manifest) {
    const item = document.createElement('div');
    item.className = 'miniapp_item_div';

    const button = document.createElement('button');
    button.id = 'btn:' + manifest.id;
    button.className = 'miniapp_item_button w100';
    button.style.display = 'flex';
    button.style.alignItems = 'center';
    button.style.padding = '10px';
    button.style.border = '1px solid #ccc';
    button.style.borderRadius = '5px';
    button.style.backgroundColor = '#f9f9f9';

    console.log("Init function: " + manifest.init);

    //button.onclick = manifest.init();     // This didn't work as intended
    button.addEventListener('click', () => {
        try {
            // Dynamically evaluate the init function
            console.log("Init function: " + manifest.init);
            //Set currentMiniAppID to the manifest.id
            currentMiniAppID = manifest.id
            setScenario("customApp:" + manifest.id);
            console.log(curr_scenario);
            eval(manifest.init);
        } catch (error) {
            console.error(`Error executing init function: ${manifest.init}`, error + " " + error.stack);
        }
    });

    const icon = document.createElement('img');
    console.log("App Icon: " + manifest.icon);
    let iconSrc = manifest.icon;
    // Remove the incorrect asset prefix if present
    if (iconSrc.startsWith("file:///android_asset/")) {
        iconSrc = iconSrc.replace("file:///android_asset/", "file://");
    }
    icon.src = iconSrc;
    console.log("Icon source: " + icon.src);
    icon.alt = `${manifest.name} icon`;
    icon.className = 'miniapp_icon';
    icon.style.width = '50px';
    icon.style.height = '50px';
    icon.style.marginRight = '10px';


    const textContainer = document.createElement('div');
    textContainer.className = 'miniapp_text_container';

    const nameElement = document.createElement('div');
    nameElement.className = 'miniapp_name';
    nameElement.textContent = manifest.name;

    const descriptionElement = document.createElement('div');
    descriptionElement.className = 'miniapp_description';
    descriptionElement.textContent = manifest.description;

    textContainer.appendChild(nameElement);
    textContainer.appendChild(descriptionElement);

    button.appendChild(icon);
    button.appendChild(textContainer);
    item.appendChild(button);

    if (manifest.extension === "True") {
        createExtensionButton(manifest);
    }

    return item;
}

/**
 * Creates a button for a chat extension.
 *
 * This function gets the manifest data and uses it to create a button for the chat extension,
 * which is then appended to the attach-menu.
 *
 * @param {Object} manifest - The manifest data of the chat extension.
 * @param {string} manifest.extensionText - The text to display on the extension button.
 * @param {string} manifest.extensionInit - The initialization function for the extension as a string.
 */
function createExtensionButton(manifest) {
    //console.log("Extension entered")
    const attachMenu = document.getElementById('attach-menu');

    // Create a new button element
    const newButton = document.createElement('button');

    // Set the button's class
    newButton.className = 'attach-menu-item-button';

    // Set the button's text content
    newButton.textContent = manifest.extensionText;

    // Set the button's onclick event
    newButton.addEventListener('click', () => {
        try {
            // Dynamically evaluate the init function
            eval(manifest.extensionInit);
        } catch (error) {
            console.error(`Error executing extensionInit function: ${manifest.extensionInit}`, error);
        }
    });

    // Append the new button to the target div
    attachMenu.appendChild(newButton);
}


