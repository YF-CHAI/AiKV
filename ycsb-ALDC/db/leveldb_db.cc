#include "db/leveldb_db.h"
#include "leveldb/filter_policy.h" 
namespace ycsbc{

Level_DB::Level_DB(
 const std::string db_path,
 const uint64_t write_buffer_size, 
 const uint64_t block_size,
 const int max_open_files, 
 const uint64_t max_file_size,
 //cyf add for static var tuning
 const int kL0_CompactionTrigger,
 const int kL0_SlowdownWritesTrigger,
 const int kL0_StopWritesTrigger,
 const int kLSMFanout,
 //cyf add for ALDC
 const int kBufferCompactStartLevel,
 const int kBufferCompactEndLevel,
 const double kLDCMergeSizeRatio,
 const bool kUseAdaptiveLDC,
 const int kThresholdBufferNum,
 const uint64_t kLDCBCCProbeInterval)
{
    if(db_path == "")   std::cout << "db_path is NULL<<std::endl";
    leveldb::Options options;
    //cyf add modify them to change performance
    options.create_if_missing = !leveldb::Options().create_if_missing;
    options.write_buffer_size = write_buffer_size;
    options.block_size = block_size;
    options.max_open_files = max_open_files;
    //师兄注释掉的,不知为何!
    options.max_file_size = max_file_size;
    options.block_cache = leveldb::NewLRUCache(8 << 20);//8MB for block cache
    options.compression = leveldb::Options().compression;
    options.filter_policy = leveldb::NewBloomFilterPolicy(10);//for bloom filter
    //options.max_open_files = 5000;
    //options.compression = leveldb::kNoCompression;

    leveldb::Status s =leveldb::DB::Open(options, db_path, &db_);
    db_->setStaticParameters(
    kL0_CompactionTrigger,
    kL0_SlowdownWritesTrigger,
    kL0_StopWritesTrigger,
    kLSMFanout,
    max_file_size,
    write_buffer_size,
    kBufferCompactStartLevel,
    kBufferCompactEndLevel,
    kLDCMergeSizeRatio,
    kUseAdaptiveLDC,
    kThresholdBufferNum,
    block_size,
    kLDCBCCProbeInterval
    );

    if (!s.ok()) {
      fprintf(stderr, "open error: %s\n", s.ToString().c_str());
      exit(1);
    }
    assert(s.ok());
}

int Level_DB::Read(const std::string &table, const std::string &key,
                        const std::vector<std::string> *fields,
                        std::vector<KVPair> &result)
{
    leveldb::ReadOptions options;
    std::string value;
    leveldb::Status s = db_->Get(options, key, &value);
    if(!s.ok()) std::cout<<"Read Not Found: "<<key <<s.ToString()<<std::endl;
    //if(value.size() < 1000) std::cout <<"Unormal value: "<<value <<std::endl;
    result.push_back(std::make_pair(key, value));
    return DB::kOK;
}

int Level_DB::Scan(const std::string &table, const std::string &key,
                        int len, const std::vector<std::string> *fields,
                        std::vector<std::vector<KVPair> > &result)
{
    std::vector<KVPair> values;
    leveldb::ReadOptions options;
    //Sint scan_num =0;
    //options.tailing =false;
    std::string value;
    leveldb::Iterator* iter = db_->NewIterator(options);
    iter->Seek(key);
    // db_->setScanNum(1);//cyf ugly method for Scan operation statistic, change later.
    //cyf 1 scan op may cotains more key serarch ops, we want to get the TKPS
    for(size_t i = 0; i < len && iter->Valid(); i++) {
        //values.push_back(iter->value().Encode());
        //scan_num++;
        values.push_back(std::make_pair(iter->key().ToString(), iter->value().ToString()));
        iter->Next();
    }
    result.push_back(values);
    delete iter;
    return DB::kOK;
    //return scan_num;

}

int Level_DB::Update(const std::string &table, const std::string &key,
                          std::vector<KVPair> &values)
{
    leveldb::Status s = db_->Put(leveldb::WriteOptions(),
                                 key, values[0].second);

}

int Level_DB::Insert(const std::string &table, const std::string &key, std::vector<DB::KVPair> &values)
{
    leveldb::WriteBatch batch;
    leveldb::Status s;
    leveldb::WriteOptions options;
    //batch.Clear();
    batch.Put(key,values[0].second);
    //cyf add for value size test
    //std::cout<<" Insert key: "<<key<<" Length: "<<key.size()<<" value size: "<<values[0].second.size()<<std::endl;
    s = db_->Write(options, &batch);
    if(!s.ok()){
        std::cout<<"Level_DB::Insert error:"<<s.ToString()<<std::endl;
    }
    return DB::kOK;

}

int Level_DB::Delete(const std::string &table, const std::string &key)
{
    leveldb::WriteBatch batch;
    leveldb::Status s;
    batch.Clear();
    batch.Delete(key);
    leveldb::WriteOptions options;
    s = db_->Write(options,&batch);
    if(!s.ok()){
        std::cout<<"Level_DB::Delete error:"<<s.ToString()<<std::endl;
    }
    return DB::kOK;
}

}//namespace ycsbc
