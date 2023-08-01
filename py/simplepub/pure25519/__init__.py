# __init__.py

import hashlib
import os

from .basic import (bytes_to_clamped_scalar,
                    bytes_to_scalar, scalar_to_bytes,
                    bytes_to_element, Base)
'''
Used only in ../tinyssb/keystore.py
__all__ = [
    'create_keypair',
    'SigningKey',
    'VerifyingKey',
    'BadSignatureError'
]
'''

# adapt pure25519/ed25519.py to behave like (C/glue) ed25519/_ed25519.py, so
# ed25519_oop.py doesn't have to change

# ed25519 secret/private/signing keys can be built from a 32-byte random seed
# (clamped and treated as a scalar). The public/verifying key is a 32-byte
# encoded group element. Signing requires both, so a common space/time
# tradeoff is to glue the two together and call the 64-byte combination the
# "secret key". Signatures are 64 bytes (one encoded group element and one
# encoded scalar). The NaCl code prefers to glue the signature to the
# message, rather than pass around detacted signatures.
#
# for clarity, we use the following notation:
#  seed: 32-byte secret random seed (unclamped)
#  sk: = seed
#  vk: 32-byte verifying key (encoded group element)
#  seed+vk: 64-byte glue thing (sometimes stored as secret key)
#  sig: 64-byte detached signature (R+S)
#  sig+msg: signature concatenated to message

# that glue provides:
#  SECRETKEYBYTES=64, PUBLICKEYBYTES=32, SIGNATUREBYTES=64
#  (vk,seed+vk)=publickey(seed)
#  sig+msg = sign(msg, seed+vk)
#  msg = open(sig+msg, vk) # or raise BadSignatureError

# pure25519/ed25519.py provides:
#  vk = publickey(sk)
#  sig = signature(msg, sk or sk+vk, vk)
#  bool = checkvalid(sig, msg, vk)

class BadSignatureError(Exception):
    pass

SECRETKEYBYTES = 64
PUBLICKEYBYTES = 32
SIGNATUREKEYBYTES = 64

def H(m):
    return hashlib.sha512(m).digest()

def Hint(m):
    h = H(m)
    return int.from_bytes(h, 'little')  # h[::-1].hex(),16)

def publickey(seed32):
    assert len(seed32) == 32
    a = bytes_to_clamped_scalar(H(seed32)[:32])
    A = Base.scalarmult(a)
    vk32 = A.to_bytes()
    return vk32, seed32 + vk32

def sign(msg, skvk):
    assert len(skvk) == 64
    sk = skvk[:32]
    vk = skvk[32:]
    h = H(sk[:32])
    a_bytes, inter = h[:32], h[32:]
    a = bytes_to_clamped_scalar(a_bytes)
    r = Hint(inter + msg)
    R = Base.scalarmult(r)
    R_bytes = R.to_bytes()
    S = r + Hint(R_bytes + vk + msg) * a
    sig = R_bytes + scalar_to_bytes(S)
    return sig + msg

def open(sigmsg, vk):
    assert len(vk) == 32
    sig = sigmsg[:64]
    msg = sigmsg[64:]
    try:
        R = bytes_to_element(sig[:32])
        A = bytes_to_element(vk)
        S = bytes_to_scalar(sig[32:])
        h = Hint(sig[:32] + vk + msg)
        v1 = Base.scalarmult(S)
        v2 = R.add(A.scalarmult(h))
    except ValueError as e:
        raise BadSignatureError(e)
    except Exception as e:
        if str(e) == "decoding point that is not on curve":
            raise BadSignatureError(e)
        raise
    if v1 != v2:
        raise BadSignatureError()
    return msg

# ed25519_oop.py ------------------------------------------------------------

# import os
# import base64
# from . import _ed25519
# BadSignatureError = _ed25519.BadSignatureError

def create_keypair(entropy=os.urandom):
    SEEDLEN = int(SECRETKEYBYTES / 2)
    assert SEEDLEN == 32
    seed = entropy(SEEDLEN)
    sk = SigningKey(seed)
    vk = sk.get_verifying_key()
    return sk, vk

class SigningKey(object):
    # this can only be used to reconstruct a key created by create_keypair().
    def __init__(self, sk_s):
        assert isinstance(sk_s, bytes)
        if len(sk_s) == 32:
            # create from seed
            vk_s, sk_s = publickey(sk_s)
        else:
            if len(sk_s) != 32 + 32:
                raise ValueError("SigningKey takes 32-byte seed or 64-byte string")
        self.sk_s = sk_s  # seed+pubkey
        self.vk_s = sk_s[32:]  # just pubkey

    def __eq__(self, them):
        if not isinstance(them, object): return False
        return (them.__class__ == self.__class__
                and them.sk_s == self.sk_s)

    def get_verifying_key(self):
        return VerifyingKey(self.vk_s)

    def sign(self, msg):
        assert isinstance(msg, bytes)
        sig_and_msg = sign(msg, self.sk_s)
        # the response is R+S+msg
        sig_R = sig_and_msg[0:32]
        sig_S = sig_and_msg[32:64]
        msg_out = sig_and_msg[64:]
        sig_out = sig_R + sig_S
        assert msg_out == msg
        return sig_out

class VerifyingKey(object):
    def __init__(self, vk_s):
        assert isinstance(vk_s, bytes)
        assert len(vk_s) == 32
        self.vk_s = vk_s

    def __eq__(self, them):
        if not isinstance(them, object): return False
        return (them.__class__ == self.__class__
                and them.vk_s == self.vk_s)

    def verify(self, sig, msg):
        assert isinstance(sig, bytes)
        assert isinstance(msg, bytes)
        assert len(sig) == 64
        sig_R = sig[:32]
        sig_S = sig[32:]
        sig_and_msg = sig_R + sig_S + msg
        # this might raise BadSignatureError
        msg2 = open(sig_and_msg, self.vk_s)
        assert msg2 == msg

__all__ = ['create_keypair', 'SigningKey', 'VerifyingKey', 'BadSignatureError']

'''
def selftest():
    message = b"crypto libraries should always test themselves at powerup"
    sk = SigningKey(b"priv0-VIsfn5OFGa09Un2MR6Hm7BQ5++xhcQskU2OGXG8jSJl4cWLZrRrVcSN2gVYMGtZT+3354J5jfmqAcuRSD9KIyg",
                    prefix="priv0-", encoding="base64")
    vk = VerifyingKey(b"pub0-eHFi2a0a1XEjdoFWDBrWU/t9+eCeY35qgHLkUg/SiMo",
                      prefix="pub0-", encoding="base64")
    assert sk.get_verifying_key() == vk
    sig = sk.sign(message, prefix="sig0-", encoding="base64")
    assert sig == b"sig0-E/QrwtSF52x8+q0l4ahA7eJbRKc777ClKNg217Q0z4fiYMCdmAOI+rTLVkiFhX6k3D+wQQfKdJYMxaTUFfv1DQ", sig
    vk.verify(sig, message, prefix="sig0-", encoding="base64")

selftest()
'''

# eof
