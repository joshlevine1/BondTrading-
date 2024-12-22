#ifndef ALGO_EXECUTION_SERVICE_HPP
#define ALGO_EXECUTION_SERVICE_HPP

#include "soa.hpp"
#include "marketdataservice.hpp"
#include "executionservice.hpp"

// class AlgoExecution
// {
// private: 
//     ExecutionOrder<Bond> executionOrder;

// public: 
//     AlgoExecution(const ExecutionOrder<Bond>& _executionOrder)
//             : executionOrder(_executionOrder) {}
    
//     const ExecutionOrder<Bond>& GetExecutionOrder() const {return executionOrder;}
// };

class BondAlgoExecutionService : public ServiceListener<OrderBook<Bond>>, public Service<string, ExecutionOrder<Bond>>
{
private: 
    void AggressTopOfBook(const OrderBook<Bond>& orderBook, const string& productId);

    unordered_map<string, ExecutionOrder<Bond>> algoExecutionMap;
    vector<ServiceListener<ExecutionOrder<Bond>>*> listeners;
    
    bool lastAggressBid;

public: 
    BondAlgoExecutionService() : lastAggressBid(false)
    {
    }
    
    ExecutionOrder<Bond>& GetData(string key) override;
    void Execute(OrderBook<Bond>& data);
    void OnMessage(ExecutionOrder<Bond>& data) override;
    void AddListener(ServiceListener<ExecutionOrder<Bond>>* listener) override;
    const vector<ServiceListener<ExecutionOrder<Bond>>*>& GetListeners() const override;

    void ProcessAdd(OrderBook<Bond> &data) override;
    void ProcessRemove(OrderBook<Bond> &data) override;
    void ProcessUpdate(OrderBook<Bond> &data) override;

};




inline ExecutionOrder<Bond>& BondAlgoExecutionService::GetData(string key)
{
    auto it = algoExecutionMap.find(key);
    if (it != algoExecutionMap.end()) {
        return it->second;
    } else {
        throw std::runtime_error("AlgoExecution not found for key: " + key);
    }
}

inline void BondAlgoExecutionService::Execute(OrderBook<Bond>& data)
{
    string productId = data.GetProduct().GetProductId();
    const auto& bidStack = data.GetBidStack();
    const auto& offerStack = data.GetOfferStack();

    if (bidStack.empty() || offerStack.empty()) {
        return;
    }

    double bestBidPrice = bidStack.front().GetPrice();
    double bestOfferPrice = offerStack.front().GetPrice();

    double spread = bestOfferPrice - bestBidPrice;
    double minSpread = 1.0 / 128.0;
    if (spread <= minSpread + 1e-9) {
        AggressTopOfBook(data, productId);

    }
}

inline void BondAlgoExecutionService::AddListener(ServiceListener<ExecutionOrder<Bond>> *listener) 
{
  listeners.push_back(listener);
}

inline const vector<ServiceListener<ExecutionOrder<Bond>>*>& BondAlgoExecutionService::GetListeners() const 
{
  return listeners;
}

inline void BondAlgoExecutionService::AggressTopOfBook(const OrderBook<Bond>& orderBook, const string& productId)
{
    PricingSide aggressSide = lastAggressBid ? OFFER : BID;
    const auto& bidStack = orderBook.GetBidStack();
    const auto& offerStack = orderBook.GetOfferStack();

    if (bidStack.empty() || offerStack.empty()) {
        cerr << "empty bid or offer stack for " << productId << endl;
        return;
    }

    double executionPrice;
    double quantity;

    if (aggressSide == BID) {
        executionPrice = bidStack.front().GetPrice();
        quantity = bidStack.front().GetQuantity();
    }
    else { 
        executionPrice = offerStack.front().GetPrice();
        quantity = offerStack.front().GetQuantity();
    }

    string orderId = productId; // + "_A_"; + to_string(orderCounter++);
    double hiddenQuantity = 0L;
    string parentOrderId = "";
    PricingSide side = (aggressSide == BID) ? BID : OFFER;
    const Bond& product = orderBook.GetProduct();

    ExecutionOrder<Bond> executionOrder(
        product, 
        side, 
        orderId, 
        MARKET, 
        executionPrice, 
        quantity, 
        hiddenQuantity, 
        parentOrderId, 
        false
    );
    algoExecutionMap.emplace(productId, executionOrder);
    for (auto listener : listeners) {
        listener->ProcessAdd(executionOrder);
    }

    lastAggressBid = !lastAggressBid;
}

inline void BondAlgoExecutionService::ProcessAdd(OrderBook<Bond> &data) 
{
    Execute(data);
}

inline void BondAlgoExecutionService::ProcessRemove(OrderBook<Bond> &data) 
{
    string productId = data.GetProduct().GetProductId();
    auto it = algoExecutionMap.find(productId);
    if (it != algoExecutionMap.end()) {
        algoExecutionMap.erase(it);
    }
}

inline void BondAlgoExecutionService::ProcessUpdate(OrderBook<Bond> &data)
{
    Execute(data);
}

inline void BondAlgoExecutionService::OnMessage(ExecutionOrder<Bond>& data) {}

#endif
