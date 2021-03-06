#include <tests.h>

void BaseTest::SetUp(){
  associationDB_ = new PolyDB();
  documentDB_ = new PolyDB();
  metaDB_ = new PolyDB();
  orderedMetaDB_ = new PolyDB();
  queueDB_ = new PolyDB();
  payloadDB_ = new PolyDB();
  miscDB_ = new PolyDB();
  associationDB_->open("%");
  documentDB_->open("%");
  metaDB_->open("%");
  orderedMetaDB_->open("%");
  queueDB_->open("%");
  payloadDB_->open("%");
  miscDB_->open("%");
  EXPECT_CALL(registry_,getWindowSize())
    .WillRepeatedly(Return(4));
  EXPECT_CALL(registry_,getPostingWindowSize())
    .WillRepeatedly(Return(2));
  EXPECT_CALL(registry_,getSlotCount())
    .WillRepeatedly(Return(4));
  EXPECT_CALL(registry_,getWhiteSpaceThreshold())
    .WillRepeatedly(Return(4));
  EXPECT_CALL(registry_,getWhiteSpaceHash(false))
    .WillRepeatedly(Return(0));
  EXPECT_CALL(registry_,getWhiteSpaceHash(true))
    .WillRepeatedly(Return(0));
  EXPECT_CALL(registry_,getMaxPostingThreshold())
    .WillRepeatedly(Return(100));
  EXPECT_CALL(registry_,getMaxDistance())
    .WillRepeatedly(Return(5));
  EXPECT_CALL(registry_,getSlotCount())
    .WillRepeatedly(Return(4));
  EXPECT_CALL(registry_,getHashWidth())
    .WillRepeatedly(Return(24));
  EXPECT_CALL(registry_,getHashMask())
    .WillRepeatedly(Return((1L<<24)-1));
  EXPECT_CALL(registry_,getMaxHashCount())
    .WillRepeatedly(Return(1<<24));
  EXPECT_CALL(registry_,getMaxLineLength())
    .WillRepeatedly(Return(1024));
  EXPECT_CALL(registry_,getNumResults())
    .WillRepeatedly(Return(20));
  EXPECT_CALL(registry_,getPageSize())
    .WillRepeatedly(Return(100));  
    
  EXPECT_CALL(registry_,getMetaDB())
    .WillRepeatedly(Return(metaDB_));
  EXPECT_CALL(registry_,getOrderedMetaDB())
    .WillRepeatedly(Return(orderedMetaDB_));
  EXPECT_CALL(registry_,getMiscDB())
    .WillRepeatedly(Return(miscDB_));
  EXPECT_CALL(registry_,getDocumentDB())
    .WillRepeatedly(Return(documentDB_));
  EXPECT_CALL(registry_,getAssociationDB())
    .WillRepeatedly(Return(associationDB_));
  EXPECT_CALL(registry_,getQueueDB())
    .WillRepeatedly(Return(queueDB_));
  EXPECT_CALL(registry_,getPayloadDB())
    .WillRepeatedly(Return(payloadDB_));

  documentManager_ = new DocumentManager(&registry_);
  associationManager_ = new AssociationManager(&registry_);
  queueManager_ = new QueueManager(&registry_);
  postings_ = new Posting(&registry_);
  logger_ = new Logger();
  EXPECT_CALL(registry_,getDocumentManager())
    .WillRepeatedly(Return(documentManager_));
  EXPECT_CALL(registry_,getAssociationManager())
    .WillRepeatedly(Return(associationManager_));
  EXPECT_CALL(registry_,getQueueManager())
    .WillRepeatedly(Return(queueManager_));
  EXPECT_CALL(registry_,getPostings())
    .WillRepeatedly(Return(postings_));
  EXPECT_CALL(registry_,getLogger())
    .WillRepeatedly(Return(logger_));
}
    
void BaseTest::TearDown(){
  associationDB_->close();
  metaDB_->close();
  orderedMetaDB_->close();
  documentDB_->close();
  queueDB_->close();
  payloadDB_->close();
  miscDB_->close();
  delete associationDB_;
  delete metaDB_;
  delete orderedMetaDB_;
  delete documentDB_;
  delete queueDB_;
  delete payloadDB_;
  delete miscDB_;
  delete documentManager_;
  delete associationManager_;
  delete queueManager_;
  delete postings_;
  delete logger_;
}

TestDocument::TestDocument(const char* filename){
  ifstream file(filename);
  stringstream buffer;
  buffer << file.rdbuf();
  text_=buffer.str();
  form_text_="text=";
  form_text_+=kyotocabinet::urlencode(text_.data(),text_.size());
  EXPECT_NE(0U,text_.size());
}

string& TestDocument::getText(){
  return text_;
}

string& TestDocument::getFormText(){
  return form_text_;
}

size_t TestDocument::getUniques(const size_t window_size){
  if (uniques_.find(window_size)==uniques_.end()){
    unordered_set<string> uniques;
    for (size_t i=0;i<text_.size()-window_size+1;i++){
      uniques.insert(text_.substr(i,window_size));
    }
    uniques_[window_size]=uniques.size();
  }
  return uniques_[window_size];
}

int main(int argc, char** argv) {
  ::testing::InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}
