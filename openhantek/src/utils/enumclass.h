// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

template < typename T, T first, T last > class Enum {
  public:
    class Iterator {
      public:
        Iterator( int value ) : m_value( value ) {}

        T operator*( void ) const { return T( m_value ); }

        void operator++( void ) { ++m_value; }

        bool operator!=( Iterator rhs ) { return m_value != rhs.m_value; }

      private:
        int m_value;
    };
};

template < typename T, T first, T last > typename Enum< T, first, last >::Iterator begin( Enum< T, first, last > ) {
    return typename Enum< T, first, last >::Iterator( int( first ) );
}

template < typename T, T first, T last > typename Enum< T, first, last >::Iterator end( Enum< T, first, last > ) {
    return typename Enum< T, first, last >::Iterator( int( last ) + 1 );
}
