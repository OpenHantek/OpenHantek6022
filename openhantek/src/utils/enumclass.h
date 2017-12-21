// SPDX-License-Identifier: GPL-2.0+

#pragma once

template< typename T >
class Enum
{
public:
   class Iterator
   {
   public:
      Iterator( int value ) :
         m_value( value )
      { }

      T operator*( void ) const
      {
         return (T)m_value;
      }

      void operator++( void )
      {
         ++m_value;
      }

      bool operator!=( Iterator rhs )
      {
         return m_value != rhs.m_value;
      }

   private:
      int m_value;
   };

};

template< typename T >
typename Enum<T>::Iterator begin( Enum<T> )
{
   return typename Enum<T>::Iterator( (int)T::First );
}

template< typename T >
typename Enum<T>::Iterator end( Enum<T> )
{
   return typename Enum<T>::Iterator( ((int)T::Last) + 1 );
}
