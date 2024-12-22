#include <iostream>
#include "marketdataservice.hpp"
#include "algoexecutionservice.hpp"
#include "executionservice.hpp"
#include "historicaldataservice.hpp"
#include "products.hpp"  
#include "productservice.hpp"
#include "tradebookingservice.hpp"

int main()
{
    

    BondMarketDataService* bondmd = new BondMarketDataService();

    BondAlgoExecutionService* algoExec = new BondAlgoExecutionService();
    

    BondExecutionService* bondExec = new BondExecutionService();
    BondTradeBookingService* tradeBook = new BondTradeBookingService();

    
    BondExecutionHistoricalDataService* hist_exec = new BondExecutionHistoricalDataService();


    bondmd->AddListener(algoExec);
    
    
    algoExec->AddListener(bondExec);
    
    
    bondExec->AddListener(hist_exec);

    bondExec->AddListener(tradeBook);
    BondPricingService* pricingService = new BondPricingService();
    
    
    BondProductService* bondProductService = new BondProductService(); 
    //BondProductService* bondProductService = new BondProductService();
        
        
        bondProductService->Add(Bond("T2Y", CUSIP, "TICKER1", 0.02f, {2024, 12, 22}));
        bondProductService->Add(Bond("T3Y", CUSIP, "TICKER2", 0.025f, {2025, 6, 15}));
        bondProductService->Add(Bond("T5Y", CUSIP, "TICKER3", 0.03f, {2027, 9, 30}));
        bondProductService->Add(Bond("T7Y", CUSIP, "TICKER3", 0.03f, {2027, 9, 30}));
        bondProductService->Add(Bond("T10Y", CUSIP, "TICKER4", 0.035f, {2030, 1, 20}));
        bondProductService->Add(Bond("T20Y", CUSIP, "TICKER3", 0.03f, {2027, 9, 30}));
        bondProductService->Add(Bond("T30Y", CUSIP, "TICKER5", 0.04f, {2050, 5, 10}));
    MarketDataConnector* marketConnector = new MarketDataConnector(bondmd, bondProductService, std::string("marketdata.txt"));
    BondPricingConnector* pricingConnector = new BondPricingConnector(
        pricingService, "prices.txt", bondProductService);
    pricingConnector->Subscribe();

    
    marketConnector->Subscribe();


    std::cout << "Finished processing market data. Check executions.txt for output." << std::endl;

    
    delete bondmd;
    delete bondProductService;
    delete algoExec;
    delete bondExec;
    delete tradeBook;
    delete hist_exec;

    
    return 0;
}
