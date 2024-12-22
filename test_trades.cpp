#include <iostream>
#include "tradebookingservice.hpp"
#include "positionservice.hpp"
#include "riskservice.hpp"
#include "historicaldataservice.hpp"
#include "products.hpp"
#include "productservice.hpp"
#include "pricingservice.hpp"
//#include "connector.hpp"

int main()
{
    
    BondProductService* bondProductService = new BondProductService();
    
    bondProductService->Add(Bond("T2Y", CUSIP, "TICKER1", 0.02f, {2026, 12, 22}));
    bondProductService->Add(Bond("T3Y", CUSIP, "TICKER2", 0.025f, {2027, 6, 15}));
    bondProductService->Add(Bond("T5Y", CUSIP, "TICKER3", 0.03f, {2029, 9, 30}));
    bondProductService->Add(Bond("T7Y", CUSIP, "TICKER4", 0.035f, {2031, 3, 10}));
    bondProductService->Add(Bond("T10Y", CUSIP, "TICKER5", 0.04f, {2034, 1, 20}));
    bondProductService->Add(Bond("T20Y", CUSIP, "TICKER6", 0.045f, {2044, 7, 25}));
    bondProductService->Add(Bond("T30Y", CUSIP, "TICKER7", 0.05f, {2054, 5, 10}));


    BondTradeBookingService* bondTradeBookingService = new BondTradeBookingService();
    BondPositionService* bondPositionService = new BondPositionService();
    BondPricingService* pricingService = new BondPricingService();
    BondRiskService* bondRiskService = new BondRiskService(*pricingService);
    BondPositionHistoricalDataService* hist_pos = new BondPositionHistoricalDataService();
    BondRiskHistoricalDataService* hist_risk = new BondRiskHistoricalDataService();

    
    bondTradeBookingService->AddListener(bondPositionService);

    
    bondPositionService->AddListener(bondRiskService);

    
    bondPositionService->AddListener(hist_pos);
    bondRiskService->AddListener(hist_risk);

    BondPricingConnector* pricingConnector = new BondPricingConnector(
        pricingService, "prices.txt", bondProductService);
    pricingConnector->Subscribe();

    
    TradeBookingServiceConnector* tradeBookingConnector = new TradeBookingServiceConnector(
        bondTradeBookingService, *bondProductService);

    
    
    tradeBookingConnector->ReadFile("trades.txt");

    
    std::cout << "Finished processing trades. Check positions.txt and risk.txt for output." << std::endl;

    
    delete hist_pos;
    delete hist_risk;
    delete pricingService;
    //delete riskHistoricalConnector;
    delete tradeBookingConnector;
    delete bondProductService;
    delete bondRiskService;
    delete bondPositionService;
    delete bondTradeBookingService;

    return 0;
}
