#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : buffer(), capacity_( capacity ){}

void Writer::push( string data )
{
  // Your code here.
  uint64_t len = data.size();  
  if (len > available_capacity()) {
    int count = (int)available_capacity();
    for (int i = 0; i < count; i++) {
      buffer.push_back(data[i]); // Push data to the buffer
      amount_pushed++;
    }
    return;
  }
  for (char c : data) {
    buffer.push_back(c);
  }
  amount_pushed += len; // Update the amount pushed
}

void Writer::close()
{
  // Your code here.
  closed = true;
   
}

bool Writer::is_closed() const
{
   // Your code here.
   return closed;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - (uint64_t) buffer.size();

}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return amount_pushed;
}

string_view Reader::peek() const
{
  // Your code here.
  return string_view(buffer.data(), buffer.size());
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  if (len >= buffer.size()) {
    amount_poped += buffer.size(); // Update the amount popped
    buffer.clear(); // Clear the buffer
    return;
  }
  buffer.erase(buffer.begin(), buffer.begin() + len); // Remove `len` bytes from the buffer
  amount_poped += len; // Update the amount popped
}

bool Reader::is_finished() const
{
  // Your code here.
  return closed && buffer.empty();
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return buffer.size();
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return amount_poped;
}
