#pragma once

#include <cstdio>
#include <cstdlib>

#define TOSTRINGIMPL( x ) #x
#define TOSTRING( x ) TOSTRINGIMPL( x )

#define STATIC_ASSERT( e ) \
    static_assert( e, "[" __FILE_NAME__ ":" TOSTRING( __LINE__ ) "] ASSERT: " #e )

#define RUNTIME_ASSERT( e ) \
    do \
    { \
        if( !( e ) ) \
        { \
            printf( "[" __FILE_NAME__ ":" TOSTRING( __LINE__ ) "] ASSERT: " #e "\n" ); \
            abort( ); \
        } \
    } while( 0 )

#define ASSERT( e ) RUNTIME_ASSERT( e )

#define PANIC( m, ... ) \
    do \
    { \
        printf( "[" __FILE_NAME__ ":" TOSTRING( __LINE__ ) "] PANIC: " m "\n", ##__VA_ARGS__ ); \
        abort( ); \
    } while( 0 )

#define WARN( m, ... ) \
    do \
    { \
        printf( "[" __FILE_NAME__ ":" TOSTRING( __LINE__ ) "] WARN: " m "\n\n", ##__VA_ARGS__ ); \
    } while( 0 )

#define REPORT( m, ... ) \
    do \
    { \
        printf( "[" __FILE_NAME__ ":" TOSTRING( __LINE__ ) "] REPORT: " m "\n", ##__VA_ARGS__ ); \
    } while( 0 )

#ifdef BUILD_DEBUG
#define DEBUG( m, ... ) \
    do \
    { \
        printf( "[" __FILE_NAME__ ":" TOSTRING( __LINE__ ) "] DEBUG: " m "\n", ##__VA_ARGS__ ); \
    } while( 0 )
#else
#define DEBUG( m, ... )
#endif
