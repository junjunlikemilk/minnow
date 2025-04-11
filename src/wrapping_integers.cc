#include "wrapping_integers.hh"
#include "debug.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  // debug( "unimplemented wrap( {}, {} ) called", n, zero_point.raw_value_ );
  return Wrap32 { zero_point.raw_value_+static_cast<uint32_t>(n)};
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  // debug( "unimplemented unwrap( {}, {} ) called", zero_point.raw_value_, checkpoint );
  Wrap32 warpCheckpoint=wrap(checkpoint,zero_point);
  uint32_t diff=raw_value_-warpCheckpoint.raw_value_;
  const uint64_t upper=static_cast<uint64_t>(UINT32_MAX)+1;
  if(diff<=(upper>>1)||checkpoint+diff<upper)
  {
    return checkpoint+diff;
  }
  return checkpoint+diff-upper;
}
