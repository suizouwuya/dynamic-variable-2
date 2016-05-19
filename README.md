# dynamic
Dynamic variable type for C++ allow you to store and dynamically cast various types of data.
STL-based, requires c++11.
```c++
dynamic var = 10;

double num = var;
std::string str = var;

var = "string value";
...
```
# dynamic_factory
Create new instances of class by it's name.

```c++
//! Define base class
class BaseClass {};

//! Define any subclasses
class PluginOne : public BaseClass {}
class PluginTwo : public BaseClass {}

//! Create dynamic factory
dynamic_factory<BaseClass>  factory;

//! Register classes in factory
factory.RegisterClass<PluginOne>("plugin one");
factory.RegisterClass<PluginTwo>("plugin two");

//! Now you can create new instances of registered classes
BaseClass* a = factory.CreateInstance("plugin one");
BaseClass* b = factory.CreateInstance("plugin two");

```
