#pragma once
#include <unordered_map>

template <typename TO, typename FROM, typename KEY, typename H, typename E, typename A>
std::unordered_map<KEY, TO, H, E, typename std::allocator_traits<A>::template rebind_alloc<std::pair<const KEY, TO>>>
convert_map_to(const std::unordered_map<KEY, FROM, H, E, A>& in)
{
	std::unordered_map<KEY, TO, H, E, typename std::allocator_traits<A>::template rebind_alloc<std::pair<const KEY, TO>>>
		out(in.bucket_count(), in.hash_function(), in.key_eq(), in.get_allocator());

	for(const auto&[k,v] : in)
	{
		out.emplace(std::move(k), TO(std::move(v)));
	}
	return out;
}
