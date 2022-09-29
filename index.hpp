#pragma once

/// \file index.hpp Types to support compile-time index generation to map between strings and numbers for enumerated types

#include "estd.hpp"
#include "estring.hpp"
#include "array.hpp"

namespace index
{
  using estd::string_view;
  
  /// \brief Structure representing a value with an associated name (e.g. an enumeration)
  template<class T>
  struct named_value { 
      T value; 
      string_view name; 
      
      static constexpr bool compare_value(const named_value& l, const named_value& r)
      {
        return l.value < r.value;
      }
      
      static constexpr bool compare_name(const named_value& l, const named_value& r)
      {
        return l.name < r.name;
      }
    };
  
  /// \brief Values indexed by string names
  template<class T, std::size_t Count>
  struct name_index
  {
    typedef named_value<T> value_type;
    typedef const named_value<T>* pointer;
    
     /// \brief Create an index by sorting the input array by name
     constexpr name_index(estd::array<named_value<T>, Count>&& strings)
       : lookup_values_{estd::sort(std::move(strings), named_value<T>::compare_name)}
       {}
       
    /// \brief Find an enumerated value from a string value, using binary search (O(log(n)))
    constexpr pointer find(const string_view& string) const
    {
      auto it = 
        estd::lower_bound(std::begin(lookup_values_), std::end(lookup_values_), string,
                       [](const value_type& l, const string_view& string) -> bool { return l.name < string; });
      
      return it != std::end(lookup_values_) && it->name == string ? &(*it) : nullptr;
    }
    
    /// \brief Get an enumerated value, or a default value if no value is found
    constexpr const T& get(const string_view& string, const T& default_value) const
    {
      auto it = 
        estd::lower_bound(std::begin(lookup_values_), std::end(lookup_values_), string,
                       [](const value_type& l, const string_view& string) -> bool { return l.name < string; });
      return it != std::end(lookup_values_) && it->name == string ? it->value : default_value;
    }
    
  private:
     /// \brief Stored index, sorted by name
     estd::array<value_type, Count> lookup_values_;
  };
  
  /// \brief Names indexed by value
  template<class T, std::size_t Count>
  struct value_index
  {
     typedef named_value<T> value_type;
     
     typedef estd::array<named_value<T>, Count> index_type;
     typedef typename index_type::const_iterator iterator;
    
     /// \brief Create an index by sorting the input array by value
     constexpr value_index(estd::array<named_value<T>, Count> strings)
       : lookup_values_{estd::sort(std::move(strings), named_value<T>::compare_value)}
       {}
       
    /// Find an enmerated value from a numeric value, using binary search (O(log(n)))
    constexpr const named_value<T>* find(T value) const
    {
      auto it = 
        estd::lower_bound(std::begin(lookup_values_), std::end(lookup_values_), value,
                          [](const value_type& l, T value) -> bool { return l.value < value; } );
      return it != std::end(lookup_values_) && it->value == value ? &(*it) : nullptr;
    }
  private:
    /// \brief Stored index sorted by value
     estd::array<named_value<T> , Count> lookup_values_;
  };
  
  /// \brief Bidirectional index between names and enumerated values
  template<class T, std::size_t Count>
  struct enumeration_index : public name_index<T, Count>, public value_index<T, Count>
  {
    typedef named_value<T> value_type;
    
    typedef estd::array<named_value<T>, Count> index_type;
    
    typedef typename index_type::const_iterator iterator;
    
    /// \brief Make a pair of indices
    constexpr enumeration_index(index_type&& strings)
      : value_index<T, Count>{strings},
        name_index<T, Count>{strings}
    {}
    
    constexpr const named_value<T>* find(const string_view& string) const { return name_index<T, Count>::find(string); }
    constexpr const named_value<T>* find(T value) const { return value_index<T, Count>::find(value); }
  };
  
  /// \brief Helper function for creating an enumeration index
  template<class EnumType, std::size_t N>
  constexpr enumeration_index<EnumType,N> make_enumeration_index(named_value<EnumType> (&&strings)[N]) NOEXCEPT
  {
    return enumeration_index<EnumType,N>(estd::to_array(strings));
  }
  
  /// \brief Helper function for creating a named value
  template<class EnumType>
  constexpr named_value<EnumType> make_name(EnumType type, string_view&& view) NOEXCEPT
  {
    return named_value<EnumType>{type, std::move(view)};
  }
  
  /// \brief Helper function for creating an index by name
  template<class T, class... Ts, size_t Count=sizeof...(Ts)+1>
  constexpr name_index<T, Count> make_name_index(named_value<T> t, named_value<Ts>... ts) NOEXCEPT
  {
    static_assert(estd::conjunction<std::is_same<T, Ts>...>::value, "Arguments are not the same type");
    return name_index<T, Count>{estd::array<named_value<T>, Count>{t, ts...}};
  }

  /// \brief Helper function for creating an index by name
  template<class T, class... Ts, size_t Count=sizeof...(Ts)+1>
  constexpr value_index<T, Count> make_value_index(named_value<T> t, named_value<Ts>... ts) NOEXCEPT
  {
    //static_assert(estd::conjunction<std::is_same<T, Ts>...>::value, "Arguments are not the same type");
    return value_index<T, Count>{estd::array<named_value<T>, Count>{t, ts...}};
  }
}