#

# tinyssb/replica.py  -- inject and ingest tinySSB content
# 2023-07-08 <christian.tschudin@unibas.ch>

import hashlib
import os
import traceback

from . import bipf

PKTTYPE_plain48    = 0x00     # ed25519 signature, single packet with 48B
PKTTYPE_chain20    = 0x01     # ed25519 signature, start of hash sidechain

'''
PKTTYPE_ischild    = 0x02     # metafeed information, only in genesis block
PKTTYPE_iscontn    = 0x03     # metafeed information, only in genesis block
PKTTYPE_mkchild    = 0x04     # metafeed information
PKTTYPE_contdas    = 0x05     # metafeed information
PKTTYPE_acknldg    = 0x06     # proof of having seen some fid:seq:sig entry

# see end of this document for the payload content of types 0x02-0x05
'''

PFX = b'tinyssb-v0'


class Replica:

    def __init__(self, datapath, fid, verify_fct, is_author=False):
        self.path = datapath + '/' + fid.hex() + '/'
        self.log_fname = self.path + 'log.bin'
        self.fnt_fname = self.path + 'frontier.bin'
        self.tmp_fname = self.path + 'frontier.tmp'
        self.fid = fid
        self.verify_fct = verify_fct
        self.is_author = is_author
        
        if not os.path.isdir(self.path):
            os.mkdir(self.path)
        if not os.path.isfile(self.log_fname):
            open(self.log_fname, 'wb').close()
            self.state = {'pend_sc': {}} # pend. sidechains {seq:[cnr,remain,hptr,pos_to_write]}
            self._persist_frontier(0, 0, fid[:20])
        # keep frontier in memory
        with open(self.fnt_fname, 'rb') as f:
            buf = f.read()
        self.state = bipf.loads(buf)
        # print(f"  replica {fid.hex()} state:")
        # print(f"    {self.state}")
        while os.path.getsize(self.log_fname) > self.state['max_pos']:
            with open(self.log_fname, 'r+b') as f:
                pos = self.state['max_pos']
                f.seek(pos, os.SEEK_SET)
                pkt = f.read(120)
                seq = self.state['max_seq'] + 1
                nam = PFX + self.fid + seq.to_bytes(4,'big') + self.state['prev']
                dmx = hashlib.sha256(nam).digest()[:7]
                if self.is_author or dmx != pkt[:7]:
                    print('truncating log file')
                    f.seek(pos, os.SEEK_SET)
                    f.truncate()
                    break
                chunk_cnt = 0
                if pkt[7] == PKTTYPE_chain20:
                    content_len, sz = bipf.varint_decode(pkt, 8)
                    content_len -= 48 - 20 - sz
                    ptr = pkt[36:56]
                    chunk_cnt = (content_len + 99) // 100
                if chunk_cnt > 0:
                    self.state['pend_sc'][seq] = [0, chunk_cnt, ptr, pos + 120]
                while chunk_cnt > 0: # allocate sidechain space in the file
                    f.write(bytes(120))
                    chunk_cnt -= 1
                f.write(pos.to_bytes(4,'big'))
                pos = f.tell()
            self._persist_frontier(seq, pos,
                                   hashlib.sha256(nam + pkt).digest()[:20])

    def _persist_frontier(self, seq, pos, prev):
        self.state['max_seq'] = seq
        self.state['max_pos'] = pos
        self.state['prev'] = prev
        with open(self.tmp_fname, 'wb') as f:
            f.write(bipf.dumps(self.state))
        os.replace(self.tmp_fname, self.fnt_fname)
    
    # ----------------------------------------------------------------------
    # public methods:

    def ingest_entry_pkt(self, pkt, seq): # True/False
        assert len(pkt) == 120
        if seq != self.state['max_seq'] + 1:
            print("   R: wrong seq nr", seq, self.state['max_seq'] + 1)
            return False
        nam = PFX + self.fid + seq.to_bytes(4,'big') + self.state['prev']
        dmx = hashlib.sha256(nam).digest()[:7]
        if dmx != pkt[:7]:
            print("   R: wrong dmx", pkt[:7].hex(), dmx.hex())
            return False
        if not self.verify_fct(self.fid, pkt[56:], nam + pkt[:56]):
            print("   R: signature verify failed")
            return False
        chunk_cnt = 0
        if pkt[7] == PKTTYPE_chain20:
            content_len, sz = bipf.varint_decode(pkt, 8)
            content_len -= 48 - 20 - sz
            ptr = pkt[36:56]
            chunk_cnt = (content_len + 99) // 100
        log_entry = pkt + bytes(chunk_cnt * 120)
        log_entry += self.state['max_pos'].to_bytes(4, 'big')
        with open(self.log_fname, 'ab') as f:
            f.write(log_entry)
        if chunk_cnt > 0:
            self.state['pend_sc'][seq] = [0, chunk_cnt, ptr,
                                          self.state['max_pos'] + 120]
        pos = self.state['max_pos'] + len(log_entry)
        # print(f"   R: fid={self.fid[:10].hex()} max_seq={seq}, max_pos={pos}")
        self._persist_frontier(seq, pos,
                               hashlib.sha256(nam + pkt).digest()[:20])
        return True

    def ingest_chunk_pkt(self, pkt, seq): # True/False
        assert len(pkt) == 120
        try:
            pend = self.state['pend_sc'][seq] # [cnr, rem, hptr, pos]
            assert pend[2] == hashlib.sha256(pkt).digest()[:20]
            with open(self.log_fname, 'r+b') as f:
                f.seek(pend[3], os.SEEK_SET)
                f.write(pkt)
                pos = f.tell()
        except:
            return False
        if pend[1] <= 1: # chain is complete
            del self.state['pend_sc'][seq]
        else:
            pend[0] += 1
            pend[1] -= 1
            pend[2] = pkt[-20:]
            pend[3] = pos
            # print(f" ? new pending state {pend}")
        with open(self.tmp_fname, 'wb') as f:
            f.write(bipf.dumps(self.state))
        os.replace(self.tmp_fname, self.fnt_fname)
        return True

    def get_next_seq(self): # (next_seq, dmx)
        seq = self.state['max_seq']+1
        nam = PFX + self.fid + seq.to_bytes(4,'big') + self.state['prev']
        return (seq, hashlib.sha256(nam).digest()[:7])

    def get_open_chains(self, cursor=0): # {seq:[cnr,rem,hptr,pos]}
        return self.state['pend_sc']

    def get_entry_pkt(self, seq):
        try:
            assert seq >= 1 and seq <= self.state['max_seq']
            with open(self.log_fname, 'rb') as f:
                pos = os.path.getsize(self.log_fname)
                cnt = self.state['max_seq'] - seq + 1
                while cnt > 0:
                    f.seek(pos-4, os.SEEK_SET)
                    pos = int.from_bytes(f.read(4), byteorder='big')
                    cnt -= 1
                f.seek(pos, os.SEEK_SET)
                return f.read(120)
        except:
            return None

    def get_content_len(self, seq):
        pkt = self.get_entry_pkt(seq)
        if pkt == None:
            return None
        if pkt[7] == PKTTYPE_plain48:
            return (48,48)
        if pkt[7] == PKTTYPE_chain20:
            content_len, sz = bipf.varint_decode(pkt, 8)
            if not seq in self.state['pend_sc']:
                return (content_len, content_len)
            available = (48-20-sz) + 100 * self.state['pend_sc'][seq][0]
            return (available, content_len)
        return None

    def get_chunk_pkt(self, seq, cnr):
        try:
            assert seq >= 1 and seq <= self.state['max_seq']
            if seq in self.state['pend_sc']:
                if cnr >= self.state['pend_sc'][seq][0]:
                    return None
            with open(self.log_fname, 'rb') as f:
                pos = os.path.getsize(self.log_fname)
                cnt = self.state['max_seq'] - seq + 1
                while cnt > 0:
                    f.seek(pos-4, os.SEEK_SET)
                    lim = pos
                    pos = int.from_bytes(f.read(4), byteorder='big')
                    cnt -= 1
                pos += 120*(cnr+1)
                if pos > lim-120:
                    return None
                f.seek(pos, os.SEEK_SET)
                return f.read(120)
        except:
            return None

    def read(self, seq): #, offs=0, lim=0):
        if self.state['max_seq'] < seq or seq < 1:
            return None
        with open(self.log_fname, 'rb') as f:
            pos = os.path.getsize(self.log_fname)
            cnt = self.state['max_seq'] - seq + 1
            while cnt > 0:
                f.seek(pos-4, os.SEEK_SET)
                pos = int.from_bytes(f.read(4), byteorder='big')
                cnt -= 1
            f.seek(pos, os.SEEK_SET)
            pkt = f.read(120)
            if pkt[7] == PKTTYPE_plain48:
                return pkt[8:56]
            if pkt[7] != PKTTYPE_chain20:
                return None
            chain_len, sz = bipf.varint_decode(pkt[8:])
            content = pkt[8+sz:36]
            blocks = (chain_len - len(content) + 99) // 100
            while blocks > 0:
                pkt = f.read(120)
                content += pkt[:100]
                blocks -= 1
        return content[:chain_len]

    # ----------------------------------------------------------------------
    # the following is not needed for mere forwarding repos (pubs)
    
    def write48(self, content, sign_fct): # publish event, returns seq or None
        assert os.path.getsize(self.log_fname) == self.state['max_pos']
        if len(content) < 48:
            content + bytes(48 - len(content))
        else:
            content = content[:48]
        seq = self.state['max_seq'] + 1
        nam = PFX + self.fid + seq.to_bytes(4,'big') + \
              self.state['prev']
        dmx = hashlib.sha256(nam).digest()[:7]
        msg = dmx + bytes([PKTTYPE_plain48]) + content
        wire = msg + sign_fct(nam + msg)
        assert len(wire) == 120
        assert self.verify_fct(self.fid, wire[56:], nam + wire[:56])
        with open(self.log_fname, 'ab') as f:
            f.write(wire + self.state['max_pos'].to_bytes(4, 'big'))
        self._persist_frontier(seq, len(wire) + 4,
                               hashlib.sha256(nam + wire).digest()[:20])
        return seq

    def write(self, content, sign_fct): # publish event, returns seq or None
        assert os.path.getsize(self.log_fname) == self.state['max_pos']
        chunks = []
        seq = self.state['max_seq'] + 1
        sz = bipf.varint_encode_to_bytes(len(content))
        payload = sz + content[:28-len(sz)]
        if len(payload) != 28:
            payload += bytes(28 - len(payload))
        content = content[28-len(sz):]
        i = len(content) % 100
        if i > 0:
            content += bytes(100-i)
        ptr = bytes(20)
        while len(content) > 0:
            buf = content[-100:] + ptr
            chunks.append(buf)
            ptr = hashlib.sha256(buf).digest()[:20]
            content = content[:-100]
        chunks.reverse()
        payload += ptr
        nam = PFX + self.fid + seq.to_bytes(4,'big') + \
              self.state['prev']
        dmx = hashlib.sha256(nam).digest()[:7]
        msg = dmx + bytes([PKTTYPE_chain20]) + payload
        wire = msg + sign_fct(nam + msg)
        assert len(wire) == 120
        assert self.verify_fct(self.fid, wire[56:], nam + wire[:56])
        chunks.insert(0, wire)
        log_entry = b''.join(chunks)
        log_entry += self.state['max_pos'].to_bytes(4, 'big')
        with open(self.log_fname, 'ab') as f:
            f.write(log_entry)
        self._persist_frontier(seq, self.state['max_pos'] + len(log_entry),
                               hashlib.sha256(nam + wire).digest()[:20])
        return seq
    
    # ----------------------------------------------------------------------
    
    pass

# eof
