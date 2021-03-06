#include <tests.h>
#include <vector>

TEST(PostLineTest,VarIntCodecHeaderTest){
  std::vector<PostLineHeader> header;
  PostLineHeader item;
  for (size_t i=1;i<=100;i++){
    item.doc_type=i;
    item.length=i;
    header.push_back(item);
  }
  unsigned char* out = new unsigned char[1024];
  PostLineCodec* codec = new VarIntCodec();
  EXPECT_EQ(201U,codec->encodeHeader(header,out));
  EXPECT_EQ(201U,codec->decodeHeader(out,header));
  EXPECT_EQ(100U,header.size());
  EXPECT_EQ(1U,header[0].doc_type);
  EXPECT_EQ(1U,header[0].length);
  EXPECT_EQ(100U,header[99].doc_type);
  EXPECT_EQ(100U,header[99].length);
  delete[] out;
}  

TEST(PostLineTest,VarIntCodecSectionTest){
  const uint32_t values[] = {12,8,56,2,12998,3434};
  vector<uint32_t> section(values,values+sizeof(values)/sizeof(uint32_t));
  const size_t length=section.size();
  unsigned char* out = new unsigned char[8];
  PostLineCodec* codec = new VarIntCodec();
  EXPECT_EQ(8U,codec->encodeSection(section,out));
  EXPECT_EQ(length,section.size());
  EXPECT_EQ(8U,codec->decodeSection(out,8,section,true));
  EXPECT_EQ(length,section.size());
  EXPECT_EQ(values[0],section[0]);
  EXPECT_EQ(values[5],section[5]);
  delete[] out;
}

TEST(PostLineTest,GroupVarIntCodecSectionTest){
  const uint32_t values[] = {12,8,56,2,12998,3434};
  vector<uint32_t> section(values,values+sizeof(values)/sizeof(uint32_t));
  const size_t length=section.size();
  unsigned char* out = new unsigned char[12];
  PostLineCodec* codec = new GroupVarIntCodec();
  EXPECT_EQ(12U,codec->encodeSection(section,out));
  EXPECT_EQ(length,section.size());
  EXPECT_EQ(12U,codec->decodeSection(out,12,section,true));
  EXPECT_EQ(length,section.size());
  EXPECT_EQ(values[0],section[0]);
  EXPECT_EQ(values[5],section[5]);
  EXPECT_EQ(12U,codec->decodeSection(out,12,section,false));
  EXPECT_EQ(length,section.size());
  EXPECT_EQ(values[0],section[0]);
  EXPECT_EQ(uint32_t(accumulate(values,values+6,0)),section[5]);
  delete[] out;
}

TEST(PostLineTest,PostLineTest){
  unsigned char* in = new unsigned char[256];
  unsigned char* out = new unsigned char[256];
  vector<uint32_t>* deltas;
  vector<uint32_t>* docids;
  memset(in,0,256);
  memset(out,0,256);
  PostLine* line = new PostLine(512);
  EXPECT_EQ(0U,line->getLength());
  line->load(in);
  line->addDocument(2,1);
  EXPECT_NE(0U,line->getLength(2));
  EXPECT_NE(0U,line->getLength());
  ASSERT_TRUE(line->commit(out));
  deltas=line->getDeltas(2);
  EXPECT_THAT(*deltas,ElementsAre(1));
  docids=line->getDocIds(2);
  EXPECT_THAT(*deltas,ElementsAre(1));
  // Add new document
  line->addDocument(2,7);
  ASSERT_TRUE(line->commit(out));
  deltas=line->getDeltas(2);
  EXPECT_THAT(*deltas,ElementsAre(1,6));
  docids=line->getDocIds(2);
  EXPECT_THAT(*docids,ElementsAre(1,7));
  docids=line->getDocIds(1);
  EXPECT_THAT(docids->size(),0);
  // Add new document
  line->addDocument(2,3);
  ASSERT_TRUE(line->commit(out));
  deltas=line->getDeltas(2);
  EXPECT_THAT(*deltas,ElementsAre(1,2,4));
  docids=line->getDocIds(2);
  EXPECT_THAT(*docids,ElementsAre(1,3,7));
  // Add new document with new doc type
  line->addDocument(1,3);
  ASSERT_TRUE(line->commit(out));
  docids=line->getDocIds(1);
  EXPECT_THAT(*docids,ElementsAre(3));
  docids=line->getDocIds(2);
  EXPECT_THAT(*docids,ElementsAre(1,3,7));
  // Add new document before
  line->addDocument(1,1);
  ASSERT_TRUE(line->commit(out));
  docids=line->getDocIds(1);
  EXPECT_THAT(*docids,ElementsAre(1,3));
  docids=line->getDocIds(2);
  EXPECT_THAT(*docids,ElementsAre(1,3,7));
  // Add new document at end
  line->addDocument(1,9);
  ASSERT_TRUE(line->commit(out));
  docids=line->getDocIds(1);
  EXPECT_THAT(*docids,ElementsAre(1,3,9));
  docids=line->getDocIds(2);
  EXPECT_THAT(*docids,ElementsAre(1,3,7));
  // Add new document with new doc type
  line->addDocument(5,9);
  ASSERT_TRUE(line->commit(out));
  docids=line->getDocIds(1);
  EXPECT_THAT(*docids,ElementsAre(1,3,9));
  docids=line->getDocIds(2);
  EXPECT_THAT(*docids,ElementsAre(1,3,7));
  docids=line->getDocIds(5);
  EXPECT_THAT(*docids,ElementsAre(9));
  // Commit with no change
  ASSERT_FALSE(line->commit(out));
  docids=line->getDocIds(1);
  EXPECT_THAT(*docids,ElementsAre(1,3,9));
  docids=line->getDocIds(2);
  EXPECT_THAT(*docids,ElementsAre(1,3,7));
  docids=line->getDocIds(5);
  EXPECT_THAT(*docids,ElementsAre(9));
  // Delete a document
  line->deleteDocument(2,3);
  line->commit(out);
  docids=line->getDocIds(1);
  EXPECT_THAT(*docids,ElementsAre(1,3,9));
  docids=line->getDocIds(2);
  EXPECT_THAT(*docids,ElementsAre(1,7));
  docids=line->getDocIds(5);
  EXPECT_THAT(*docids,ElementsAre(9));
  // Delete a document
  line->deleteDocument(5,9);
  line->commit(out);
  docids=line->getDocIds(1);
  EXPECT_THAT(*docids,ElementsAre(1,3,9));
  docids=line->getDocIds(2);
  EXPECT_THAT(*docids,ElementsAre(1,7));
  docids=line->getDocIds(5);
  EXPECT_EQ(0U,docids->size());
  delete[] out;
  delete[] in;
}

TEST(PostLineTest,BadParametersPostLineTest){
  unsigned char* first = new unsigned char[8];
  memset(first,0,8);
  PostLine* line = new PostLine(512);
  line->load(first);
  EXPECT_EQ(1U,line->getLength());
  ASSERT_FALSE(line->addDocument(2,0));
  EXPECT_EQ(1U,line->getLength());
  ASSERT_FALSE(line->addDocument(0,0));
  EXPECT_EQ(1U,line->getLength());
  ASSERT_FALSE(line->addDocument(0,1));
  EXPECT_EQ(1U,line->getLength());
  ASSERT_FALSE(line->deleteDocument(0,1));
  EXPECT_EQ(1U,line->getLength());
  ASSERT_FALSE(line->deleteDocument(1,0));
  EXPECT_EQ(1U,line->getLength());
  ASSERT_FALSE(line->deleteDocument(0,0));
  EXPECT_EQ(1U,line->getLength());
  // Delete non existent document
  ASSERT_FALSE(line->deleteDocument(2,56));
  EXPECT_EQ(1U,line->getLength());
  delete[] first;
}

TEST(PostLineTest,RealisticPostLineTest){
  unsigned char* first = new unsigned char[32];
  unsigned char* second = new unsigned char[32];
  memset(first,0,32);
  memset(second,0,32);
  vector<uint32_t>* docids;
  PostLine* line = new PostLine(512);
  line->load(first);
  // This is the single 0 marking the end of the header!
  EXPECT_EQ(1U,line->getLength());
  line->addDocument(2,1);
  EXPECT_EQ(8U,line->getLength());
  line->commit(first);
  EXPECT_EQ(8U,line->getLength());
  line->addDocument(3,97);
  EXPECT_EQ(15U,line->getLength());
  line->commit(first);
  EXPECT_EQ(15U,line->getLength());
  line->addDocument(4,65);
  EXPECT_EQ(22U,line->getLength());
  line->commit(second);
  memset(first,0,32);
  EXPECT_EQ(22U,line->getLength());
  line->addDocument(4,64);
  line->commit(second);
  docids=line->getDocIds(2);
  EXPECT_THAT(*docids,ElementsAre(1));
  docids=line->getDocIds(3);
  EXPECT_THAT(*docids,ElementsAre(97));
  docids=line->getDocIds(4);
  EXPECT_THAT(*docids,ElementsAre(64,65));
  line->deleteDocument(2,1);
  line->commit(second);
  line->deleteDocument(3,97);
  line->commit(second);
  line->deleteDocument(4,64);
  line->commit(second);
  line->deleteDocument(4,65);
  line->commit(first);
  EXPECT_EQ(1U,line->getLength());
  delete[] first;
  delete[] second;
}

TEST(PostLineTest,BigPostLineTest){
  unsigned char* forwards = new unsigned char[4096];
  unsigned char* backwards = new unsigned char[4096];
  vector<uint32_t>* f_deltas;
  vector<uint32_t>* b_deltas;
  memset(forwards,0,4096);
  memset(backwards,0,4096);
  PostLine* line = new PostLine(4096*5);
  line->load(forwards);
  for (size_t doc_type=1;doc_type<12;doc_type++){
    for (size_t doc_id=1;doc_id<=256;doc_id++){
      line->addDocument(doc_type,doc_id);
      line->commit(forwards);
    }
  }
  line->load(backwards);
  for (size_t doc_type=12;doc_type>0;doc_type--){
    for (size_t doc_id=256;doc_id>0;doc_id--){
      line->addDocument(doc_type,doc_id);
      line->commit(backwards);
    }
  }
  for (size_t doc_type=1;doc_type<12;doc_type++){
    line->load(forwards);
    f_deltas=line->getDeltas(doc_type);
    line->load(backwards);
    b_deltas=line->getDeltas(doc_type);
    EXPECT_EQ(f_deltas->size(),b_deltas->size());
    EXPECT_THAT(*f_deltas,ContainerEq(*b_deltas));
  }
  delete[] forwards;
  delete[] backwards;
}

