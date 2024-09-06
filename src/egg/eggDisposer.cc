#include <egg/eggDisposer.hh>

#include <egg/eggHeap.hh>

namespace EGG
{

Disposer::Disposer( )
{
    mHeap = Heap::findContainHeap( this );
    if( mHeap )
    {
        mHeap->appendDisposer( this );
    }
}

Disposer::~Disposer( )
{
    if( mHeap )
    {
        mHeap->removeDisposer( this );
    }
}

} // namespace EGG
