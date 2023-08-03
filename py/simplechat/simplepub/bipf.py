# 

# library for BIPF (Binary In-Place Format, https://github.com/ssbc/bipf)
# 2022-04-23 <christian.tschudin@unibas.ch>, MIT License

# comments were retained from above URL despite being JS-specific

__all__ = [
    'dumps',
    'loads',
    'TYPE_STRING',
    'TYPE_BYTES',
    'TYPE_INT',
    'TYPE_DOUBLE',
    'TYPE_LIST',
    'TYPE_DICT',
    'TYPE_BOOLNONE',
    'TYPE_RESERVED',
    'varint_decode',
    'varint_encode',
    'varint_encoding_length',
    'encode',
    'decode',
    'encodingLength',
    'getValueType',
    'getEncodedType',
    'generator',
    'seekKey',
    'seekPath',
]

from functools import reduce
from struct import pack, unpack

TYPE_STRING   = 0
TYPE_BYTES    = 1 # 001
TYPE_INT      = 2 # 010 // little endian, we write the minimal number of bytes
TYPE_DOUBLE   = 3 # 011 // little endian, 64bit IEEE float
TYPE_LIST     = 4 # 100
TYPE_DICT     = 5 # 101 // aka obj, map etc
TYPE_BOOLNONE = 6 # 110 // length==0 -> None, else one byte with 0 or 1
TYPE_RESERVED = 7 # 111

_TAG_SIZE = 3
_TAG_MASK = 7

# ----------------------------------------------------------------------

_type2valueType = {
    bool:  TYPE_BOOLNONE,
    bytes: TYPE_BYTES,
    dict:  TYPE_DICT,
    float: TYPE_DOUBLE,
    int:   TYPE_INT,
    list:  TYPE_LIST,
    str:   TYPE_STRING,
}

def _dictEncLen(x):
    return reduce(lambda a,b:a+b, [encodingLength(k)+encodingLength(v) \
                                                     for k,v in x.items()], 0)
def _listEncLen(x):
    return reduce(lambda a,b:a+b, [encodingLength(i) for i in x], 0)

def _intEncLen(x):
    if x < 0: x = -x - 1
    return 1 + x.bit_length()//8
                             
_type2encLen = {
    bool:  lambda x: 1,
    bytes: lambda x: len(x),
    dict: _dictEncLen,
    float: lambda x: 8,
    int:   _intEncLen,
    list:  _listEncLen,
    str:   lambda x: len(x),
}

def _enc_bool(val,buf,pos):
    buf[pos] = 1 if val else 0
    return 1

def _enc_bytes(val,buf,pos):
    sz = len(val)
    buf[pos:pos+sz] = val
    return sz

def _enc_dict(val,buf,pos):
    old = pos
    for k,v in val.items():
        pos += encode(k, buf, pos)
        pos += encode(v, buf, pos)
    return pos - old

def _enc_double(val,buf,pos):
    buf[pos:pos+8] = pack('<d', val)
    return 8

def _enc_int(val,buf,pos):
    sz = _intEncLen(val)
    buf[pos:pos+sz] = pack('<q', val)[:sz]
    return sz

def _enc_list(val,buf,pos):
    old = pos
    for i in val:
        pos += encode(i, buf, pos)
    return pos - old

def _enc_str(val,buf,pos):
    s = val.encode()
    sz = len(s)
    buf[pos:pos+sz] = s
    return sz

_type2encoder = {
    bool:  _enc_bool,
    bytes: _enc_bytes,
    dict:  _enc_dict,
    float: _enc_double,
    int:   _enc_int,
    list:  _enc_list,
    str:   _enc_str,
}

# ----------------------------------------------------------------------

def _dec_inner(tag, buf, pos):
    if tag == TYPE_BOOLNONE: # i.e., length is 0
        return None, 0
    t = tag & _TAG_MASK
    if t == TYPE_RESERVED:
        raise Exception('reserved type:value')
    return _type2decoder[t](buf, pos, tag >> _TAG_SIZE)

def _dec_int(buf, pos, lim): # little endian
    val = 0
    for i in range(0, 8*lim, 8):
        val |= buf[pos] << i
        pos += 1
    m = 1 << i+7
    return (val if val & m == 0 else val - (m << 1)), lim

def _dec_list(buf, pos, lim):
    old = pos
    lst = []
    end = pos + lim
    while pos < end:
        tag,sz = varint_decode(buf, pos)
        pos += sz
        i, sz = _dec_inner(tag, buf, pos)
        pos += sz
        lst.append(i)
    return lst, lim

def _dec_dict(buf, pos, lim):
    d = {}
    end = pos + lim
    while pos < end:
        tag,sz = varint_decode(buf, pos)
        pos += sz
        t = tag & _TAG_MASK 
        if t == TYPE_LIST or t == TYPE_DICT:
            raise Exception("key type can't be list or dict")
        key,sz = _dec_inner(tag, buf, pos)
        pos += sz
        tag,sz = varint_decode(buf, pos)
        pos += sz
        val,sz = _dec_inner(tag, buf, pos)
        pos += sz
        d[key] = val
    return d, lim

_type2decoder = {
    TYPE_BOOLNONE: lambda buf,pos,lim: (False if buf[pos] == 0 else True, 1),
    TYPE_BYTES:    lambda buf,pos,lim: (bytes(buf[pos:pos+lim]), lim),
    TYPE_DICT:     _dec_dict,
    TYPE_DOUBLE:   lambda buf,pos,lim: (unpack('<d',buf[pos:pos+8])[0], 8),
    TYPE_INT:      _dec_int,
    TYPE_LIST:     _dec_list,
    TYPE_STRING:   lambda buf,pos,lim: (bytes(buf[pos:pos+lim]).decode(), lim)
}

# ----------------------------------------------------------------------
# we provide our own version to avoid a dependency

def varint_decode(buf, pos=0):
    # return val,sz
    val = 0
    shift = 0
    old = pos - 1
    while True:
        b = buf[pos]
        val |= (b & 0x7f) << shift
        if (b & 0x80) == 0:
            return (val,pos-old)
        shift += 7
        pos += 1

def varint_decode_max(buf, pos, maxPos):
    val = 0
    shift = 0
    old = pos - 1
    while pos < maxPos:
        b = buf[pos]
        val |= (b & 0x7f) << shift
        if (b & 0x80) == 0:
            return (val,pos-old)
        shift += 7
        pos += 1
    
def varint_encoding_length(val):
    return 1 if val == 0 else (val.bit_length()+6) // 7

def varint_encode(val, buf, pos=0):
    old = pos - 1
    while True:
        buf[pos] = val & 0x7f
        val >>= 7
        if val:
            buf[pos] |= 0x80
        else:
            return pos - old
        pos += 1

def varint_encode_to_bytes(val):
    buf = bytearray(20)
    sz = varint_encode(val, buf)
    return bytes(buf[:sz])

# ----------------------------------------------------------------------

def dumps(val):
    buf = bytearray(encodingLength(val))
    encode(val, buf)
    return bytes(buf)

def loads(buf):
    return decode(buf)[0]

def encode(val, buf, pos=0):
    '''write value to buffer from start. returns the number of bytes used'''
    if val == None:
        buf[pos] = TYPE_BOOLNONE
        return 1
    sz  = _type2encLen[type(val)](val) << _TAG_SIZE
    sz |= _type2valueType[type(val)]
    sz1 = varint_encode(sz, buf, pos)
    return sz1 + _type2encoder[type(val)](val, buf, pos+sz1)

def decode(buf, pos=0):
    '''read the next value from buffer at start.
       returns a tuple with the value and the consumed bytes'''
    assert pos >= 0
    tag,sz1 = varint_decode(buf, pos)
    val,sz2 = _dec_inner(tag, memoryview(buf), pos + sz1)
    return val, sz1 + sz2

def encodingLength(val):
    '''returns the length needed to encode value'''
    if val == None:
        return 1
    sz = _type2encLen[type(val)](val)
    return varint_encoding_length(sz << _TAG_SIZE) + sz

def getValueType(val):
    '''returns the type tag that will be used to encode this type'''
    return TYPE_BOOLNONE if val == None else _type2valueType[type(val)]

def getEncodedType(buf, pos=0):
    '''get the type tag at start'''
    return buf[pos] & _TAG_MASK

def generator(buf, pos=0):
    '''a more pythonic version of iterate (see below):
       usage:   "for k,v in generator(buf): ..."
       returns a tuple with key and val memorviews,
       key is an integer in case of an array'''
    buf = memoryview(buf)
    tag,sz = varint_decode(buf, pos)
    pos += sz
    t = tag & _TAG_MASK
    end = pos + (tag >> _TAG_SIZE)
    if t == TYPE_DICT:
        while pos < end:
            tag,sz = varint_decode(buf, pos)
            sz += tag >> _TAG_SIZE
            key = buf[pos:pos+sz]
            pos += sz
            tag,sz = varint_decode(buf, pos)
            sz += tag >> _TAG_SIZE
            yield key, buf[pos:pos+sz]
            pos += sz
        return
    if t == TYPE_LIST:
        i = 0
        while pos < end:
            tag,sz = varint_decode(buf, pos)
            sz += tag >> _TAG_SIZE 
            yield i, buf[pos:pos+sz]
            pos += sz
            i += 1
        return
    return
    
def iterate(buf, fct, pos=0):
    '''If the field at start is an object or array, then iterate will call
       the fn with arguments fn(buffer, pointer, key) for each
       subfield. If the field at start is not an array or object, this
       returns -1. You can stop/abort the iteration by making fn
       return any truthy value.'''
    assert pos >= 0
    old = pos
    tag,sz = varint_decode(buf, pos)
    pos += sz
    t = tag & _TAG_MASK
    end = pos + (tag >> _TAG_SIZE)
    if t == TYPE_DICT:
        while pos < end:
            keystart = pos
            tag,sz = varint_decode(buf, pos)
            pos += sz + (tag >> _TAG_SIZE)
            if fct(buf, pos, keystart) != 0:
                return old
            tag,sz = varint_decode(buf, pos)
            pos += sz + (tag >> _TAG_SIZE)
        return old
    if t == TYPE_LIST:
        i = 0
        while pos < end:
            if fct(buf, pos, i) != 0:
                return old
            tag,sz = varint_decode(buf, pos)
            pos += sz + (tag >> _TAG_SIZE)
            i += 1
        return old
    return -1

def seekKey(buf, target, pos=0):
    '''seek for a key target within an object. If getEncodedType(buffer,
       start) !== types.object then will return -1. Otherwise, seekKey
       will iterate over the encoding object and return a pointer to
       where it starts.
       Since this defines a recursive encoding, a pointer to any valid
       sub-encoding is a valid start value.'''
    tag,sz = varint_decode(buf, pos)
    pos += sz
    t = tag & _TAG_MASK
    if t != TYPE_DICT:
        return -1
    if type(target) == str: # string key comparison requires utf8 encoding
        target = target.encode()
    end = pos + (tag >> _TAG_SIZE)
    while pos < end:
        tag, sz = varint_decode(buf, pos)
        pos += sz
        strlen = tag >> _TAG_SIZE
        t = tag & _TAG_MASK
        if t == TYPE_STRING and strlen == len(target) and \
                                       buf[pos:pos+strlen] == target:
            return pos+strlen
        pos += strlen
        tag,sz = varint_decode(buf, pos)
        pos += sz + (tag >> _TAG_SIZE)
    return -1

def seekPath(buf, target_lst, pos=0):
    '''The same as seekKey, except for a recursive path.
       path should be an array of node buffers, just holding the key
       values, not encoded as bipf.'''
    for i in range(0,len(target_lst)-1):
        pos = seekKey(buf, target_lst[i], pos)
        if pos < 0:
            return -1
    return seekKey(buf, target_lst[-1], pos)


''' (non-reentrant) hidden state, and JSism? --> not implemented

seekKeyCached (buffer, start, target) => pointer

Same as seekKey, but uses a cache to avoid re-seeking the pointers if
the same arguments have been provided in the past. However, target
must be a string, not a buffer.


createSeekPath(path) => seekPath(buffer, start)

compiles a javascript function that does a seekPath. this is
significantly faster than iterating over a javascript array and then
looking for each thing, because it will get optimized by the js
engine's jit compiler.
'''

# eof
