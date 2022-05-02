#pragma once

//tuple
template < typename T, typename U >
struct typelist {
  typedef T element_type;
  typedef U next_type;
};
template < class T, int INDEX >
struct get_type {
    typedef typename get_type < typename T::next_type,
                                INDEX-1 >::type type;
};
template < class T >
struct get_type < T, 0 > {
    typedef typename T::element_type type;
};



class StorageManager{
public:
	using StorageIdType = uint32_t;

	template<typename T, typename K>
	ObjectStorage<T,K> &getObjectStorageInstance(StorageIdType id);

	DataStorage &getDataStorageInstance(StorageIdType id);
};
