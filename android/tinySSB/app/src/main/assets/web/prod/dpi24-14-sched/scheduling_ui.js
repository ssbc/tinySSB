//event_ui.js

"use strict";

const ColorTest = { // all available colors for card title
    BLACK: 'black',
    RED: 'red',
    GREEN: 'green',
    BLUE: 'blue',
    YELLOW: 'yellow',
    CYAN: 'cyan',
    MAGENTA: 'magenta',
    ORANGE: 'orange'
}

var display_create_personal_event = true // Whether to prompt the user to create a personal event when they open the scheduling application

function dpi_allowDrop(ev) {
    ev.preventDefault();
}

function dpi_myTouchFct(ev) {
    console.log('touch');
    console.log(ev);
}

function dpi_dragStart(ev) {
    console.log('drag started ' + ev.target.id);
    ev.dataTransfer.setData("text", ev.target.id);
}

function dpi_dragDrop(ev) {
    ev.preventDefault();
    console.log("event", ev)
    console.log('dragDrop', ev.target);
    console.log(ev.target);
    var s = ev.dataTransfer.getData("text").split('-');
    var t = ev.target.id.split('-');
    if (t.length == 1) {
        t = ev.target.parentNode.id.split('-');
        if (t.length == 1)
            t = ev.target.parentNode.parentNode.id.split('-');
    }
    console.log(s, t);

    var itemTargets = ['item', 'itemDescr', 'itemAssignees']
    if (itemTargets.indexOf(s[1]) >= 0) {
        if (itemTargets.indexOf(t[1]) >= 0) {
            var oldColID = tremola.event[curr_event].items[s[0]].curr_column;
            var newColID = tremola.event[curr_event].items[t[0]].curr_column;
            var newPos = tremola.event[curr_event].items[t[0]].position;
            if (oldColID != newColID)
                dpi_moveItem(curr_event, s[0], newColID);
            dpi_ui_change_item_order(s[0], newPos);
        } else if (t[1] == 'columnHdr')
            dpi_moveItem(curr_event, s[0], t[0]);
        return;
    }
    if (s[1] == 'columnWrapper') {
        var colID;
        if (t[1] == 'columnWrapper' || t[1] == 'columnHdr')
            colID = t[0];
        else if (itemTargets.indexOf(t[1]) >= 0)
            colID = tremola.event[curr_event].items[t[0]].curr_column;
        // console.log('colID', colID);
        var targetPos = tremola.event[curr_event].columns[colID].position;
        dpi_ui_move_column(s[0], targetPos);
        return;
    }
}

function dpi_load_event_list() {
    document.getElementById('lst:scheduling').innerHTML = '';
    // console.log("tremola in load_ebent+list " + tremola)
    // if (tremola == undefined || !('event' in tremola) || Object.keys(tremola.event).length === 0)
    if (!tremola.event || Object.keys(tremola.event).length === 0)
        return
    var subeventIds = Object.keys(tremola.event).filter(key => tremola.event[key].subscribed && !tremola.event[key].removed).map(key => ({[key]: tremola.event[key]}))
    if (subeventIds.length > 0) {
        var subscribedevents = Object.assign(...subeventIds)
        var bidTimestamp = Object.keys(subscribedevents).map(function (key) {
            return [key, subscribedevents[key].lastUpdate]
        }) // [0] = bid, [1] = timestamp
        bidTimestamp.sort(function (a, b) {
            return b[1] - a[1];
        })

        for (var i in bidTimestamp) {
            var bid = bidTimestamp[i][0]
            var event = tremola.event[bid]
            var date = new Date(bidTimestamp[i][1])
            date = date.toDateString() + ' ' + date.toTimeString().substring(0, 5);
            if (event.forgotten && tremola.settings.hide_forgotten_events)
                continue
            var cl, mem, item, bg, row, badge, badgeId, cnt;
            cl = document.getElementById('lst:scheduling');
            mem = recps2display(event.members)
            item = document.createElement('div');
            item.setAttribute('style', "padding: 0px 5px 10px 5px; margin: 3px 3px 6px 3px;");
            if (event.forgotten) bg = ' gray'; else bg = ' light';
            row = "<div id='event-${bid}'><button class='event_item_button w100" + bg + "' onclick='dpi_load_event(\"" + bid + "\");' style='overflow: hidden; position: relative;'>";
            row += "<div style='white-space: nowrap;'><div style='text-overflow: ellipsis; overflow: hidden;'>Event: " + event.name + "</div>";
            row += "<div style='text-overflow: clip; overflow: ellipsis;'><font size=-2>" + escapeHTML(mem) + ", </font><font size=-3>last changed: " + date + "</font> </div></div>";
            badgeId = bid + "-badge_event"
            badge = "<div id='" + badgeId + "' style='display: none; position: absolute; right: 0.5em; bottom: 0.9em; text-align: center; border-radius: 1em; height: 2em; width: 2em; background: var(--red); color: white; font-size: small; line-height:2em;'>&gt;9</div>";
            row += badge + "</button>";
            row += "</div>"
            item.innerHTML = row;
            cl.appendChild(item);
            dpi_ui_set_event_list_badge(bid)
        }
    }
}

function dpi_ui_set_event_list_badge(bid) {
    var event = tremola.event[bid]
    var e = document.getElementById(bid + "-badge_event")
    var cnt
    if (event.unreadEvents == 0) {
        e.style.display = 'none'
        return
    }
    e.style.display = null
    if (event.unreadEvents > 9) cnt = ">9"; else cnt = "" + event.unreadEvents
    e.innerHTML = cnt
}

function dpi_create_personal_event() {
    dpi_createevent('Personal event', [FLAG.PERSONAL])
}

function dpi_menu_create_personal_event() {
    closeOverlay()
    document.getElementById('scheduling-create-personal-event-overlay').style.display = 'initial'
    //document.getElementById('overlay-trans-core').style.display = 'initial'
}

function dpi_btn_create_personal_event_accept() {
    dpi_create_personal_event()
    closeOverlay()
}

function dpi_btn_create_personal_event_decline() {
    closeOverlay()
    display_create_personal_event = false
}

function dpi_load_event(bid) { //switches scene to event and changes title to event name

    console.log("LOAD EVENTS!!!")
    curr_event = bid
    var b = tremola.event[bid]

    b.unreadEvents = 0
    persist()
    dpi_ui_set_event_list_badge(bid)

        var deleteButton = '';
        if (b.creator === myId) {
            deleteButton = `<button onclick="dpi_delete_event('${bid}')" class="delete-button" style='margin-bottom:30px'>Delete Event</button>`;
        }

    var title = document.getElementById("conversationTitle"), bg, box;
    title.style.display = null;
    title.setAttribute('classList', bid.forgotten ? ['gray'] : []);
    box = "<div style='white-space: nowrap;'><div style='text-overflow: ellipsis; overflow: hidden;text-align: left;'><font size=+2><strong>" + "List of Times for " + escapeHTML(b.name) + "</strong></font></div>";
    box += "<div style='color: black; text-overflow: ellipsis; overflow: hidden;text-align: left;'>" + escapeHTML(recps2display(b.members)) + "</div></div>";
    box += deleteButton;
    title.innerHTML = box;

    document.getElementById("div:columns_container").innerHTML = "" //clear old content
    setScenario('event')
    document.getElementById("tremolaTitle").style.display = 'none';
    document.getElementById("tremolaTitle").style.position = 'fixed';
    title.style.display = null;

    dpi_load_all_columns()
    dpi_load_all_items()

}


function dpi_ui_remove_event(bid) {
    // Remove the event from the list
    var eventElement = document.getElementById(`event-${bid}`);
    if (eventElement) {
        eventElement.remove();
    }

    // If we're currently viewing this event, go back to the event list
    if (curr_event === bid) {
        setScenario('scheduling');
    }
    dpi_load_event_list();
}


function dpi_delete_event(bid) {
        dpi_deleteEvent(bid);
        dpi_ui_remove_event(bid);
        setScenario('scheduling');
}

/**
 * Compares the previous snapshot of the scheduling event with the current one and updates the elements accordingly
 *
 * @param {string} bid - Id of the scheduling event
 * @param {object} old_state - Previous snapshot of the scheduling event
 */
function dpi_ui_update_event(bid, old_state) {
    var event = tremola.event[bid]
    console.log("DEBUG UPDATE")

    if (curr_event != bid)
        return

    console.log("DEBUG UPDATE NOT INTERUPPTED")

    console.log("DEBUG oldstate members: " + String(old_state.members))
    console.log("DEBUG curr members: " + String(event.members))

    // event title (name + members)
    if (event.name != old_state.name || !equalArrays(event.members, old_state.members)) {
        dpi_ui_update_event_title(bid)
        var changed_members = event.members.filter(member => !old_state.members.includes(member))
        changed_members.concat(old_state.members.filter(member => !event.members.includes(member)))
        console.log("Changed members: " + changed_members)
        for (var member of changed_members)
            dpi_menu_invite_create_entry(member)
    }

    // invite menu
    if (!equalArrays(Object.keys(event.pendingInvitations), Object.keys(old_state.pendingInvitations))) {
        var changed_members = event.members.filter(member => !old_state.members.includes(member))
        changed_members.concat(old_state.members.filter(member => !event.members.includes(member)))
        console.log("Changed members (invite):" + changed_members)
        for (var member of changed_members)
            dpi_menu_invite_create_entry(member)
    }


    // columns
    for (var i in event.appointments) {
        var old_column = old_state.appointments[i]
        var new_column = event.appointments[i]

        if (!old_column) {
            dpi_load_column(i)
            return
        }
        if (new_column.removed && (old_column.removed != new_column.removed)) {
            dpi_ui_remove_column(i)
            return
        }
        if (old_column.name != new_column.name)
            dpi_ui_rename_column(i, new_column.name)
    }

    //items
    for (var i in event.items) {
        var old_item = old_state.items[i]
        var new_item = event.items[i]

        if (!old_item) {
            dpi_load_item(i)
            return
        }
        if (new_item.removed && (old_item.removed != new_item.removed)) {
            dpi_ui_remove_item(i)
            return
        }
        if (old_item.name != new_item.name)
            dpi_ui_update_item_name(i, new_item.name)
        if (old_item.description != new_item.description)
            dpi_ui_update_item_description(i, new_item.description)
        if (!equalArrays(old_item.assignees, new_item.assignees))
            dpi_ui_update_item_assignees(i)
        if (!equalArrays(old_item.comments, new_item.comments))
            dpi_ui_item_update_chat(i)
        if (old_item.curr_column != new_item.curr_column)
            dpi_ui_update_item_move_to_column(i, new_item.curr_column, new_item.position)
        if (old_item.color != new_item.color)
            dpi_ui_update_item_color(i, new_item.color)
    }
}

/**
 * Compares two arrays and returns whether the are equal
 *
 * @param {Array} array1
 * @param {Array} array2
 * @return {boolean} - Whether the two given arrays are equal
 */
function dpi_equalArrays(array1, array2) {
    if (array1.length != array2.length)
        return false

    for (var i in array1) {
        if (array1[i] instanceof Array && array2[i] instanceof Array) {
            if (!dpi_equalArrays(array1[i], array2[i]))
                return false
        } else if (array1[i] != array2[i]) {
            return false
        }
    }
    return true
}

function dpi_close_event_context_menu() {
    if (curr_context_menu) {
        var context_menu = document.getElementById(curr_context_menu)
        if (context_menu)
            context_menu.style.display = 'none';
    }
    curr_context_menu = null
}

function dpi_menu_history() {
    closeOverlay()
    document.getElementById('overlay-bg').style.display = 'initial';
    document.getElementById('menu_history_content').innerHTML = ''
    overlayIsActive = true;
    var event = tremola.event[curr_event]

    var reversedHistory = event.history.slice().reverse()

    for (var i in reversedHistory) {

        var author_name = tremola.contacts[reversedHistory[i][0]].alias
        var author_color = tremola.contacts[reversedHistory[i][0]].color
        var author_initial = tremola.contacts[reversedHistory[i][0]].initial

        var entryHTML = "<div class='w100' style='padding: 5px 5px 5px;'>"
        entryHTML += "<button class=contact_picture style='float:left;margin-right: 0.75em; background: " + author_color + ";'>" + author_initial + "</button>"
        entryHTML += "<div class='chat_item_button light' style='display: table;float:right;overflow: hidden; width: calc(100% - 4.4em);word-break: break-word;margin-right:10px'>"
        entryHTML += "<div style='display: table-cell;padding-left:10px;vertical-align: middle;overflow-wrap: break-word;'>" + reversedHistory[i][1] + "</div>"
        entryHTML += "</div></div>"

        document.getElementById('menu_history_content').innerHTML += entryHTML

    }
    document.getElementById('div:menu_history').style.display = 'initial'
}

function dpi_history_sort_select(obj) {
    var history = document.getElementById('menu_history_content')
    switch (obj.value) {
        case('latest_first'):
            if (history.style.flexDirection != 'column')
                history.style.flexDirection = 'column'
            break
        case('oldest_first'):
            if (history.style.flexDirection != 'column-reverse') {
                history.style.flexDirection = 'column-reverse'
                history.scrollTo(0, -history.scrollHeight)
            }

            break
    }
}

function dpi_menu_event_invitations() {
    closeOverlay()
    document.getElementById("scheduling-invitations-overlay").style.display = 'initial';
    document.getElementById("overlay-bg").style.display = 'initial';
    document.getElementById("scheduling_invitations_list").innerHTML = ""

    for (var bid in tremola.event) {
        dpi_menu_event_invitation_create_entry(bid)
    }
}

// creates new entry in invitation (to accept or reject invitations) or updates existing entry
function dpi_menu_event_invitation_create_entry(bid) {

    var event = tremola.event[bid]

    if (document.getElementById("scheduling_invitation_" + bid)) {
        if (event.subscribed || !(myId in event.pendingInvitations))
            document.getElementById("scheduling_invitation_" + bid).outerHTML = ""
        else
            document.getElementById("scheduling_invitation_" + bid + "_name").innerHTML = event.name.length < 15 ? event.name : event.name.slice(0, 15) + '...'
        return
    }


    if (event.subscribed) // already subscribed
        return

    console.log("Create invitation for event: " + bid)
    console.log("PENDING LIST: " + Object.keys(event.pendingInvitations))

    if (!(myId in event.pendingInvitations)) // not invited
        return

    var invitationId = event.pendingInvitations[myId][0]
    var inviteUserId = event.operations[invitationId].fid
    var inviteUserName = tremola.contacts[inviteUserId].alias
    var event_name = event.name.length < 15 ? event.name : event.name.slice(0, 15) + '...'


    var invHTML = "<div id='scheduling_invitation_" + bid + "' class='scheduling_invitation_container'>"
    invHTML += "<div class='scheduling_invitation_text_container'>"
    invHTML += "<div id='scheduling_invitation_" + bid + "_name' style='grid-area: name; padding-top: 5px; padding-left: 10px;font-size:15px'>" + event_name + "</div>"
    invHTML += "<div style='grid-area: author; padding-top: 2px; padding-left: 10px;font-size:8px'>From: " + inviteUserName + "</div></div>"

    invHTML += "<div style='grid-area: btns;justify-self:end;display: flex;justify-content: center;align-items: center;'>"
    invHTML += "<div style='padding-right:8px;'>"
    //invHTML += "<div style='padding-right:10px;'>"
    invHTML += "<button class='flat passive buttontext' style=\"height: 40px; background-image: url('img/checked.svg'); width: 35px;margin-right:10px;background-color: var(--passive)\" onclick='dpi_btn_invite_accept(\"" + bid + "\")'>&nbsp;</button>"//</div>"
    invHTML += "<button class='flat passive buttontext' style=\"height: 40px; color: red; background-image: url('img/cancel.svg');width: 35px;background-color: var(--passive)\" onclick='dpi_btn_invite_decline(\"" + bid + "\")'>&nbsp;</button>"
    invHTML += "</div></div></div>"

    document.getElementById("scheduling_invitations_list").innerHTML += invHTML
}

function dpi_btn_invite_accept(bid) {
    dpi_inviteAccept(bid, tremola.event[bid].pendingInvitations[myId])
    delete tremola.event[bid].pendingInvitations[myId]
    var inv = document.getElementById("scheduling_invitation_" + bid)
    launch_snackbar("Invitation accepted")
    if (inv)
        inv.outerHTML = ""
}

function dpi_btn_invite_decline(bid) {
    dpi_inviteDecline(bid, tremola.event[bid].pendingInvitations[myId])
    delete tremola.event[bid].pendingInvitations[myId]
    var inv = document.getElementById("scheduling_invitation_" + bid)
    launch_snackbar("Invitation declined")
    if (inv)
        inv.outerHTML = ""
}

function dpi_menu_new_event() {
    closeOverlay()
    fill_members();
    prev_scenario = 'scheduling';
    setScenario("members");

    document.getElementById("div:textarea").style.display = 'none';
    document.getElementById("div:confirm-members").style.display = 'flex';
    document.getElementById("tremolaTitle").style.display = 'none';
    var c = document.getElementById("conversationTitle");
    c.style.display = null;
    c.innerHTML = "<font size=+1><strong>Create New Event</strong></font><br>Select members to invite";
    document.getElementById('plus').style.display = 'none';
}


function dpi_menu_new_event_name() {
    menu_edit('new_event', 'Enter the name of the new event', '')
}

function dpi_menu_rename_event() {
    var event = tremola.event[curr_event]
    menu_edit('event_rename', 'Enter a new name for this event', event.name)
}

function dpi_ui_update_event_title(bid) {
    var event = tremola.event[bid]
    // update event list
    dpi_load_event_list()
    // update title name
    if (curr_event == bid) {
        var title = document.getElementById("conversationTitle"), bg, box;
        title.style.display = null;
        title.setAttribute('classList', bid.forgotten ? ['gray'] : []);
        box = "<div style='white-space: nowrap;'><div style='text-overflow: ellipsis; overflow: hidden;text-align: left;'><font size=+2><strong>" + "scheduling: " + escapeHTML(event.name) + "</strong></font></div>";
        box += "<div style='color: black; text-overflow: ellipsis; overflow: hidden;text-align: left;'>" + escapeHTML(recps2display(event.members)) + "</div></div>";
        title.innerHTML = box;
    }
}

function dpi_event_toggle_forget() {
    var event = tremola.event[curr_event]
    event.forgotten = !event.forgotten
    persist()
    closeOverlay()
    dpi_load_event_list()
    setScenario('scheduling')
}

function dpi_menu_invite() {
    var event = tremola.event[curr_event]
    closeOverlay()

    if (event.flags.includes(FLAG.PERSONAL)) {
        launch_snackbar("You can't invite other people to your personal event!")
        return
    }

    document.getElementById("div:invite_menu").style.display = 'initial';
    document.getElementById("overlay-bg").style.display = 'initial';

    document.getElementById("menu_invite_content").innerHTML = ''

    for (var c in tremola.contacts) {
        dpi_menu_invite_create_entry(c)
    }
}

// adds an entry to the invite menu or updates an already existing entry
function dpi_menu_invite_create_entry(id) {
    var event = tremola.event[curr_event]

    if (document.getElementById("div:invite_menu").style.display == 'none')
        return

    if (document.getElementById('invite_' + id)) {
        if (event.members.indexOf(id) >= 0)
            document.getElementById('invite_' + id).outerHTML = ''
        else if (id in event.pendingInvitations) {
            document.getElementById('invite_' + id).classList.add("gray")
            document.getElementById('invite_author_' + id).innerHTML = 'Already Invited'
            document.getElementById('invite_btn_' + id).style.display = 'none'
        } else {
            console.log("enable invite for" + id)
            document.getElementById('invite_' + id).classList.remove("gray")
            document.getElementById('invite_author_' + id).innerHTML = ''
            document.getElementById('invite_btn_' + id).style.display = 'initial'
        }


        return
    }

    if (id == myId || event.members.indexOf(id) >= 0)
        return

    var isAlreadyInvited = id in event.pendingInvitations
    var bg = isAlreadyInvited ? ' gray' : ' light'

    var invHTML = "<div id='invite_" + id + "' class='scheduling_invitation_container " + bg + "' style='width:95%; margin: 5px 0px 7px 5px;' >"
    invHTML += "<div class='scheduling_invitation_text_container' >"
    invHTML += "<div style='grid-area: name; padding-top: 5px; padding-left: 10px;font-size:15px'>" + tremola.contacts[id].alias + "</div>"

    if (isAlreadyInvited)
        invHTML += "<div id='invite_author_" + id + "' style='grid-area: author; padding-top: 2px; padding-left: 10px;font-size:8px'>Already Invited</div></div>"
    else
        invHTML += "<div id='invite_author_" + id + "' style='grid-area: author; padding-top: 2px; padding-left: 10px;font-size:8px'></div></div>"

    invHTML += "<div style='grid-area: btns;justify-self:end;display: flex;justify-content: center;align-items: center;'>"
    invHTML += "<div style='padding-right:8px;'>"
    if (!isAlreadyInvited)
        invHTML += "<button id='invite_btn_" + id + "' class='flat passive buttontext' style=\"height: 40px; color: red; background-image: url('img/send.svg');width: 35px;\" onclick='dpi_btn_invite(\"" + id + "\", \"" + curr_event + "\")'>&nbsp;</button>"
    invHTML += "</div></div></div>"

    document.getElementById("menu_invite_content").innerHTML += invHTML
}

function dpi_btn_invite(userId, bid) {
    console.log("INVITE: " + userId + ", bid: " + bid)
    dpi_inviteUser(bid, userId)
    launch_snackbar("User invited")
}

function dpi_leave_curr_event() {
    closeOverlay()

    if (tremola.event[curr_event].flags.includes(FLAG.PERSONAL)) {
        launch_snackbar("You can't leave your personal event!")
        return
    }

    dpi_leave(curr_event)
    setScenario('scheduling')
}

/*
  Columns
*/
function dpi_menu_new_appointment() {
    closeOverlay();
    document.getElementById('add-appointment-overlay').style.display = 'initial';
    document.getElementById('overlay-bg').style.display = 'initial';
    document.getElementById('appointment_date').value = '';
    document.getElementById('appointment_time').value = '';
    overlayIsActive = true;
}

function dpi_ui_add_appointment(id, date, time) {
    var appointmentsContainer = document.getElementById('appointments-container');
    var event = tremola.event[curr_event];
    var appointment = event.appointments[id];

    var deleteButton = '';
    if (appointment.creator === myId) {
        deleteButton = `<button onclick="dpi_delete_appointment('${id}')" class="delete-button">Delete</button>`;
    }

    var appointmentHTML = `
    <div id='appointment-${id}' class="chat_item_div appointment-item">
        <div class="chat_item_button w100 light">
            <div style="white-space: nowrap;">
                <div style="text-overflow: ellipsis; overflow: hidden;"><strong>${date} at ${time}</strong></div>
                <div id="attendance-summary-${id}"></div>
            </div>
            <div class="attendance-buttons">
                <button onclick="dpi_set_attendance('${id}', 'attending')" class="attendance-button attending">Attending</button>
                <button onclick="dpi_set_attendance('${id}', 'notAttending')" class="attendance-button not-attending">Not Attending</button>
                ${deleteButton}
            </div>
            <div id="attendance-list-${id}" class="attendance-list"></div>
        </div>
    </div>
    `;

    appointmentsContainer.innerHTML += appointmentHTML;
    dpi_ui_update_appointment_attendance(id);
}

function dpi_delete_appointment(appointmentId) {
        dpi_deleteAppointment(curr_event, appointmentId);
}

function dpi_ui_remove_appointment(appointmentId) {
    var appointmentElement = document.getElementById(`appointment-${appointmentId}`);
    if (appointmentElement) {
        appointmentElement.remove();
    }
}

function dpi_set_attendance(appointmentId, status) {
    dpi_setAppointmentAttendance(curr_event, appointmentId, status);
}

function dpi_ui_update_appointment_attendance(appointmentId) {
    var event = tremola.event[curr_event];
    var appointment = event.appointments[appointmentId];
    var attendanceList = document.getElementById(`attendance-list-${appointmentId}`);
    var attendanceSummary = document.getElementById(`attendance-summary-${appointmentId}`);

    if (!attendanceList || !attendanceSummary) return;

    // Update summary
    attendanceSummary.innerHTML = `
        <span style="color: green;">${appointment.attending.length} attending</span> |
        <span style="color: red;">${appointment.notAttending.length} not attending</span> |
        <span style="color: gray;">${appointment.unknown.length} unknown</span>
    `;

    // Update attendance list
    attendanceList.innerHTML = '';
    var allMembers = appointment.attending.concat(appointment.notAttending, appointment.unknown);

    allMembers.forEach(userId => {
        var userName = tremola.contacts[userId].alias;
        var status = appointment.attending.includes(userId) ? 'Attending' :
                     appointment.notAttending.includes(userId) ? 'Not Attending' : 'Unknown';
        var statusColor = status === 'Attending' ? 'green' :
                          status === 'Not Attending' ? 'red' : 'gray';

        attendanceList.innerHTML += `<div><span style="color: ${statusColor};">${userName}: ${status}</span></div>`;
    });

    // Update button styles
    var attendingButton = document.querySelector(`#appointment-${appointmentId} .attendance-button.attending`);
    var notAttendingButton = document.querySelector(`#appointment-${appointmentId} .attendance-button.not-attending`);

    attendingButton.classList.remove('active');
    notAttendingButton.classList.remove('active');

    if (appointment.attending.includes(myId)) {
        attendingButton.classList.add('active');
    } else if (appointment.notAttending.includes(myId)) {
        notAttendingButton.classList.add('active');
    }
}


function dpi_btn_add_appointment() {
    var date = document.getElementById('appointment_date').value;
    var time = document.getElementById('appointment_time').value;

    if (date && time) {
        dpi_createAppointment(curr_event, date, time);
        closeOverlay();
    } else {
        launch_snackbar("Please enter both date and time");
    }
}

function dpi_load_column(columnID) {
    var event = tremola.event[curr_event]

    console.log("EVENT:", JSON.stringify(event.appointments[columnID]))

    /*
    if (event.appointments[columnID].removed) {
        return
    }*/

    dpi_ui_add_appointment(event.appointments[columnID].id, event.appointments[columnID].date,event.appointments[columnID].time)


}

function dpi_load_all_columns() {
    var event = tremola.event[curr_event]
    for (var i in event.appointments) {
        console.log("appointment:" + JSON.stringify(i))
        dpi_load_column(i)
    }
}

function dpi_context_menu_column_options(columnID) {
    dpi_close_event_context_menu()
    document.getElementById("overlay-trans").style.display = 'initial';
    var context_menu = document.getElementById('context_options-' + columnID)
    context_menu.style.display = 'block'
    curr_context_menu = 'context_options-' + columnID
    context_menu.innerHTML = "<button class='context_options_btn' onclick='dpi_menu_rename_column(\"" + columnID + "\")'>Rename list</button>"
    context_menu.innerHTML += "<button class='context_options_btn' onclick='dpi_menu_create_column_item(\"" + columnID + "\")'>Add new card</button>"
    // context_menu.innerHTML += "<button class='context_options_btn' onclick='contextmenu_move_column(\"" + columnID + "\")'>Move List...</button>"
    context_menu.innerHTML += "<button class='context_options_btn' onclick='dpi_btn_remove_column(\"" + columnID + "\")' style='color:red;'>Delete</button>"
    overlayIsActive = true
}

function dpi_contextmenu_move_column(columnID) {
    document.getElementById('context_options-' + columnID).innerHTML = ''
    var event = tremola.event[curr_event]
    var availablePos = [];
    for (var i = 1; i <= event.numOfActiveColumns; i++) {
        availablePos.push(i);
    }
    availablePos.splice(availablePos.indexOf(event.columns[columnID].position), 1)
    var menuHTML = "<button class='context_options_btn' onclick='dpi_context_menu_column_options(\"" + columnID + "\")'> ... </button>"

    for (var i in availablePos) {
        menuHTML += "<button class='context_options_btn' onclick='dpi_ui_move_column(\"" + columnID + "\",\"" + availablePos[i] + "\")'>" + availablePos[i] + "</button>"
    }

    document.getElementById('context_options-' + columnID).innerHTML = menuHTML
}

function dpi_menu_rename_column(columnID) {
    curr_column = columnID
    menu_edit('event_rename_column', 'Enter new name: ', tremola.event[curr_event].columns[columnID].name)
}

function dpi_btn_remove_column(columnID) {
    dpi_removeColumn(curr_event, columnID)
    closeOverlay()
}

function dpi_ui_rename_column(columnID, new_name) {
    var wrapper = document.getElementById(columnID + '-columnWrapper')

    if (!wrapper) {
        dpi_load_column(columnID)
        return
    }

    wrapper.getElementsByClassName('column_hdr')[0].getElementsByTagName('b')[0].innerHTML = new_name
}

function dpi_ui_remove_column(columnID) {
    var event = tremola.event[curr_event]

    if (!document.getElementById(columnID + "-columnWrapper"))
        return
    document.getElementById(columnID + "-columnWrapper").outerHTML = "" //remove column from ui
}

function dpi_ui_move_column(columnID, insertPos) {
    console.log('move', columnID, insertPos);
    closeOverlay()
    insertPos = parseInt(insertPos)
    var event = tremola.event[curr_event]
    var oldPos = event.columns[columnID].position
    if (oldPos == insertPos) // column is already on given position
        return

    for (var i in event.columns) {
        var curr_column = event.columns[i]

        if (curr_column.removed)
            continue

        if (i == columnID)
            continue

        if (oldPos > insertPos) {
            if (curr_column.position < insertPos) {
                continue
            } else if (curr_column.position > event.columns[columnID].position) {
                continue
            } else {
                document.getElementById(i + "-columnWrapper").style.order = ++curr_column.position
            }
        } else {
            if (curr_column.position < event.columns[columnID].position) {
                continue
            } else if (curr_column.position > insertPos) {
                continue
            } else {
                document.getElementById(i + "-columnWrapper").style.order = --curr_column.position
            }
        }

    }
    document.getElementById(columnID + '-columnWrapper').style.order = insertPos
    event.columns[columnID].position = insertPos
    persist()
}

/*
  Items
*/

function dpi_menu_create_item(columnID) {
    curr_column = columnID
    menu_edit('event_new_item', 'Enter name of new Card', '')
}

function dpi_load_item(itemID) {
    var event = tremola.event[curr_event]

    var name = event.items[itemID].name
    var pos = event.items[itemID].position
    var columnID = event.items[itemID].curr_column
    var color = event.items[itemID].color

    if (document.getElementById(itemID + "-item")) {
        document.getElementById(itemID + "-item").outerHTML = ""
    }

    if (event.items[itemID].removed || event.columns[columnID].removed)
        return

    var itemHTML = "<div class='column_item' style='order:" + pos + ";overflow: auto;' id='" + itemID + "-item' onclick='dpi_item_menu(\"" + itemID + "\")'  draggable='true' ondragstart='dpi_dragStart(event)' ondrop='dpi_dragDrop(event)' ondragover='dpi_allowDrop(event)'>" //event_item_button
    itemHTML += "<div style='padding-top: 10px; padding-left: 10px; padding-bottom: 10px'> <b><font id='" + itemID + "-itemHdr' color='" + color + "'>" + name + "</font></b></div>"
    itemHTML += "<div id='" + itemID + "-itemDescr' style='font: 12px Helvetica Neue, sans-serif; color: #808080;overflow-wrap: break-word;max-height: 4.8em;overflow: hidden;padding-left: 10px;padding-right: 10px;'>" + event.items[itemID].description + "</div>"
    itemHTML += "<div id='" + itemID + "-itemAssignees' style='padding-left: 10px;padding-right: 10px;display:flex;justify-content: flex-start;flex-direction: row;padding-bottom:10px;padding-top:5px;'>"

    for (var i in event.items[itemID].assignees) {
        var assigneeID = event.items[itemID].assignees[i]
        var assigneeInitial = tremola.contacts[assigneeID].initial
        var assigneeColor = tremola.contacts[assigneeID].color

        itemHTML += "<div style=';padding-left:3px;overflow: auto;'>"
        itemHTML += "<button class=contact_picture style='height: 1.5em; width: 1.5em; box-shadow: none; background: " + assigneeColor + ";'>" + assigneeInitial + "</button>"
        itemHTML += "</div>"
    }

    itemHTML += "</div>" //</button>

    document.getElementById(columnID + "-columnContent").innerHTML += itemHTML;
    document.getElementById(itemID + "-item").addEventListener('mousedown', dpi_myTouchFct, false);
}

function dpi_load_all_items() {
    var event = tremola.event[curr_event]
    for (var i in event.items) {
        dpi_load_item(i)
    }
}

function dpi_item_menu(itemID) {
    closeOverlay()
    curr_rename_item = itemID
    curr_item = itemID
    document.getElementById('overlay-bg').style.display = 'initial';
    overlayIsActive = true;

    var item = tremola.event[curr_event].items[itemID]
    document.getElementById('div:item_menu').style.display = 'initial'
    document.getElementById('btn:item_menu_description_save').style.display = 'none'
    document.getElementById('btn:item_menu_description_cancel').style.display = 'none'
    document.getElementById('item_menu_comment_text').value = ''
    document.getElementById('item_menu_title').innerHTML = "<font color='" + item.color + "'>" + item.name + "</font>"
    // console.log(item)
    document.getElementById('item_menu_title').innerHTML = "<font id='" + itemID + "-itemMenuHdr' color='" + item.color + "'>" + item.name + "</font>"

    //load description
    var descDiv = document.getElementById('div:item_menu_description')
    document.getElementById('div:item_menu_description_text').value = item.description

    document.getElementById('div:item_menu_description_text').addEventListener('focus',
        function (event) {
            document.getElementById('btn:item_menu_description_save').style.display = 'initial'
            document.getElementById('btn:item_menu_description_cancel').style.display = 'initial'
            event.target.style.border = "solid 2px black"
            document.getElementById('div:item_menu_description_text').removeEventListener('focus', event)
        });

    //load chat
    var draftDiv = document.getElementById('div:item_menu_comments')
    var commentsHTML = ''
    var commentsList = item.comments.slice().reverse()
    for (var i in commentsList) {
        var author_name = tremola.contacts[commentsList[i][0]].alias
        var author_color = tremola.contacts[commentsList[i][0]].color
        var author_initial = tremola.contacts[commentsList[i][0]].initial

        commentsHTML += "<div class='w100' style='padding: 5px 5px 5px;overflow:auto;'>"
        commentsHTML += "<button class=contact_picture style='float:left;margin-right: 0.75em; background: " + author_color + ";'>" + author_initial + "</button>"
        commentsHTML += "<div class='chat_item_button light' style='display: table;float:right;overflow: hidden; width: calc(100% - 4.4em);word-break: break-word;margin-right:10px'>"
        commentsHTML += "<div style='display: table-cell;padding-left:10px;vertical-align: middle;overflow-wrap: break-word;'>" + commentsList[i][1] + "</div>"
        commentsHTML += "</div></div>"
    }
    document.getElementById('lst:item_menu_posts').innerHTML = commentsHTML

    //load assignees display
    var assigneesDiv = document.getElementById('div:item_menu_assignees')
    var order = 0;
    var assigneesHTML = ""
    for (var i in item.assignees) {
        var author_color = tremola.contacts[item.assignees[i]].color
        var author_initial = tremola.contacts[item.assignees[i]].initial
        assigneesHTML += "<button class=contact_picture style='float:left;margin-right: 0.75em; background: " + author_color + ";'>" + author_initial + "</button>"
    }
    document.getElementById("div:item_menu_assignees").innerHTML = assigneesHTML
}

function dpi_item_menu_save_description() {
    var new_description = document.getElementById('div:item_menu_description_text').value
    var item = tremola.event[curr_event].items[curr_item]

    if (item.description != new_description) {
        dpi_setItemDescription(curr_event, curr_item, new_description)
    }
    document.getElementById('btn:item_menu_description_save').style.display = 'none'
    document.getElementById('div:item_menu_description_text').style.border = 'none'
    document.getElementById('btn:item_menu_description_cancel').style.display = 'none'
}

function dpi_item_menu_cancel_description() {
    document.getElementById('div:item_menu_description_text').style.border = 'none'
    document.getElementById('div:item_menu_description_text').value = tremola.event[curr_event].items[curr_item].description
    document.getElementById('btn:item_menu_description_save').style.display = 'none'
    document.getElementById('btn:item_menu_description_cancel').style.display = 'none'
}


function dpi_contextmenu_change_column() {
    dpi_close_event_context_menu()
    document.getElementById("overlay-trans").style.display = 'initial';

    var event = tremola.event[curr_event]
    var OptionHTML = "<div class='context_menu' id='context_options-" + curr_item + "-changeColumn'>"

    if (event.numOfActiveColumns <= 1) {
        launch_snackbar("There is only one list")
    }

    var columnPositionList = Object.keys(event.columns).map(function (key) {
        return [key, event.columns[key].position];
    })
    columnPositionList.sort(function (a, b) {
        return a[1] - b[1];
    })

    for (var i in columnPositionList) {
        var columnID = columnPositionList[i][0]
        if ((event.items[curr_item].curr_column == columnID) || (event.columns[columnID].removed))
            continue

        OptionHTML += "<button class='context_options_btn' onclick='dpi_btn_move_item(\"" + curr_item + "\",\"" + columnID + "\")'>" + event.columns[columnID].name + "</button>"
    }
    OptionHTML += "</div>"

    curr_context_menu = 'context_options-' + curr_item + "-changeColumn"
    document.getElementById("change_column_options").innerHTML = OptionHTML
    document.getElementById(curr_context_menu).style.display = 'block'
    overlayIsActive = true
}

function dpi_btn_move_item(item, new_column) {
    dpi_moveItem(curr_event, item, new_column)
    dpi_close_event_context_menu()
}

function dpi_btn_remove_item() {
    dpi_removeItem(curr_event, curr_item)
    closeOverlay()
}

function dpi_contextmenu_item_change_position() {
    dpi_close_event_context_menu()

    var event = tremola.event[curr_event]
    var itemList = event.columns[event.items[curr_item].curr_column].item_ids
    var itemPosList = []

    for (var i in itemList) {
        var item = event.items[itemList[i]]

        if (item.removed)
            continue
        if (item.id == event.items[curr_item].id)
            continue

        itemPosList.push([i, item.position])
    }
    itemPosList.sort(function (a, b) {
        return a[1] - b[1];
    })
    // console.log(itemPosList)
    var posHTML = "<div class='context_menu' id='context_options-" + curr_item + "-changePosition'>"
    for (var i in itemPosList) {
        posHTML += "<button class='context_options_btn' onclick='dpi_ui_change_item_order(\"" + curr_item + "\",\"" + itemPosList[i][1] + "\")'>" + itemPosList[i][1] + "</button>"
    }
    curr_context_menu = 'context_options-' + curr_item + "-changePosition"
    document.getElementById("change_position_options").innerHTML = posHTML
    document.getElementById(curr_context_menu).style.display = 'block'
    overlayIsActive = true
}

function dpi_contextmenu_item_assign() {
    dpi_close_event_context_menu()
    var event = tremola.event[curr_event]

    var aliasList = []

    for (var m in event.members) {
        aliasList.push([tremola.contacts[event.members[m]].alias, event.members[m]])
    }
    aliasList.sort()

    var assignHTML = "<div class='context_menu' id='context_options-" + curr_item + "-assign'>"
    for (var i in aliasList) {
        var alias = aliasList[i][0]
        var fid = aliasList[i][1]
        var assigned = event.items[curr_item].assignees.indexOf(fid) >= 0
        var color = assigned ? "background-color: #aaf19f;" : "";

        assignHTML += "<button class='context_options_btn' style='" + color + "' onclick='dpi_ui_item_assign(\"" + fid + "\")'>" + alias + "</button>"
    }
    curr_context_menu = 'context_options-' + curr_item + "-assign"
    document.getElementById("assign_options").innerHTML = assignHTML
    document.getElementById(curr_context_menu).style.display = 'block'
    overlayIsActive = true
}

function dpi_btn_post_comment() {
    var comment = document.getElementById('item_menu_comment_text').value
    if (comment == '')
        return

    dpi_postItemComment(curr_event, curr_item, comment)
    dpi_item_menu(curr_item)
}

function dpi_btn_rename_item() {

}

function dpi_ui_change_item_order(itemID, newPos) {
    dpi_close_event_context_menu()
    newPos = parseInt(newPos)
    var event = tremola.event[curr_event]
    var oldPos = event.items[itemID].position
    var column = event.columns[event.items[itemID].curr_column]

    if (oldPos == newPos) // column is already on given position
        return

    for (var i of column.item_ids) {
        var curr_item = event.items[i]

        if (curr_item.removed)
            continue

        if (i == itemID)
            continue

        if (oldPos > newPos) {
            if (curr_item.position < newPos) {
                continue
            } else if (curr_item.position > event.items[itemID].position) {
                continue
            } else {
                document.getElementById(i + "-item").style.order = ++curr_item.position
            }
        } else {
            if (curr_item.position < event.items[itemID].position) {
                continue
            } else if (curr_item.position > newPos) {
                continue
            } else {
                document.getElementById(i + "-item").style.order = --curr_item.position
            }
        }

    }
    document.getElementById(itemID + '-item').style.order = newPos
    event.items[itemID].position = newPos
    persist()
}

function dpi_ui_remove_item(itemID) {
    var event = tremola.event[curr_event]
    var column = event.columns[event.items[itemID].curr_column]

    if (!document.getElementById(itemID + '-item'))
        return

    for (var i in column.item_ids) {
        var curr_item = event.items[column.item_ids[i]]
        if (curr_item.position > event.items[itemID].position) {
            curr_item.position--
            document.getElementById(curr_item.id + '-item').style.order = curr_item.position
        }
    }

    document.getElementById(itemID + '-item').outerHTML = ""
}

function dpi_ui_item_assign(fid) {
    //dpi_close_event_context_menu()
    var event = tremola.event[curr_event]
    var alreadyAssigned = event.items[curr_item].assignees.includes(fid)

    if (alreadyAssigned)
        dpi_unassignFromItem(curr_event, curr_item, fid)
    else
        dpi_assignToItem(curr_event, curr_item, fid)
}

function dpi_ui_move_item(itemID, old_pos) {
    var event = tremola.event[curr_event]
    var item = event.items[itemID]
    var old_column = event.columns[item.curr_column]

    document.getElementById(itemID + '-item').style.order = item.position
    for (var i in old_column.item_ids) {
        if (event.items[old_column.item_ids[i]].position > old_pos)
            document.getElementById(old_column.item_ids[i] + '-item').style.order = --event.items[old_column.item_ids[i]].position
    }
    dpi_load_item(curr_op.body.cmd[1])
}

function dpi_contextmenu_change_color() {
    dpi_close_event_context_menu()
    var event = tremola.event[curr_event]
    var item = event.items[curr_item]

    var colors = Object.values(Color)
    colors.splice(colors.indexOf(item.color), 1)

    var colorHTML = "<div class='context_menu' id='context_options-" + curr_item + "-color'>"

    for (var c of colors) {
        colorHTML += "<button class='context_options_btn' onclick='dpi_btn_change_item_color(\"" + curr_item + "\", \"" + c + "\")'>" + c + "</button>"
    }

    curr_context_menu = 'context_options-' + curr_item + "-color"
    document.getElementById("change_color_options").innerHTML = colorHTML
    document.getElementById(curr_context_menu).style.display = 'block'
    overlayIsActive = true
}

function dpi_btn_change_item_color(iid, color) {
    dpi_close_event_context_menu()
    dpi_setItemColor(curr_event, iid, color)
}

function dpi_ui_update_item_name(itemID, new_name) {
    var hdr = document.getElementById(itemID + '-itemHdr')

    if (hdr) {
        hdr.innerHTML = new_name
    } else {
        dpi_load_item(itemID)
    }

    if (curr_item == itemID) {
        var itemMenuHdr = document.getElementById(itemID + '-itemMenuHdr')
        if (itemMenuHdr) {
            itemMenuHdr.innerHTML = new_name
        } else {
            dpi_item_menu(itemID)
        }
    }
}

function dpi_ui_update_item_color(itemID, new_color) {
    var hdr = document.getElementById(itemID + '-itemHdr')

    if (hdr) {
        hdr.style.color = new_color
    } else {
        dpi_load_item(itemID)
    }

    if (curr_item == itemID) {
        var itemMenuHdr = document.getElementById(itemID + '-itemMenuHdr')
        if (itemMenuHdr) {
            itemMenuHdr.style.color = new_color
        } else {
            dpi_item_menu(itemID)
        }
        if (curr_context_menu && curr_context_menu.split('-')[2] == 'color')
            dpi_contextmenu_change_color()
    }
}

function dpi_ui_update_item_description(itemID, new_descr) {
    var itemDescr = document.getElementById(itemID + '-itemDescr')


    if (itemDescr) {
        itemDescr.innerHTML = new_descr
    } else {
        dpi_load_item(itemID)
    }

    if (curr_item == itemID) {
        var itemMenuDescr = document.getElementById('div:item_menu_description_text')
        if (itemMenuDescr) {
            itemMenuDescr.value = new_descr
        } else {
            dpi_item_menu(itemID)
        }
    }
}

function dpi_ui_update_item_assignees(itemID) {
    var event = tremola.event[curr_event]
    var item = event.items[itemID]

    var assigneesWrapper = document.getElementById(itemID + '-itemAssignees')
    if (assigneesWrapper) {
        var assigneesHTML = ''
        for (var i in event.items[itemID].assignees) {
            var assigneeID = event.items[itemID].assignees[i]
            var assigneeInitial = tremola.contacts[assigneeID].initial
            var assigneeColor = tremola.contacts[assigneeID].color

            assigneesHTML += "<button class=contact_picture style='height: 1.5em; width: 1.5em; box-shadow: none; background: " + assigneeColor + ";'>" + assigneeInitial + "</button>"
        }
        assigneesWrapper.innerHTML = assigneesHTML
    } else {
        dpi_load_item(itemID)
    }

    if (curr_item == itemID) {
        var assigneesMenuWrapper = document.getElementById('div:item_menu_assignees')
        if (assigneesMenuWrapper) {
            var assigneesMenuHTML = ""
            for (var i in item.assignees) {
                var author_color = tremola.contacts[item.assignees[i]].color
                var author_initial = tremola.contacts[item.assignees[i]].initial
                assigneesMenuHTML += "<button class=contact_picture style='float:left;margin-right: 0.75em; background: " + author_color + ";'>" + author_initial + "</button>"
            }
            assigneesMenuWrapper.innerHTML = assigneesMenuHTML
        } else {
            dpi_item_menu(itemID)
        }
        if (curr_context_menu && curr_context_menu.split('-')[2] == 'assign') {
            dpi_contextmenu_item_assign()
        }
    }
}

function dpi_ui_item_update_chat(itemID) {
    var event = tremola.event[curr_event]
    var item = event.items[itemID]

    if (curr_item == itemID) {
        var chatWrapper = document.getElementById('lst:item_menu_posts')
        if (chatWrapper) {
            var commentsHTML = ''
            var commentsList = [...item.comments].reverse()
            for (var i in commentsList) {
                var author_name = tremola.contacts[commentsList[i][0]].alias
                var author_color = tremola.contacts[commentsList[i][0]].color
                var author_initial = tremola.contacts[commentsList[i][0]].initial

                commentsHTML += "<div class='w100' style='padding: 5px 5px 5px;overflow:auto;'>"
                commentsHTML += "<button class=contact_picture style='float:left;margin-right: 0.75em; background: " + author_color + ";'>" + author_initial + "</button>"
                commentsHTML += "<div class='chat_item_button light' style='display: table;float:right;overflow: hidden; width: calc(100% - 4.4em);word-break: break-word;margin-right:10px'>"
                commentsHTML += "<div style='display: table-cell;padding-left:10px;vertical-align: middle;overflow-wrap: break-word;'>" + commentsList[i][1] + "</div>"
                commentsHTML += "</div></div>"
            }
            chatWrapper.innerHTML = commentsHTML
        } else {
            dpi_item_menu(itemID)
        }
    }
}

function dpi_ui_update_item_move_to_column(itemID, columnID, newPos) {
    var itemHTML = document.getElementById(itemID + "-item").outerHTML
    document.getElementById(itemID + "-item").outerHTML = ''
    document.getElementById(columnID + "-columnContent").innerHTML += itemHTML
    document.getElementById(itemID + '-item').style.order = newPos

    if (curr_item == itemID) {
        if (curr_context_menu) {
            if (curr_context_menu.split('-')[2] == 'changePosition')
                dpi_contextmenu_item_change_position()
            else if (curr_context_menu.split('-')[2] == 'changeColumn')
                dpi_contextmenu_change_column()
        }
    }
}


