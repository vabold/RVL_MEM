#include <egg/eggExpHeap.hh>

#include <vector>

struct DemoStruct : EGG::Disposer
{
    DemoStruct( )
    {
        auto *heap = EGG::Heap::findContainHeap( this );
        ASSERT( heap );
        DEBUG( "DemoStruct instance created at %p in heap %p (%s)", this, heap, heap->getName( ) );
    }

    ~DemoStruct( ) override
    {
        auto *heap = EGG::Heap::findContainHeap( this );
        ASSERT( heap );
        DEBUG( "DemoStruct instance deleted at %p in heap %p (%s)", this, heap, heap->getName( ) );
    }
};

class DemoSingleton : EGG::Disposer
{
public:
    static DemoSingleton *CreateInstance( )
    {
        ASSERT( !s_instance );
        s_instance = new DemoSingleton;
        return s_instance;
    }

    static void DestroyInstance( )
    {
        ASSERT( s_instance );
        auto *instance = s_instance;
        s_instance = nullptr;
        delete instance;
    }

    static DemoSingleton *Instance( )
    {
        return s_instance;
    }

private:
    DemoSingleton( ) = default;
    ~DemoSingleton( ) override
    {
        if( s_instance )
        {
            s_instance = nullptr;
            WARN( "DemoSingleton instance not explicitly handled!" );
        }
    }

    static DemoSingleton *s_instance;
};

static void TestBasic_( )
{
    auto *rootHeap = EGG::ExpHeap::getRootHeap( );
    size_t spacePre = rootHeap->getAllocatableSize( 0x20 );

    // Check if memory is allocated in the memory space
    u32 *pX = new( 0x20 ) u32;
    ASSERT( GetAddrNum( pX ) > GetAddrNum( EGG::Heap::getMemorySpace( ) ) );
    ASSERT( GetAddrNum( pX ) < GetAddrNum( EGG::Heap::getMemorySpace( ) ) + 0x1000000 );

    // Check if memory is being allocated correctly
    ASSERT( *pX == 0xf3f3f3f3 );

    // Check if pointer is aligned correctly
    ASSERT( ( GetAddrNum( pX ) & 0x1f ) == 0 );

    // Check if memory is being freed correctly
    delete pX;
    ASSERT( *pX == 0xd3d3d3d3 );

    // Check if uninitialized memory is filled correctly
    pX = &pX[ 100 ];
    ASSERT( *pX == 0xc3c3c3c3 );

    // Check if heap is unnecessarily fragmented
    size_t spacePost = rootHeap->getAllocatableSize( 0x20 );
    ASSERT( spacePre == spacePost );

    // Check if allocating beyond the heap size fails
    u8 *pY = new u8[ rootHeap->getAllocatableSize( ) + 1 ];
    ASSERT( !pY );
}

static void TestHeap_( )
{
    auto *rootHeap = EGG::ExpHeap::getRootHeap( );
    size_t spacePre = rootHeap->getAllocatableSize( 0x20 );

    // Check if creating a heap with an invalid size fails
    auto *pNoHeap = EGG::ExpHeap::create( rootHeap->getAllocatableSize( ) + 1, rootHeap, 2 );
    ASSERT( !pNoHeap );

    // Check if the heap hierarchy is working correctly
    auto *pHeap0 = EGG::ExpHeap::create( 0x10000, rootHeap, 2 );
    auto *pHeap1 = EGG::ExpHeap::create( 0x10000, rootHeap, 2 );

    pHeap0->setName( "TestHeap0" );
    pHeap1->setName( "TestHeap1" );

    ASSERT( pHeap0->getParentHeap( ) == rootHeap );
    ASSERT( pHeap1->getParentHeap( ) == rootHeap );

    // Check if locking a heap works
    pHeap0->disableAllocation( );

#ifdef COMMENT
    // We need to comment this, because allocating from a locked heap panics
    u32 *pX = new( pHeap0, 0x4 ) u32;
#endif

    // Check if the allocatable heap takes priority
    pHeap1->becomeAllocatableHeap( );
    u32 *pY = new u32;
    ASSERT( !pY );

    // Check if heaps are freed correctly
    pHeap0->destroy( );
    ASSERT( *reinterpret_cast<u32 *>( pHeap0 ) == 0xd3d3d3d3 );

    // Reset the allocatable heap - we need to allocate later
    pHeap0 = nullptr;
    pHeap0->becomeAllocatableHeap( );

    auto *pPrevHeap = pHeap1->becomeCurrentHeap( );

    // Check if the heap hierarchy handles Disposers correctly
    auto *pSubHeap0 = EGG::ExpHeap::create( 0x1000, pHeap1, 2 );
    auto *pSubHeap1 = EGG::ExpHeap::create( 0x1000, pHeap1, 2 );

    pSubHeap0->setName( "TestSubHeap0" );
    pSubHeap1->setName( "TestSubHeap1" );

    DemoStruct *pDemoStruct0 = new( pSubHeap0, 0x4 ) DemoStruct;
    DemoStruct *pDemoStruct1 = new( pSubHeap1, 0x4 ) DemoStruct;

    static_cast<void>( pDemoStruct0 );
    static_cast<void>( pDemoStruct1 );

    // Singletons are more prone to use-after-free, so we want to destroy them with the heap
    // The solution is to have the singleton inherit Disposer, which calls the destructor
    // However, singletons are unique in that their destructors are handled by DestroyInstance
    // The destructor will set the static instance to nullptr and warn that the Disposer handles it
    DemoSingleton::CreateInstance( );
    pHeap1->destroy( );
    ASSERT( !DemoSingleton::Instance( ) );

    pPrevHeap->becomeCurrentHeap( );

    // Check if heap is unnecessarily fragmented
    size_t spacePost = rootHeap->getAllocatableSize( 0x20 );
    ASSERT( spacePre == spacePost );
}

static void TestUse_( )
{
    // Check if std::vector reallocates correctly
    std::vector<int> v;
    v.push_back( 0x55 );
    v.push_back( 0xaa );
    v.push_back( 0x38 );
    v.push_back( 0x2d );

    ASSERT( GetAddrNum( v.data( ) ) > GetAddrNum( EGG::Heap::getMemorySpace( ) ) );
    ASSERT( GetAddrNum( v.data( ) ) < GetAddrNum( EGG::Heap::getMemorySpace( ) ) + 0x1000000 );
}

int main( )
{
#ifdef COMMENT
    // Ensure heaps cannot be created before Heap::initialize
    constexpr size_t PANIC_TEST_SPACE = 0x10000;
    void *addr = malloc( PANIC_TEST_SPACE );
    EGG::ExpHeap::initRootHeap( addr, PANIC_TEST_SPACE );
#endif

    EGG::Heap::initialize( );

    TestBasic_( );
    TestHeap_( );
    TestUse_( );

    REPORT( "Tests successful!" );
}

DemoSingleton *DemoSingleton::s_instance = nullptr;
