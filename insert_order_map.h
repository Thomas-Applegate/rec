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
#include <type_traits>

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
	typedef typename std::vector<value_type>::iterator iterator;
	typedef typename std::vector<value_type>::const_iterator const_iterator;
	typedef std::array<size_type, 8> bucket_type;
		
private:
	class b_storage : public H
	{
	public:
		b_storage() noexcept(noexcept(H())) : H(), buckets(nullptr), bucket_count(0) {}
		b_storage(size_type bucket_count) : H(), buckets(new bucket_type[bucket_count]()),
			bucket_count(bucket_count) {}
		b_storage(const H& h) noexcept(std::is_nothrow_copy_constructible_v<H>)
			: H(h), buckets(nullptr), bucket_count(0) {}
		b_storage(const H& h, size_type bucket_count)
			: H(h), buckets(new bucket_type[bucket_count]()), bucket_count(bucket_count) {}
		b_storage(const b_storage& oth)
			: H(oth), buckets(new bucket_type[oth.bucket_count]()), bucket_count(oth.bucket_count)
		{
			std::copy(oth.cbegin(), oth.cend(), begin());
		}
		b_storage(b_storage&& oth) noexcept(std::is_nothrow_move_constructible_v<H>)
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
				H::operator=(oth);
				resize(oth.bucket_count);
				clear();
				std::copy(oth.cbegin(), oth.cend(), begin());
			}
			return *this;
		}
		
		b_storage& operator=(b_storage&& oth) noexcept(std::is_nothrow_move_assignable_v<H>)
		{
			if(this != &oth)
			{
				H::operator=(std::move(oth));
				delete[] buckets;
				buckets = oth.buckets;
				bucket_count = oth.bucket_count;
				oth.buckets = nullptr;
				oth.bucket_count = 0;
			}
			return *this;
		}
		
		void swap(b_storage& oth) noexcept(std::is_nothrow_swappable_v<H>)
		{
			using std::swap;
			swap(static_cast<H&>(*this), static_cast<H&>(oth));
			swap(buckets, oth.buckets);
			swap(bucket_count, oth.bucket_count);
		}
		
		bucket_type* begin() noexcept { return buckets; }
		bucket_type* end() noexcept { return buckets+bucket_count; }
		const bucket_type* begin() const noexcept { return buckets; }
		const bucket_type* end() const noexcept { return buckets+bucket_count; }
		const bucket_type* cbegin() const noexcept { return buckets; }
		const bucket_type* cend() const noexcept { return buckets+bucket_count; }
		
		size_type size() const noexcept { return bucket_count; }
		
		void clear() { std::memset(buckets, 0, sizeof(bucket_type)*bucket_count); }
		bool resize(size_type count)
		{
			if(count > bucket_count)
			{
				delete[] buckets;
				buckets = new bucket_type[count]();
				bucket_count = count;
				return true;
			}
			return false;
		}
		void realloc(size_type count)
		{
			delete[] buckets;
			buckets = new bucket_type[count]();
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
	
		bucket_type* buckets;
		size_type bucket_count;
	};
	
	class e_storage : public E
	{
	public:
		e_storage() noexcept(noexcept(E()) && noexcept(std::vector<value_type>())) = default;
		
		e_storage(const E& e) noexcept(std::is_nothrow_copy_constructible_v<E>
			&& noexcept(std::vector<value_type>())) : E(e), v() {}
		
		e_storage(std::initializer_list<value_type> ilist) : E(), v(ilist) {}
		e_storage(const E& e, std::initializer_list<value_type> ilist) : E(e), v(ilist) {}
		
		template <typename InputIt>
		e_storage(InputIt first, InputIt last) : E(), v(first, last) {}
		template <typename InputIt>
		e_storage(const E& e, InputIt first, InputIt last) : E(e), v(first, last) {}
		
		e_storage(const e_storage& oth) = default;
		e_storage(e_storage&& oth) noexcept(std::is_nothrow_move_constructible_v<E>
			&& std::is_nothrow_move_constructible_v<std::vector<value_type>>) = default;
		
		e_storage& operator=(const e_storage& oth)
		{
			if(this != &oth)
			{
				E::operator=(oth);
				std::vector<value_type> tmp(oth.v);
				v.swap(tmp);
			}
			return *this;
		}
		e_storage& operator=(e_storage&& oth) noexcept(std::is_nothrow_move_assignable_v<E>
			&& std::is_nothrow_move_assignable_v<std::vector<value_type>>) = default;
	
		/*
		std::vector<value_type>& operator*() noexcept { return elements; }
		const std::vector<value_type>& operator*() noexcept const { return elements; }
		*/
		std::vector<value_type>* operator->() noexcept { return &v; }
		const std::vector<value_type>* operator->() const noexcept { return &v; }
		
		void swap(e_storage& oth)
			noexcept(std::is_nothrow_swappable_v<E>
			&& std::is_nothrow_swappable_v<std::vector<value_type>>)
		{
			using std::swap;
			swap(static_cast<E&>(*this), static_cast<E&>(oth));
			v.swap(oth.v);
		}
		
		std::vector<value_type> v;
	};
	
	void rehash()
	{
		if(me->size() == 0) return;
		if(mh.buckets == nullptr) //empty bucket array
		{
			mh.realloc(4);
		}else
		{
			mh.clear();
		}
		for(size_type i = 0; i < me->size(); i++)
		{
			if(!mh.insert(bucket(me.v[i].first), i))
			{
				mh.realloc(mh.bucket_count + (mh.bucket_count>>1));
				i = 0;
			}
		}
	}
	
	std::pair<iterator, size_t> lookup(const K& k)
	{
		size_type b = bucket(k);
		if(mh.buckets != nullptr)
		{
			for(size_t idx : mh.buckets[b])
			{
				if(idx>0)
				{
					if(me(me.v[idx-1].first, k))
					{
						return {std::next(me->begin(), idx-1), b};
					}
				}
			}
		}
		return {me->end(), b};
	}
	
	b_storage mh;
	e_storage me;
	
public:
	
	insert_order_map() noexcept(noexcept(b_storage()) && noexcept(e_storage()))
		: mh(), me() {}
	insert_order_map(const H& h, const E& e)
		noexcept(noexcept(b_storage(h)) && noexcept(e_storage(e)))
		: mh(h), me(e) {}
	explicit insert_order_map(size_type bucket_count) : mh(bucket_count), me() {}
	explicit insert_order_map(size_type bucket_count, const H& h, const E& e)
		: mh(h, bucket_count), me(e) {}
	insert_order_map(std::initializer_list<value_type> ilist) : mh(), me(ilist)
	{ rehash(); }
	insert_order_map(std::initializer_list<value_type> ilist, const H& h, const E& e)
		: mh(h), me(e, ilist) { rehash(); }
	
	template <typename InputIt>
	insert_order_map(InputIt first, InputIt last)
		: mh(), me(first, last) { rehash(); }
	template <typename InputIt>
	insert_order_map(InputIt first, InputIt last, const H& h, const E& e)
		: mh(h), me(e, first, last) { rehash(); }
	
	insert_order_map(const insert_order_map& oth) = default;
	insert_order_map(insert_order_map&& oth)
		noexcept(std::is_nothrow_move_constructible_v<b_storage>
		&& std::is_nothrow_move_constructible_v<e_storage>) = default;
		
	
	template<typename HOTH>
	insert_order_map(const insert_order_map<K, V, HOTH, E>& oth)
		: mh(oth.mh.bucket_count), me(oth.me) { rehash(); }
	template<typename HOTH>
	insert_order_map(const insert_order_map<K, V, HOTH, E>& oth, const H& h)
		: mh(h, oth.mh.bucket_count), me(oth.me) { rehash(); }
	template<typename HOTH>
	insert_order_map(insert_order_map<K, V, HOTH, E>&& oth) : mh(), me(std::move(oth.me))
	{
		mh.buckets = oth.mh.buckets;
		mh.bucket_count = oth.mh.bucket_count;
		oth.mh.buckets = nullptr;
		oth.mh.bucket_count = 0;
		rehash();
	}
	template<typename HOTH>
	insert_order_map(insert_order_map<K, V, HOTH, E>&& oth, const H& h)
	: mh(h), me(std::move(oth.me))
	{
		mh.buckets = oth.mh.buckets;
		mh.bucket_count = oth.mh.bucket_count;
		oth.mh.buckets = nullptr;
		oth.mh.bucket_count = 0;
		rehash();
	}
		
	insert_order_map& operator=(const insert_order_map& oth) = default;
	insert_order_map& operator=(insert_order_map&& oth)
		noexcept(std::is_nothrow_move_assignable_v<b_storage>
		&& std::is_nothrow_move_assignable_v<e_storage>) = default;
	
	insert_order_map& operator=(std::initializer_list<value_type> ilist)
	{
		me.v = ilist;
		rehash();
		return *this;
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
		auto [it, b] = lookup(val.first);
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
		auto [it, b] = lookup(val.first);
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
	std::pair<iterator, bool> emplace_back(const K& key, Args... args)
	{
		auto [it, b] = lookup(key);
		if(it == me->end())
		{
			me->emplace_back(key, std::forward<Args>(args)...);
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
		auto [it, b] = lookup(key);
		if(it == me->end())
		{
			me->emplace_back(std::move(key), std::forward<Args>(args)...);
			if(!mh.insert(b, me->size())) rehash();
			return {std::prev(me->end()), true};
		}else
		{
			return {it, false};
		}
	}
	
	template <typename... Args>
	std::pair<iterator, bool> emplace_back(Args... args)
	{
		me->emplace_back(std::forward<Args>(args)...);
		auto [it, b] = lookup(me->back().first);
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
		for(size_t& idx : mh.buckets[bucket(me->back().first)])
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
		auto [it, b] = lookup(k);
		if(k == me->end())
		{
			me->emplace_back(k, std::forward<M>(obj));
			if(!mh.insert(b, me->size())) rehash();
			return {std::prev(me->end()), true};
		}else{
			it->second = std::forward<M>(obj);
			return {it, false};
		}
	}
	
	template <typename M>
	std::pair<iterator, bool> insert_or_assign(K&& k, M&& obj)
	{
		auto [it, b] = lookup(k);
		if(k == me->end())
		{
			me->emplace_back(std::move(k), std::forward<M>(obj));
			if(!mh.insert(b, me->size())) rehash();
			return {std::prev(me->end()), true};
		}else{
			it->second = std::forward<M>(obj);
			return {it, false};
		}
	}
	
	template <typename M>
	std::pair<iterator, bool> insert_or_assign(const_iterator pos, const K& k, M&& obj)
	{
		auto [it, b] = lookup(k);
		if(k == me->end())
		{
			me->emplace(pos, k, std::forward<M>(obj));
			if(!mh.insert(b, me->size())) rehash();
			return {std::prev(me->end()), true};
		}else{
			it->second = std::forward<M>(obj);
			return {it, false};
		}
	}
	
	template <typename M>
	std::pair<iterator, bool> insert_or_assign(const_iterator pos, K&& k, M&& obj)
	{
		auto [it, b] = lookup(k);
		if(k == me->end())
		{
			me->emplace(pos, std::move(k), std::forward<M>(obj));
			if(!mh.insert(b, me->size())) rehash();
			return {std::prev(me->end()), true};
		}else{
			it->second = std::forward<M>(obj);
			return {it, false};
		}
	}
	
	template <typename... Args>
	std::pair<iterator, bool> emplace(const_iterator pos, K&& key, Args... args)
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
	std::pair<iterator, bool> emplace(const_iterator pos, Args... args)
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
		return emplace(std::next(me->begin(), idx), args...);
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
	
	void swap(insert_order_map& oth) noexcept(std::is_nothrow_swappable_v<b_storage>
		&& std::is_nothrow_swappable_v<e_storage>)
	{
		mh.swap(oth.mh);
		me.swap(oth.me);
	}
	
	V& at(const K& k)
	{
		for(size_t idx : mh.buckets[bucket(k)])
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
		for(size_t idx : mh.buckets[bucket(k)])
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
			me->emplace_back(k, V());
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
			me->emplace_back(std::move(k), V());
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
		for(size_t idx : mh.buckets[bucket(k)])
		{
			if(idx>0)
			{
				if(me(me.v[idx-1].first, k))
				{
					return std::next(me->begin(), idx-1);
				}
			}
		}
		return me->end();
	}
	
	const_iterator find(const K& k) const
	{
		for(size_t idx : mh.buckets[bucket(k)])
		{
			if(idx>0)
			{
				if(me(me.v[idx-1].first, k))
				{
					return std::next(me->cbegin(), idx-1);
				}
			}
		}
		return me->cend();
	}
	
	bool contains(const K& k) const
	{
		for(size_t idx : mh.buckets[bucket(k)])
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
		if(mh.buckets == nullptr) return 0;
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
};

template <typename K, typename V, typename H, typename E>
void swap(insert_order_map<K,V,H,E>& a, insert_order_map<K,V,H,E>& b)
	noexcept(noexcept(a.swap(b)))
{
	a.swap(b);
}
