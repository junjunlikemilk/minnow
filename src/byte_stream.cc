#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

void Writer::push( string data )
{
  if (input_end_)
  {
    return ;
  }
  size_t push_count=min(data.size(),capacity_-buffer_.size());

  write_bytes_+=push_count;
  buffer_.insert(buffer_.end(),data.begin(),data.begin()+push_count);
}

void Writer::close()
{
  // Your code here.
  input_end_=true;
}

bool Writer::is_closed() const
{
  return input_end_;
}

uint64_t Writer::available_capacity() const
{
  return capacity_-buffer_.size(); // Your code here.
}

uint64_t Writer::bytes_pushed() const
{
  return write_bytes_; // Your code here.
}

string_view Reader::peek() const
{
  return std::string_view(buffer_); // Your code here.
}

void Reader::pop( uint64_t len )
{

  // Your code here.
  ssize_t pop_count=min(len,buffer_.size());
  read_bytes_+=pop_count;
  buffer_.erase(buffer_.begin(),buffer_.begin()+pop_count);
}

bool Reader::is_finished() const
{
  if(buffer_.empty()&&input_end_)
  {
    return true;
  }else return false;
   // Your code here.
}
 
uint64_t Reader::bytes_buffered() const
{
  return buffer_.size(); // Your code here.
}

uint64_t Reader::bytes_popped() const
{
  return read_bytes_; // Your code here.
}

