/**
 * inquiryservice.hpp
 * Defines the data types and Service for customer inquiries.
 *
 * @author Breman Thuraisingham
 */
#ifndef INQUIRY_SERVICE_HPP
#define INQUIRY_SERVICE_HPP

#include "soa.hpp"
#include "tradebookingservice.hpp"
#include "products.hpp"
#include <iostream> 
#include <fstream>
#include <sstream>


// Various inqyury states

enum InquiryState { RECEIVED, QUOTED, DONE, REJECTED, CUSTOMER_REJECTED };



/**
 * Inquiry object modeling a customer inquiry from a client.
 * Type T is the product type.
 */
template<typename T>
class Inquiry
{

public:

  // ctor for an inquiry
  Inquiry(string _inquiryId, const T &_product, Side _side, long _quantity, double _price, InquiryState _state);

  // Get the inquiry ID
  const string& GetInquiryId() const;

  // Get the product
  const T& GetProduct() const;

  // Get the side on the inquiry
  Side GetSide() const;

  // Get the quantity that the client is inquiring for
  long GetQuantity() const;

  // Get the price that we have responded back with
  double GetPrice() const;

  // Get the current state on the inquiry
  InquiryState GetState() const;

  void SetState(InquiryState _state);
  void SetPrice(double _price);

private:
  string inquiryId;
  T product;
  Side side;
  long quantity;
  double price = 100;
  InquiryState state;

};

/**
 * Service for customer inquirry objects.
 * Keyed on inquiry identifier (NOTE: this is NOT a product identifier since each inquiry must be unique).
 * Type T is the product type.
 */
template<typename T>
class InquiryService : public Service<string,Inquiry <T> >
{

public:

  // Send a quote back to the client
  virtual void SendQuote(const string &inquiryId, double price) = 0;

  // Reject an inquiry from the client
  virtual void RejectInquiry(const string &inquiryId) = 0;

};

template<typename T>
Inquiry<T>::Inquiry(string _inquiryId, const T &_product, Side _side, long _quantity, double _price, InquiryState _state) :
  product(_product)
{
  inquiryId = _inquiryId;
  side = _side;
  quantity = _quantity;
  price = _price;
  state = _state;
}

template<typename T>
const string& Inquiry<T>::GetInquiryId() const
{
  return inquiryId;
}

template<typename T>
const T& Inquiry<T>::GetProduct() const
{
  return product;
}

template<typename T>
Side Inquiry<T>::GetSide() const
{
  return side;
}

template<typename T>
long Inquiry<T>::GetQuantity() const
{
  return quantity;
}

template<typename T>
double Inquiry<T>::GetPrice() const
{
  return price;
}

template<typename T>
InquiryState Inquiry<T>::GetState() const
{
  return state;
}

template<typename T>
void Inquiry<T>::SetState(InquiryState _state) 
{
  state = _state;
}
template<typename T>
void Inquiry<T>::SetPrice(double _price) 
{
  price = _price;
}


class BondInquiryService : public InquiryService<Bond>
{
private: 
  map<string, Inquiry<Bond>> inquiryMap;
  vector<ServiceListener<Inquiry<Bond>>*> listeners;
  //Connector<Inquiry<Bond>>* connector;

public:
  BondInquiryService();
  Inquiry<Bond>& GetData(string key) override;
  void OnMessage(Inquiry<Bond> &data) override;
  void AddListener(ServiceListener<Inquiry<Bond>>* listener) override;
  const vector<ServiceListener<Inquiry<Bond>>*>& GetListeners() const override;
  void SendQuote(const string& inquiryId, double price) override;
  void RejectInquiry(const string& inquiryId) override;

};

BondInquiryService::BondInquiryService()
{
  //connector = _connector;
}

inline Inquiry<Bond>& BondInquiryService::GetData(string key)
{
  return inquiryMap.at(key);
}

inline void BondInquiryService::OnMessage(Inquiry<Bond> &data)
{
  string inquiryId = data.GetInquiryId();
  auto it = inquiryMap.find(inquiryId);
  bool isNew = (it == inquiryMap.end());
  
  inquiryMap.emplace(inquiryId, data);
  
  if (data.GetState() == RECEIVED) {
    SendQuote(inquiryId, 100.0);
  }
  else if (data.GetState() == QUOTED) {
    data.SetState(DONE);

  }
  // }
  for (auto listener : listeners) {
    if (isNew) {
      listener->ProcessAdd(data);
    }
    else {
      listener->ProcessUpdate(data);
    }
  }
  
  
}

inline void BondInquiryService::AddListener(ServiceListener<Inquiry<Bond>>* listener)
{
  listeners.push_back(listener);
}

inline const vector<ServiceListener<Inquiry<Bond>>*>& BondInquiryService::GetListeners() const
{
  return listeners;
}

inline void BondInquiryService::SendQuote(const string& inquiryId, double price)
{
  auto& inquiry = inquiryMap.at(inquiryId);
  Inquiry<Bond> quotedInquiry(inquiryId, inquiry.GetProduct(), inquiry.GetSide(), inquiry.GetQuantity(), price, QUOTED);
  
  //connector->Publish(quotedInquiry);
  OnMessage(quotedInquiry);

  Inquiry<Bond> doneInquiry(inquiryId, inquiry.GetProduct(), inquiry.GetSide(), inquiry.GetQuantity(), price, DONE);
  OnMessage(doneInquiry);
}

inline void BondInquiryService::RejectInquiry(const string& inquiryId)
{
  auto& inquiry = inquiryMap.at(inquiryId);
  Inquiry<Bond> rejectedInquiry(inquiryId, inquiry.GetProduct(), inquiry.GetSide(), inquiry.GetQuantity(), inquiry.GetPrice(), REJECTED);
  
  //connector->Publish(rejectedInquiry);

  for (auto listener : listeners) {
    listener->ProcessUpdate(rejectedInquiry);
  }
}

class InquiryConnector : public Connector<Inquiry<Bond>>
{
private: 
  BondInquiryService *service;
  BondProductService *bondProductService;
  string filename;

public: 
  InquiryConnector(BondInquiryService *_service, BondProductService *_bondProductService, const string& filename) 
                  : service(_service), bondProductService(_bondProductService), filename(filename) {}
  void Publish(Inquiry<Bond> &inquiry) override;
  void Subscribe();

};

// InquiryConnector::InquiryConnector(BondInquiryService &_service, BondProductService &_bondProductService) 
//                   : service(_service), bondProductService(_bondProductService) {}

inline void InquiryConnector::Publish(Inquiry<Bond> &inquiry)
{
  service->OnMessage(inquiry);
}

inline void InquiryConnector::Subscribe()
{
  ifstream file("/Users/joshlevine/Desktop/tradingsystem/inquiries.txt");
  string line;

  if (!file.is_open()) {
    cerr << "Could not open file " << filename << endl;
    return;
  }

  while (getline(file, line)) {
    if (line.empty()) continue;
    istringstream ss(line);
    string inquiryId, productId, sideStr;
    long quantity;
    Side side;

    if (ss >> inquiryId >> productId >> sideStr >> quantity) {
      if (sideStr == "BUY") { side = BUY; }
      else { side = SELL; }

      Bond bond = bondProductService->GetData(productId);
      Inquiry<Bond> inquiry(inquiryId, bond, BUY, quantity, 0.0, RECEIVED);
      service->OnMessage(inquiry);
    }
    else {
      cerr << "Invalid line format in " << filename << ": " << line << endl;
    }
  }

  file.close();
}

#endif
