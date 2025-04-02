#include "reassembler.hh"
#include "debug.hh"
#include <algorithm>
using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{

  // debug( "unimplemented insert({}, {}, {}) called", first_index, data, is_last_substring );
  Writer output = output_.writer();
  const size_t unacceptable_index = expected_index_ + output.available_capacity();
  if ( output.is_closed() || first_index >= unacceptable_index || output.available_capacity() == 0 ) {
    return;
  } else if ( first_index + data.size() >= unacceptable_index ) {
    /* code */
    is_last_substring = false;
    data.resize( unacceptable_index - first_index );
  }
  if ( first_index > expected_index_ ) {
    cache_bytes( first_index, std::move( data ), is_last_substring );
  } else
    push_bytes( first_index, std::move( data ), is_last_substring );

  flush_buffer();
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  // debug( "unimplemented count_bytes_pending() called" );
  return pending_size_;
}

void Reassembler::push_bytes( size_t fisrt_index, std::string data, bool is_last_substring )
{
  if ( fisrt_index < expected_index_ ) {
    data.erase( 0, expected_index_ - fisrt_index );
    expected_index_ += data.length();
    output_.writer().push( std::move( data ) );
    if ( is_last_substring ) {
      buffer_.clear();
      pending_size_ = 0;
      output_.writer().close();
    }
  }
}

void Reassembler::cache_bytes( size_t first_index, std::string data, bool is_last_substring )
{
  auto end = buffer_.end();
  auto left = lower_bound( buffer_.begin(), end, first_index, []( auto&& e, size_t idx ) -> bool {
    return idx > ( std::get<0>( e ) + std::get<1>( e ).size() );
  } );
  auto right = upper_bound( left, end, first_index + data.size(), []( uint64_t nxt_idx, auto&& e ) -> bool {
    return nxt_idx < get<0>( e );
  } );
  if ( const uint64_t next_index = first_index + data.size(); left != end ) {
    auto& [l_point, dat, _] = *left;
    if ( const uint64_t r_point = l_point + dat.size(); first_index >= l_point && next_index <= r_point )
      return; // data 已经存在
    else if ( next_index < l_point ) {
      right = left;                                                      // data 和 dat 没有重叠部分
    } else if ( !( first_index <= l_point && r_point <= next_index ) ) { // 重叠了
      if ( first_index >= l_point ) {                                    // 并且 dat 没有被完全覆盖
        data.insert( 0, string_view( dat.c_str(), dat.size() - ( r_point - first_index ) ) );
      } else {
        data.resize( data.size() - ( next_index - l_point ) );
        data.append( dat ); // data 在前
      }
      first_index = min( first_index, l_point );
    }
  }
  if ( const uint64_t next_index = first_index + data.size(); right != left && !buffer_.empty() ) {
    // 如果 right 指向 left，表示两种可能：没有重叠区间、或者只需要合并 left 这个元素
    auto& [l_point, dat, _] = *prev( right );
    if ( const uint64_t r_point = l_point + dat.size(); r_point > next_index ) {
      data.resize( data.size() - ( next_index - l_point ) );
      data.append( dat );
    }
  }

  for ( ; left != right; left = buffer_.erase( left ) ) {
    pending_size_ -= get<1>( *left ).size();
    is_last_substring |= get<2>( *left );
  }
  pending_size_ += data.size();
  buffer_.insert( left, { first_index, move( data ), is_last_substring } );
}

void Reassembler::flush_buffer() 
{
  while ( !buffer_.empty() ) {
    auto& [idx, dat, last] = buffer_.front();
    if ( idx > expected_index_ )
      break;                          // 乱序的，不做任何动作
    pending_size_ -= dat.size(); // 数据已经被填补上了，立即推入写端
    push_bytes( idx, move( dat ), last );
    if ( !buffer_.empty() )
      buffer_.pop_front();
  }
}
