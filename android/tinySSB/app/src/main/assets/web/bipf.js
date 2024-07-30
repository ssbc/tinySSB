/*

BIPF in JS and one file, replace Buffer with ArrayBuffer

collected from https://github.com/ssbc/bipf and
merged by <christian.tschudin@unibas.ch>, Jul 2024

*/

// --- constants

const BIPF_STRING = 0 // 000
const BIPF_BUFFER = 1 // 001
const BIPF_INT = 2 // 010 // 32bit int
const BIPF_DOUBLE = 3 // 011 // use next 8 bytes to encode 64bit float
const BIPF_ARRAY = 4 // 100
const BIPF_OBJECT = 5 // 101
const BIPF_BOOLNULL = 6 // 110 // and use the rest of the byte as true/false/null
const BIPF_RESERVED = 7 // 111
const BIPF_ALREADY_BIPF = 8
const BIPF_TAG_SIZE = 3
const BIPF_TAG_MASK = 7

const BIPF_types = {
  string: BIPF_STRING,
  buffer: BIPF_BUFFER,
  int: BIPF_INT,
  double: BIPF_DOUBLE,
  array: BIPF_ARRAY,
  object: BIPF_OBJECT,
  boolnull: BIPF_BOOLNULL,
  reserved: BIPF_RESERVED,
}

// --- fast_varint

const BIPF_MSB = 0x80
const BIPF_REST = 0x7f
const BIPF_MATH_POW_4 = Math.pow(2, 4*7)
const BIPF_MATH_POW_5 = Math.pow(2, 5*7)
const BIPF_MATH_POW_6 = Math.pow(2, 6*7)
const BIPF_MATH_POW_7 = Math.pow(2, 7*7)

function bipf_fvdecode(buf, offset) {
  offset = offset || 0

  buf = new Uint8Array(buf)
  let b = buf[offset]
  let res = 0

  res += b & BIPF_REST
  if (b < BIPF_MSB) {
    bipf_fvdecode.bytes = 1
    return res
  }

  b = buf[offset + 1]
  res += (b & BIPF_REST) << 7
  if (b < BIPF_MSB) {
    bipf_fvdecode.bytes = 2
    return res
  }

  b = buf[offset + 2]
  res += (b & BIPF_REST) << 14
  if (b < BIPF_MSB) {
    bipf_fvdecode.bytes = 3
    return res
  }

  b = buf[offset + 3]
  res += (b & BIPF_REST) << 21
  if (b < BIPF_MSB) {
    bipf_fvdecode.bytes = 4
    return res
  }

  b = buf[offset + 4]
  res += (b & BIPF_REST) * BIPF_MATH_POW_4
  if (b < BIPF_MSB) {
    bipf_fvdecode.bytes = 5
    return res
  }

  b = buf[offset + 5]
  res += (b & BIPF_REST) * BIPF_MATH_POW_5
  if (b < BIPF_MSB) {
    bipf_fvdecode.bytes = 6
    return res
  }

  b = buf[offset + 6]
  res += (b & BIPF_REST) * BIPF_MATH_POW_6
  if (b < BIPF_MSB) {
    bipf_fvdecode.bytes = 7
    return res
  }

  b = buf[offset + 7]
  res += (b & BIPF_REST) * BIPF_MATH_POW_7
  if (b < BIPF_MSB) {
    bipf_fvdecode.bytes = 8
    return res
  }

  bipf_fvdecode.bytes = 0
  throw new RangeError('Could not decode varint')
}

var BIPF_MSBALL = ~BIPF_REST
var BIPF_INT31 = Math.pow(2, 31)

function bipf_fvencode(num, out, offset) {
  if (Number.MAX_SAFE_INTEGER && num > Number.MAX_SAFE_INTEGER) {
    bipf_fvencode.bytes = 0
    throw new RangeError('Could not encode varint')
  }
  // out = out || []
  out = new Uint8Array(out)
  offset = offset || 0
  let oldOffset = offset

  while(num >= BIPF_INT31) {
    out[offset++] = (num & 0xFF) | BIPF_MSB
    num /= 128
  }
  while(num & BIPF_MSBALL) {
    out[offset++] = (num & 0xFF) | BIPF_MSB
    num >>>= 7
  }
  out[offset] = num | 0

  bipf_fvencode.bytes = offset - oldOffset + 1

  return out
}

var BIFP_N1 = Math.pow(2,  7)
var BIFP_N2 = Math.pow(2, 14)
var BIFP_N3 = Math.pow(2, 21)
var BIFP_N4 = Math.pow(2, 28)
var BIFP_N5 = Math.pow(2, 35)
var BIFP_N6 = Math.pow(2, 42)
var BIFP_N7 = Math.pow(2, 49)
var BIFP_N8 = Math.pow(2, 56)
var BIFP_N9 = Math.pow(2, 63)

function bipf_fvencodingLength (value) {
  if (value >= 0)
      return (
          value < BIFP_N1 ? 1
              : value < BIFP_N2 ? 2
                  : value < BIFP_N3 ? 3
                      : value < BIFP_N4 ? 4
                          : value < BIFP_N5 ? 5
                              : value < BIFP_N6 ? 6
                                  : value < BIFP_N7 ? 7
                                      : value < BIFP_N8 ? 8
                                          : value < BIFP_N9 ? 9
                                              :              10
      )
  return -1 // FIXME: handle negative numbers
}

// --- encode

//sets buffer, and returns length
const BIPF_encoders = [
  function String(string, buffer, start) {
    let t = new TextEncoder().encode(string)
    dst = new Uint8Array(buffer, start, t.length)
    dst.set(t);
    return t.length
  },
  function Buffer(b, buffer, start) {
    buffer.set(b,start)
    return b.length
  },
  function Integer(i, buffer, start) {
    let dv = new DataView(buffer);
    dv.setInt32(start, i, true);
    return 4
  },
  function Double(d, buffer, start) {
    let dv = new DataView(buffer);
    dv.setFloat64(start, d, true)
    return 8
  },
  function Array(a, buffer, start) {
    let p = start
    for (let i = 0; i < a.length; i++) {
      p += bipf_encode(a[i], buffer, p)
    }
    return p - start
  },
  function Object(o, buffer, start) {
    let p = start
    for (let k in o) {
      //TODO filter non json types
      p += bipf_encode(k, buffer, p)
      p += bipf_encode(o[k], buffer, p)
    }
    return p - start
  },
  function Boolean(b, buffer, start) {
    if (b !== null) buffer[start] = b === false ? 0 : b === true ? 1 : 2 // undefined
    return b === null ? 0 : 1
  },
]

const BIPF_encodingLengthers = [
  function String(string) {
    return (new TextEncoder().encode(string)).length
  },
  function Uint8Array(b) {
    return b.length
  },
  function Integer(i) {
    // return bipf_fvencodingLength(i)
    return 4
  },
  function Double(d) {
    return 8
  },
  function Array(a) {
    var bytes = 0
    for (var i = 0; i < a.length; i++) bytes += bipf_encodingLength(a[i])
    return bytes
  },
  function Object(o) {
    var bytes = 0
    for (var k in o) bytes += bipf_encodingLength(k) + bipf_encodingLength(o[k])
    return bytes
  },
  function boolnull(b, buffer, start) {
    return b === null ? 0 : 1 // encode null as zero length!
  },
]

function bipf_getType(value) {
  if ('string' === typeof value || value instanceof Date)
    return BIPF_STRING
  else if (value instanceof ArrayBuffer) {
    if (value._IS_BIPF_ENCODED) return BIPF_ALREADY_BIPF
    else return BIPF_BUFFER
  } else if (Number.isInteger(value) && Math.abs(value) <= 2147483647)
    return BIPF_INT
  else if ('number' === typeof value && Number.isFinite(value))
    //do not support Infinity or NaN (because JSON)
    return BIPF_DOUBLE
  else if (Array.isArray(value)) return BIPF_ARRAY
  else if (value && 'object' === typeof value) return BIPF_OBJECT
  else if ('boolean' === typeof value || null == value) return BIPF_BOOLNULL //boolean, null, undefined
}

function bipf_encodingLength(value) {
  const type = bipf_getType(value)
  if (type === void 0) throw new Error('unknown type: ' + JSON.stringify(value))
  if (type === BIPF_ALREADY_BIPF) return value.length
  const len = BIPF_encodingLengthers[type](value)
  return bipf_fvencodingLength(len << BIPF_TAG_SIZE) + len
}

function bipf_encode(value, buffer, start, _len) {
  start = start | 0
  const type = bipf_getType(value)
  if (type === void 0) throw new Error('unknown type: ' + JSON.stringify(value))
  if (type === BIPF_ALREADY_BIPF) {
    value.copy(buffer, start, 0, value.length)
    return value.length
  }
  const len = _len === undefined ? BIPF_encodingLengthers[type](value) : _len
  //  if(!buffer)
  //    buffer = Buffer.allocUnsafe(len)
  //throw new Error('buffer must be provided')
  bipf_fvencode((len << BIPF_TAG_SIZE) | type, buffer, start)
  const bytes = bipf_fvencode.bytes
  return BIPF_encoders[type](value, buffer, start + bytes) + bytes
}

function bipf_encodeIdempotent(value, buffer, start) {
  const len = bipf_encode(value, buffer, start)
  buffer._IS_BIPF_ENCODED = true
  return len
}

function bipf_markIdempotent(buffer) {
  buffer._IS_BIPF_ENCODED = true
  return buffer
}

function bipf_isIdempotent(buffer) {
  return !!buffer._IS_BIPF_ENCODED
}

function bipf_getEncodedLength(buffer, start) {
  return bipf_fvdecode(buffer, start) >> BIPF_TAG_SIZE
}

function bipf_getEncodedType(buffer, start) {
  return bipf_fvdecode(buffer, start) & BIPF_TAG_MASK
}

function bipf_allocAndEncode(value) {
  const len = bipf_encodingLength(value)
  const buffer = Buffer.allocUnsafe(len)
  bipf_encode(value, buffer, 0)
  return buffer
}

function bipf_allocAndEncodeIdempotent(value) {
  const len = bipf_encodingLength(value)
  const buffer = Buffer.allocUnsafe(len)
  bipf_encodeIdempotent(value, buffer, 0)
  return buffer
}


// --- decode

function bipf_decodeString(buffer, start, length) {
  return new TextDecoder().decode(new Uint8Array(buffer, start, length))
}

function bipf_decodeBuffer(buffer, start, length) {
  return buffer.slice(start, start + length)
}

function bipf_decodeInteger(buf, start, length) {
  let dv = new DataView(buf);
  return dv.getInt32(start, true); //TODO: encode in minimum bytes
}

function bipf_decodeDouble(buffer, start, length) {
  let dv = new DataView(buffer);
  return dv.getFloat64(start, true) //TODO: encode in minimum bytes
}

function bipf_decodeArray(buffer, start, length) {
  const a = []
  for (let c = 0; c < length; ) {
    const tag = bipf_fvdecode(buffer, start + c)
    const type = tag & BIPF_TAG_MASK
    if (type === 7) throw new Error('reserved type')
    const len = tag >> BIPF_TAG_SIZE
    c += bipf_fvdecode.bytes
    const value = bipf_decodeType(type, buffer, start + c, len)
    a.push(value)
    c += len
  }
  return a
}

function bipf_decodeObject(buffer, start, length) {
  const o = {}
  for (let c = 0; c < length; ) {
    const tag = bipf_fvdecode(buffer, start + c)
    // JavaScript only allows string-valued and Symbol keys for objects
    if (tag & BIPF_TAG_MASK) throw new Error('required type:string')
    const len = tag >> BIPF_TAG_SIZE
    c += bipf_fvdecode.bytes
    const key = bipf_decodeString(buffer, start + c, len)
    c += len

    const tag2 = bipf_fvdecode(buffer, start + c)
    const type2 = tag2 & BIPF_TAG_MASK
    if (type2 === 7) throw new Error('reserved type:value')
    const len2 = tag2 >> BIPF_TAG_SIZE
    c += bipf_fvdecode.bytes
    const value = bipf_decodeType(type2, buffer, start + c, len2)
    c += len2
    o[key] = value
  }
  return o
}

function bipf_decodeBoolnull(buffer, start, length) {
  if (length === 0) return null
  if (buffer[start] > 2) throw new Error('invalid boolnull')
  if (length > 1) throw new Error('invalid boolnull, length must = 1')
  return buffer[start] === 0 ? false : buffer[start] === 1 ? true : undefined
}

function bipf_decodeType(type, buffer, start, len) {
  switch (type) {
    case BIPF_STRING:
      return bipf_decodeString(buffer, start, len)
    case BIPF_BUFFER:
      return bipf_decodeBuffer(buffer, start, len)
    case BIPF_INT:
      return bipf_decodeInteger(buffer, start, len)
    case BIPF_DOUBLE:
      return bipf_decodeDouble(buffer, start, len)
    case BIPF_ARRAY:
      return bipf_decodeArray(buffer, start, len)
    case BIPF_OBJECT:
      return bipf_decodeObject(buffer, start, len)
    case BIPF_BOOLNULL:
      return bipf_decodeBoolnull(buffer, start, len)
    default:
      throw new Error('unable to decode type=' + type + ' ' + buffer)
  }
}

function bipf_decode(buffer, start) {
  start = start | 0
  const tag = bipf_fvdecode(buffer, start)
  const type = tag & BIPF_TAG_MASK
  const len = tag >> BIPF_TAG_SIZE
  const bytes = bipf_fvdecode.bytes
  start += bytes
  const value = bipf_decodeType(type, buffer, start, len)
  bipf_decode.bytes = len + bytes
  return value
}

/*
// --- test
//
const isDeepEqual = (object1, object2) => {

  const objKeys1 = Object.keys(object1);
  const objKeys2 = Object.keys(object2);

  if (objKeys1.length !== objKeys2.length) return false;

  for (var key of objKeys1) {
    const value1 = object1[key];
    const value2 = object2[key];

    const isObjects = isObject(value1) && isObject(value2);

    if ((isObjects && !isDeepEqual(value1, value2)) ||
      (!isObjects && value1 !== value2)
    ) {
      return false;
    }
  }
  return true;
}

const isObject = (object) => {
  return object != null && typeof object === "object";
}

function testEncodeDecode(value) {
    const buf = new ArrayBuffer(bipf_encodingLength(value))
    const len = bipf_encode(value, buf, 0)
    console.log('in:', JSON.stringify(value), '--> encoded:', buf.slice(0, len))
    //''+jsonString to get 'undefined' string.
    // const jsonLen = Buffer.byteLength('' + JSON.stringify(value))
    // console.log('length:', len, 'JSON-length:', jsonLen)
    // if (len > jsonLen)
    //     console.log('WARNING: binary encoding longer than json for:', value)
    if (len === 1) {
        const rest = buf[0] >> 3
        if (rest != 0)
            console.log('ERROR: single byte encodings must have zero length in tag')
    }
    if (value == null || value == undefined) {
        if (bipf_decode(buf, 0) != value)
            console.log('ERROR for', JSON.stringify(value), 'decode fails 0')
    } else {
        let d = bipf_decode(buf, 0)
        if (JSON.stringify(value) != JSON.stringify(d))
            console.log('  ** not equal:', JSON.stringify(value), JSON.stringify(d))
        console.log('ide', d)
        // if (!isDeepEqual(d, value))
        //     console.log('ERROR for', JSON.stringify(value), 'decode fails 1')
        // if (!isDeepEqual(bipf_decode(buf.slice(0,len), 0), value))
        //     console.log('ERROR for', JSON.stringify(value), 'decode fails 2')
    }
}

pkg = {
  name: 'test',
  version: '1.0.0',
  description: 'test package',
  repository: {
    type: 'git',
    url: 'git://github.com/ssbc/bipf.git',
  },
  dependencies: {
    varint: '^5.0.0',
  },
  husky: {
    hooks: {
      'pre-commit': 'npm run format-code-staged',
    },
  },
}

testEncodeDecode(100)
testEncodeDecode(0)
testEncodeDecode(1)
testEncodeDecode(-1)
testEncodeDecode(-100)
testEncodeDecode(123.456)
testEncodeDecode(2147483647)
testEncodeDecode(2200000000)
testEncodeDecode(-123.456)

testEncodeDecode(true)
testEncodeDecode(false)
testEncodeDecode(null)
testEncodeDecode(undefined) // added undefined for compatibility with charwise
testEncodeDecode('')
// testEncodeDecode(Buffer.alloc(0))
testEncodeDecode([])
testEncodeDecode({})
testEncodeDecode([1, 2, 3, 4, 5, 6, 7, 8, 9])
testEncodeDecode('hello')
testEncodeDecode({ foo: true })
// testEncodeDecode([-1, { foo: true }, Buffer.from('deadbeef', 'hex')])
testEncodeDecode(pkg)
testEncodeDecode({ 1: true })

*/

// eof
