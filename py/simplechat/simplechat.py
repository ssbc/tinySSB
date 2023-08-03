#!/usr/bin/env python3

# simplechat.py
# TUI frontend for the text-and-voice app of tinySSB

# Aug 2023 <christian.tschudin@unibas.ch>

# ---------------------------------------------------------------------------

import os

import spub
from simplepub import bipf, replica

def shortname(buf):
    map = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567"
    long = 0
    buf += bytes(4)
    sn = ''
    while len(buf) >= 5:
        long = 0
        for i in range(5):
            long = (long << 8) | buf[i]
        buf = buf[5:]
        s1 = ''
        for i in range(8):
            s1 = map[long & 0x1f] + s1
            long >>= 5
        sn += s1
    return sn[:10]

def extract_tav(fid, buf):
    try:
        sn = shortname(fid[:10])
        c = bipf.loads( buf )
        if c[0] == 'TAV':
            t = c[1] if type(c[1]) == str else ''
            if c[2] != None:
                t = "<voice msg> " + t
            return (sn, t, int(c[3]))
    except:
        pass
    return None

def load_posts(dirname):
    posts = []

    keys = [ bytes.fromhex(fn) for fn in os.listdir(args.data)
             if len(fn) == 64 and os.path.isdir(args.data + '/' + fn)]
    for i in range(len(keys)):
        fid = keys[i];
        r = replica.Replica(args.data,fid,None)
        maxseq = r.state['max_seq']
        for k in range(maxseq):
            p = extract_tav(fid, r.read(k+1))
            if p:
                posts.append(p)
    posts.sort(key=lambda e: e[2])
    return posts

# ---------------------------------------------------------------------------

import asyncio
import curses
import time

class MyDisplay(): # Display):
    def __init__(self, stdscr, args):
        self.stdscr = stdscr
        self.args = args
        self.done = False
        
        self.input = ''
        self.input_offs = 0
        self.voffs = 0
        self.posts = load_posts(args.data)
        self.blocks = [] # formatted posts

        self.log_lines = []
        self.show_log = False

        self.stdscr.leaveok(True)
        curses.update_lines_cols()

        self.lstscr = curses.newwin(curses.LINES-4, curses.COLS-2, 1, 1)
        self.lstscr.scrollok(True)
        self.logscr = curses.newwin(curses.LINES-4, curses.COLS-2, 1, 1)
        self.logscr.scrollok(True)

        self.in1scr = curses.newwin(3, curses.COLS, curses.LINES - 3, 0)
        self.in1scr.addstr('>')
        self.in2scr = curses.newwin(1, curses.COLS-4, curses.LINES - 2, 3)

        self.status_len = 25
        self.status_str = ''

    async def run(self):
        curses.curs_set(1)
        self.stdscr.nodelay(True)
        self.make_display()

        while not self.done:
            try:
                char = self.stdscr.get_wch()
                if char == curses.ERR:
                    await asyncio.sleep(0.1)
                elif char == '\x1b': # ESC
                    self.show_log = not self.show_log
                    self.make_display()
            except curses.error:
                await asyncio.sleep(0.1)
                continue
            if char == curses.KEY_RESIZE:
                self.make_display()
            elif char == curses.KEY_UP:
                if self.voffs < len(self.posts)-1:
                    self.voffs += 1
                    self.make_display()
            elif char == curses.KEY_DOWN:
                if self.voffs > 0:
                    self.voffs -= 1
                    self.make_display()
            else:
                self.handle_char(char)

    def make_display(self):
        curses.update_lines_cols()

        self.stdscr.resize(curses.LINES,curses.COLS)
        self.stdscr.erase()
        self.stdscr.box()
        s = " tinySSB public chat "
        self.stdscr.addstr(0, (curses.COLS - len(s)) // 2, s)

        if self.show_log:
            self.lstscr.resize(curses.LINES-4, curses.COLS-2)
            self.lstscr.erase()
            for s in self.log_lines:
                self.logscr.scroll()
                self.logscr.addstr(curses.LINES-5, 1, s[:curses.COLS-4])
            self.logscr.redrawwin()
        else:
            self.lstscr.resize(curses.LINES-4, curses.COLS-2)
            self.lstscr.erase()
            self.lstscr.idlok(True)
            self.blocks = []
            for i in range(len(self.posts) - self.voffs):
                block = self.format_and_add_txt(self.posts[i], i+1)
                self.show_end_block(block)

        self.in1scr.resize(3, curses.COLS)
        self.in1scr.mvwin(curses.LINES - 3, 0)
        self.in1scr.box()
        s = " ESC=log/chat ^C=exit "
        self.in1scr.addstr(1, 1, '>')
        self.in1scr.addstr(2, curses.COLS-len(s)-2, s)
        self.in1scr.addstr(0, curses.COLS-len(args.data)-2, self.args.data)

        x = self.in2scr.getyx()[1]
        self.in2scr.resize(1, curses.COLS-4)
        self.in2scr.mvwin(curses.LINES - 2, 3)
        mx = self.in2scr.getmaxyx()[1] - 1
        if x >= mx:
            self.input_offs += mx - x + 1
            x = mx - 1
        self.in2scr.erase()
        self.in2scr.addstr(0,0,self.input[self.input_offs:self.input_offs+mx])
        self.in2scr.move(0,x)
        self.in2scr.redrawwin()

        self.stdscr.refresh()
        if self.show_log: self.logscr.refresh()
        else:             self.lstscr.refresh()
        self.in1scr.refresh()
        self.in2scr.refresh()
        
    def handle_char(self, c):
        x = self.in2scr.getyx()[1]
        mx = self.in2scr.getmaxyx()[1]
        txt = self.input[self.input_offs:]

        self.stdscr.addstr(0,5,f"{c}")
        if c == '\n': # newline
            self.in2scr.move(0, 0)
            self.in2scr.clrtoeol()
            self.in2scr.move(0, 0)
            txt = self.input
            self.input = ''
            self.input_offs = 0
            msg = ['TAV', txt, None, int(time.time())]
            buf = bytearray(bipf.encodingLength(msg))
            bipf.encode(msg, buf)
            r = spub.the_node.reps[args.pk] # our log
            seq = r.write(buf, args.signfct)
            if seq != None:
                self.incoming(args.pk, r.read(seq))
            return
        # argh, we have to implement our own line editing because
        # curses.textpad does not support UTF8
        if c == curses.KEY_BACKSPACE:
            c = '\x08'
        if type(c) == int:
            if c == curses.KEY_LEFT:
                c = '\x02' # ctrl-b
            elif c == curses.KEY_RIGHT:
                c = '\x06' # ctrl-f
            else:
                return
        if c == '\x01': # ctrl-a
            if self.input_offs > 0:
                self.input_offs = 0
                self.in2scr.addstr(0,0,self.input[:mx-1])
            self.in2scr.move(0,0)
        if c == '\x02': # ctrl-b
            if x > 0:
                self.in2scr.move(0,x-1)
            elif self.input_offs > 0:
                self.input_offs -= 1
                self.in2scr.addstr(0, 0,
                             self.input[self.input_offs:self.input_offs+mx-1])
                self.in2scr.move(0,0)
        elif c == '\x04': # ctrl-d
            self.input = self.input[:self.input_offs+x] + \
                         self.input[self.input_offs+x+1:]
            self.in2scr.addstr(0, x, txt[x+1:mx])
            self.in2scr.clrtoeol()
            self.in2scr.move(0, x)
        elif c == '\x05': # ctrl-e
            if len(txt) >= mx:
                self.input_offs = len(self.input) - mx + 1
                txt = self.input[self.input_offs:]
                self.in2scr.addstr(0, 0, txt)
            self.in2scr.move(0, len(txt))
        elif c == '\x06': # ctrl-f
            if x < len(txt):
                if x >= mx-1:
                    self.in2scr.addstr(0, 0, txt[1:mx])
                    self.in2scr.move(0, x)
                    self.input_offs += 1
                else:
                    self.in2scr.move(0,x+1)
        elif c == '\x08' or ord(c) == 127: # bs or delete-left
            if self.input_offs+x > 0:
                self.input = self.input[:self.input_offs+x-1] + \
                    self.input[self.input_offs+x:]
            if x > 0:
                self.in2scr.addstr(0, x-1, txt[x:mx])
                self.in2scr.clrtoeol()
                self.in2scr.move(0, x-1)
        elif ord(c) >= 32:
            self.input = self.input[:self.input_offs+x] + c + \
                         self.input[self.input_offs+x:]
            if len(txt) + len(c) >= mx:
                if x >= mx-2:
                    self.in2scr.addstr(0, 0, txt[len(c):mx-1] + ' ' * len(c))
                    self.in2scr.insstr(0, x-1, c)
                    self.in2scr.move(0, x)
                    self.input_offs += len(c)
                else:
                    self.in2scr.addstr(0, x, txt[x:mx-1-len(c)] + ' ' * len(c))
                    self.in2scr.insstr(0, x, c)
                    self.in2scr.move(0, x+len(c))
            else:
                self.in2scr.insstr(c)
                self.in2scr.move(0, x+len(c))
        self.in2scr.refresh()

    def post2block(self, txt): # word wrapping of paragraphs separated by '\n'
        r,c = self.lstscr.getmaxyx()
        c -= 2
        lines = []
        for para in txt.split('\n'):
            words = para.split(' ') # split paragraph in words
            row = ''
            while len(words) > 0:
                w = words[0]
                while True:
                    if len(row) != 0:
                        row += ' '
                    if len(w) >= c:
                        cnt = c - len(row)
                        row += w[:cnt]
                        w = w[cnt:]
                    if len(row) + len(w) <= c:
                        row += w;
                        if len(row) == c:
                            lines.append(row)
                            row = ''
                        break
                    row += ' ' * (c - len(row))
                    lines.append(row)
                    row = ''
                del words[0]
            if len(row) > 0:
                row += ' ' * (c - len(row))
                lines.append(row)
                row = ''
        if len(row) > 0:
            row += ' ' * (c - len(row))
            lines.append(row)
        return lines

    def incoming(self, fid, buf):
        p = extract_tav(fid, buf)
        if p == None:
            return
        self.posts.append(p)
        self.posts.sort(key=lambda e: e[2])
        self.make_display()
    
    def format_and_add_txt(self, post, nr):
        shortname,txt,ts = post
        _,c = self.lstscr.getmaxyx()

        t = time.strftime(' ⏲ %Y-%m-%d %H:%M:%S ', time.localtime(ts))
        txt = f"#{nr} [{shortname[:5]}-{shortname[5:]}]\n{txt}\n{'_' + ' ' * (c-len(t)-3)}{t}"
        block = self.post2block(txt)
        self.blocks.append(block)

        return block

    def show_end_block(self, block):
        r,c = self.lstscr.getmaxyx()
        
        self.lstscr.scroll(len(block)+1)

        self.lstscr.standout()
        for i in range(len(block)):
            self.lstscr.move(r - len(block) + i - 1, 1)
            self.lstscr.addstr(block[i])
        self.lstscr.standend()
        self.lstscr.redrawwin()
        
    def new_post(self, post):
        self.posts.append(post)
        block = self.format_and_add_txt(post, len(self.posts))
        if self.voffs == 0:
            self.show_end_block(block)
            self.lstscr.refresh()
        else:
            self.voffs = 0
            self.make_display()
    
    async def wheel(self):
        w = '|/-\\'
        pos = 0
        while True:
            self.stdscr.hline(0, curses.COLS - self.status_len - 2, curses.ACS_HLINE, self.status_len)
            self.stdscr.addstr(0, curses.COLS - 2 - len(self.status_str), self.status_str)

            self.stdscr.addstr(0,0,w[pos])
            self.stdscr.refresh()
            self.in2scr.refresh() # because of refocusing the cursor to this window
            pos = (pos+1) % len(w)

            await asyncio.sleep(.1)

    def status(self, txt):
        self.status_str = txt[:self.status_len]

    def log(self, txt):
        self.log_lines.append(txt)
        self.logscr.scroll()
        self.logscr.addstr(curses.LINES-5, 2, txt[:curses.COLS-4])
        if self.show_log:
            self.logscr.refresh()

    pass

async def display_main(stdscr, args):
    display = MyDisplay(stdscr, args)
    args.status_update = lambda s: display.status(s)
    args.log = lambda s: display.log(s)

    asyncio.create_task(spub.main(args, lambda fid,buf: display.incoming(fid,buf)))
    asyncio.create_task(display.wheel())
    await display.run()

def main(stdscr, args):
    try:
        return asyncio.run(display_main(stdscr, args))
    except KeyboardInterrupt:
        pass

# ---------------------------------------------------------------------------

if __name__ == "__main__":

    import argparse
    import json
    import sys

    import pure25519

    ap = argparse.ArgumentParser()
    ap.add_argument('-ble', action='store_true', default=False,
                    help='enable Bluetooth Low Energ (default: off)')
    ap.add_argument('-data', type=str, default='./data', metavar='DATAPATH',
                    help='path to persistency directory (default: ./data)')
    ap.add_argument('-id', type=str, default='~/.tinySSB', metavar='IDPATH',
                    help='path to tinySSB private directory (default: ~/.tinySSB)')
    ap.add_argument('-role', choices=['in','inout','out'], default='inout',
                    help='direction of data flow (default: inout)')
    ap.add_argument('uri_or_port', type=str, nargs='?',
                    default='ws://127.0.0.1:8080',
                    help='TCP port if responder, URI if intiator (default is ws://127.0.0.1:8080)')
    ap.add_argument('-v', action='store_true', default=False,
                    help='print i/o timestamps')
    args = ap.parse_args()
    if args.uri_or_port.isdigit():
        args.uri_or_port = int(args.uri_or_port)

    # access ed25519 secret:
    if args.id[0] == '~':
        args.id = os.environ['HOME'] + args.id[1:]
    os.makedirs(args.id, exist_ok=True)
    idfn = args.id + '/ed25519_id.bipf'
    if not os.path.exists(idfn):
        d = { 'seed' : os.urandom(32).hex() }
        with open(idfn, 'w') as f:
            json.dump(d, f)
    with open(idfn, 'r') as f:
        cfg = json.load(f)
    args.pk, args.sk = pure25519.publickey(bytes.fromhex(cfg['seed']))
    args.signfct = lambda msg: pure25519.sign(msg,args.sk)[:64]
    # make sure our feed exists:
    if not os.path.exists(args.data + '/' + args.pk.hex()):
        replica.Replica(args.data, args.pk, None) # just force the creation

    curses.wrapper(lambda scr: main(scr, args))

# eof
