/**
 * riskservice.hpp
 * Defines the data types and Service for fixed income risk.
 *
 * @author Breman Thuraisingham
 */
#ifndef RISK_SERVICE_HPP
#define RISK_SERVICE_HPP

#include "soa.hpp"
#include "positionservice.hpp"
#include "pricingservice.hpp"

/**
 * PV01 risk.
 * Type T is the product type.
 */
template<typename T>
class PV01
{

public:

  // ctor for a PV01 value
  PV01(const T &_product, double _pv01, long _quantity);

  // Get the product on this PV01 value
  const T& GetProduct() const;

  // Get the PV01 value
  double GetPV01() const;

  // Get the quantity that this risk value is associated with
  long GetQuantity() const;

private:
  T product;
  double pv01;
  long quantity;

};

/**
 * A bucket sector to bucket a group of securities.
 * We can then aggregate bucketed risk to this bucket.
 * Type T is the product type.
 */
template<typename T>
class BucketedSector
{

public:

  // ctor for a bucket sector
  BucketedSector(const vector<T> &_products, string _name);

  // Get the products associated with this bucket
  const vector<T>& GetProducts() const;

  // Get the name of the bucket
  const string& GetName() const;

private:
  vector<T> products;
  string name;

};

/**
 * Risk Service to vend out risk for a particular security and across a risk bucketed sector.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class RiskService : public Service<string,PV01 <T> >
{

public:

  // Add a position that the service will risk
  virtual void AddPosition(Position<T> &position) = 0;

  // Get the bucketed risk for the bucket sector
  virtual PV01< BucketedSector<T> > GetBucketedRisk(const BucketedSector<T> &sector) const = 0;

};

template<typename T>
PV01<T>::PV01(const T &_product, double _pv01, long _quantity) :
  product(_product)
{
  pv01 = _pv01;
  quantity = _quantity;
}

template<typename T>
const T& PV01<T>::GetProduct() const
{
  return product;
}

template<typename T>
double PV01<T>::GetPV01() const
{
  return pv01;
}

template<typename T>
long PV01<T>::GetQuantity() const
{
  return quantity;
}


template<typename T>
BucketedSector<T>::BucketedSector(const vector<T>& _products, string _name) :
  products(_products)
{
  name = _name;
}

template<typename T>
const vector<T>& BucketedSector<T>::GetProducts() const
{
  return products;
}

template<typename T>
const string& BucketedSector<T>::GetName() const
{
  return name;
}

class BondRiskService : public RiskService<Bond>, public ServiceListener<Position<Bond>>
{
private: 
  map<string, PV01<Bond>> riskMap;
  vector<ServiceListener<PV01<Bond>>*> listeners;
  PricingService<Bond> &pricingService;

public: 
  BondRiskService(BondPricingService &pricingService) : pricingService(pricingService) {}
  PV01<Bond>& GetData(string key) override;
  void OnMessage(PV01<Bond> &data) override;
  void AddListener(ServiceListener<PV01<Bond>> *listener) override;
  const vector<ServiceListener<PV01<Bond>>*>& GetListeners() const override;
  void AddPosition(Position<Bond> &position) override;
  PV01<BucketedSector<Bond>> GetBucketedRisk(const BucketedSector<Bond> &sector) const override;

  void ProcessAdd(Position<Bond> &data) override;
  void ProcessRemove(Position<Bond> &data) override;
  void ProcessUpdate(Position<Bond> &data) override;

};


inline PV01<Bond>& BondRiskService::GetData(string key) 
{
  auto it = riskMap.find(key);
  if (it != riskMap.end()) {
    return it->second;
  }
  throw runtime_error("Risk key not found: " + key);
}

inline void BondRiskService::OnMessage(PV01<Bond> &data) 
{
}

inline void BondRiskService::AddListener(ServiceListener<PV01<Bond>> *listener) 
{
  listeners.push_back(listener);
}

inline const vector<ServiceListener<PV01<Bond>>*>& BondRiskService::GetListeners() const 
{
  return listeners;
}

inline void BondRiskService::AddPosition(Position<Bond> &position) 
{
  string productId = position.GetProduct().GetProductId();

  Price<Bond> bondPrice = pricingService.GetData(productId);
  double midPrice = bondPrice.GetMid();

  double yield = (position.GetProduct()).ComputeYield(midPrice, 2);
  //cout << "computed yield for " << position.GetProduct().GetProductId() << ": " << yield << endl;

  double modifiedDuration = position.GetProduct().CalculateDuration(yield, position.GetProduct().GetFaceValue(), 2);
  //cout << "Modified Duration for " << position.GetProduct().GetProductId() << ": " << modifiedDuration << endl;
  //double modifiedDuration = 1;
  
  double pv01PerUnit = modifiedDuration * midPrice * 0.0001; // fix this value 
  
  double pv01Risk = pv01PerUnit * position.GetAggregatePosition();
  // cout << "Aggregate Position for " << position.GetProduct().GetProductId() << ": " << position.GetAggregatePosition() << endl;
  // cout << "PV01 Risk for " << position.GetProduct().GetProductId() << ": " << pv01Risk << endl;

  auto it = riskMap.find(productId);
  bool isNew = (it == riskMap.end());
  if (isNew) {
    PV01<Bond> pv01(position.GetProduct(), pv01Risk, position.GetAggregatePosition());
    it = riskMap.insert(make_pair(productId, pv01)).first;
  }
  else {
    it->second = PV01<Bond>(position.GetProduct(), pv01Risk, position.GetAggregatePosition());
    riskMap.emplace(productId, it->second);
  }

  // cout << "Storing in riskMap - product " << productId
  //     << ", pv01: " << pv01Risk 
  //     << ", Quantity: " << position.GetAggregatePosition() << endl;

  if (isNew) {
    for (auto listener : listeners) {
      listener->ProcessAdd(it->second);
    }
  }
  else {
    for (auto listener : listeners) {
      listener->ProcessUpdate(it->second);
    }
  }
}

inline PV01<BucketedSector<Bond>> BondRiskService::GetBucketedRisk(const BucketedSector<Bond> &sector) const 
{
  //static PV01<BucketedSector<Bond>> bucketedPV01(sector, 0.0, 0);
  double totalPV01 = 0.0;
  long totalQuantity = 0;

  for (const Bond &bond : sector.GetProducts()) {
    string productId = bond.GetProductId();
    auto it = riskMap.find(productId);
    if (it != riskMap.end()) {
      totalPV01 += it->second.GetPV01();
      totalQuantity += it->second.GetQuantity();
    }
  }

  //bucketedPV01 = PV01<BucketedSector<Bond>>(sector, totalPV01, totalQuantity);
  return PV01<BucketedSector<Bond>>(sector, totalPV01, totalQuantity);

}

inline void BondRiskService::ProcessAdd(Position<Bond> &data) 
{
  AddPosition(data);
}
inline void BondRiskService::ProcessRemove(Position<Bond> &data) 
{
  string productId = data.GetProduct().GetProductId();
  auto it = riskMap.find(productId);
  if (it != riskMap.end()) {
    Price<Bond> bondPrice = pricingService.GetData(productId);
    double midPrice = bondPrice.GetMid();

    double yield = data.GetProduct().ComputeYield(midPrice, 2);

    double modifiedDuration = data.GetProduct().CalculateDuration(yield, 1000, 2);
    //double modifiedDuration = 1;
    
    double pv01PerUnit = modifiedDuration * midPrice * 0.0001; // fix this value 

    double pv01Risk = pv01PerUnit * (-data.GetAggregatePosition());

    double updatedPV01 = it->second.GetPV01() + pv01Risk;
    long updatedQuant = it->second.GetQuantity() + (-data.GetAggregatePosition());
    
    if (updatedQuant == 0) {
      riskMap.erase(it);  
    }
    else {
      it->second = PV01<Bond>(data.GetProduct(), updatedPV01, updatedQuant);
      for (auto listener : listeners) {
      listener->ProcessUpdate(it->second);
    }

    
    }
    
  }
}
inline void BondRiskService::ProcessUpdate(Position<Bond> &data) 
{
  AddPosition(data);
}

#endif
