#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>
#include <string.h>
#include "tinypb_codec.h"
#include "../byte.h"
#include "../../log/log.h"
#include "../abstract_data.h"
#include "tinypb_data.h"


namespace tinyrpc {

const char PB_START= 0x02;
const char PB_END = 0x03;

TinyPbCodeC::TinyPbCodeC() {

}

TinyPbCodeC::~TinyPbCodeC() {

}

void TinyPbCodeC::encode(TcpBuffer::ptr buf, AbstractData* data) {
  DebugLog << "test encode start";

  std::stringstream ss;
  ss << PB_START << buf->getBuffer();
  // int pk_len = 1;

  DebugLog << "test encode end";
}

void TinyPbCodeC::decode(TcpBuffer::ptr buf, AbstractData* data) {
  std::vector<char> tmp = buf->getBufferVector();
  int total_size = tmp.size();
  int start_index = -1;
  int end_index = -1;
  int32_t pk_len= -1; 

  bool parse_flag = false;
  
  for (int i = 0; i < total_size; ++i) {
    // first find start
    if (tmp[i] == PB_START) {
      if (i + 1 < total_size) {
        pk_len = getInt32FromNetByte(&tmp[i+1]);
        DebugLog << "prase pk_len =" << pk_len;
        int j = i + pk_len - 1;
        DebugLog << "j =" << j << ", i=" << i;

        if (j >= total_size) {
          DebugLog << "recv package not complete, or pk_start find error, continue next parse";
          continue;
        }
        if (tmp[j] == PB_END) {
          start_index = i;
          end_index = j;
          DebugLog << "parse succ, now break";
          parse_flag = true;
          break;
        }
        
      }
      
    }
  }

  if (!parse_flag) {
    DebugLog << "parse error, return";
    return;
  }
  
  int service_name_len_index = start_index + sizeof(char) + sizeof(int32_t);
  if (service_name_len_index >= end_index) {
    ErrorLog << "parse error, service_name_len_index[" << service_name_len_index << "] >= end_index[" << end_index << "]";
    // drop this error package
    buf->recycle(pk_len);
    return;
  }
  DebugLog << "service_name_len_index = " << service_name_len_index;
  int service_name_index = service_name_len_index + sizeof(int32_t);

  if (service_name_index >= end_index) {
    ErrorLog << "parse error, service_name_index[" << service_name_index << "] >= end_index[" << end_index << "]";
    buf->recycle(pk_len);
    return;
  }

  // TinyPbStruct pb_struct;
  TinyPbStruct* pb_struct = dynamic_cast<TinyPbStruct*>(data);
  pb_struct->pk_len = pk_len;

  pb_struct->service_name_len = getInt32FromNetByte(&tmp[service_name_len_index]);

  if (pb_struct->service_name_len > pk_len) {
    ErrorLog << "parse error, service_name_len[" << pb_struct->service_name_len << "] >= pk_len [" << pk_len << "]";
    buf->recycle(pk_len);
    return;
  }
  // DebugLog << "service_name_len = " << pb_struct.service_name_len;
  char service_name[pb_struct->service_name_len];

  memcpy(&service_name[0], &tmp[service_name_index], pb_struct->service_name_len);
  pb_struct->service_name = service_name;
  // DebugLog << "service_name = " << pb_struct.service_name;

  int pb_data_len = pb_struct->pk_len - pb_struct->service_name_len - 2 * sizeof(char) - 3 * sizeof(int32_t);

  int pb_data_index = service_name_index + pb_struct->service_name_len;

  if (pb_data_index >= end_index) {
    ErrorLog << "parse error, pb_data_index[" << pb_data_index << "] >= end_index[" << end_index << "]";
    buf->recycle(pk_len);
    return;
  }

  memcpy(&(pb_struct->pb_data[0]), &tmp[pb_data_index], pb_data_len);

  DebugLog << "decode succ,  pk_len = " << pk_len << ", service_name = " << pb_struct->service_name; 
  buf->recycle(pk_len);

  pb_struct->parse_succ = true;
  data = pb_struct;

}

CodeCType TinyPbCodeC::type() const {
  return CODEC_HTTP;
}

}
