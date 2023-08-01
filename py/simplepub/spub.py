#!/usr/bin/env python3

# new_server.py
# tinySSB simplepub websocket server

import asyncio
import hashlib
import signal
import sys
import time
import traceback
import websockets

WS_PORT = 8080

import simplepub.node
import simplepub.ble

# ---------------------------------------------------------------------------

start_time = None

i_pkt_cnt = 0
o_pkt_cnt = 0

def nowstr():
    global start_time
    t = time.time()
    if start_time == None:
        start_time = t
    t -= start_time
    return f"{t:.03f}"

async def launch_adv(sock, get_adv_fct, args):
    global o_pkt_cnt
    while True:
        pkts, tout = get_adv_fct()
        for p in pkts:
            o_pkt_cnt += 1
            if args.v:
                print(f"{sock.nm}> {nowstr()} o={o_pkt_cnt:<4} {len(p):3}B 0x{p[:32].hex()}..")
            await sock.send(p)
        await asyncio.sleep(tout)
        
async def onConnect(sock, node, args):
    global i_pkt_cnt, o_pkt_cnt
    if args.v:
        print(f"-- <{sock.nm}> connection up")
    tasks = [ asyncio.create_task(launch_adv(sock,fct,args)) for fct in
              [ lambda: node.get_entry_adv(),
                lambda: node.get_chain_adv(),
                lambda: node.get_GOset_adv() ] ]

    pkt_cnt = 0
    while True:
        try:
            pkt = await sock.recv()
            i_pkt_cnt += 1
            if args.v:
                print(f"<{sock.nm} {nowstr()} i={i_pkt_cnt:<4} {len(pkt):3}B 0x{pkt[:20].hex()}.. h={hashlib.sha256(pkt).digest()[:10].hex()}..")
            for p in node.rx(pkt):
                o_pkt_cnt += 1
                if args.v:
                    print(f"{sock.nm}> {nowstr()} o={o_pkt_cnt:<4} {len(p):3}B 0x{p[:32].hex()}..")
                await sock.send(p)
            await asyncio.sleep(0)
        except (websockets.exceptions.ConnectionClosedOK,
                websockets.exceptions.ConnectionClosedError,
                simplepub.ble.ConnectionGone):
            break
        except Exception as e:
            traceback.print_exc()
            break
    for t in tasks:
        try:    t.cancel()
        except: pass
    if args.v:
        print(f"-- <{sock.nm}> connection down")

async def main(args):
    loop = asyncio.get_running_loop()
    stop = loop.create_future()
    loop.add_signal_handler(signal.SIGTERM, stop.set_result, None)

    node = simplepub.node.PubNode(args.d, args.role, args.v)

    try:
        if type(args.uri_or_port) == int:
            if args.ble:
                if simplepub.ble.is_installed:
                    asyncio.create_task(
                        simplepub.ble.serve(lambda s: onConnect(s, node, args)))
                else:
                    print(f"BLE interface not supported")
            print(f"Starting websocket responder on port {args.uri_or_port}")
            async with websockets.serve(lambda s: onConnect(s, node, args),
                                        "0.0.0.0", args.uri_or_port):
                await stop
        else:
            if args.ble:
                print(f"Ignoring BLE interface when connecting.")
            print(f"Connecting to {args.uri_or_port}")
            async with websockets.connect(args.uri_or_port) as wsock:
                wsock.nm = 'w'
                await onConnect(wsock, node, args)
    except (KeyboardInterrupt, asyncio.exceptions.CancelledError):
        pass

# ---------------------------------------------------------------------------

if __name__ == "__main__":
    import argparse

    ap = argparse.ArgumentParser()
    ap.add_argument('-ble', action='store_true', default=False,
                    help='enable Bluetooth Low Energ (default: off)')
    ap.add_argument('-data', type=str, default='./data', metavar='DATAPATH',
                    help='path to persistency directory (default: ./data)')
    ap.add_argument('-role', choices=['in','inout','out'], default='in',
                    help='direction of data flow (default: in)')
    ap.add_argument('uri_or_port', type=str, nargs='?',
                    default='ws://127.0.0.1:8080',
                    help='TCP port if responder, URI if intiator (default is ws://127.0.0.1:8080)')
    ap.add_argument('-v', action='store_true', default=False,
                    help='print i/o timestamps')
    
    args = ap.parse_args()
    if args.uri_or_port.isdigit():
        args.uri_or_port = int(args.uri_or_port)

    asyncio.run(main(args))

# eof
