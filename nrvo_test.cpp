#include <cstdio>                // printf, puts
#include <cstring>               // strcpy, strcmp
#include <new>                   // placement 'new'
#include <optional>              // optional

#include "nrvo.hpp"

unsigned counter = 0u;

struct Mutex {
    char name[64u];
    int val, val2;

    // ---------- cannot move and cannot copy ----------
    Mutex(Mutex       &&)            = delete;
    Mutex(Mutex const & )            = delete;
    Mutex &operator=(Mutex       &&) = delete;
    Mutex &operator=(Mutex const & ) = delete;

    Mutex(char const *const arg_name, int const arg_num) : val(arg_num), val2(arg_num * 3)
    {
        if ( 0 == std::strcmp(arg_name,"ThrowBeforeFullyConstructed") )
        {
            std::printf("construct '%s' - %i %i - OBJECT NOT CONSTRUCTED\n", name, val++, val2 -= 6);
            throw -1;
        }

        std::strcpy(name, arg_name);
        unsigned const len = std::strlen(name);
        name[ len + 0 ] = '0' + ++counter;
        name[ len + 1 ] = '\0';
        std::printf("construct '%s' - %i %i\n", name, val++, val2 -= 6);
    }

    void lock(void)   { std::printf("lock '%s' - %i %i\n"  , name, val++, val2 -= 7); }
    void unlock(void) { std::printf("unlock '%s' - %i %i\n", name, val++, val2 -= 8); }

    ~Mutex(void)
    {
        std::printf("destroy '%s' - %i %i\n", name, val++, val2 -= 9);
    }
};

Mutex GiveMeMutex(int   a1, int   a2, int   a3, int   a4, int   a5, int   a6, int   a7, int   a8, int   a9, int   a10,
                  float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8, float f9, float f10)
{
    std::printf("Args: %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i\n",
           a1,a2,a3,a4,a5,a6,a7,a8,a9,a10, (int)f1,(int)f2,(int)f3,(int)f4,(int)f5,(int)f6,(int)f7,(int)f8,(int)f9,(int)f10 );

    //cout << "Inside GiveMeLockedMutex: About to placement new at address: " << (void*)pm << endl;

    return Mutex("FromGiveMeMutex",3);
}

void GiveMeLockedMutex(Mutex *const pm,
                       int   a1, int   a2, int   a3, int   a4, int   a5, int   a6, int   a7, int   a8, int   a9, int   a10,
                       float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8, float f9, float f10)
{
    std::printf("Args: %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i\n",
           a1,a2,a3,a4,a5,a6,a7,a8,a9,a10, (int)f1,(int)f2,(int)f3,(int)f4,(int)f5,(int)f6,(int)f7,(int)f8,(int)f9,(int)f10 );

    if ( nullptr == pm )
    {
        std::puts("- - - - - - You have more debugging to do! 'pm' is a nullptr\n");
        // let it crash
    }

    //std::printf("- - - - - - About to construct object at: 0x%llu\n", (long long unsigned)pm);
    //cout << "Inside GiveMeLockedMutex: About to placement new at address: " << (void*)pm << endl;

    if ( 54321 == a1 ) ::new(pm) Mutex("ThrowBeforeFullyConstructed", 8);   // this will throw

    ::new(pm) Mutex("FromGiveMeLockedMutex", 7);

    try
    {
        pm->lock();
        if ( 12345 == a1 ) throw -1;
    }
    catch(...)
    {
        pm->~Mutex();
        throw;
    }
}

std::optional<Mutex> myglobal;

int main(int const argc, char **const argv)
{
    std::puts("First line in main");

    auto mtx = nrvo(GiveMeLockedMutex,3,4,5,6,7,8,9,10,1,2,13.0f,14.0f,15.0f,16.0f,17.0f,18.0f,19.0f,20.0f,11.0f,12.0f);

    //cout << "Inside main: Address of retval = " << (void*)&mtx << endl;

    mtx.unlock();

#ifdef __BORLANDC__
    // The Embarcadero (formerly Borland) compiler cannot deduce the
    // template parameters for the 'std::function' object
    std::function<decltype(GiveMeLockedMutex)> f1(GiveMeLockedMutex);
    std::function<decltype(GiveMeMutex      )> f2(GiveMeMutex      );
#else
    std::function f1(GiveMeLockedMutex);
    std::function f2(GiveMeMutex      );
#endif

    auto mtx2 = nrvo(f1,83,84,85,86,87,88,89,810,81,82,813.0f,814.0f,815.0f,816.0f,817.0f,818.0f,819.0f,820.0f,811.0f,812.0f);

    alignas(Mutex) char unsigned buf[ sizeof(Mutex) ];
    //printf("Address of buf = %llu\n", (long long unsigned)&buf);

    PutRetvalIn(buf)(GiveMeMutex)( 1,2,3,4,5,6,7,8,9,10,11.0f,12.0f,13.0f,14.0f,15.0f,16.0f,17.0f,18.0f,19.0f,20.0f );
    Mutex *const pmtx = static_cast<Mutex*>(static_cast<void*>(&buf));
    pmtx->lock();
    pmtx->unlock();
    pmtx->~Mutex();

    PutRetvalIn(buf)(f2)( 71,72,73,74,75,76,77,78,79,710,711.0f,712.0f,713.0f,714.0f,715.0f,716.0f,717.0f,718.0f,719.0f,720.0f );
    pmtx->lock();
    pmtx->unlock();
    pmtx->~Mutex();

    PutRetvalIn(myglobal)(GiveMeMutex)(35,36,37,38,39,310,31,32,33,34,316.0f,317.0f,318.0f,319.0f,320.0f,311.0f,312.0f,313.0f,314.0f,315.0f);
    myglobal->lock();
    myglobal->unlock();

    PutRetvalIn(myglobal)(f2)(95,96,97,98,99,910,91,92,93,94,916.0f,917.0f,918.0f,919.0f,920.0f,911.0f,912.0f,913.0f,914.0f,915.0f);
    myglobal->lock();
    myglobal->unlock();

    nrvo_PutRetvalIn(buf)(f1)(29,210,21,2,3,4,5,6,7,8,19.0f,20.0f,11.0f,12.0f,13.0f,14.0f,15.0f,16.0f,17.0f,18.0f);
    pmtx->lock();
    pmtx->unlock();
    pmtx->~Mutex();

    nrvo_PutRetvalIn(myglobal)(GiveMeLockedMutex)(9,10,1,2,3,4,5,6,7,8,19.0f,20.0f,11.0f,12.0f,13.0f,14.0f,15.0f,16.0f,17.0f,18.0f);

    try
    {
        // The next line will throw 'before fully constructed' because the first argument is 54321
        nrvo_PutRetvalIn(myglobal)(GiveMeLockedMutex)(54321,1,2,3,4,5,6,7,8,9,13.0f,14.0f,15.0f,16.0f,17.0f,18.0f,19.0f,20.0f,21.0f,22.0f);
    }
    catch(int)
    {
        std::puts("Caught an int");
    }

    try
    {
        // The next line will throw 'after constructed' because the first argument is 12345
        nrvo_PutRetvalIn(myglobal)(GiveMeLockedMutex)(12345,1,2,3,4,5,6,7,8,9,13.0f,14.0f,15.0f,16.0f,17.0f,18.0f,19.0f,20.0f,21.0f,22.0f);
    }
    catch(int)
    {
        std::puts("Caught an int");
    }

    std::puts("Last line in main");
}
