//event.js

"use strict";

var curr_event;
var curr_context_menu;
var curr_column;
var curr_item;

var curr_rename_item;

// all available operations
const OperationTest = {
    EVENT_CREATE: 'event/create',
 APPOINTMENT_CREATE: 'appointment/create',
    APPOINTMENT_REMOVE: 'appointment/remove',
     INVITE: 'invite',
     INVITE_ACCEPT: 'invite/accept',
     INVITE_DECLINE: 'invite/decline',
 APPOINTMENT_SET_ATTENDANCE: 'appointment/setAttendance',
 APPOINTMENT_DELETE: 'appointment/delete',
  EVENT_DELETE: 'event/delete',


    event_RENAME: 'event/rename',
    COLUMN_CREATE: 'column/create',
    ITEM_CREATE: 'item/create',
    COLUMN_REMOVE: 'column/remove',
    COLUMN_RENAME: 'column/rename',
    ITEM_REMOVE: 'item/remove',
    ITEM_RENAME: 'item/rename',
    ITEM_MOVE: 'item/moveTo',
    ITEM_SET_DESCRIPTION: 'item/setDiscription',
    ITEM_POST_COMMENT: 'item/post',
    ITEM_ASSIGN: 'item/assign',
    ITEM_UNASSIGN: 'item/unassign',
    ITEM_COLOR: 'item/color',
    LEAVE: 'leave'
}

const FLAGTest = {
    PERSONAL: 'personal'
}


function dpi_createEvent(name, flags) {
    var cmd = [OperationTest.EVENT_CREATE, name]
    if (flags != null)
        cmd = cmd.concat(flags)

    var data = {
        'bid': null,
        'cmd': cmd,
        'prev': null
    }
    dpi_event_send_to_backend(data)
}


function dpi_renameevent(bid, name) {
    var event = tremola.event[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.event_RENAME, name],
        'prev': event.curr_prev
    }
    dpi_event_send_to_backend(data)
}

function dpi_createColumn(bid, name) {
    console.log("create column")
    var event = tremola.event[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.COLUMN_CREATE, name],
        'prev': event.curr_prev
    }
    dpi_event_send_to_backend(data)
}

function dpi_createColumnItem(bid, cid, name) {
    var event = tremola.event[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.ITEM_CREATE, cid, name],
        'prev': event.curr_prev
    }
    dpi_event_send_to_backend(data)
}

function dpi_removeColumn(bid, cid) {
    var event = tremola.event[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.COLUMN_REMOVE, cid],
        'prev': event.curr_prev
    }
    dpi_event_send_to_backend(data)
}

function dpi_renameColumn(bid, cid, newName) {
    var event = tremola.event[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.COLUMN_RENAME, cid, newName],
        'prev': event.curr_prev
    }
    dpi_event_send_to_backend(data)
}

function dpi_removeItem(bid, iid) {
    var event = tremola.event[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.ITEM_REMOVE, iid],
        'prev': event.curr_prev
    }
    dpi_event_send_to_backend(data)
}

function dpi_renameItem(bid, iid, new_name) {
    var event = tremola.event[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.ITEM_RENAME, iid, new_name],
        'prev': event.curr_prev
    }
    dpi_event_send_to_backend(data)
}

function dpi_moveItem(bid, iid, new_cid) {
    var event = tremola.event[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.ITEM_MOVE, iid, new_cid],
        'prev': event.curr_prev
    }
    dpi_event_send_to_backend(data)
}

function dpi_setItemDescription(bid, iid, description) {
    var event = tremola.event[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.ITEM_SET_DESCRIPTION, iid, description],
        'prev': event.curr_prev
    }
    dpi_event_send_to_backend(data)
}

function dpi_postItemComment(bid, iid, comment) {
    var event = tremola.event[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.ITEM_POST_COMMENT, iid, comment],
        'prev': event.curr_prev
    }
    dpi_event_send_to_backend(data)
}

function dpi_assignToItem(bid, iid, assigned) {
    var event = tremola.event[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.ITEM_ASSIGN, iid, assigned],
        'prev': event.curr_prev
    }
    dpi_event_send_to_backend(data)
}

function dpi_unassignFromItem(bid, iid, unassign) {
    var event = tremola.event[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.ITEM_UNASSIGN, iid, unassign],
        'prev': event.curr_prev
    }
    dpi_event_send_to_backend(data)
}

function dpi_setItemColor(bid, iid, color) {
    var event = tremola.event[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.ITEM_COLOR, iid, color],
        'prev': event.curr_prev
    }
    dpi_event_send_to_backend(data)
}

function dpi_inviteUser(bid, userID) {
    var event = tremola.event[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.INVITE, userID],
        'prev': event.curr_prev
    }
    dpi_event_send_to_backend(data)
}

function dpi_inviteAccept(bid, prev) {
    var event = tremola.event[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.INVITE_ACCEPT],
        'prev': prev
    }
    dpi_event_send_to_backend(data)
}

function dpi_inviteDecline(bid, prev) {
    var event = tremola.event[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.INVITE_DECLINE],
        'prev': prev
    }
    dpi_event_send_to_backend(data)
}

function dpi_leave(bid) {
    var event = tremola.event[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.LEAVE],
        'prev': event.curr_prev
    }
    dpi_event_send_to_backend(data)
}


function dpi_event_send_to_backend(data) {
    var bid = data['bid'] != null ? data['bid'] : "null"
    var prevs = data['prev'] != null ? btoa(data['prev'].map(btoa)) : "null"
    var op = data['cmd'][0] //btoa(data['cmd'][0])
    var args = data['cmd'].length > 1 ? btoa(data['cmd'].slice(1).map(unicodeStringToTypedArray).map(btoa)) : "null"
    var to_backend = ['scheduling', bid, prevs, op, args]
    console.log("SEND TO BACKEND", to_backend.join(" "))
    backend(to_backend.join(" "))
}

function dpi_scheduling_new_event(e) {

if (!tremola.event) {
        tremola.event = {};
    }

    // parse data
    var op = e.public[3]
    var bid = op == OperationTest.EVENT_CREATE ? e.header.ref : e.public[1]
    var prev = e.public[2] != "null" ? e.public[2] : [] // TODO: change to null instead of "null" if backend sends this field as Bipf.mkNone()
    var args = e.public.length > 4 ? e.public.slice(4) : []



    // add new entry if it is a new event
    if (!(bid in tremola.event)) {
        tremola.event[bid] = {
            "operations": {}, // all received operations for this event
            "sortedOperations": new Timeline(), // "linear timeline", sorted list of operationIds
            "members": [e.header.fid], // members of the event
            "forgotten": false, // flag for hiding this event from the event list
            "name": bid.toString().slice(0, 15) + '...', // name of the event
            "curr_prev": [], // prev pointer
            "appointments": {},
            "numOfActiveColumns": 0,
            "history": [],
            "lastUpdate": Date.now(),
            "unreadEvents": 0,
            "subscribed": false,
            "pendingInvitations": {}, // User: [inviteIds]
            "key": bid.toString(),
            "flags": []
        }
    }

    var event = tremola.event[bid]

    if (op == OperationTest.EVENT_CREATE) {
        event.name = args[0]
        event.flags = args.slice(1)
        event.creator = e.header.fid;
        if (document.getElementById('scheduling-invitations-overlay').style.display != 'none')
            if (document.getElementById("scheduling_invitation_" + bid))
                dpi_menu_event_invitation_create_entry(bid)
        if (e.header.fid == myId)
            event.subscribed = true // the creator of the event is automatically subscribed
    }

    if (!(event.sortedOperations instanceof Timeline)) { // deserialize ScuttleSort-Timeline
        event.sortedOperations = Timeline.fromJSON(event.sortedOperations)
    }

    if (e.header.ref in event.operations)
        return


    // translation of the event format into the scheduling event format
    var body = {
        'bid': bid,
        'cmd': [op].concat(args),
        'prev': prev
    }

    // store new event
    var p = {"key": e.header.ref, "fid": e.header.fid, "fid_seq": e.header.seq, "body": body, "when": e.header.tst};
    event["operations"][e.header.ref] = p;

    if (op == Operation.LEAVE && e.header.fid == myId) {
        delete event.pendingInvitations[myId]
        event.subscribed = false
       dpi_load_event_list()
    }

    if (event.subscribed) {
        event.sortedOperations.add(e.header.ref, prev)

        var independentOPs = [OperationTest.APPOINTMENT_CREATE, OperationTest.APPOINTMENT_REMOVE, OperationTest.LEAVE]

        if (event.sortedOperations.name2p[e.header.ref].indx == event.sortedOperations.linear.length - 1 || independentOPs.indexOf(event.operations[e.header.ref].body.cmd[0]) >= 0) {
            if (curr_scenario == 'event' && curr_event == bid)
                dpi_apply_operation(bid, e.header.ref, true)
            else
                dpi_apply_operation(bid, e.header.ref, false)
        } else {
            console.log("DEBUG APPLYALL")
            dpi_apply_all_operations(bid)
        }

        event.curr_prev = event.sortedOperations.get_tips()
        event.lastUpdate = Date.now()

        if (curr_scenario != 'event' || curr_event != bid)
            event.unreadEvents++

        dpi_load_event_list()

        if (op == OperationTest.EVENT_CREATE && e.header.fid == myId) {
            var pendingInvites = []
            for (var m in tremola.contacts) {
                if (m != myId && document.getElementById(m).checked) {
                    dpi_inviteUser(bid, m)
                    console.log("Invited: " + m)
                }
            }
            if (curr_scenario == 'members')
                dpi_load_event(bid)
        }
    } else {
        if (op == OperationTest.INVITE && body.cmd[1] == myId) {
            if (myId in event.pendingInvitations)
                event.pendingInvitations[myId].push(e.header.ref)
            else {
                event.pendingInvitations[myId] = [e.header.ref]
                launch_snackbar('New event invitation received')
                if (document.getElementById('scheduling-invitations-overlay').style.display != 'none') {
                    dpi_menu_event_invitation_create_entry(bid)
                }
            }
        }

        if (op == OperationTest.INVITE_ACCEPT && e.header.fid == myId) {
            event.subscribed = true
            dpi_event_reload(bid)
            event.lastUpdate = Date.now()
            event.unreadEvents++
            event.curr_prev = event.sortedOperations.get_tips()
            dpi_load_event_list()
            return
        }

        if (op == OperationTest.INVITE_DECLINE && e.header.fid == myId) {
            delete event.pendingInvitations[myId]
        }
    }
}

function dpi_reload_curr_event() {
    if (curr_event)
        dpi_event_reload(curr_event)
}

function dpi_event_reload(bid) {
    console.log("event reload " + bid)
    var event = tremola.event[bid]
    event.columns = {}
    event.numOfActiveColumns = 0
    event.items = {}
    event.pendingOperations = {}
    event.pendingInvitations = {}
    event.members = []
    event.sortedOperations = new Timeline()

    for (var op in event.operations) {
        console.log("ADD op: " + op + ", prev:" + event.operations[op].body.prev)
        event.sortedOperations.add(op, event.operations[op].body.prev)
    }
    dpi_apply_all_operations(bid)

    if (curr_scenario == 'event' && curr_event == bid) {
        closeOverlay()
        curr_item = null
        curr_column = null
        curr_context_menu = null
        dpi_load_event(bid)
    }
}

/**
 * Creates a snapshot of the given scheduling event. It applies all operations and updates the user interface
 *
 * @param {string} bid - Id of the scheduling event
 */
function dpi_apply_all_operations(bid) {
    var event = tremola.event[bid]
    event.history = []

    var old_state = JSON.parse(JSON.stringify(event));

    //execute operations and save results to local storage
    var validOps = dpi_helper_linear_timeline_without_pending_prevs(event.sortedOperations)
    for (var i in validOps) {
        dpi_apply_operation(bid, validOps[i], false)
    }

    if (curr_scenario == 'event' && curr_event == bid) { // update ui
        dpi_ui_update_event(bid, old_state)
        console.log("UP CURR")
    }
}

// returns linear timeline that does not contain nodes which have only pending predecessors
function dpi_helper_linear_timeline_without_pending_prevs(timeline) {
    var lst = []
    for (let n of timeline.linear) {
        var validPrevs = 0
        for (let p of n.prev) {
            if ((typeof p != "string") && !(p.name in timeline.pending))
                validPrevs++
        }
        if (validPrevs > 0 || n.prev.length == 0) {
            lst.push(n.name);
        }
    }

    return lst;
}

function dpi_apply_operation(bid, operationID, apply_on_ui) {
    console.log("Apply:" + operationID)
    var event = tremola.event[bid]
    var curr_op = event['operations'][operationID]

    var author_name = tremola.contacts[curr_op.fid].alias
    var historyMessage = author_name + " "

    switch (curr_op.body.cmd[0]) {
        case OperationTest.EVENT_CREATE:
            historyMessage += "created the event \"" + curr_op.body.cmd[1] + "\""
            event.name = curr_op.body.cmd[1]
            if (event.members.indexOf(curr_op.fid) < 0)
                event.members.push(curr_op.fid)
            if (curr_op.fid == myId)
                event.subscribed = true
            /*if(event.members.indexOf(curr_op.fid) < 0)
              event.members.push(curr_op.fid)
            */
            break;

         case OperationTest.EVENT_DELETE:
                    if (curr_op.fid === event.creator) {
                        event.removed = true;
                        event.subscribed = false;
                        historyMessage += `deleted event ${bid}`;
                        if (apply_on_ui) {
                            dpi_ui_remove_event(bid);
                        }
                    } else {
                        historyMessage += `attempted to delete event ${bid} but was not the creator`;
                    }
                    break;

        case Operation.event_RENAME:
            historyMessage += "renamed the event \"" + event.name + "\" to \"" + curr_op.body.cmd[1] + "\""
            event.name = curr_op.body.cmd[1]

            if (apply_on_ui)
                dpi_ui_update_event_title(bid)
            break

        case OperationTest.APPOINTMENT_CREATE:
           var date = curr_op.body.cmd[1];
           var time = curr_op.body.cmd[2];
           historyMessage += "created an appointment on " + date + " at " + time;
           event.appointments[curr_op.key] = {
                            'date': curr_op.body.cmd[1],
                            'time': curr_op.body.cmd[2],
                            'id': curr_op.key.toString(),
                            'removed': false,
                            'attending': [],
                            'notAttending': [],
                            'unknown': event.members.slice(), // Initialize with all members
                             'creator': curr_op.fid
                        }
                if (apply_on_ui)
                   dpi_ui_add_appointment(curr_op.key, date, time);
             break;

        case OperationTest.APPOINTMENT_DELETE:
                    var appointmentId = curr_op.body.cmd[1];
                    var appointment = event.appointments[appointmentId];
                    if (appointment && curr_op.fid === appointment.creator) {
                        appointment.removed = true;
                        historyMessage += `deleted appointment ${appointmentId}`;
                        if (apply_on_ui) {
                            dpi_ui_remove_appointment(appointmentId);
                        }
                    } else {
                        historyMessage += `attempted to delete appointment ${appointmentId} but was not the creator`;
                    }
                    break;

        case OperationTest.APPOINTMENT_SET_ATTENDANCE:
                    var appointmentId = curr_op.body.cmd[1];
                    var status = curr_op.body.cmd[2];
                    var appointment = event.appointments[appointmentId];
                    var userId = curr_op.fid;

                    // Remove user from all arrays
                    appointment.attending = appointment.attending.filter(id => id !== userId);
                    appointment.notAttending = appointment.notAttending.filter(id => id !== userId);
                    appointment.unknown = appointment.unknown.filter(id => id !== userId);

                    // Add user to appropriate array
                    if (status === 'attending') {
                        appointment.attending.push(userId);
                    } else if (status === 'notAttending') {
                        appointment.notAttending.push(userId);
                    } else {
                        appointment.unknown.push(userId);
                    }

                    historyMessage += `set attendance status for appointment ${appointmentId} to ${status}`;

                    if (apply_on_ui) {
                        dpi_ui_update_appointment_attendance(appointmentId);
                    }
                    break;

        case Operation.COLUMN_CREATE:
            historyMessage += "created the list \"" + curr_op.body.cmd[1] + "\""
            var newPos = 0
            if (curr_op.key in event.columns) {
                if (event.columns[curr_op.key].removed)
                    break
                newPos = event.columns[curr_op.key].position
            } else
                newPos = ++event.numOfActiveColumns

            event.columns[curr_op.key] = {
                'name': curr_op.body.cmd[1],
                'id': curr_op.key.toString(),
                'item_ids': [],
                'position': newPos,
                'numOfActiveItems': 0,
                'removed': false
            }

            if (apply_on_ui)
                load_column(curr_op.key)
            break
        case Operation.COLUMN_REMOVE:
            if (!(curr_op.body.cmd[1] in event.columns))
                return
            historyMessage += "removed list \"" + event.columns[curr_op.body.cmd[1]].name + "\""
            event.columns[curr_op.body.cmd[1]].removed = true

            for (var i in event.columns) {
                if (event.columns[i].removed)
                    continue

                if (event.columns[i].position > event.columns[curr_op.body.cmd[1]].position) {
                    --event.columns[i].position
                }

            }
            event.numOfActiveColumns--

            if (apply_on_ui)
                ui_remove_column(curr_op.body.cmd[1])
            break
        case Operation.COLUMN_RENAME:
            if (!(curr_op.body.cmd[1] in event.columns))
                break
            historyMessage += "renamed list \"" + event.columns[curr_op.body.cmd[1]].name + "\" to \"" + curr_op.body.cmd[2] + "\""
            event.columns[curr_op.body.cmd[1]].name = curr_op.body.cmd[2]

            if (apply_on_ui)
                ui_rename_column(curr_op.body.cmd[1], curr_op.body.cmd[2])
            break
        case Operation.ITEM_CREATE:
            if (!(curr_op.body.cmd[1] in event.columns))
                break
            historyMessage += "created a card in list \"" + event.columns[curr_op.body.cmd[1]].name + "\" with the name: \"" + curr_op.body.cmd[2] + "\""
            var newPos = 0
            if (curr_op.key in event.items) {
                if (event.items[curr_op.key].removed) {
                    break
                }
                newPos = event.items[curr_op.key].position //there is already a position assigned to the item
            } else {
                newPos = ++event.columns[curr_op.body.cmd[1]].numOfActiveItems
            }

            event.items[curr_op.key] = {
                'name': curr_op.body.cmd[2],
                'id': curr_op.key.toString(),
                'curr_column': curr_op.body.cmd[1],
                'assignees': [],
                'comments': [],
                'description': "",
                'position': newPos,
                'color': Color.BLACK,
                'removed': false
            }
            event.columns[curr_op.body.cmd[1]].item_ids.push(curr_op.key.toString())

            if (apply_on_ui)
                load_item(curr_op.key)
            break
        case Operation.ITEM_REMOVE:
            if (!(curr_op.body.cmd[1] in event.items))
                break
            var item = event.items[curr_op.body.cmd[1]]
            var column = event.columns[item.curr_column]
            historyMessage += "removed card \"" + item.name + "\" from list \"" + column.name + "\""
            if (item.removed)
                break
            item.removed = true
            column.numOfActiveItems--
            column.item_ids.splice(column.item_ids.indexOf(curr_op.body.cmd[1]), 1)

            for (var i in column.item_ids) {
                var curr_item = event.items[column.item_ids[i]]
                if (curr_item.position > event.items[curr_op.body.cmd[1]].position) {
                    curr_item.position--
                }
            }

            if (apply_on_ui)
                ui_remove_item(curr_op.body.cmd[1])
            break
        case Operation.ITEM_RENAME:
            if (!(curr_op.body.cmd[1] in event.items))
                break
            var item = event.items[curr_op.body.cmd[1]]
            historyMessage += "renamed card \"" + item.name + "\" of list \"" + event.columns[item.curr_column].name + "\" to \"" + curr_op.body.cmd[2] + "\""
            item.name = curr_op.body.cmd[2]

            if (apply_on_ui)
                ui_update_item_name(curr_op.body.cmd[1], curr_op.body.cmd[2])
            break
        case Operation.ITEM_MOVE:
            if (!(curr_op.body.cmd[1] in event.items))
                break
            if (!(curr_op.body.cmd[2] in event.columns))
                break

            var item = event.items[curr_op.body.cmd[1]]
            historyMessage += "moved card \"" + item.name + "\" of list \"" + event.columns[item.curr_column].name + "\" to list \"" + event.columns[curr_op.body.cmd[2]].name + "\""

            var old_column = event.columns[item.curr_column]
            var old_pos = item.position
            old_column.item_ids.splice(old_column.item_ids.indexOf(curr_op.body.cmd[1]), 1)
            old_column.numOfActiveItems--
            event.columns[curr_op.body.cmd[2]].numOfActiveItems++
            item.position = event.columns[curr_op.body.cmd[2]].numOfActiveItems
            event.columns[curr_op.body.cmd[2]].item_ids.push(curr_op.body.cmd[1])
            item.curr_column = curr_op.body.cmd[2].toString()
            for (var iid of old_column.item_ids) {
                let i = event.items[iid]
                if (i.position > old_pos) {
                    i.position--
                }
            }

            if (apply_on_ui)
                ui_update_item_move_to_column(curr_op.body.cmd[1], curr_op.body.cmd[2], item.position)
            break
        case Operation.ITEM_SET_DESCRIPTION:
            if (!(curr_op.body.cmd[1] in event.items))
                break
            var item = event.items[curr_op.body.cmd[1]]
            historyMessage += "changed description of card \"" + item.name + "\" of list \"" + event.columns[item.curr_column].name + "\" from \"" + item.description + "\" to \"" + curr_op.body.cmd[2] + "\""
            item.description = curr_op.body.cmd[2]

            if (apply_on_ui)
                ui_update_item_description(curr_op.body.cmd[1], curr_op.body.cmd[2])
            break
        case Operation.ITEM_POST_COMMENT:
            if (!(curr_op.body.cmd[1] in event.items))
                break

            var item = event.items[curr_op.body.cmd[1]]
            historyMessage += "posted \"" + curr_op.body.cmd[2] + "\" on card \"" + item.name + "\" of list \"" + event.columns[item.curr_column].name + "\""
            item.comments.push([curr_op.fid, curr_op.body.cmd[2]])

            if (apply_on_ui)
                ui_item_update_chat(curr_op.body.cmd[1])
            break
        case Operation.ITEM_ASSIGN:
            if (!(curr_op.body.cmd[1] in event.items))
                break
            if (!(curr_op.body.cmd[2] in tremola.contacts))
                break

            var item = event.items[curr_op.body.cmd[1]]
            historyMessage += "assigned \"" + tremola.contacts[curr_op.body.cmd[2]].alias + "\" to card \"" + item.name + "\" of list \"" + event.columns[item.curr_column].name + "\""
            if (item.assignees.indexOf(curr_op.body.cmd[2]) < 0)
                item.assignees.push(curr_op.body.cmd[2])

            if (apply_on_ui)
                ui_update_item_assignees(curr_op.body.cmd[1])
            break
        case Operation.ITEM_UNASSIGN:
            if (!(curr_op.body.cmd[1] in event.items))
                break
            if (!(curr_op.body.cmd[2] in tremola.contacts))
                break
            var item = event.items[curr_op.body.cmd[1]]
            historyMessage += "unassigned \"" + tremola.contacts[curr_op.body.cmd[2]].alias + "\" from card \"" + item.name + "\" of list \"" + event.columns[item.curr_column].name + "\""
            if (item.assignees.indexOf(curr_op.body.cmd[2]) >= 0)
                item.assignees.splice(item.assignees.indexOf(curr_op.body.cmd[2]), 1)

            if (apply_on_ui)
                ui_update_item_assignees(curr_op.body.cmd[1])
            break
        case Operation.ITEM_COLOR:
            if (!(curr_op.body.cmd[1] in event.items))
                break
            var item = event.items[curr_op.body.cmd[1]]
            historyMessage += "changed color of card \"" + item.name + "\" to " + curr_op.body.cmd[2]
            item.color = curr_op.body.cmd[2]

            if (apply_on_ui)
                ui_update_item_color(curr_op.body.cmd[1], curr_op.body.cmd[2])
            break
        case Operation.INVITE:
            historyMessage += "invited " + curr_op.body.cmd[1] + "."

            console.log("IDX: " + event.members.indexOf(curr_op.body.cmd[1]))
            console.log("INVITE USER: " + curr_op.body.cmd[1])
            console.log("PENDING: " + event.pendingInvitations)

            if (event.members.indexOf(curr_op.body.cmd[1]) < 0) {
                if (!(curr_op.body.cmd[1] in event.pendingInvitations))
                    event.pendingInvitations[curr_op.body.cmd[1]] = []
                console.log("PENDING: " + event.pendingInvitations)
                event.pendingInvitations[curr_op.body.cmd[1]].push(curr_op.key)
            }

            if (apply_on_ui)
                menu_invite_create_entry(curr_op.body.cmd[1])

            break
        case Operation.INVITE_ACCEPT:
            if (curr_op.fid in event.pendingInvitations) { // check if the invite accept operation is valid
                // check if one of the prevs of the accept message is actual a valid invitation
                if (event.pendingInvitations[curr_op.fid].filter(op => event.operations[curr_op.key].body.prev.includes(op)).length > 0) {
                    historyMessage += "accepted invitation"
                    delete event.pendingInvitations[curr_op.fid]
                    if (event.members.indexOf(curr_op.fid) < 0) { //should always be the case
                        event.members.push(curr_op.fid)
                        console.log("MEMBERS" + event.members)
                    }
                    if (curr_op.fid == myId)
                        event.subscribed = true
                    if (apply_on_ui) {
                        dpi_ui_update_event_title(bid)
                        menu_invite_create_entry(curr_op.fid)
                        if (curr_op.fid != myId)
                            launch_snackbar("A new user joined the scheduling event")
                    }
                    break
                }
            }
            console.log("WRONG INVITATION")
            break
        case Operation.INVITE_DECLINE:
            if (curr_op.fid in event.pendingInvitations) { // check if the invite accept operation is valid
                if (event.pendingInvitations[curr_op.fid].filter(op => event.operations[curr_op.key].body.prev.includes(op)).length > 0) {
                    historyMessage += "declined invitation"
                    delete event.pendingInvitations[curr_op.fid]
                    var idx = event.members.indexOf(curr_op.fid)
                    if (idx >= 0) { // should never be the case
                        event.members.splice(idx, 1)
                    }
                }
                if (apply_on_ui)
                    menu_invite_create_entry(curr_op.fid)
            }
            break
        case Operation.LEAVE:
            historyMessage += "left"
            var idx = event.members.indexOf(curr_op.fid)
            if (idx >= 0) {
                event.members.splice(idx, 1)
            }
            delete event.pendingInvitations[curr_op.fid]

            if (apply_on_ui) {
                dpi_ui_update_event_title(bid)
                menu_invite_create_entry(curr_op.fid)
                if (curr_op.fid != myId)
                    launch_snackbar("A user has left the scheduling event")
            }


            break
    }
    //historyMessage += ",  " + curr_op.key // debug
    event.history.push([curr_op.fid, historyMessage])
    persist()
}

function dpi_clear_event() { // removes all active columns from the event
    var event = tremola.event[curr_event]

    for (var i in event.columns) {
        dpi_removeColumn(curr_event, i)
    }
    closeOverlay()
}

/*
    Debug menu
*/

function dpi_ui_debug() {
    closeOverlay()
    document.getElementById('div:debugDPI').style.display = 'initial'
    document.getElementById('txt:debugDPI').value = dpi_debug_toDot()//JSON.stringify(tremola.event[curr_event])
    document.getElementById("overlay-bg").style.display = 'initial';
}

function dpi_debug_toDot() {
    var event = tremola.event[curr_event]
    var exportStr = "digraph {\n"
    exportStr += "  rankdir=RL;\n"
    exportStr += "  splines=true;\n"
    exportStr += "  subgraph dag {\n"
    exportStr += "    node[shape=Mrecord];\n"

    if (!(event.sortedOperations instanceof Timeline)) { // deserialize ScuttleSort-Timeline
        event.sortedOperations = Timeline.fromJSON(event.sortedOperations)
    }
    for (var sortNode of event.sortedOperations.linear) {
        exportStr += '    ' + '"' + sortNode.name + '"' + ' [label="hash=' + sortNode.name + '\\nop=' + tremola.event[curr_event].operations[sortNode.name].body.cmd + '\\nr=' + sortNode.rank + '\\nindx=' + sortNode.indx + '"]\n'
        for (var prev of sortNode.prev) {
            exportStr += '    "' + sortNode.name + '" -> "' + prev.name + '"\n'
        }
    }
    exportStr += "  }\n"
    exportStr += "  subgraph time {\n"
    exportStr += "    node[shape=plain];\n"
    exportStr += '   " t" -> " " [dir=back];\n'
    exportStr += "  }\n"
    exportStr += "}"

    return exportStr
}


function dpi_createAppointment(bid, date, time) {
    var event = tremola.event[bid];
    var data = {
        'bid': bid,
        'cmd': [OperationTest.APPOINTMENT_CREATE, date, time],
        'prev': event.curr_prev
    };
    dpi_event_send_to_backend(data);
}

function dpi_setAppointmentAttendance(bid, appointmentId, status) {
    var event = tremola.event[bid];
    var data = {
        'bid': bid,
        'cmd': [OperationTest.APPOINTMENT_SET_ATTENDANCE, appointmentId, status],
        'prev': event.curr_prev
    };
    dpi_event_send_to_backend(data);
}

function dpi_removeAppointment(bid, appointmentId) {
    var event = tremola.event[bid];
    var data = {
        'bid': bid,
        'cmd': [OperationTest.APPOINTMENT_REMOVE, appointmentId],
        'prev': event.curr_prev
    };
    dpi_event_send_to_backend(data);
}

function dpi_deleteAppointment(bid, appointmentId) {
    var event = tremola.event[bid];
    var data = {
        'bid': bid,
        'cmd': [OperationTest.APPOINTMENT_DELETE, appointmentId],
        'prev': event.curr_prev
    };
    dpi_event_send_to_backend(data);
}

function dpi_deleteEvent(bid) {
    var event = tremola.event[bid];
    var data = {
        'bid': bid,
        'cmd': [OperationTest.EVENT_DELETE],
        'prev': event.curr_prev
    };
    dpi_event_send_to_backend(data);
}