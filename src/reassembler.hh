#pragma once

#include "byte_stream.hh"

#include <map>
#include <string>
#include <iostream>
#include <vector>

using namespace std;

class Reassembler
{
public:
  explicit Reassembler( ByteStream&& output ) : output_( std::move( output ) ), buffer_(), add_(), delete_() {}


  void insert( uint64_t first_index, std::string data, bool is_last_substring );

  uint64_t bytes_pending() const;

  Reader& reader() { return output_.reader(); }
  const Reader& reader() const { return output_.reader(); }

  const Writer& writer() const { return output_.writer(); }

private:
  ByteStream output_;
  map<uint64_t, string> buffer_;
  map<uint64_t, string> add_;
  vector<uint64_t> delete_;
  uint64_t expect_index_ = 0;
  uint64_t last_index_ = 0;
  bool eof_ = false;
  bool first_input_ = true;
};
