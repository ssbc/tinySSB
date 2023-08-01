#

# simplepub/node.py

import copy
import hashlib
import os
import time
import traceback

import pure25519
from . import bipf
from . import goset
from . import replica

DMX_LEN = 7
DMX_PFX = b'tinyssb-v0'

class PubNode:

    def __init__(self, datapath, role='in', verbose=False):
        self.start_time = time.time()
        print(f"Simplepub for directory {datapath}, role is '{role}'")
        self.datapath = datapath
        self.role = role
        self.verbose = verbose
        self.vf = lambda pk,sig,msg: pure25519.open(sig+msg,pk)
        self.reps  = { fid: replica.Replica(datapath,fid,self.vf) for fid in [
                    bytes.fromhex(fn) for fn in os.listdir(datapath)
                    if len(fn) == 64 and os.path.isdir(datapath + '/' + fn)] }
        self.chkt  = {}    # chunk filter bank
        self.dmxt  = {}    # DMX filter bank
        self.in_entry = lambda dmx, buf: self.incoming_entry(dmx, buf)
        self.in_chunk = lambda dmx, aux, buf: self.incoming_chunk(dmx, aux, buf)
        self.in_wnt = lambda dmx, buf: self.incoming_want_msg(dmx, buf)
        self.in_chk = lambda dmx, buf: self.incoming_chnk_msg(dmx, buf)
        self.want_dmx = None
        self.chnk_dmx = None
        self.goset = goset.GOset(self, self.reps.keys(), verbose)
        self.arm_dmx(self.goset.goset_dmx,
                     lambda dmx, buf: self.goset.incoming_goset_msg(dmx, buf),
                     None, 'GOset')
        self.set_want_dmx(self.goset.state)
        if len(self.reps) == 0:
            self.log_offs = 0
        else:
            self.log_offs = os.urandom(1)[0] % len(self.reps)
        if self.verbose:
            print("gset dmx", self.goset.goset_dmx.hex())
            print("want dmx", '-' if self.want_dmx == None else self.want_dmx.hex())
            print("chnk dmx", '-' if self.chnk_dmx == None else self.chnk_dmx.hex())
        if role != 'out': # 'in' or 'inout': listen to req
            for ndx in range(len(self.goset.keys)):
                fid = self.goset.keys[ndx]
                seq = self.reps[fid].state['max_seq'] + 1
                nam = fid + seq.to_bytes(4, 'big') + self.reps[fid].state['prev']
                dmx = self.compute_dmx(nam)
                self.arm_dmx(dmx, self.in_entry, (fid, seq), f"{ndx}.{seq}")
                for seq,p in self.reps[fid].state['pend_sc'].items():
                    # print(p)
                    cnr = p[0]
                    self.arm_chk(p[2], self.in_chunk, (fid,seq,cnr), f"{ndx}.{seq}.{cnr}")
        # print(f"len of chkt is {len(self.chkt)}")
        self.rtt = 4          # sec
        self.last_e_adv = 0   # timestamp
        self.incoming_cnt = 0 # either entry or chunk
                

    def get_entry_adv(self):
        if self.role == 'out':
            return [],4 # don't request stuff
        if self.incoming_cnt == 0:
            self.rtt *= 1.5
            if self.rtt > 4.0:
                self.rtt = 4.0
            self.last_e_adv = 0
        else:
            self.last_e_adv = time.time()
            self.incoming_cnt = 0
        print(f"   RTT {self.rtt} @{time.time() - self.start_time}")
        lst = [self.log_offs]
        enc_len = 0
        for i in range(len(self.goset.keys)):
            ndx = (self.log_offs + i) % len(self.goset.keys)
            fid = self.goset.keys[ndx]
            seq = self.reps[fid].state['max_seq'] + 1
            lst.append(seq)
            enc_len += bipf.encodingLength(seq)
            if enc_len > 100:
                break
        if len(self.goset.keys) > 0:
            self.log_offs = (self.log_offs + 1) % len(self.goset.keys)
        s = [(lst[0] + x)%len(self.goset.keys) for x in range(len(lst)-1)]
        s = ' '.join([ f"{s[i]}.{lst[1+i]}" for i in range(len(s))])
        print(f"   new w=[ {s} ]")
        if lst != []:
            lst = [self.want_dmx + bipf.dumps(lst)]
        return lst, self.rtt

    def get_chain_adv(self):
        if self.role == 'out': return [],4 # don't request stuff
        # FIXME: only send this out if we recently received new chunks
        # (don't be aggressive for dead sidechains)
        lst = []
        enc_len = 0
        for i in range(len(self.goset.keys)):
            ndx = (self.log_offs + i) % len(self.goset.keys)
            pend = self.reps[self.goset.keys[ndx]].state['pend_sc']
            for s,p in pend.items():
                t = [ndx,s,p[0]]
                lst.append(t)
                enc_len += bipf.encodingLength(t)
                if enc_len > 100:
                    break
            if enc_len > 100:
                break
        print(f"   new c=[ {' '.join(['.'.join([str(y) for y in x]) for x in lst])} ]")
        if lst != []:
            lst = [self.chnk_dmx + bipf.dumps(lst)]
        return lst, self.rtt

    def get_GOset_adv(self):
        return self.goset.get_adv()

    def rx(self, pkt) -> list:
        # print(f"<< incoming {pkt[:20].hex()}.. ({len(pkt)}B)")
        lst = []
        dmx = pkt[:7] # DMX_LEN = 7
        if dmx in self.dmxt:
            # if not dmx in [self.want_dmx,self.chnk_dmx,self.goset.goset_dmx]:
            #     print("  ", dmx.hex(), self.dmxt[dmx])
            lst += self.dmxt[dmx][0](dmx, pkt)
        # else:
        #     print("not in dmxt", [x.hex() for x in self.dmxt.keys()])
        hptr = hashlib.sha256(pkt).digest()[:20] # HASH_LEN = 20
        if hptr in self.chkt:
            plst = [x for x in self.chkt[hptr].items()]
            for a,v in plst:
                lst += v[0](hptr, a, pkt)
        # print("to send:", [x[:30].hex() for x in lst])
        return lst

    # -----------------------------------------------------------------

    def activate_feed(self, fid) -> None:
        if not fid in self.reps:
            self.reps[fid] = replica.Replica(self.datapath, fid, self.vf)
            # arm dmx for the activated feed
            seq = self.reps[fid].state['max_seq'] + 1
            nam = fid + seq.to_bytes(4, 'big') + self.reps[fid].state['prev']
            dmx = self.compute_dmx(nam)
            self.arm_dmx(dmx, self.in_entry, (fid, seq), f"{self.goset._key_to_ndx(fid)}.{seq}")

    def arm_dmx(self, dmx, fct=None, aux=None, comment=None):
        if fct == None:
            if dmx in self.dmxt:
                del self.dmxt[dmx]
        else:
            # print(f"   +dmx {dmx.hex()} / {comment}")
            self.dmxt[dmx] = (fct,aux,comment)

    def arm_chk(self, hptr, fct=None, aux=None, comment=None):
        if fct == None:
            if hptr in self.chkt:
                if aux in self.chkt[hptr]:
                    # nm = f"{self.goset._key_to_ndx(aux[0])}.{aux[1]}.{aux[2]}"
                    # print(f"   -chk {hptr.hex()}-{nm} / {self.chkt[hptr][aux][2]}")
                    del self.chkt[hptr][aux]
                if len(self.chkt[hptr]) == 0:
                    del self.chkt[hptr]
        else:
            # print(f"   +chk {hptr.hex()} / {comment}")
            if not hptr in self.chkt:
                self.chkt[hptr] = {}
            self.chkt[hptr][aux] = (fct,aux,comment)

    def compute_dmx(self, buf: bytes) -> bytes:
        return hashlib.sha256(DMX_PFX + buf).digest()[:DMX_LEN]

    def set_want_dmx(self, goset_state: bytes) -> None:
        if self.role != 'in': # listen to requests, erase old vals
            self.arm_dmx(self.want_dmx)
            self.arm_dmx(self.chnk_dmx)

        self.want_dmx = self.compute_dmx(b'want' + self.goset.state)
        self.chnk_dmx = self.compute_dmx(b'blob' + self.goset.state)
        # print("   set_want_dmx(): set new wnt to", self.want_dmx.hex())
        # print("                   set new chk to", self.chnk_dmx.hex())
        # if self.role == 'in': return   # don't listen to requests
        if self.role != 'in': # listen to requests, set new vals
            self.arm_dmx(self.want_dmx, self.in_wnt, None, 'WANT')
            self.arm_dmx(self.chnk_dmx, self.in_chk, None, 'CHNK')

    def adjust_RTT(self):
        if self.last_e_adv != 0 and self.incoming_cnt == 0:
            self.rtt = 0.5*self.rtt + 0.5*(time.time() - self.last_e_adv)
            if self.rtt < 0.01:
                self.rtt = 0.01
        self.incoming_cnt += 1
        
    def incoming_entry(self, dmx, buf): # dmx, fid, seq, buf):
        self.adjust_RTT()
        #global LAST_E_ADV, RTT, E_REPLY_CNT
        #E_REPLY_CNT += 1
        #if LAST_E_ADV != 0:
        #    # print(f"  -RTT-E {time.time()} {LAST_E_ADV} {time.time() - LAST_E_ADV}")
        #    RTT = 0.8*RTT + 0.2*(time.time() - LAST_E_ADV)
        #    if RTT < 0.1: RTT = 0.1
        fid, seq = self.dmxt[dmx][1]
        ndx = self.goset._key_to_ndx(fid)
        try:
            c = f" /{self.dmxt[dmx][2]}"
        except:
            c = ""
        # for d,c in self.dmxt.items():
        #     print("   - dmxt", d.hex(), c)
        rc = self.reps[fid].ingest_entry_pkt(buf, seq)
        if rc: # success
            if self.verbose:
                print(f"   ingested new entry dmx={dmx.hex()} {ndx}.{seq}{c}")
            self.arm_dmx(dmx)
            seq += 1
            nam = fid + seq.to_bytes(4, 'big') + self.reps[fid].state['prev']
            dmx = self.compute_dmx(nam)
            self.arm_dmx(dmx, self.in_entry, (fid, seq),
                         f"{ndx}.{seq}")
            # print(f"   pend_sc: {self.reps[fid].state['pend_sc']}")
            seq -= 1
            if seq in self.reps[fid].state['pend_sc']:
                p = self.reps[fid].state['pend_sc'][seq]
                cnr = p[0]
                self.arm_chk(p[2], self.in_chunk, (fid,seq,cnr),
                             f"{ndx}.{seq}.{cnr}")
        else:
            if self.verbose:
                print(f"   failed to ingest new entry dmx={dmx.hex()} {ndx}.{seq}{c}")
        # for dmx in self.dmxt:
        #     print(f"   dmxt {dmx.hex()} {self.dmxt[dmx][2]}")
        return []

    def incoming_chunk(self, hptr, aux, buf):
        self.adjust_RTT()
#        global LAST_C_ADV, RTT, C_REPLY_CNT
#        C_REPLY_CNT += 1
#        if LAST_C_ADV != 0:
#            # print(f"  -RTT-C {time.time()} {LAST_C_ADV} {time.time() - LAST_C_ADV}")
#            RTT = 0.8*RTT + 0.2*(time.time() - LAST_C_ADV)
#            if RTT < 0.1: RTT = 0.1
        fid, seq, cnr = self.chkt[hptr][aux][1]
        ndx = self.goset._key_to_ndx(fid)
        try:
            c = f" /{self.chkt[hptr][2]}"
        except:
            c = ""
        rc = self.reps[fid].ingest_chunk_pkt(buf, seq)
        if rc: # success
            if self.verbose:

                print(f"   ingested new chunk hptr={hptr.hex()} {ndx}.{seq}.{cnr}{c}")
            self.arm_chk(hptr, None, aux)
            if seq in self.reps[fid].state['pend_sc']:
                hptr = self.reps[fid].state['pend_sc'][seq][2]
                cnr += 1
                self.arm_chk(hptr, self.in_chunk, (fid,seq,cnr), f"{ndx}.{seq}.{cnr}")
            else:
                if self.verbose:
                    print(f"   chain {ndx}.{seq} closed")
        else:
            if self.verbose:
                print(f"   failed to ingest new chunk hptr={hptr.hex()} {ndx}.{seq}.{cnr}{c}")
        # cnt = 0
        # for hptr in self.chkt:
        #     cnt += len(self.chkt[hptr])
        #     print(f"   chkt {hptr.hex()} {[x[2] for x in self.chkt[hptr].values()]}")
        #psc = []
        #for fid in self.reps:
        #    for s,p in self.reps[fid].state['pend_sc'].items():
        #        psc.append( (self.goset._key_to_ndx(fid),s,p[0]) )
        #if len(psc) != cnt:
        #    print(f"!! mismatch {cnt}/{len(psc)} {psc}")

        return []

    def incoming_want_msg(self, dmx, buf) -> list:
        # print("   incoming WANT")
        want = bipf.loads(buf[DMX_LEN:])
        if not want or type(want) is not list:
            print("   error decoding WANT")
            return []
        if len(want) < 1 or type(want[0]) is not int:
            print("   error decoding WANT with offset")
            return []
        lst = []
        cnt = (len(want)-1) * [0]
        offs = want[0]
        credit = 3
        found_something = True
        while found_something:
            found_something = False
            for i in range(len(want)-1):
                try:
                    ndx = (offs + i) % len(self.goset.keys)
                    fid = self.goset.keys[ndx]
                    seq = want[i+1] + cnt[i]
                    pkt = self.reps[fid].get_entry_pkt(seq)
                    if pkt != None:
                        # print(f"   {ndx}.{seq} found")
                        lst.append(copy.copy(pkt))
                        cnt[i] += 1
                        credit -= 1
                        if credit <= 0:
                            found_something = False
                            break
                        found_something = True
                except Exception as e:
                    print("   error incoming WANT")
                    traceback.print_exc()

        v = "   =W ["
        for i in range(len(cnt)):
            ndx = (offs + i) % len(self.goset.keys)
            seq = want[i+1]
            v += f' {ndx}.{seq}' + cnt[i] * '*'
        v += " ]"
        if self.verbose:
            print(v, [x[:10].hex()+".." for x in lst])
        return lst

    def incoming_chnk_msg(self, dmx, buf) -> list:
        vect = bipf.loads(buf[DMX_LEN:])
        if vect == None or type(vect) != list:
            print("   error decoding CHNK")
            return []
        lst = []
        cnt = len(vect) * [0]
        credit = 3
        found_something = True
        while found_something:
            found_something = False
            for i in range(len(vect)):
                try:
                    fNDX, seq, cnr = vect[i]
                    fid = self.goset.keys[fNDX]
                    chunk = self.reps[fid].get_chunk_pkt(seq, cnr + cnt[i])
                except Exception as e:
                    print("   incoming CHNK error")
                    print(vect[i])
                    continue
                if chunk == None:
                    continue
                lst.append(chunk)
                cnt[i] += 1
                credit -= 1
                if credit <= 0:
                    found_something = False
                    break
                found_something = True
        v = "   =C ["
        for i in range(len(cnt)):
            fNDX, seq, cnr = vect[i]
            fid = self.goset.keys[fNDX]
            v += f" {fNDX}.{seq}.{cnr}" + cnt[i] * "*"
        v += " ]"
        if self.verbose:
            print(v, [x[:10].hex()+".." for x in lst])
        return lst
    
    
# eof
