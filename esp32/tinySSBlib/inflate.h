// tinySSBlib/inflate.h

// decompress zlib data (that incl the zlib headers)

#include <rom/miniz.h>

static void inflate(unsigned char *dst, size_t dst_len,
                    const unsigned char* src, size_t src_len)
{
#define INFLATOR_BUFFER_SIZE 20*1024

  unsigned char *buffer = (unsigned char*) malloc(INFLATOR_BUFFER_SIZE);
  size_t out_count = 0;
  size_t in_count = 0;

  tinfl_decompressor *decomp = (tinfl_decompressor*)
                                      malloc(sizeof(tinfl_decompressor));
  tinfl_init(decomp);

  while (1) {
    size_t out_size = INFLATOR_BUFFER_SIZE - out_count;
    size_t in_size = src_len - in_count;
    int status = tinfl_decompress(decomp,
                                  src + in_count, &in_size,
                                  buffer, buffer + out_count, &out_size,
                                  TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF
                                  | TINFL_FLAG_PARSE_ZLIB_HEADER);
    // | src_len > 0 ? TINFL_FLAG_HAS_MORE_INPUT : 0);
    in_count += in_size;
    if (out_size > 0)
      memcpy(dst + out_count, buffer + out_count, out_size);
    out_count += out_size;
    if (status == TINFL_STATUS_DONE)
      break;
    if (status != TINFL_STATUS_HAS_MORE_OUTPUT) {
      Serial.printf("tinfl_decompress failed %d\r\n", status);
      break;
    }
  }
  free(decomp);
  free(buffer);
}

// eof
