#include <codec.h>


namespace superfastmatch
{
  // VarIntCodec implementation
  // --------------------------

  size_t VarIntCodec::encodeHeader(const vector<PostLineHeader>& header,unsigned char* start){
    size_t offset=kc::writevarnum(start,header.size());
    for (size_t i=0;i<header.size();i++){
      offset+=kc::writevarnum(start+offset,header[i].doc_type);
      offset+=kc::writevarnum(start+offset,header[i].length);
    }
    return offset;
  }

  size_t VarIntCodec::decodeHeader(const unsigned char* start, vector<PostLineHeader>& header){
    uint64_t length;
    unsigned char* cursor=const_cast<unsigned char*>(start);
    cursor+=kc::readvarnum(cursor,5,&length);
    header.resize(0);
    for (size_t i=length;i>0;i--){
      uint64_t doc_type;
      uint64_t length;
      cursor+=kc::readvarnum(cursor,5,&doc_type);
      cursor+=kc::readvarnum(cursor,5,&length);
      header.push_back(PostLineHeader(doc_type,length));
    }
    return cursor-start;
  };
  
  size_t VarIntCodec::encodeSection(vector<uint32_t>& section,unsigned char* start){
    size_t offset=0;
    for(vector<uint32_t>::const_iterator it=section.begin(),ite=section.end();it!=ite;++it){
      offset+=kc::writevarnum(start+offset,*it);
    }
    return offset;
  }
  
  size_t VarIntCodec::decodeSection(const unsigned char* start, const size_t length, vector<uint32_t>& section,bool asDeltas){
    size_t offset=0;
    uint64_t value;
    section.resize(0);
    if (asDeltas){
      while (offset!=length){
        offset+=kc::readvarnum(start+offset,5,&value);
        section.push_back(value);
      }
    }else{
      uint64_t previous=0;
      while (offset!=length){
        offset+=kc::readvarnum(start+offset,5,&value);
        previous+=value;
        section.push_back(previous);
      }
    }
    return offset;
  }
  
  // GroupVarIntCodec implementation derived from:
  // http://www.ir.uwaterloo.ca/book/addenda-06-index-compression.html#groupvarint
  // --------------------------

  size_t GroupVarIntCodec::encodeHeader(const vector<PostLineHeader>& header,unsigned char* start){
    size_t offset=kc::writevarnum(start,header.size());
    for (size_t i=0;i<header.size();i++){
      offset+=kc::writevarnum(start+offset,header[i].doc_type);
      offset+=kc::writevarnum(start+offset,header[i].length);
    }
    return offset;
  }

  size_t GroupVarIntCodec::decodeHeader(const unsigned char* start, vector<PostLineHeader>& header){
    uint64_t length;
    unsigned char* cursor=const_cast<unsigned char*>(start);
    cursor+=kc::readvarnum(cursor,5,&length);
    header.resize(0);
    for (size_t i=length;i>0;i--){
      uint64_t doc_type;
      uint64_t length;
      cursor+=kc::readvarnum(cursor,5,&doc_type);
      cursor+=kc::readvarnum(cursor,5,&length);
      header.push_back(PostLineHeader(doc_type,length));
    }
    return cursor-start;
  };
  
  size_t GroupVarIntCodec::encodeSection(vector<uint32_t>& section,unsigned char* start){
    unsigned char* offset=start;
    size_t padding=4-(section.size()%4);
    if (padding<4){
      section.insert(section.end(),padding,0);
    }
    for(vector<uint32_t>::const_iterator it=section.begin(),ite=section.end();it!=ite;){
      unsigned char* selector=offset++;
      *selector=0;
      for (size_t i=0;i<4;i++){
        const uint32_t value=*it++;
        *(uint32_t*)offset=value;
        const bool highBit=value&0xFFFF0000;
        const bool lowBit=bool(value&0xFF000000)^bool(value&0x00FFFF00);
        const uint32_t numBytes=(highBit<<1)|(highBit^lowBit);
        *selector |= numBytes << (i*2);
        offset+=numBytes+1;
      }
    }
    if (padding<4){
      section.erase(section.end()-padding,section.end());
    }
    return offset-start;
  }
  
  size_t GroupVarIntCodec::decodeSection(const unsigned char* start, const size_t length, vector<uint32_t>& section,bool asDeltas){
    const uint32_t mask[4] = { 0xFF, 0xFFFF, 0xFFFFFF, 0xFFFFFFFF };
    unsigned char* offset=const_cast<unsigned char*>(start);
    const unsigned char* end=offset+length;
    const uint32_t deltas_mask=(asDeltas)-1;
    section.resize(0);
    uint32_t previous=0;
    while (offset!=end){
      const uint32_t selector=*offset++;
      const uint32_t selector1=(selector & 3);
      previous=(previous&deltas_mask)+(*((uint32_t*)(offset)) & mask[selector1]);
      section.push_back(previous);
      offset += selector1 + 1;
      const uint32_t selector2 = ((selector >> 2) & 3);
      previous=(previous&deltas_mask)+(*((uint32_t*)(offset)) & mask[selector2]);
      section.push_back(previous);
      offset += selector2 + 1;
      const uint32_t selector3 = ((selector >> 4) & 3);
      previous=(previous&deltas_mask)+(*((uint32_t*)(offset)) & mask[selector3]);
      section.push_back(previous);
      offset += selector3 + 1;
      const uint32_t selector4 = (selector >> 6);
      previous=(previous&deltas_mask)+(*((uint32_t*)(offset)) & mask[selector4]);
      section.push_back(previous);
      offset += selector4 + 1;
    }
    const uint32_t padNumber=asDeltas?0:section.back();
    size_t padding=0;
    for (vector<uint32_t>::reverse_iterator it=section.rbegin(),ite=section.rend();it!=ite && *it==padNumber;++it){
      padding++;
    };
    if (!asDeltas && padding>0){
      padding--;
    }
    section.erase(section.end()-padding,section.end());
    return offset-start;
  }
}