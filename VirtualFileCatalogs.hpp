#pragma once

#include <DataStorage.hpp>


/*
i. Interface
1) KEY -> KEY
KEY -> address
2) tuple<N...>
3) tag

DB table

What we want:
<Exchange, Ticker, Resolution, Expiration, UniqueKey> -> address

Collection<T> get_list<Ticker>(Filter)

Collection<T> create(Exchange, Ticker, Resolution, Expiration, UniqueKey);

ii. What to store?
1) Address
2) DataStorage:Address
3) VirtualFile, which has same interface as ObjectStorage
 
*/

template <typename ...Keys>
class VirtualFileCatalog{
	StorageAddress base;
	DataStorage &storage;
	std::map<std::tuple<Keys...>, StorageAddress> m;
	void deserialize(){
		m.clear();

	}
	void serialize(){
		//put m into 
	}
public:
	VirtualFileCatalog(DataStorage &storage): storage(storage){
		//init base
		deserialize();
	}

	Result add(const Keys... &keys, const StorageAddress &addr){
		m.emplcate(std::make_tuple(keys...), addr);
		return Result::Success;
	}
	StorageAddress get(const Keys... &keys){
		auto it = m.find(std::make_tuple(keys...));
		ASSERT_ON(it == m.end());
		return it->second;
	}
	~VirtualFileCatalog(){
		serialize();
	}

};