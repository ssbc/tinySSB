#

# simplepub/goset.py


import hashlib
from collections import deque

from . import node #NODE, LOGTYPE_remote
import time


FID_LEN = 32
GOSET_DMX_STR = "tinySSB-0.1 GOset 1"
DMX_LEN = 7


class Claim:
    typ = b'c'
    lo = bytes(0)  # FID_LEN
    hi = bytes(0)  # FID_LEN
    xo = bytes(0)  # FID_LEN
    sz = 0
    wire = bytes(0)


class Novelty:
    typ = b'n'
    key = bytes(FID_LEN)
    wire = bytes(0)


NOVELTY_LEN = 33 # sizeof(struct novelty_s)
CLAIM_LEN = 98 # sizeof(struct claim_s)
ZAP_LEN = 33 # sizeof(struct zap_s)

GOSET_KEY_LEN   = FID_LEN
GOSET_MAX_KEYS  =    100
GOSET_ROUND_LEN = 10
MAX_PENDING     =     20
ASK_PER_ROUND   =      1
HELP_PER_ROUND  =      3
ZAP_ROUND_LEN   =   4500


class GOset():

    def __init__(self, node, keys=[], verbose=False, log=None) -> None:
        def glob(x): pass
        self.node = node
        self.keys = sorted(keys)
        self.verbose = verbose
        self.log = log if log != None else glob
        self.state = bytes(FID_LEN) if self.keys == [] else \
                     self._xor(0, len(self.keys)-1)
        # self.node.set_want_dmx(self.state)

    goset_dmx = hashlib.sha256(GOSET_DMX_STR.encode()).digest()[:DMX_LEN]

    pending_claims = []
    pending_novelty = deque()
    largest_claim_span = 0

    def _key_to_ndx(self, key) -> int:
        try:
            return self.keys.index(key)
        except:
            return -1

    def incoming_goset_msg(self, dmx, pkt: bytes) -> list:
        lst = []
        if len(pkt) <= DMX_LEN:
            return []
        buf = pkt[DMX_LEN:]
        # print("   =G len:", len(buf), chr(buf[0]))

        if len(buf) == NOVELTY_LEN and buf[0] == ord('n'):
            key = buf[1:NOVELTY_LEN]
            if self.verbose:
                self.log(f"   =G novelty = {key.hex()}")
            self._add_key(key)
            return []
        
        if len(buf) != CLAIM_LEN or buf[0] != ord('c'):
            if self.verbose:
                self.log(f"   =G unknown msg {buf[:1]}")
            return []
        
        cl = self.mkClaim_from_bytes(buf)
        i1,i2 = self._key_to_ndx(cl.lo), self._key_to_ndx(cl.hi)
        if cl.sz == len(self.keys) and self.state == cl.xo:
            syn = "/synced"
        else:
            syn = "/not in sync"
        if self.verbose:
            self.log(f"   =G claim span={cl.sz} lo={i1}:{cl.lo[:10].hex()}.. hi={i2}:{cl.hi[:10].hex()}...  {syn}")

        if cl.sz > self.largest_claim_span:
            self.largest_claim_span = cl.sz
        if i2 - i1 == 1 and cl.sz == 3: # we just lack the key in the middle
            xor = bytearray(cl.xo)
            for i in range(len(xor)):
                xor[i] ^= cl.lo[i]
                xor[i] ^= cl.hi[i]
            if self.verbose:
                self.log(f"   computed the missing middle key {xor.hex()}")
            self._add_key(bytes(xor))
            # FIXME: should we update the state, or do it at ADV time?
        else:
            self._add_key(cl.lo)
            self._add_key(cl.hi)
            self._add_pending_claim(cl)
            # print("   add pending claim")
            # FIXME: should we update the state, or do it at ADV time?
        return []


    def get_adv(self) -> (list, int):
        lst = []
        if len(self.keys) == 0:
            return lst, 15
        cl = self.mkClaim(0, len(self.keys) - 1)
        if cl.xo != self.state:
            if self.verbose:
                self.log(f"   Gadv - new GOset state {cl.xo.hex()} |keys|={len(self.keys)}")
            self.state = cl.xo
            self.node.set_want_dmx(self.state)
        # if len(self.pending_claims) == 0:
        lst.append(self.goset_dmx + cl.wire)
        # return lst, 15

        self.pending_claims.sort(key=lambda x: x.sz)
        max_ask = ASK_PER_ROUND
        max_help = HELP_PER_ROUND

        retain = []
        for c in self.pending_claims:
            if c.sz == 0:
                continue
            lo = self._key_to_ndx(c.lo) # next((i for i, x in enumerate(self.keys) if x == c.lo), -1)
            hi = self._key_to_ndx(c.hi) # next((i for i, x in enumerate(self.keys) if x == c.hi), -1)
            if lo == -1 or hi == -1 or lo > hi:
                continue
            partial = self.mkClaim(lo,hi)
            if partial.xo == c.xo:
                continue
            if partial.sz <= c.sz:
                if max_ask > 0:
                    max_ask -= 1
                    if self.verbose:
                        self.log(f"   Gadv need {lo}..{hi} @{len(self.keys)}")
                    lst.append(self.goset_dmx + partial.wire)
                if partial.sz < c.sz:
                    retain.append(c)
                    continue

            if max_help > 0:
                max_help -= 1
                hi -= 1
                lo += 1
                if hi <= lo:
                    lst.append(self.goset_dmx + self.mkNovelty_from_key(self.keys[lo]).wire)
                elif hi - lo <= 2: # span of 2 or 3
                    if self.verbose:
                        self.log(f"   Gadv have {lo}..{hi} @{len(self.keys)}")
                    lst.append(self.goset_dmx + self.mkClaim(lo,hi).wire)
                else: # split span in two intervals
                    sz = (hi + 1 -lo) // 2
                    if self.verbose:
                        self.log(f"   Gadv have {lo}..{lo+sz-1} @{len(self.keys)}")
                        self.log(f"   Gadv have {lo+sz}..{hi} @{len(self.keys)}")
                    lst.append(self.goset_dmx + self.mkClaim(lo,lo+sz-1).wire)
                    lst.append(self.goset_dmx + self.mkClaim(lo+sz, hi).wire)
                continue
            retain.append(c)
        
        while len(retain) >= MAX_PENDING - 5:
            retain.removeLast()
        self.pending_claims = retain
        return lst, 15


    def _include_key(self, key: bytes) -> None:
        if key == bytes(GOSET_KEY_LEN):
            return False
        if key in self.keys:
            # print("GOset _include_key(): key already exists")
            return False
        if len(self.keys) >= GOSET_MAX_KEYS:
            # print("GOset _include_key(): too many keys")
            return False
        # print("GOset _include_key", key[:8])
        self.keys.append(key)
        return True
            

    def _add_key(self, key: bytes) -> None:
        if key == bytes(GOSET_KEY_LEN) or key in self.keys:
            return
        if len(self.keys) >= GOSET_MAX_KEYS:
            self.log("   too many keys")
            return
        # print(f"   new key {key.hex()}")
        self.keys.append(key)
        self.keys.sort()
        self.node.activate_feed(key)
        '''
        if len(self.keys) >= self.largest_claim_span:
            n = self.mkNovelty_from_key(key)
            if self.novelty_credit > 0:
                self._enqueue(n.wire, self.goset_dmx)
                self.novelty_credit -= 1
            elif len(self.pending_novelty) < MAX_PENDING:
                self.pending_novelty.append(n)
        '''
        self.log(f"   added key {key.hex()}")


    def _add_pending_claim(self, cl: Claim) -> None:
        for c in self.pending_claims:
            if c.sz == cl.sz and c.xo == cl.xo:
                return
        self.pending_claims.append(cl)
    
    '''
    def mkNovelty_from_key(self, key: bytes) -> Novelty:
        n = Novelty()
        n.wire = b'n' + key
        n.key = key
        return n
    '''
    

    def mkClaim_from_bytes(self, pkt: bytes) -> Claim:
        cl = Claim()
        cl.lo = pkt[1:33]
        cl.hi = pkt[33:65]
        cl.xo = pkt[65:97]
        cl.sz = pkt[97]
        cl.wire = pkt
        return cl
    

    def mkClaim(self, lo: int, hi: int) -> Claim:
        cl = Claim()
        cl.lo = self.keys[lo]
        cl.hi = self.keys[hi]
        cl.xo = self._xor(lo, hi)
        cl.sz = hi - lo + 1
        b = bytes([cl.sz])
        cl.wire = cl.typ + cl.lo + cl.hi + cl.xo + b
        return cl


    def _xor(self, lo: int, hi: int) -> bytes:
        if len(self.keys) == 0:
            return bytes(FID_LEN)
        xor = bytearray(self.keys[lo])
        for k in self.keys[lo+1 : hi+1]:
            for i in range(len(xor)):
                xor[i] ^= k[i]
        return xor


    '''
    def adjust_state(self) -> None:
        self.keys.sort()
        if len(self.keys) > 0:
            self.state = self.xor(0,len(self.keys)-1))
        else:
            self.state = bytes(FID_LEN)
        # print("GOset adjust_state() for", len(self.keys), "resulted in", self.state.hex())
        self.node.set_want_dmx(self.state)
    '''
