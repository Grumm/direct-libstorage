#include <libstorage/SequenceStorage.hpp>

/****************************************************/

constexpr std::string diskfolder = "/cygdrive/h/stmp/";
constexpr std::string ssdfolder = "/cygdrive/g/stmp/";
constexpr std::string serveraddress = "127.0.0.1";

StorageManager _storage_manager; //global

using tu = trading::utils;

class ExampleObject: public Serializable<ExampleObject>{
public:
	ExampleObject(ObjectAddress addr);
	tu::Result synchronizeImpl(const StorageRawBuffer &raw_buffer) override;
	tu::Result serializeImpl(const StorageRawBuffer &raw_buffer) override;
	static ExampleObject deserializeImpl(const StorageRawBuffer &raw_buffer) override;
	constexpr size_t getSizeImpl() override;
};

//Addressing raw data(DataStorage)
using ExampleMRStorage = MultiRankStorage<MemoryStorage, SSDStorage, DiskStorage, NetworkStorage>;

//Addressing raw bytes
using ExampleDataStorage = DataStorage<ExampleMRStorage>;

//template<typename T>
using ExampleObjectStorage = ObjectStorage<ExampleDataStorage, ExampleObject>;


//StorageAddress
//ObjectAddress

void init_storage(){
	MemoryStorage _memory_storage(MemoryStorage::limit<4, GB>);
	SSDStorage _ssd_storage(ssdfolder);
	DiskStorage _disk_storage(diskfolder);
	NetworkStorage _network_storage(serveraddress);

	ExampleMRStorage _multi_rank_storage(_memory_storage, _ssd_storage, _disk_storage, _network_storage);

	ExampleDataStorage _data_storage(_multi_rank_storage);

	ExampleObjectStorage _example_object_storage(_data_storage);

	_storage_manager.register(_example_object_storage);
}

//C++ 20 concepts
#include <ranges>

template<typename T>
concept ElementIterable = std::ranges::range<std::ranges::range_value_t<T>>;

void testf(){
	constexpr std::string seqfolder = "/cygdrive/g/trading/data/finam/";
	static std::vector<std::string> filenames{
		"USD000UTSTOM/monthly/USD000UTSTOM_131201_131231.txt",
	};
	std::string ticker = "USD000UTSTOM";
	for(auto f: filenames){
		ObjectsSequence<ExampleObject> seq;
		seq.load_from_file(seqfolder + f); // .txt file
		ElementIterable<ExampleObject> collection = seq.getSequence();
		for(auto e: collection){
			StorageKey key(ticker, StorageManager::Precision::Tick);
			//StorageKey key(ticker, StorageManager::Precision::Custom{"test1"});
			//DataStorage ds = _storage_manager.get<ObjectStorage<T>>(key);?
			ObjectAddress addr = e.serialize(key); //just put sequence under this key
			ObjectAddress position;
			ObjectAddress addr2 = e.serialize(key, position); //update sequence with this key(add new ticks at the end)
		}
	}
	{
		StorageKey key(ticker, StorageManager::Precision::Tick);
		ObjectAddress addr;
		size_t seq_size = 10000; //limit to sequence size
		const ObjectsSequence<ExampleObject> seq(seq_size, key, addr);
		size_t offset = seq.size();
		ObjectAddress addr2(addr, offset);
		const ObjectsSequence<ExampleObject> seq2(seq_size, key, addr2);

		ObjectAddress addr3(addr2, seq2.size());
		ObjectsSequence<ExampleObject> seq3;
		seq3.deserialize(seq_size, key, addr3);
		//...change seq3 values
		seq3.synchronize(key, addr3); //only if we changed values for the sequence
		//TODO if we added/deleted values, how to synchronize
		ObjectsSequence<ExampleObject> seq4(seq3);
		ObjectAddress position(ObjectAddress::LAST);
		seq4.serialize(key, position);
		GetStorageManager().sync(); //force write to disk all caches
	}
	{
		StorageKey key(ticker, StorageManager::Precision::Tick);
		ExampleObjectStorage o_storage = GetStorageManager().GetStorage(key);
		ObjectAddress o_addr;
		StorageAddress s_addr = o_storage.GetStorageAddress(o_addr);
		
		ExampleDataStorage d_storage = o_storage.GetDataStorage(...)
		StorageRawBuffer raw_buffer = d_storage.GetRawBuffer(s_addr);
	}
}


void testf2(){
	constexpr std::string seqfolder = "/cygdrive/g/trading/data/finam/";
	static std::vector<std::string> filenames{
		"USD000UTSTOM/monthly/USD000UTSTOM_131201_131231.txt",
	};
	std::string ticker = "USD000UTSTOM";
	for(auto f: filenames){
		ObjectsSequence<ExampleObject> seq;
		seq.load_from_file(seqfolder + f); // .txt file
		ElementIterable<ExampleObject> collection = seq.getSequence();
		for(auto e: collection){
			StorageKey key(ticker, StorageManager::Precision::Tick);
			//StorageKey key(ticker, StorageManager::Precision::Custom{"test1"});
			//DataStorage ds = _storage_manager.get<ObjectStorage<T>>(key);?
			ObjectAddress addr = e.serialize(key); //just put sequence under this key
			ObjectAddress position;
			ObjectAddress addr2 = e.serialize(key, position); //update sequence with this key(add new ticks at the end)
		}
	}
	{
		StorageKey key(ticker, StorageManager::Precision::Tick);
		ObjectAddress addr;
		size_t seq_size = 10000; //limit to sequence size
		const ObjectsSequence<ExampleObject> seq(seq_size, key, addr);
		size_t offset = seq.size();
		ObjectAddress addr2(addr, offset);
		const ObjectsSequence<ExampleObject> seq2(seq_size, key, addr2);

		ObjectAddress addr3(addr2, seq2.size());
		ObjectsSequence<ExampleObject> seq3;
		seq3.deserialize(seq_size, key, addr3);
		//...change seq3 values
		seq3.synchronize(key, addr3); //only if we changed values for the sequence
		//TODO if we added/deleted values, how to synchronize
		ObjectsSequence<ExampleObject> seq4(seq3);
		ObjectAddress position(ObjectAddress::LAST);
		seq4.serialize(key, position);
		GetStorageManager().sync(); //force write to disk all caches
	}
	{
		StorageKey key(ticker, StorageManager::Precision::Tick);
		ExampleObjectStorage o_storage = GetStorageManager().GetStorage(key);
		ObjectAddress o_addr;
		StorageAddress s_addr = o_storage.GetStorageAddress(o_addr);
		
		ExampleDataStorage d_storage = o_storage.GetDataStorage(...)
		StorageRawBuffer raw_buffer = d_storage.GetRawBuffer(s_addr);
	}
}
