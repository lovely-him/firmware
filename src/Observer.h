#pragma once

#include <Arduino.h>
#include <list>

template <class T> class Observable;

#if defined(EINK_RA01S_GP02)
// Non-template helpers — implemented in Observer.cpp where LOG_DEBUG is available.
// Avoids circular include: Observer.h ← DebugConfiguration.h ← … ← Observer.h
void _observableLogNotify(const char *name, unsigned count);
void _observableLogResult(const char *name, int idx, int result);
void _observableLogRegister(const char *name, unsigned count);
#endif // EINK_RA01S_GP02

/**
 * An observer which can be mixed in as a baseclass.  Implement onNotify as a method in your class.
 */
template <class T> class Observer
{
    std::list<Observable<T> *> observables;

  public:
    virtual ~Observer();

    /// Stop watching the observable
    void unobserve(Observable<T> *o);

    /// Start watching a specified observable
    void observe(Observable<T> *o);

  private:
    friend class Observable<T>;

  protected:
    /**
     * returns 0 if other observers should continue to be called
     * returns !0 if the observe calls should be aborted and this result code returned for notifyObservers
     **/
    virtual int onNotify(T arg) = 0;
};

/**
 * An observer that calls an arbitrary method
 */
template <class Callback, class T> class CallbackObserver : public Observer<T>
{
    typedef int (Callback::*ObserverCallback)(T arg);

    Callback *objPtr;
    ObserverCallback method;

  public:
    CallbackObserver(Callback *_objPtr, ObserverCallback _method) : objPtr(_objPtr), method(_method) {}

  protected:
    virtual int onNotify(T arg) override { return (objPtr->*method)(arg); }
};

/**
 * An observable class that will notify observers anytime notifyObservers is called.  Argument type T can be any type, but for
 * performance reasons a pointer or word sized object is recommended.
 */
template <class T> class Observable
{
    std::list<Observer<T> *> observers;
#if defined(EINK_RA01S_GP02)
    const char *_name = nullptr;
#endif // EINK_RA01S_GP02

  public:
#if defined(EINK_RA01S_GP02)
    Observable(const char *name = nullptr) : _name(name) {}
#endif // EINK_RA01S_GP02
    /**
     * Tell all observers about a change, observers can process arg as they wish
     *
     * returns !0 if an observer chose to abort processing by returning this code
     */
    int notifyObservers(T arg)
    {
#if defined(EINK_RA01S_GP02)
        _observableLogNotify(_name, (unsigned)observers.size());
        int idx = 0;
#endif // EINK_RA01S_GP02
        for (typename std::list<Observer<T> *>::const_iterator iterator = observers.begin(); iterator != observers.end();
             ++iterator) {
            int result = (*iterator)->onNotify(arg);
#if defined(EINK_RA01S_GP02)
            _observableLogResult(_name, idx, result);
            idx++;
#endif // EINK_RA01S_GP02
            if (result != 0)
                return result;
        }

        return 0;
    }

  private:
    friend class Observer<T>;

    // Not called directly, instead call observer.observe
#if !defined(EINK_RA01S_GP02)
    void addObserver(Observer<T> *o) { observers.push_back(o); }
#else
    void addObserver(Observer<T> *o)
    {
        observers.push_back(o);
        _observableLogRegister(_name, (unsigned)observers.size());
    }
#endif // EINK_RA01S_GP02

    void removeObserver(Observer<T> *o) { observers.remove(o); }
};

template <class T> Observer<T>::~Observer()
{
    for (typename std::list<Observable<T> *>::const_iterator iterator = observables.begin(); iterator != observables.end();
         ++iterator) {
        (*iterator)->removeObserver(this);
    }
    observables.clear();
}

template <class T> void Observer<T>::unobserve(Observable<T> *o)
{
    o->removeObserver(this);
    observables.remove(o);
}

template <class T> void Observer<T>::observe(Observable<T> *o)
{
    observables.push_back(o);
    o->addObserver(this);
}