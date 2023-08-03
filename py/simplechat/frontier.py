#!/usr/bin/env python3

# frontier.py
# display local frontier state

# Jul 2023 <christian.tschudin@unibas.ch>

# ---------------------------------------------------------------------------

def bytes2hex(d):
    if type(d) == list:
        return [bytes2hex(x) for x in d]
    if type(d) == dict:
        return {bytes2hex(k):bytes2hex(v) for k,v in d.items()}
    if type(d) == bytes:
        return f'#{d.hex()}#'
    return d

def bytes2content(buf):
    if buf == None:
        return '||'
    try:
        buf = bipf.loads(buf)
        return f"bipf({bytes2hex(buf)})"
    except:
        return buf.decode('utf-8', 'replace')
    
if __name__ == '__main__':

    import argparse
    import os

    from simplepub import bipf, replica

    ap = argparse.ArgumentParser()
    ap.add_argument('-d', type=str, default='./data', metavar='DATAPATH',
                    help='path to persistency directory')
    ap.add_argument('-raw', action='store_true', default=False,
                    help='dump raw log entries and side chain packets')
    ap.add_argument('-stat', action="store_true", default=False,
                    help='only show stats (no content), default: False')
    args = ap.parse_args()

    keys = [ bytes.fromhex(fn) for fn in os.listdir(args.d)
             if len(fn) == 64 and os.path.isdir(args.d + '/' + fn)]
    keys.sort()

    cnt_entries = 0
    cnt_chunks = 0
    cnt_missing = 0
    missing = []

    for i in range(len(keys)):
        fid = keys[i];
        r = replica.Replica(args.d,fid,None)
        if not args.stat:
            print(f"* key {i}  {fid.hex()}")
        ms = r.state['max_seq']
        cnt_entries += ms
        if not args.stat:
            print(f"  max_seq = {ms}, prev = {r.state['prev'].hex()}")
        psc = r.state['pend_sc']
        for k in range(ms):
            e = r.get_entry_pkt(k+1)
            if e[7] == 1 and not k+1 in psc: # chain20 with full sidechain
                clen, sz = bipf.varint_decode(e[8:])
                clen -= 48 - 20 - sz
                if clen > 0:
                    cnt_chunks += (clen + 99) // 100
        if len(psc) != 0:
            for s,v in psc.items():
                missing.append(f"{i}.{s}.{v[0]}ff")
                cnt_chunks += v[0]
                cnt_missing += v[1]
            psc = {s:f"{v[0]}/{v[0]+v[1]}" for s,v in psc.items()}
            if not args.stat:
                print(f"  pend_sc = {psc}")
        if ms > 0 and not args.stat:
            for k in range(ms):
                a,l = r.get_content_len(k+1)
                print(f"  #{k+1}      "[:10] + f"           {a}/{l} "[-13:], end='')
                if args.raw:
                    print(r.read(k+1).hex())
                else:
                    print(bytes2content(r.read(k+1)))

    if not args.stat:
        print()
    print("Stats:")
    print(f"- {len(keys)} feeds")
    print(f"- {cnt_entries} available entries")
    print(f"- {cnt_chunks} available chunks")
    print(f"- {cnt_missing} missing chunks: {', '.join(missing)}")
# eof
