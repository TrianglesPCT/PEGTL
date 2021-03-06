// Copyright (c) 2014-2015 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/ColinH/PEGTL/

#include <cassert>

#include <pegtl.hh>
#include <pegtl/contrib/json.hh>
#include <pegtl/contrib/unescape.hh>

#include "json_errors.hh"
#include "json_changes.hh"
#include "json_classes.hh"

namespace examples
{
   // Basic state class that stores the result of a JSON parsing run -- a single JSON object.

   struct result_state
   {
      std::shared_ptr< json_base > result;
   };

   // State class for parsing literal strings, uses the PEGTL unescape utilities, cf. unescape.cc.

   struct string_state
   {
      string_state( const pegtl::input &, result_state & )
      { }

      void success( const pegtl::input &, result_state & result )
      {
         result.result = std::make_shared< string_json >( unescaped );
      }

      std::string unescaped;
   };

   template< typename Rule > struct string_action : pegtl::nothing< Rule > {};

   template<> struct string_action< pegtl::json::unicode > : pegtl::unescape::unescape_u {};
   template<> struct string_action< pegtl::json::escaped_char > : pegtl::unescape::unescape_c< pegtl::json::escaped_char, '"', '\\', '/', '\b', '\f', '\n', '\r', '\t' > {};
   template<> struct string_action< pegtl::json::unescaped > : pegtl::unescape::append_all {};

   // Action class for the simple cases...

   template< typename Rule > struct value_action : string_action< Rule > {};

   template<>
   struct value_action< pegtl::json::null >
   {
      static void apply( const pegtl::input &, result_state & result )
      {
         result.result = std::make_shared< null_json >();
      }
   };

   template<>
   struct value_action< pegtl::json::true_ >
   {
      static void apply( const pegtl::input &, result_state & result )
      {
         result.result = std::make_shared< boolean_json >( true );
      }
   };

   template<>
   struct value_action< pegtl::json::false_ >
   {
      static void apply( const pegtl::input &, result_state & result )
      {
         result.result = std::make_shared< boolean_json >( false );
      }
   };

   template<>
   struct value_action< pegtl::json::number >
   {
      static void apply( const pegtl::input & in, result_state & result )
      {
         result.result = std::make_shared< number_json >( std::stod( in.string() ) );  // NOTE: stod() is not quite correct for JSON but we'll use it for this example.
      }
   };

   // State and action classes to accumulate the data for a JSON array.

   struct array_state
         : public result_state
   {
      template< typename Input >
      array_state( const Input &, result_state & )
      { }

      std::shared_ptr< array_json > array = std::make_shared< array_json >();

      void push_back()
      {
         array->data.push_back( std::move( result ) );
         result.reset();
      }

      template< typename Input >
      void success( const Input &, result_state & result )
      {
         if ( this->result ) {
            push_back();
         }
         result.result = array;
      }
   };

   template< typename Rule > struct array_action : pegtl::nothing< Rule > {};

   template<>
   struct array_action< pegtl::json::value_separator >
   {
      static void apply( const pegtl::input &, array_state & result )
      {
         result.push_back();
      }
   };

   // State and action classes to accumulate the data for a JSON object.

   struct object_state
         : public result_state
   {
      template< typename Input >
      object_state( const Input &, result_state & )
      { }

      std::string unescaped;
      std::shared_ptr< object_json > object = std::make_shared< object_json >();

      void insert()
      {
         object->data.insert( std::make_pair( std::move( unescaped ), std::move( result ) ) );
         unescaped.clear();
         result.reset();
      }

      template< typename Input >
      void success( const Input &, result_state & result )
      {
         if ( this->result ) {
            insert();
         }
         result.result = object;
      }
   };

   template< typename Rule > struct object_action : value_action< Rule > {};

   template<>
   struct object_action< pegtl::json::value_separator >
   {
      static void apply( const pegtl::input &, object_state & result )
      {
         result.insert();
      }
   };

   // Put together a control class that changes the actions and states as required.

   template< typename Rule > struct control : errors< Rule > {};  // Inherit from json_errors.hh.

   template<> struct control< pegtl::json::value > : change_action< pegtl::json::value, value_action, errors > {};
   template<> struct control< pegtl::json::string > : change_state< pegtl::json::string, string_state, errors > {};
   template<> struct control< pegtl::json::array > : change_state_and_action< pegtl::json::array, array_state, array_action, errors > {};
   template<> struct control< pegtl::json::object > : change_state_and_action< pegtl::json::object, object_state, object_action, errors > {};

   struct grammar : pegtl::must< pegtl::json::text, pegtl::eof > {};

} // examples

int main( int argc, char ** argv )
{
   for ( int i = 1; i < argc; ++i ) {
      examples::result_state result;
      pegtl::read_parser( argv[ i ] ).parse< examples::grammar, pegtl::nothing, examples::control >( result );
      assert( result.result );
      std::cout << result.result << std::endl;
   }
   return 0;
}
