#include "reassembler.hh"
#include <iostream>
#include <utility>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // Your code here.
  uint64_t capacity = output_.writer().available_capacity();
  // 可以接受的序号范围为[current_index, current_index+capacity)  左闭右开
  // data中数据的序号范围为[first_index, first_index+data.size())
  // 二者取交集，若为空说明该串过期或者太早到来
  uint64_t left_bound = max( first_index, current_index_ );
  uint64_t right_bound = min( current_index_ + capacity, first_index + data.size() );
  if ( right_bound < left_bound ) { // 相等为空串，也能接受（可能标志了last_string）
    return;                         // 对于buffer_没有更新操作，后续不会向缓冲区写入
  }

  reassembler_item item = reassembler_item( data.substr( left_bound - first_index, right_bound - left_bound ),
                                            left_bound,
                                            right_bound,
                                            is_last_substring && right_bound == first_index + data.size() );
  pending_size_ += item.data.size(); // 先全部加进去，后面根据覆盖的内容再移除
  auto insert_iter = lower_bound( buffer_.begin(), buffer_.end(), item );
  // 先判断item是否向后覆盖了其它已插入buffer_的数据,如果有则合并
  auto iter = insert_iter;
  while ( iter != buffer_.end() && item.last_index >= iter->first_index ) {
    if ( item.last_index < iter->last_index ) { // 只有部分覆盖才要合并，全覆盖直接erase即可
      item.data += iter->data.substr( item.last_index - iter->first_index );
      // 覆盖长度为item_last-iter_first
      pending_size_ -= item.last_index - iter->first_index;
      item.last_index = iter->last_index;
      item.is_last |= iter->is_last;
    } else {
      pending_size_ -= iter->data.size();
    }
    iter = buffer_.erase( iter );
  }
  // 再判断前一个数据是否覆盖了item
  // 被前一个覆盖直接在前一个元素中修改，而不需要再插入item了
  if ( insert_iter != buffer_.begin() ) {
    iter = insert_iter - 1;
    if ( iter->last_index >= item.first_index ) {
      if ( iter->last_index < item.last_index ) { // 非完全覆盖
        iter->data += item.data.substr( iter->last_index - item.first_index );
        pending_size_ -= iter->last_index - item.first_index;
        iter->last_index = item.last_index;
        iter->is_last |= item.is_last;
      } else { // 完全覆盖
        pending_size_ -= item.data.size();
      }
      // 没插入，不需要删除的代码
      // 直接return，不要运行后面插入insert代码
      return;
    }
  }
  // insert item into buffer_
  buffer_.insert( insert_iter, item );
  // 只有插入了新的item，才有可能需要向缓冲区写入
  if ( buffer_[0].first_index == current_index_ ) {
    auto& to_write_item = buffer_[0];
    output_.writer().push( to_write_item.data );
    pending_size_ -= to_write_item.data.size();
    current_index_ = to_write_item.last_index;
    if ( to_write_item.is_last ) {
      output_.writer().close();
    }
    buffer_.erase( buffer_.begin() );
  }
}

uint64_t Reassembler::count_bytes_pending() const
{
  return pending_size_;
}
