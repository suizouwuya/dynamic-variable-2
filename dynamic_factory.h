#pragma once
#include <memory>
#include <mutex>
#include <map>

/**
 * @class DynamicFactory
 * @author Andrey Bezborodov
 * @brief Dynamic class factory. Create instance of class by it's string name.
 */
template <class Base>
class dynamic_factory {
public:
  /**
   * @class abstract_instantiator
   * @author Андрей
   * @date 06/05/16
   * @file upoco_dynamic_factory.h
   * @brief Abstract instantiator for dynamic factory
   */
  template <class BaseType>
  class abstract_instantiator {
  public:
    //! Constructor
    abstract_instantiator() {}

    //! Destructor
    virtual ~abstract_instantiator() {}

    //! Create instance of concrete subclass of Base
    virtual BaseType* CreateInstance() const = 0;
  private:
    abstract_instantiator(const abstract_instantiator&);
    abstract_instantiator& operator = (const abstract_instantiator&);
  };

  //! Instantiator class
  template <class C, class BaseType>
  class instantiator : public abstract_instantiator<BaseType> {
  public:
    //! Constructor
    instantiator() {}

    //! Destructor
    virtual ~instantiator() {}

    //! Create instance
    BaseType* CreateInstance() const override {
      return new C;
    }
  };

public:
  typedef abstract_instantiator<Base>  AbstractFactory;

  //! Constructor
  dynamic_factory() {}

  //! Destructor
  ~dynamic_factory() {
    // - Remove instantiators
    for (typename FactoryMap::iterator it = map_.begin(); it != map_.end(); ++it) {
      delete it->second;
    }
  }

  //! Create a new instance of the class with given name
  Base* CreateInstance(const std::string& class_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    // - Find class by name and create instance if exists, return null otherwise
    typename FactoryMap::const_iterator it = map_.find(class_name);
    if (it == map_.end()) return nullptr;
    else return it->second->CreateInstance();
  }

  //! Register class of specified type
  template <class ClassType>
  bool RegisterClass(const std::string& class_name) {
    return RegisterClass(class_name, new instantiator<ClassType, Base>);
  }

  //! Unregister class
  bool UnregisterClass(const std::string& class_name) {
    std::lock_guard<std::mutex> lock(mutex_);

    // - Find class by name
    typename FactoryMap::iterator it = map_.find(class_name);
    if (it == map_.end()) return false;

    // - Delete class instantiator
    delete it->second;
    map_.erase(it);
    return true;
  }

  //! Unregister all classes
  void UnregisterAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (typename FactoryMap::iterator it = map_.begin(); it != map_.end(); ++it) {
      delete it->second;
    }
    map_.clear();
  }

  //! Check is class registered
  bool HasClass(const std::string& class_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return (map_.find(class_name) != map_.end());
  }

private:
	dynamic_factory(const dynamic_factory&);
	dynamic_factory& operator = (const dynamic_factory&);

  //! Register class of specified type (impementation)
  bool RegisterClass(const std::string& class_name, AbstractFactory* factory_ptr) {
    std::lock_guard<std::mutex> lock(mutex_);

    // - Take ownership on instantiator
    std::auto_ptr<AbstractFactory> ptr(factory_ptr);

    // - Check is class name already registered
    typename FactoryMap::iterator it = map_.find(class_name);
    if (it != map_.end()) return false;

    // - Register class
    map_[class_name] = ptr.release();
    return true;
  }

  //! Factory map type
  typedef std::map<std::string, AbstractFactory*> FactoryMap;

  //! Map of registered classes
  FactoryMap          map_;

  //! Mutex for prevent concurent access ot map in MT environment
  mutable std::mutex  mutex_;
};

