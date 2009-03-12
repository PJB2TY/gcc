// { dg-options "-std=gnu++0x" }

// Copyright (C) 2008 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without Pred the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING.  If not, write to the Free
// Software Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
// USA.

// 23.2.3.n forward_list xxx [lib.forward_list.xxx]

#include <forward_list>
#include <testsuite_hooks.h>
#include <ext/extptr_allocator.h>

using __gnu_cxx::_ExtPtr_allocator;

bool test __attribute__((unused)) = true;

//  Comparison functor.
template<typename Num>
  class Comp
  {
  public:
    Comp(const Num & num)
      {
        n = num;
      }
    bool operator()(const Num i, const Num j)
      {
        return (n * i) < (n * j);
      }
  private:
    Num n;
  };

// This test verifies the following:
//   
void
test01()
{
  typedef std::forward_list<int, _ExtPtr_allocator<int> > fwd_list_type;

  const unsigned int n = 13;
  int order[][n] = {
    { 0,1,2,3,4,5,6,7,8,9,10,11,12 },
    { 6,2,8,4,11,1,12,7,3,9,5,0,10 },
    { 12,11,10,9,8,7,6,5,4,3,2,1,0 },
  };
  fwd_list_type sorted(order[0], order[0] + n);

  for (unsigned int i = 0; i < sizeof(order)/sizeof(*order); ++i)
    {
      fwd_list_type head(order[i], order[i] + n);

      head.sort();

      VERIFY(head == sorted);
    }

  fwd_list_type reversed(order[2], order[2] + n);
  for (unsigned int i = 0; i < sizeof(order)/sizeof(*order); ++i)
    {
      fwd_list_type head(order[i], order[i] + n);

      Comp<int> comp(-1);
      head.sort( comp );

      VERIFY(head == reversed);
    }
}

int
main()
{
  test01();
  return 0;
}
