/**
 * positionservice.hpp
 * Defines the data types and Service for positions.
 *
 * @author Breman Thuraisingham
 */
#ifndef POSITION_SERVICE_HPP
#define POSITION_SERVICE_HPP

#include <string>
#include <map>
#include "soa.hpp"
#include "tradebookingservice.hpp"
#include "products.hpp"

using namespace std;

/**
 * Position class in a particular book.
 * Type T is the product type.
 */
template<typename T>
class Position
{

public:

  // ctor for a position
  Position(const T &_product);

  // Get the product
  const T& GetProduct() const;

  // Get the position quantity
  long& GetPosition(string &book);

  const std::map<std::string, long>& GetPositions() const;

  // Get the aggregate position
  long GetAggregatePosition() const;

private:
  T product;
  map<string,long> positions;

};

/**
 * Position Service to manage positions across multiple books and secruties.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class PositionService : public Service<string,Position <T> >
{

public:

  // Add a trade to the service
  virtual void AddTrade(const Trade<T> &trade) = 0;

};

template<typename T>
Position<T>::Position(const T &_product) :
  product(_product)
{
}

template<typename T>
const T& Position<T>::GetProduct() const
{
  return product;
}

template<typename T>
long& Position<T>::GetPosition(string &book)
{
  return positions[book];
}

template<typename T>
const std::map<std::string, long>& Position<T>::GetPositions() const {
    return positions;
}

template<typename T>
long Position<T>::GetAggregatePosition() const
{
  // No-op implementation - should be filled out for implementations
  long aggregate = 0;
  for (auto &kv : positions) {
    aggregate += kv.second;
  }
  return aggregate;
}

class BondPositionService : public PositionService<Bond>, public ServiceListener<Trade<Bond>>
{
private: 
  map<string, Position<Bond>> positionMap;
  vector<ServiceListener<Position<Bond>>*> listeners;

public:
  Position<Bond>& GetData(string key) override;
  void OnMessage(Position<Bond> &data) override;
  void AddListener(ServiceListener<Position<Bond>> *listener) override;
  const vector<ServiceListener<Position<Bond>>*>& GetListeners() const override;
  void AddTrade(const Trade<Bond> &trade) override;

  void ProcessAdd(Trade<Bond> &data) override;
  void ProcessRemove(Trade<Bond> &data) override;
  void ProcessUpdate(Trade<Bond> &data) override;
};


inline Position<Bond>& BondPositionService::GetData(string key) 
{
  auto it = positionMap.find(key);
  if (it != positionMap.end()) {
    return it->second;
  }
  throw runtime_error("Position key not found: " + key);
}

inline void BondPositionService::OnMessage(Position<Bond> &data)
{
}

inline void BondPositionService::AddListener(ServiceListener<Position<Bond>> *listener)
{
  listeners.push_back(listener);
}

inline const vector<ServiceListener<Position<Bond>>*>& BondPositionService::GetListeners() const 
{
  return listeners;
}

inline void BondPositionService::AddTrade(const Trade<Bond> &trade) 
{
  string productId = trade.GetProduct().GetProductId();
  auto it = positionMap.find(productId);
  bool isNew = (it == positionMap.end());
  if (isNew) {
    Position<Bond> pos(trade.GetProduct());
    positionMap.insert(make_pair(productId, pos));
    it = positionMap.find(productId);
  }

  Position<Bond> &pos = it->second;

  long quantity = trade.GetQuantity();
  if (trade.GetSide() == SELL) {
    quantity = -quantity;
  }

  string book = trade.GetBook();
  pos.GetPosition(book) += quantity;

  for (auto listener : listeners) {
    if (isNew) {
      listener->ProcessAdd(pos);
    }
    else {
      listener->ProcessUpdate(pos);
    }
  }

}

inline void BondPositionService::ProcessAdd(Trade<Bond> &data)
{
  AddTrade(data);
}

inline void BondPositionService::ProcessRemove(Trade<Bond> &data)
{
  string productId = data.GetProduct().GetProductId();
  auto it = positionMap.find(productId);
  if (it != positionMap.end()) {
    Position<Bond> &pos = it->second;
    long quantity = data.GetQuantity();
    if (data.GetSide() == BUY) {
      quantity = -quantity;
    }
    string book = data.GetBook();
    pos.GetPosition(book) += quantity;
    for (auto listener : listeners) {
      listener->ProcessUpdate(pos);
    }
  }
}

inline void BondPositionService::ProcessUpdate(Trade<Bond> &data)
{
  AddTrade(data);
}





#endif
