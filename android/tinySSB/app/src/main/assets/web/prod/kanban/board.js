//board.js

"use strict";

var curr_board;
var curr_context_menu;
var curr_column;
var curr_item;

var curr_rename_item;

// all available operations
const Operation = {
    BOARD_CREATE: 'board/create',
    BOARD_RENAME: 'board/rename',
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
    INVITE: 'invite',
    INVITE_ACCEPT: 'invite/accept',
    INVITE_DECLINE: 'invite/decline',
    LEAVE: 'leave'
}

const FLAG = {
    PERSONAL: 'personal'
}


function createBoard(name, flags) {
    var cmd = [Operation.BOARD_CREATE, name]
    if (flags != null)
        cmd = cmd.concat(flags)

    var data = {
        'bid': null,
        'cmd': cmd,
        'prev': null
    }
    board_send_to_backend(data)
}

function renameBoard(bid, name) {
    var board = tremola.board[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.BOARD_RENAME, name],
        'prev': board.curr_prev
    }
    board_send_to_backend(data)
}

function createColumn(bid, name) {
    console.log("create column")
    var board = tremola.board[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.COLUMN_CREATE, name],
        'prev': board.curr_prev
    }
    board_send_to_backend(data)
}

function createColumnItem(bid, cid, name) {
    var board = tremola.board[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.ITEM_CREATE, cid, name],
        'prev': board.curr_prev
    }
    board_send_to_backend(data)
}

function removeColumn(bid, cid) {
    var board = tremola.board[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.COLUMN_REMOVE, cid],
        'prev': board.curr_prev
    }
    board_send_to_backend(data)
}

function renameColumn(bid, cid, newName) {
    var board = tremola.board[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.COLUMN_RENAME, cid, newName],
        'prev': board.curr_prev
    }
    board_send_to_backend(data)
}

function removeItem(bid, iid) {
    var board = tremola.board[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.ITEM_REMOVE, iid],
        'prev': board.curr_prev
    }
    board_send_to_backend(data)
}

function renameItem(bid, iid, new_name) {
    var board = tremola.board[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.ITEM_RENAME, iid, new_name],
        'prev': board.curr_prev
    }
    board_send_to_backend(data)
}

function moveItem(bid, iid, new_cid) {
    var board = tremola.board[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.ITEM_MOVE, iid, new_cid],
        'prev': board.curr_prev
    }
    board_send_to_backend(data)
}

function setItemDescription(bid, iid, description) {
    var board = tremola.board[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.ITEM_SET_DESCRIPTION, iid, description],
        'prev': board.curr_prev
    }
    board_send_to_backend(data)
}

function postItemComment(bid, iid, comment) {
    var board = tremola.board[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.ITEM_POST_COMMENT, iid, comment],
        'prev': board.curr_prev
    }
    board_send_to_backend(data)
}

function assignToItem(bid, iid, assigned) {
    var board = tremola.board[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.ITEM_ASSIGN, iid, assigned],
        'prev': board.curr_prev
    }
    board_send_to_backend(data)
}

function unassignFromItem(bid, iid, unassign) {
    var board = tremola.board[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.ITEM_UNASSIGN, iid, unassign],
        'prev': board.curr_prev
    }
    board_send_to_backend(data)
}

function setItemColor(bid, iid, color) {
    var board = tremola.board[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.ITEM_COLOR, iid, color],
        'prev': board.curr_prev
    }
    board_send_to_backend(data)
}

function inviteUser(bid, userID) {
    var board = tremola.board[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.INVITE, userID],
        'prev': board.curr_prev
    }
    board_send_to_backend(data)
}

function inviteAccept(bid, prev) {
    var board = tremola.board[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.INVITE_ACCEPT],
        'prev': prev
    }
    board_send_to_backend(data)
}

function inviteDecline(bid, prev) {
    var board = tremola.board[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.INVITE_DECLINE],
        'prev': prev
    }
    board_send_to_backend(data)
}

function leave(bid) {
    var board = tremola.board[bid]
    var data = {
        'bid': bid,
        'cmd': [Operation.LEAVE],
        'prev': board.curr_prev
    }
    board_send_to_backend(data)
}


function board_send_to_backend(data) {
    var bid = data['bid'] != null ? data['bid'] : "null"
    var prevs = data['prev'] != null ? btoa(data['prev'].map(btoa)) : "null"
    var op = data['cmd'][0] //btoa(data['cmd'][0])
    var args = data['cmd'].length > 1 ? btoa(data['cmd'].slice(1).map(unicodeStringToTypedArray).map(btoa)) : "null"
    var to_backend = ['kanban', bid, prevs, op, args]
    backend(to_backend.join(" "))
}

function kanban_new_event(e) {
    // parse data
    var op = e.public[3]
    var bid = op == Operation.BOARD_CREATE ? e.header.ref : e.public[1]
    var prev = e.public[2] != "null" ? e.public[2] : [] // TODO: change to null instead of "null" if backend sends this field as Bipf.mkNone()
    var args = e.public.length > 4 ? e.public.slice(4) : []

    // add new entry if it is a new board
    if (!(bid in tremola.board)) {
        tremola.board[bid] = {
            "operations": {}, // all received operations for this board
            "sortedOperations": new Timeline(), // "linear timeline", sorted list of operationIds
            "members": [e.header.fid], // members of the board
            "forgotten": false, // flag for hiding this board from the board list
            "name": bid.toString().slice(0, 15) + '...', // name of the board
            "curr_prev": [], // prev pointer
            "columns": {},
            "items": {},
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

    var board = tremola.board[bid]

    if (op == Operation.BOARD_CREATE) {
        board.name = args[0]
        board.flags = args.slice(1)
        if (document.getElementById('kanban-invitations-overlay').style.display != 'none')
            if (document.getElementById("kanban_invitation_" + bid))
                menu_board_invitation_create_entry(bid)
        if (e.header.fid == myId)
            board.subscribed = true // the creator of the board is automatically subscribed
    }

    if (!(board.sortedOperations instanceof Timeline)) { // deserialize ScuttleSort-Timeline
        board.sortedOperations = Timeline.fromJSON(board.sortedOperations)
    }

    if (e.header.ref in board.operations)
        return


    // translation of the event format into the kanban board format
    var body = {
        'bid': bid,
        'cmd': [op].concat(args),
        'prev': prev
    }

    // store new event
    var p = {"key": e.header.ref, "fid": e.header.fid, "fid_seq": e.header.seq, "body": body, "when": e.header.tst};
    board["operations"][e.header.ref] = p;

    if (op == Operation.LEAVE && e.header.fid == myId) {
        delete board.pendingInvitations[myId]
        board.subscribed = false
        load_kanban_list()
    }

    if (board.subscribed) {
        console.log(`board add ${prev}`)
        board.sortedOperations.add(e.header.ref, prev)

        var independentOPs = [Operation.COLUMN_CREATE, Operation.ITEM_CREATE, Operation.COLUMN_REMOVE, Operation.ITEM_REMOVE, Operation.LEAVE] // these operations cannot be overwritten; their position in the linear timeline does not affect the resulting board

        //  Ui update + update optimization // board.operations[e.header.ref].indx == board.sortedOperations.length -1
        if (board.sortedOperations.name2p[e.header.ref].indx == board.sortedOperations.linear.length - 1 || independentOPs.indexOf(board.operations[e.header.ref].body.cmd[0]) >= 0) { //if the new event is inserted at the end of the linear timeline or the position is irrelevant for this operation
            if (curr_scenario == 'board' && curr_board == bid)
                apply_operation(bid, e.header.ref, true) // the board is currently displayed; additionally perform operation on UI
            else
                apply_operation(bid, e.header.ref, false)
        } else {
            console.log("DEBUG APPLYALL")
            apply_all_operations(bid)
        }

        board.curr_prev = board.sortedOperations.get_tips()
        board.lastUpdate = Date.now()

        if (curr_scenario != 'board' || curr_board != bid)
            board.unreadEvents++

        load_kanban_list()

        // invite selected users (during Kanban board creation)
        if (op == Operation.BOARD_CREATE && e.header.fid == myId) {
            var pendingInvites = []
            for (var m in tremola.contacts) {
                if (m != myId && document.getElementById(m).checked) {
                    inviteUser(bid, m)
                    console.log("Invited: " + m)
                }
            }
            if (curr_scenario == 'members')
                load_board(bid)
        }

        // creates Personal Board
        if (board.flags.includes(FLAG.PERSONAL) && !restream) {
            if (op == Operation.BOARD_CREATE && Object.values(board.columns).length == 0)
                createColumn(bid, 'Your Kanban Board')
            else if (Object.values(board.columns).length == 1 && Object.values(board.columns)[0].item_ids.length == 0)
                createColumnItem(bid, Object.values(board.columns)[0].id, 'Click me!')
            else if (Object.values(board.columns)[0].item_ids.length == 1 && board.items[Object.values(board.columns)[0].item_ids[0]].description == "") {
                setItemDescription(bid, Object.values(board.columns)[0].item_ids[0], "Use cards and lists to organize your projects")
                board.unreadEvents = 1
            }
        }
    } else {
        if (op == Operation.INVITE && body.cmd[1] == myId) { // received invitation to board
            if (myId in board.pendingInvitations)
                board.pendingInvitations[myId].push(e.header.ref)
            else {
                board.pendingInvitations[myId] = [e.header.ref]
                launch_snackbar('New invitation received')
                if (document.getElementById('kanban-invitations-overlay').style.display != 'none') {
                    menu_board_invitation_create_entry(bid)
                    console.log("create invite NAME:" + tremola.board['bid'].name)

                }

            }
        }

        if (op == Operation.INVITE_ACCEPT && e.header.fid == myId) { // invitation accepted -> start sorting all events
            board.subscribed = true
            board_reload(bid)
            board.lastUpdate = Date.now()
            board.unreadEvents++
            board.curr_prev = board.sortedOperations.get_tips()
            load_kanban_list()
            return
        }

        if (op == Operation.INVITE_DECLINE && e.header.fid == myId) {
            delete board.pendingInvitations[myId]
        }
    }

}

function reload_curr_board() {
    if (curr_board)
        board_reload(curr_board)
}

function board_reload(bid) {
    console.log("Board reload " + bid)
    var board = tremola.board[bid]
    board.columns = {}
    board.numOfActiveColumns = 0
    board.items = {}
    board.pendingOperations = {}
    board.pendingInvitations = {}
    board.members = []
    board.sortedOperations = new Timeline()

    for (var op in board.operations) {
        console.log("ADD op: " + op + ", prev:" + board.operations[op].body.prev)
        board.sortedOperations.add(op, board.operations[op].body.prev)
    }
    apply_all_operations(bid)

    if (curr_scenario == 'board' && curr_board == bid) {
        closeOverlay()
        curr_item = null
        curr_column = null
        curr_context_menu = null
        load_board(bid)
    }
}

/**
 * Creates a snapshot of the given kanban board. It applies all operations and updates the user interface
 *
 * @param {string} bid - Id of the kanban board
 */
function apply_all_operations(bid) {
    var board = tremola.board[bid]
    board.history = []

    var old_state = JSON.parse(JSON.stringify(board));

    //execute operations and save results to local storage
    var validOps = helper_linear_timeline_without_pending_prevs(board.sortedOperations)
    for (var i in validOps) {
        apply_operation(bid, validOps[i], false)
    }

    if (curr_scenario == 'board' && curr_board == bid) { // update ui
        ui_update_board(bid, old_state)
        console.log("UP CURR")
    }
}

// returns linear timeline that does not contain nodes which have only pending predecessors
function helper_linear_timeline_without_pending_prevs(timeline) {
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

function apply_operation(bid, operationID, apply_on_ui) {
    console.log("Apply:" + operationID)
    var board = tremola.board[bid]
    var curr_op = board['operations'][operationID]

    var author_name = tremola.contacts[curr_op.fid].alias
    var historyMessage = author_name + " "

    switch (curr_op.body.cmd[0]) {
        case Operation.BOARD_CREATE:
            historyMessage += "created the board \"" + curr_op.body.cmd[1] + "\""
            board.name = curr_op.body.cmd[1]
            if (board.members.indexOf(curr_op.fid) < 0)
                board.members.push(curr_op.fid)
            if (curr_op.fid == myId)
                board.subscribed = true
            /*if(board.members.indexOf(curr_op.fid) < 0)
              board.members.push(curr_op.fid)
            */
            break
        case Operation.BOARD_RENAME:
            historyMessage += "renamed the board \"" + board.name + "\" to \"" + curr_op.body.cmd[1] + "\""
            board.name = curr_op.body.cmd[1]

            if (apply_on_ui)
                ui_update_board_title(bid)
            break
        case Operation.COLUMN_CREATE:
            historyMessage += "created the list \"" + curr_op.body.cmd[1] + "\""
            var newPos = 0
            if (curr_op.key in board.columns) {
                if (board.columns[curr_op.key].removed)
                    break
                newPos = board.columns[curr_op.key].position
            } else
                newPos = ++board.numOfActiveColumns

            board.columns[curr_op.key] = {
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
            if (!(curr_op.body.cmd[1] in board.columns))
                return
            historyMessage += "removed list \"" + board.columns[curr_op.body.cmd[1]].name + "\""
            board.columns[curr_op.body.cmd[1]].removed = true

            for (var i in board.columns) {
                if (board.columns[i].removed)
                    continue

                if (board.columns[i].position > board.columns[curr_op.body.cmd[1]].position) {
                    --board.columns[i].position
                }

            }
            board.numOfActiveColumns--

            if (apply_on_ui)
                ui_remove_column(curr_op.body.cmd[1])
            break
        case Operation.COLUMN_RENAME:
            if (!(curr_op.body.cmd[1] in board.columns))
                break
            historyMessage += "renamed list \"" + board.columns[curr_op.body.cmd[1]].name + "\" to \"" + curr_op.body.cmd[2] + "\""
            board.columns[curr_op.body.cmd[1]].name = curr_op.body.cmd[2]

            if (apply_on_ui)
                ui_rename_column(curr_op.body.cmd[1], curr_op.body.cmd[2])
            break
        case Operation.ITEM_CREATE:
            if (!(curr_op.body.cmd[1] in board.columns))
                break
            historyMessage += "created a card in list \"" + board.columns[curr_op.body.cmd[1]].name + "\" with the name: \"" + curr_op.body.cmd[2] + "\""
            var newPos = 0
            if (curr_op.key in board.items) {
                if (board.items[curr_op.key].removed) {
                    break
                }
                newPos = board.items[curr_op.key].position //there is already a position assigned to the item
            } else {
                newPos = ++board.columns[curr_op.body.cmd[1]].numOfActiveItems
            }

            board.items[curr_op.key] = {
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
            board.columns[curr_op.body.cmd[1]].item_ids.push(curr_op.key.toString())

            if (apply_on_ui)
                load_item(curr_op.key)
            break
        case Operation.ITEM_REMOVE:
            if (!(curr_op.body.cmd[1] in board.items))
                break
            var item = board.items[curr_op.body.cmd[1]]
            var column = board.columns[item.curr_column]
            historyMessage += "removed card \"" + item.name + "\" from list \"" + column.name + "\""
            if (item.removed)
                break
            item.removed = true
            column.numOfActiveItems--
            column.item_ids.splice(column.item_ids.indexOf(curr_op.body.cmd[1]), 1)

            for (var i in column.item_ids) {
                var curr_item = board.items[column.item_ids[i]]
                if (curr_item.position > board.items[curr_op.body.cmd[1]].position) {
                    curr_item.position--
                }
            }

            if (apply_on_ui)
                ui_remove_item(curr_op.body.cmd[1])
            break
        case Operation.ITEM_RENAME:
            if (!(curr_op.body.cmd[1] in board.items))
                break
            var item = board.items[curr_op.body.cmd[1]]
            historyMessage += "renamed card \"" + item.name + "\" of list \"" + board.columns[item.curr_column].name + "\" to \"" + curr_op.body.cmd[2] + "\""
            item.name = curr_op.body.cmd[2]

            if (apply_on_ui)
                ui_update_item_name(curr_op.body.cmd[1], curr_op.body.cmd[2])
            break
        case Operation.ITEM_MOVE:
            if (!(curr_op.body.cmd[1] in board.items))
                break
            if (!(curr_op.body.cmd[2] in board.columns))
                break

            var item = board.items[curr_op.body.cmd[1]]
            historyMessage += "moved card \"" + item.name + "\" of list \"" + board.columns[item.curr_column].name + "\" to list \"" + board.columns[curr_op.body.cmd[2]].name + "\""

            var old_column = board.columns[item.curr_column]
            var old_pos = item.position
            old_column.item_ids.splice(old_column.item_ids.indexOf(curr_op.body.cmd[1]), 1)
            old_column.numOfActiveItems--
            board.columns[curr_op.body.cmd[2]].numOfActiveItems++
            item.position = board.columns[curr_op.body.cmd[2]].numOfActiveItems
            board.columns[curr_op.body.cmd[2]].item_ids.push(curr_op.body.cmd[1])
            item.curr_column = curr_op.body.cmd[2].toString()
            for (var iid of old_column.item_ids) {
                let i = board.items[iid]
                if (i.position > old_pos) {
                    i.position--
                }
            }

            if (apply_on_ui)
                ui_update_item_move_to_column(curr_op.body.cmd[1], curr_op.body.cmd[2], item.position)
            break
        case Operation.ITEM_SET_DESCRIPTION:
            if (!(curr_op.body.cmd[1] in board.items))
                break
            var item = board.items[curr_op.body.cmd[1]]
            historyMessage += "changed description of card \"" + item.name + "\" of list \"" + board.columns[item.curr_column].name + "\" from \"" + item.description + "\" to \"" + curr_op.body.cmd[2] + "\""
            item.description = curr_op.body.cmd[2]

            if (apply_on_ui)
                ui_update_item_description(curr_op.body.cmd[1], curr_op.body.cmd[2])
            break
        case Operation.ITEM_POST_COMMENT:
            if (!(curr_op.body.cmd[1] in board.items))
                break

            var item = board.items[curr_op.body.cmd[1]]
            historyMessage += "posted \"" + curr_op.body.cmd[2] + "\" on card \"" + item.name + "\" of list \"" + board.columns[item.curr_column].name + "\""
            item.comments.push([curr_op.fid, curr_op.body.cmd[2]])

            if (apply_on_ui)
                ui_item_update_chat(curr_op.body.cmd[1])
            break
        case Operation.ITEM_ASSIGN:
            if (!(curr_op.body.cmd[1] in board.items))
                break
            if (!(curr_op.body.cmd[2] in tremola.contacts))
                break

            var item = board.items[curr_op.body.cmd[1]]
            historyMessage += "assigned \"" + tremola.contacts[curr_op.body.cmd[2]].alias + "\" to card \"" + item.name + "\" of list \"" + board.columns[item.curr_column].name + "\""
            if (item.assignees.indexOf(curr_op.body.cmd[2]) < 0)
                item.assignees.push(curr_op.body.cmd[2])

            if (apply_on_ui)
                ui_update_item_assignees(curr_op.body.cmd[1])
            break
        case Operation.ITEM_UNASSIGN:
            if (!(curr_op.body.cmd[1] in board.items))
                break
            if (!(curr_op.body.cmd[2] in tremola.contacts))
                break
            var item = board.items[curr_op.body.cmd[1]]
            historyMessage += "unassigned \"" + tremola.contacts[curr_op.body.cmd[2]].alias + "\" from card \"" + item.name + "\" of list \"" + board.columns[item.curr_column].name + "\""
            if (item.assignees.indexOf(curr_op.body.cmd[2]) >= 0)
                item.assignees.splice(item.assignees.indexOf(curr_op.body.cmd[2]), 1)

            if (apply_on_ui)
                ui_update_item_assignees(curr_op.body.cmd[1])
            break
        case Operation.ITEM_COLOR:
            if (!(curr_op.body.cmd[1] in board.items))
                break
            var item = board.items[curr_op.body.cmd[1]]
            historyMessage += "changed color of card \"" + item.name + "\" to " + curr_op.body.cmd[2]
            item.color = curr_op.body.cmd[2]

            if (apply_on_ui)
                ui_update_item_color(curr_op.body.cmd[1], curr_op.body.cmd[2])
            break
        case Operation.INVITE:
            historyMessage += "invited " + curr_op.body.cmd[1] + "."

            console.log("IDX: " + board.members.indexOf(curr_op.body.cmd[1]))
            console.log("INVITE USER: " + curr_op.body.cmd[1])
            console.log("PENDING: " + board.pendingInvitations)

            if (board.members.indexOf(curr_op.body.cmd[1]) < 0) {
                if (!(curr_op.body.cmd[1] in board.pendingInvitations))
                    board.pendingInvitations[curr_op.body.cmd[1]] = []
                console.log("PENDING: " + board.pendingInvitations)
                board.pendingInvitations[curr_op.body.cmd[1]].push(curr_op.key)
            }

            if (apply_on_ui)
                menu_invite_create_entry(curr_op.body.cmd[1])

            break
        case Operation.INVITE_ACCEPT:
            if (curr_op.fid in board.pendingInvitations) { // check if the invite accept operation is valid
                // check if one of the prevs of the accept message is actual a valid invitation
                if (board.pendingInvitations[curr_op.fid].filter(op => board.operations[curr_op.key].body.prev.includes(op)).length > 0) {
                    historyMessage += "accepted invitation"
                    delete board.pendingInvitations[curr_op.fid]
                    if (board.members.indexOf(curr_op.fid) < 0) { //should always be the case
                        board.members.push(curr_op.fid)
                        console.log("MEMBERS" + board.members)
                    }
                    if (curr_op.fid == myId)
                        board.subscribed = true
                    if (apply_on_ui) {
                        ui_update_board_title(bid)
                        menu_invite_create_entry(curr_op.fid)
                        if (curr_op.fid != myId)
                            launch_snackbar("A new user joined the kanban board")
                    }
                    break
                }
            }
            console.log("WRONG INVITATION")
            break
        case Operation.INVITE_DECLINE:
            if (curr_op.fid in board.pendingInvitations) { // check if the invite accept operation is valid
                if (board.pendingInvitations[curr_op.fid].filter(op => board.operations[curr_op.key].body.prev.includes(op)).length > 0) {
                    historyMessage += "declined invitation"
                    delete board.pendingInvitations[curr_op.fid]
                    var idx = board.members.indexOf(curr_op.fid)
                    if (idx >= 0) { // should never be the case
                        board.members.splice(idx, 1)
                    }
                }
                if (apply_on_ui)
                    menu_invite_create_entry(curr_op.fid)
            }
            break
        case Operation.LEAVE:
            historyMessage += "left"
            var idx = board.members.indexOf(curr_op.fid)
            if (idx >= 0) {
                board.members.splice(idx, 1)
            }
            delete board.pendingInvitations[curr_op.fid]

            if (apply_on_ui) {
                ui_update_board_title(bid)
                menu_invite_create_entry(curr_op.fid)
                if (curr_op.fid != myId)
                    launch_snackbar("A user has left the kanban board")
            }


            break
    }
    //historyMessage += ",  " + curr_op.key // debug
    board.history.push([curr_op.fid, historyMessage])
    persist()
}

function clear_board() { // removes all active columns from the board
    var board = tremola.board[curr_board]

    for (var i in board.columns) {
        removeColumn(curr_board, i)
    }
    closeOverlay()
}

/*
    Debug menu
*/

function ui_debug() {
    closeOverlay()
    document.getElementById('div:debug').style.display = 'initial'
    document.getElementById('txt:debug').value = debug_toDot()//JSON.stringify(tremola.board[curr_board])
    document.getElementById("overlay-bg").style.display = 'initial';
}

function debug_toDot() {
    var board = tremola.board[curr_board]
    var exportStr = "digraph {\n"
    exportStr += "  rankdir=RL;\n"
    exportStr += "  splines=true;\n"
    exportStr += "  subgraph dag {\n"
    exportStr += "    node[shape=Mrecord];\n"

    if (!(board.sortedOperations instanceof Timeline)) { // deserialize ScuttleSort-Timeline
        board.sortedOperations = Timeline.fromJSON(board.sortedOperations)
    }
    for (var sortNode of board.sortedOperations.linear) {
        exportStr += '    ' + '"' + sortNode.name + '"' + ' [label="hash=' + sortNode.name + '\\nop=' + tremola.board[curr_board].operations[sortNode.name].body.cmd + '\\nr=' + sortNode.rank + '\\nindx=' + sortNode.indx + '"]\n'
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
