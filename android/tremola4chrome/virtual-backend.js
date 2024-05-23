
// virtual-backend.js
// 2022-04-28 christian.tschudin@unibas.ch

'use strict'

var ether = new BroadcastChannel('log-exchange')
var myname, myseqno, vectorClock, inqueue, outqueue, offline

var IDs = {
    'Alice': '@AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=.ed25519',
    'Bob'  : '@CBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB=.ed25519',
    'Carol': '@ECCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC=.ed25519'
};
var frontEnd = null;

function setup(name) {
    myname = name
    myseqno = 0

    vectorClock = {}
    inqueue = []
    outqueue = []
    offline = false

    var hdr = document.createElement("h2")
    hdr.innerHTML= "<label id='rst' style='color: red;font-size: 100%;border: 2mm outset;'>&#x2A37;</label>&nbsp; " +
        "This is " + name +
        ", status=<label id='lbl' style='background-color: green; border: 2mm outset;padding: 3px;'>ONLINE</label> " +
        "(<div id='icnt' style='display: inline-block;'>0</div>" +
        "/<div id='ocnt' style='display: inline-block;'>0</div> queued)"

    var tre = document.createElement("div")
    tre.innerHTML = '<hr>\n' +
        '<iframe id=tre src="tremola.html?virtualBackend=true" style="width: 100%; height: 600;"></iframe> <hr>\n'

    var vec = document.createElement("p")
    vec.id = 'vec'

    /*
    var inp = document.createElement("input")
    inp.id = "inp"
    inp.type = "text"
    inp.placeholder = "value to broadcast"
    inp.addEventListener("keyup", evt => { if (evt.key == 'Enter') sendit() })

    var btn = document.createElement('button')
    btn.onclick = sendit
    btn.innerHTML = "SEND!"
    */
    
    var lst = document.createElement("ul")
    lst.id = 'lst'

    document.body.innerHTML = null
    document.body.style = "font-family: monospace;";
    [hdr, tre, vec, /* inp, btn, */ lst].forEach( e => {
        document.body.append(e)
    } )

    document.getElementById('rst').onclick =  evt => {
        ether.postMessage(null) // we defined this to restart all apps
        reset();
    }
    document.getElementById('lbl').onclick =  evt => { toggle() }

    for (var nm in IDs)
        vectorClock[IDs[nm]] = 0;
    ether.onmessage =   msg => {
        if (msg.data == null)  reset(myname);
        else if (!offline)     incoming(msg.data);
        else {
            inqueue.push(msg.data)
            document.getElementById('icnt').innerHTML = inqueue.length
        }
    }

    console.log('started as:', name)
}

function reset(name) {
    document.getElementById('vec').innerHTML = '';
    for (var nm in IDs)
        vectorClock[IDs[nm]] = 0;
    myseqno = 0;
    inqueue = []
    outqueue = []
    offline = false

    document.getElementById('lst').innerHTML = '';
    frontEnd.postMessage(['b2f', 'reset', IDs[myname]], '*');
    frontEnd.postMessage(['b2f', 'initialize', IDs[myname]], '*');
    add_peers();
}

/*
function sendit() {
    myseqno += 1
    var txt = document.getElementById("inp")
    var data = {fid: IDs[myname], seq: myseqno, val: txt.value}
    if (offline) {
        outqueue.push(data)
        document.getElementById('ocnt').innerHTML = outqueue.length
    } else {
        ether.postMessage(data)
        ether.onmessage({'data':data}) // send also to ourself
    }
    txt.value = null
}
*/

function broadcast(e) {
    if (offline) {
        outqueue.push(e)
        document.getElementById('ocnt').innerHTML = outqueue.length
    } else {
        ether.postMessage(e)
        ether.onmessage({'data':e}) // send also to ourself
    }
}

function incoming(data) {
    console.log("incoming", data);
    var hdr = data.header;
    if (vectorClock[hdr.fid] >= hdr.seq)
        alert(myname + ", did you forget to restart your client?");
    else if (hdr.seq != (vectorClock[hdr.fid] + 1))
        alert(myname + ", you should have started at the same time as the others!");
    vectorClock[hdr.fid] = hdr.seq;
    var v = {};
    var sender = null;
    for (var nm in IDs) {
        v[nm] = vectorClock[IDs[nm]];
        if (IDs[nm] == hdr.fid)
            sender = nm;
    }
    // console.log('v', v);
    document.getElementById('vec').innerHTML =
        "vectorClock = " + JSON.stringify(v)
        var li = document.createElement('li')
    li.innerHTML = (new Date()).toLocaleTimeString().split(' ')[0] + ' ' +
        JSON.stringify({'from':sender, 'seq': hdr.seq, 'app': data.public[0]});
    document.getElementById('lst').prepend(li)

    frontEnd.postMessage(['b2f', 'new_event', data], '*')
}

function toggle() {
    var lbl = document.getElementById('lbl')
    lbl.innerHTML = offline ? 'ONLINE' : 'OFFLINE'
    lbl.style = 'background-color: ' + (offline ? 'green;' : 'red;') +
                'border: 2mm outset;padding: 3px;'
    offline = offline == false
    if (!offline) {
        while (inqueue.length > 0) incoming(inqueue.shift())
        while (outqueue.length > 0) {
            var data = outqueue.shift()
            ether.postMessage(data)
            ether.onmessage({'data':data}) // send also to ourself
        }
        document.getElementById('icnt').innerHTML = 0
        document.getElementById('ocnt').innerHTML = 0
    }
}

// ----------------------------------------------------------------------


function we_can_start_now() {
    window.addEventListener('message', virtualBackend, false);
    frontEnd = window.frames['tre'].contentWindow;
    frontEnd.postMessage(['b2f', 'initialize', IDs[myname]], '*');
    add_peers();
}

function add_peers() {
    for (var nm in IDs) {
        if (nm != myname)
            frontEnd.postMessage(['b2f', 'new_contact', IDs[nm]], '*');
            // [IDs[nm], {'alias': nm}] ], '*');
    }
}

function virtualBackend(event) {
    console.log('virtBE1: ', event.data);
    if (event.data[0] != 'f2b')
        return;
    let cmd = event.data[1].split(' ');
    console.log('virtBE2: ', cmd);

    if (cmd[0] == 'wipe') {
        frontEnd.postMessage(['b2f', 'reset', IDs[myname]], '*');
        add_peers();
    }
    if (cmd[0] == 'exportSecret')
        event.source.postMessage(['b2f', 'exportSecret',
                                  'secret_of_id_which_is@AAAA==.ed25519'], '*');
    if (cmd[0] == 'publ:post') {
        console.log('cmd1 =', cmd)
        var draft = atob(cmd[2])
        // cmd.splice(0,2)
        myseqno += 1;
        var e = { 'header': {
	            'tst': Date.now(),
	            'ref': Math.floor(1000000*Math.random()),
	            'fid': IDs[myname],
	            'seq' : myseqno},
                  'public': ['TAV', draft, null, Math.floor(Date.now() / 1000)]
                }
        frontEnd.postMessage(['b2f', 'new_event', e], '*')
        broadcast(e);
    }
    if (cmd[0] == 'priv:post') {
        var draft = atob(cmd[1])
        cmd.splice(0,2)
        myseqno += 1;
        var e = { 'header': {
	            'tst': Date.now(),
	            'ref': Math.floor(1000000*Math.random()),
	            'fid': IDs[myname],
	            'seq' : myseqno},
                  'confid': {'type': 'post', 'text': draft, 'recps': cmd },
                  'public': {}
                }
        frontEnd.postMessage(['b2f', 'new_event', e], '*')
        broadcast(e);
    }
    if (cmd[0] == 'kanban') {
        console.log('kanban', cmd[1], cmd[4])
        // var operation = JSON.parse(atob(cmd[1]));
        // console.log(operation);
        // cmd.splice(0,2)
        cmd[0] = 'KAN';
        var prev = cmd[2] //== "null" ? null : cmdStr[2]
        if (prev != "null") {
            prev = atob(cmd[2])
            cmd[2] = prev.split(",").map(atob)
        }
        if (cmd[4] != "null") {
            var args = atob(cmd[4]).split(',').map(atob)
            cmd = cmd.splice(0,4).concat(args)
        }
        myseqno += 1;
        var e = { 'header': {
	            'tst': Date.now(),
	            'ref': Math.floor(1000000*Math.random()),
	            'fid': IDs[myname],
	            'seq': myseqno},
                  'public': cmd
                }
        frontEnd.postMessage(['b2f', 'new_event', e], '*')
        broadcast(e);
    }
    if (cmd[0] == 'iam') {
        console.log('iam', atob(cmd[1]));
        cmd[0] = 'IAM';
        cmd[1] = atob(cmd[1]);
        myseqno += 1;
        var e = { 'header': {
	            'tst': Date.now(),
	            'ref': Math.floor(1000000*Math.random()),
	            'fid': IDs[myname],
	            'seq': myseqno},
                  'public': cmd
                }
        frontEnd.postMessage(['b2f', 'new_event', e], '*')
        broadcast(e);
    }
}

// eof
