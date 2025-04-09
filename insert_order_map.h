#pragma once
#include <functional>
#include <cstddef>
#include <utility>
#include <array>
#include <vector>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <iterator>
#include <initializer_list>

template <typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>>
class insert_order_map
{
public:
	typedef K key_type;
	typedef V mapped_type;
	typedef std::pair<const K, V> value_type;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	typedef H hasher;
	typedef E key_equal;
	typedef value_type& reference;
	typedef value_type* pointer;
	typedef const value_type* const_pointer;
	typedef std::vector<value_type>::iterator iterator;
	typedef std::vector<value_type>::const_iterator const_iterator;
	typedef std::array<size_type, 8> bucket_type;
	
	insert_order_map() noexcept(noexcept(b_storage()) && noexcept(e_storage()))
		: mh(), me() {}
	explicit insert_order_map(size_type bucket_count, const H& h = H(), const E& e = E())
		: mh(h, bucket_count), me(e) {}
	
	insert_order_map(const insert_order_map& oth) : mh(oth.mh), me(oth.me) {}
	insert_order_map(insert_order_map&& oth) : mh(std::move(oth.mh)), me(std::move(oth.me)) {}
	
	template<typename HOTH>
	insert_order_map(const insert_order_map<K, V, HOTH, E>& oth)
		: mh(H(), oth.mh.bucket_count), me(oth.me) { rehash(); }
	template<typename HOTH>
	insert_order_map(insert_order_map<K, V, HOTH, E>&& oth) : mh(), me(std::move(oth.me))
	{
		mh.buckets = oth.mh.buckets;
		mh.bucket_count = oth.mh.bucket_count;
		oth.mh.buckets = nullptr;
		oth.mh.bucket_count = 0;
		rehash();
	}
	
	insert_order_map(std::initializer_list<value_type> ilist, const H& h = H(), const E& e = E())
		: mh(h), me(e, ilist) { rehash(); }
	template <typename InputIt>
	insert_order_map(InputIt first, InputIt last, const H& h = H(), const E& e = E())
		: mh(h), me(e, first, last) { rehash(); }
	
	insert_order_map& operator=(std::initializer_list<value_type> ilist)
	{
		me.v = ilist;
	}
	
	iterator begin() noexcept { return me->begin(); }
	iterator end() noexcept { return me->end(); }
	const_iterator begin() const noexcept { return me->cbegin(); }
	const_iterator end() const noexcept { return me->cend(); }
	const_iterator cbegin() const noexcept { return me->cbegin(); }
	const_iterator cend() const noexcept { return me->cend(); }
	
	bool empty() const noexcept { return me->empty(); }
	size_type size() const noexcept { return me->size(); }
	size_type max_size() const noexcept { return me->max_size(); }
	
	void clear()
	{
		mh.clear();
		me->clear();
	}
	
	std::pair<iterator, bool> push_back(const value_type& val)
	{
		auto [it, b] = find(val.first);
		if(it == me->end())
		{
			me->push_back(val);
			if(!mh.insert(b, me->size())) rehash();
			return {std::prev(me->end()), true};
		}else
		{
			return {it, false};
		}
	}
	
	std::pair<iterator, bool> push_back(value_type&& val)
	{
		auto [it, b] = find(val.first);
		if(it == me->end())
		{
			me->push_back(std::move(val));
			if(!mh.insert(b, me->size())) rehash();
			return {std::prev(me->end()), true};
		}else
		{
			return {it, false};
		}
	}
	
	template <typename... Args>
	std::pair<iterator, bool> emplace_back(K&& key, Args... args)
	{
		auto [it, b] = find(key);
		if(it == me->end())
		{
			me->emplace_back(std::move(key), std::forward<Args>(args)...);
			if(!mh.insert(b, me->size())) rehash();
			return {std::prev(me->end(), true};
		}else
		{
			return {it, false};
		}
	}
	
	template <typename... Args>
	std::pair<iterator, bool> emplace_back(Args... args)
	{
		const value_type& val = me->emplace_back(std::forward<Args>(args)...);
		auto [it, b] = find(val.first)
		if(it == me->end())
		{
			if(!mh.insert(b, me->size())) rehash();
			return {std::prev(it), true};
		}else
		{
			me->pop_back();
			return {it, false};
		}
	}
	
	void pop_back()
	{
		bucket_type& b = bucket(me->back().first);
		for(size_t& idx : b)
			if(idx == me->size())
			{
				idx = 0;
				break;
			}
		me->pop_back();
	}
	
	std::pair<iterator, bool> insert(const_iterator pos, const value_type& val)
	{
		auto it = find(val.first);
		if(it == me->end())
		{
			it = me->insert(it, val);
			rehash();
			return {it, true};
		}else
		{
			return {it, false};
		}
	}
	
	std::pair<iterator, bool> insert(const_iterator pos, value_type&& val)
	{
		auto it = find(val.first);
		if(it == me->end())
		{
			it = me->insert(it, val);
			rehash();
			return {it, true};
		}else
		{
			return {it, false};
		}
	}
	
	template <typename M>
	std::pair<iterator, bool> insert_or_assign(const K& k, M&& obj)
	{
		auto [it, b] = find(k);
		if(k == me->end())
		{
			me->emplace_back(k, std::forward<M>(obj));
			if(!mh.insert(b, me->size())) rehash();
			return {std::prev(me->end(), true};
		}else{
			it->second = std::forward<M>(obj);
			return {it, false}
		}
	}
	
	template <typename M>
	std::pair<iterator, bool> insert_or_assign(K&& k, M&& obj)
	{
		auto [it, b] = find(k);
		if(k == me->end())
		{
			me->emplace_back(std::move(k), std::forward<M>(obj));
			if(!mh.insert(b, me->size())) rehash();
			return {std::prev(me->end(), true};
		}else{
			it->second = std::forward<M>(obj);
			return {it, false}
		}
	}
	
	template <typename M>
	std::pair<iterator, bool> insert_or_assign(const_iterator pos, const K& k, M&& obj)
	{
		auto [it, b] = find(k);
		if(k == me->end())
		{
			me->emplace(pos, k, std::forward<M>(obj));
			if(!mh.insert(b, me->size())) rehash();
			return {std::prev(me->end(), true};
		}else{
			it->second = std::forward<M>(obj);
			return {it, false}
		}
	}
	
	template <typename M>
	std::pair<iterator, bool> insert_or_assign(const_iterator pos, K&& k, M&& obj)
	{
		auto [it, b] = find(k);
		if(k == me->end())
		{
			me->emplace(pos, std::move(k), std::forward<M>(obj));
			if(!mh.insert(b, me->size())) rehash();
			return {std::prev(me->end(), true};
		}else{
			it->second = std::forward<M>(obj);
			return {it, false}
		}
	}
	
	template <typename... Args>
	std::pair<iterator, bool> emplace(const_iterator it, K&& key, Args... args)
	{
		auto it = find(key);
		if(it == me->end())
		{
			it = me->emplace(it, std::move(key), std::forward<Args>(args)...);
			rehash();
			return {it, true};
		}else
		{
			return {it, false};
		}
	}
	
	template <typename... Args>
	std::pair<iterator, bool> emplace(const_iterator it, Args... args)
	{
		value_type val(std::forward<Args>(args)...);
		auto it = find(val.first);
		if(it == me->end())
		{
			it = me->emplace(it, std::move(val));
			rehash();
			return {it, true};
		}else
		{
			return {it, false};
		}
	}
	
	template <typename... Args>
	std::pair<iterator, bool> emplace(size_t idx, Args... args)
	{
		if(idx > me->size()) return {me->end(), false};
		return emplace(std::advance(me->begin(), idx), args);
	}
	
	iterator erase(iterator pos)
	{
		auto it = me->erase(pos);
		rehash();
		return it;
	}
	
	iterator erase(const_iterator pos)
	{
		auto it = me->erase(pos);
		rehash();
		return it;
	}
	
	iterator erase(iterator first, iterator last)
	{
		auto it = me->erase(first, last);
		rehash();
		return it;
	}
	
	iterator erase(const_iterator first, const_iterator last)
	{
		auto it = me->erase(first, last);
		rehash();
		return it;
	}
	
	iterator erase(const K& key)
	{
		auto it = find(key);
		if(it != me->end())
		{
			it = me->erase(it);
			rehash();
		}
		return it;
	}
	
	template <typename Pred>
	size_t erase_if(Pred&& pred)
	{
		size_t acc = 0;
		for(iterator it = me->begin; it != me->end(); it++)
		{
			const auto& [k, v] = *it;
			if(pred(k, v))
			{
				acc++;
				it = me->erase(it);
			}
		}
		if(acc > 0) rehash();
		return acc;
	}
	
	void swap(insert_order_map& oth)
	{
		swap(mh, oth.mh);
		swap(me, oth.me);
	}
	
	V& at(const K& k)
	{
		const bucket_type& b = bucket(k);
		for(size_t idx : b)
		{
			if(idx>0)
			{
				value_type& test = me.v[idx-1];
				if(me(test.first, k))
				{
					return test.second;
				}
			}
		}
		throw std::out_of_range("key not found in insert_order_map");
	}
	
	const V& at(const K& k) const
	{
		const bucket_type& b = bucket(k);
		for(size_t idx : b)
		{
			if(idx>0)
			{
				const value_type& test = me.v[idx-1];
				if(me(test.first, k))
				{
					return test.second;
				}
			}
		}
		throw std::out_of_range("key not found in insert_order_map");
	}
	
	const V& operator[](const K& k) const { return at(k); }
	
	V& operator[](const K& k)
	{
		auto [it, b] = find(k);
		if(it == me->end())
		{
			me->emplace_back(key, V());
			if(!mh.insert(b, me->size())) rehash();
			return me->back().second();
		}else
		{
			return it->second;
		}
	}
	
	V& operator[](K&& k)
	{
		auto [it, b] = find(k);
		if(it == me->end())
		{
			me->emplace_back(std::move(key), V());
			if(!mh.insert(b, me->size())) rehash();
			return me->back().second();
		}else
		{
			return it->second;
		}
	}
	
	value_type& front() { return me->front(); }
	const value_type& front() const { return me->front(); }
	value_type& back() { return me->back(); }
	const value_type& back() const { return me->back(); }
	value_type* data() { return me->data(); }
	const value_type* data() const { return me->data(); }
	
	iterator find(const K& k)
	{
		const bucket_type& b = bucket(k);
		for(size_t idx : b)
		{
			if(idx>0)
			{
				if(me(me.v[idx-1].first, k))
				{
					return std::advance(me->begin(), idx-1);
				}
			}
		}
		return me->end();
	}
	
	const_iterator find(const K& k) const
	{
		const bucket_type& b = bucket(k);
		for(size_t idx : b)
		{
			if(idx>0)
			{
				if(me(me.v[idx-1].first, k))
				{
					return std::advance(me->cbegin(), idx-1);
				}
			}
		}
		return me->cend();
	}
	
	bool contains(const K& k) const
	{
		const bucket_type& b = bucket(k);
		for(size_t idx : b)
		{
			if(idx>0)
			{
				if(me(me.v[idx-1].first, k))
				{
					return true;
				}
			}
		}
		return false;
	}
	
	size_type bucket_count() const noexcept { return mh.bucket_count; };
	size_type bucket_size(size_type n) const
	{
		if(n >= mh.bucket_count) throw std::out_of_range("bucket_count: index out of bounds");
		size_type acc;
		for(size_type i : mh.buckets[n])
			if(i > 0) acc++;
		return acc;
	}
	size_type bucket(const K& k) const
	{
		return mh(k)%mh.bucket_count;
	}
	
	double load_factor() const noexcept
	{
		return static_cast<double>(size())/static_cast<double>(mh.bucket_count);
	}
	void rehash( size_type count )
	{
		if(mh.resize(count)) rehash();
	}
	void reserve(size_type count) { me->reserve(count); }
	
	hasher hash_function() const { return static_cast<H>(mh); }
	key_equal key_eq() const { return static_cast<E>(me); }
	
	template <typename HOTH>
	friend bool operator==(const insert_order_map& lhs, const insert_order_map<K, V, HOTH, E>& rhs)
	{
		if(lhs.size() != rhs.size()) return false;
		for(size_type i = 0; i < lhs.size(); i++)
		{
			if(!lhs.me(lhs.me.v[i].first, rhs.me.v[i].first)
				|| lhs.me.v[i].second != rhs.me.v[i].second) return false;
		}
		return true;
	}
	template <typename HOTH>
	friend bool operator!=(const insert_order_map& lhs, const insert_order_map<K, V, HOTH, E>& rhs)
	{
		return !(lhs==rhs);
	}
	
	friend void swap(insert_order_map& a, insert_order_map& b)
	{
		swap(a.mh, b.mh);
		swap(a.me, b.me);
	}
private:
	class b_storage : public H
	{
	public:
		b_storage() noexcept(noexcept(H())) : H(), buckets(nullptr), bucket_count(0) {}
		b_storage(const H& h) : H(h), buckets(nullptr), bucket_count(0) {}
		b_storage(const H& h, size_type bucket_count)
			: H(h), buckets(new bucket_type[bucket_count]({0})), bucket_count(bucket_count) {}
		b_storage(const b_storage& oth)
			: H(oth), buckets(new bucket_type[oth.bucket_count]({0})), bucket_count(oth.bucket_count)
		{
			std::copy(oth.cbegin(), oth.cend(), begin());
		}
		b_storage(b_storage&& oth)
			: H(std::move(oth)), buckets(oth.buckets), bucket_count(oth.bucket_count)
		{
			oth.buckets = nullptr;
			oth.bucket_count = 0;
		}
		~b_storage() { delete[] buckets; }
		
		b_storage& operator=(const b_storage& oth)
		{
			if(this != &oth)
			{
				realloc(oth.mh.size());
				std::copy(oth.cbegin(), oth.cend(), begin());
			}
			return *this;
		}
		b_storage& operator=(b_storage&& oth)
		{
			if(this != &oth)
			{
				delete[] buckets;
				buckets = oth.buckets;
				bucket_count = oth.bucket_count;
				oth.buckets = nullptr;
				oth.bucket_count = 0;
			}
			return *this;
		}
		
		friend void swap(b_storage& a, b_storage& b)
		{
			std::swap(static_cast<H&>(a), static_cast<H&>(b));
			std::swap(a.buckets, b.buckets);
			std::swap(a.bucket_count, b.bucket_count);
		}
		
		bucket_type* begin() noexcept { return buckets; }
		bucket_type* end() noexcept { return buckets+bucket_count; }
		const bucket_type* begin() noexcept const { return buckets; }
		const bucket_type* end() noexcept const { return buckets+bucket_count; }
		const bucket_type* cbegin() noexcept const { return buckets; }
		const bucket_type* cend() noexcept const { return buckets+bucket_count; }
		
		size_type size() noexcept const { return bucket_count; }
		
		void clear() { std::memset(buckets, 0, sizeof(bucket_type)*bucket_count); }
		bool resize(size_type count)
		{
			if(count > bucket_count)
			{
				delete[] buckets;
				buckets = new bucket_type[count]({0});
				bucket_count = count;
				return true;
			}
			return false;
		}
		void realloc(size_type count)
		{
			delete[] buckets;
			buckets = new bucket_type[count]({0});
			bucket_count = count;
		}
		
		bool insert(size_type bucket, size_type i) noexcept
		{
			if(bucket >= bucket_count) return false;
			for(size_type& s : buckets[bucket])
			{
				if(s == 0)
				{
					s=++i;
					return true;
				}
			}
			return false;
		}
	
		std::array<size_type, 8>* buckets;
		size_type bucket_count;
	} mh;
	
	class e_storage : public E
	{
	public:
		e_storage() noexcept(noexcept(E())) : E(), v() {}
		e_storage(const E& e) : E(e), v() {}
		e_storage(const E& e, std::initializer_list<value_type> ilist) : E(e), v(ilist) {}
		template <typename InputIt>
		e_storage(const E& e, InputIt first, InputIt last) : E(e), v(first, last) {}
	
		/*
		std::vector<value_type>& operator*() noexcept { return elements; }
		const std::vector<value_type>& operator*() noexcept const { return elements; }
		*/
		std::vector<value_type>* operator->() noexcept { return &v; }
		const std::vector<value_type>* operator->() noexcept const { return &v; }
		
		friend void swap(e_storage& a, e_storage& b)
		{
			std::swap(static_cast<E&>(a), static_cast<E&>(b);
			std::swap(a.v, b.v);
		}
		
		std::vector<value_type> v;
	} me;
	
	bucket_type& bucket(const K& k) { return mh.buckets[bucket(k)]; }
	
	void rehash()
	{
		mh.clear();
		if(me->size() == 0) return;
		//if no buckets reallocate to vector size divided by 4 and at least 4
		if(mh.bucket_count == 0)
		{
			size_t min_size = me->size()/4 > 4 ? me->size()/4 : 4;
			mh.realloc(min_size);
		}
		for(size_type i = 0; i < me->size(); i++)
		{
			if(!mh.insert(bucket(me.[i].first), i))
			{
				mh.realloc(mh.bucket_count + (mh.bucket_count>>1));
				i = 0;
			}
		}
	}
	
	std::pair<iterator, size_t> find(const K& k)
	{
		size_t b = bucket(k);
		for(size_t idx : mh.buckets[b])
		{
			if(idx>0)
			{
				if(me(me.v[idx-1].first, k))
				{
					return {std::advance(me->begin(), idx-1), b};
				}
			}
		}
		return {me->end(), b};
	}
};
