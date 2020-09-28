#ifndef __FACTORY_HH_
#define __FACTORY_HH_

#include <cstdint>
#include <cstdio>

#include <unordered_map>
#include <memory>
#include <utility>
#include <functional>
#include <typeindex>

template <typename B>
class Factory{

public:

  using UP = std::unique_ptr<B, std::function<void(B*)> >;
  using SP = std::shared_ptr<B>;
  using WP = std::weak_ptr<B>;
  using SPC = std::shared_ptr<const B>;

  template <typename ...ARGS>
  static typename Factory<B>::UP
  MakeUnique(const std::type_index& id, ARGS&& ...args);

  template <typename... ARGS>
  static std::unordered_map<std::type_index,
                            UP (*)(ARGS&&...)>&
  Instance();

  template <typename D, typename... ARGS>
  static std::uint64_t
  Register(const std::type_index& id);

private:
  template <typename D, typename... ARGS>
  static UP MakerFun(ARGS&& ...args){
    return UP(new D(std::forward<ARGS>(args)...), [](B *p) {delete p; });
  }
};


template <typename B>
template <typename ...ARGS>
typename Factory<B>::UP
Factory<B>::MakeUnique(const std::type_index& id, ARGS&& ...args){
  auto &ins = Instance<ARGS&&...>();
  std::remove_reference_t<decltype( ins.at(id) )> it;
  try{
    it = ins.at(id);
  }
  catch (const std::out_of_range& oor){
    std::fprintf(stderr, "Factory:: ERROR (%s), unable to find %s in %s\n", oor.what(), id.name(), (typeid(ins)).name() );
    return nullptr;
  }
  return (*it)(std::forward<ARGS>(args)...);
};


template <typename B>
template <typename... ARGS>
std::unordered_map<std::type_index,
                   typename Factory<B>::UP (*)(ARGS&&...)>&
Factory<B>::Instance(){
  static std::unordered_map<std::type_index,
                            typename Factory<B>::UP (*)(ARGS&&...)> m;
  static bool init = true;
  if(init){
    // std::cout<<"Instance a new Factory<"<<static_cast<const void *>(&m)<<">"<<std::endl;
    init=false;
  }
  return m;
};

template <typename B>
template <typename D, typename... ARGS>
std::uint64_t
Factory<B>::Register(const std::type_index& id){
  auto &ins = Instance<ARGS&&...>();
  ins[id] = &MakerFun<D, ARGS&&...>;
  return reinterpret_cast<std::uintptr_t>(&ins);
};

#endif
