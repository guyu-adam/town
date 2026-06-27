#pragma once

#include <map>
#include <algorithm>
#include <string>
#include <vector>

namespace town {

struct MarketStall {
    std::string goodId;
    std::string label;
    double price{0.0};
    double previousPrice{0.0};
    double basePrice{0.0};
    double productionCost{0.0};
    double inventory{0.0};
    double demand{0.0};
    double unitsSoldToday{0.0};
    int visitorsToday{0};
    int positionIndex{0};
    int shortageTicks{0};
    int noTradeTicks{0};
    int bornDay{0};
    bool active{true};
};

struct MarketEvolutionEntry {
    int day{};
    std::string goodId;
    double price{};
    double supply{};
    double demand{};
    double profitSignal{};
    std::string note;
};

enum class OrderSide {
    Bid,
    Ask
};

struct Order {
    OrderSide side{OrderSide::Bid};
    long long orderId{0};
    long long sequence{0};
    int ownerId{-1};
    std::string goodId;
    double price{0.0};
    double quantity{0.0};
    int quality{3};
};

struct CommodityFill {
    long long buyOrderId{0};
    long long sellOrderId{0};
    int buyerId{-1};
    int sellerId{-1};
    std::string goodId;
    double price{0.0};
    double quantity{0.0};
    int quality{3};
    long long transactionId{0};
};

struct OrderBook {
    std::string goodId;
    std::vector<Order> bids;
    std::vector<Order> asks;
    std::vector<CommodityFill> fills;
    double lastTradePrice{0.0};
    double tradedQuantity{0.0};
    double tradedValue{0.0};
    double unmatchedBidQuantity{0.0};
    double unmatchedAskQuantity{0.0};
};

struct Marketplace {
    std::vector<MarketStall> stalls;
    std::vector<MarketEvolutionEntry> evolutionLog;
    std::map<std::string, OrderBook> orderBooks;
    int totalTrades{0};
    long long nextOrderId{1};
    long long nextSequence{1};
};

inline OrderBook& BookFor(Marketplace& marketplace, const std::string& goodId) {
    auto& book = marketplace.orderBooks[goodId];
    book.goodId = goodId;
    return book;
}

inline void BeginOrderBooks(Marketplace& marketplace) {
    for (auto& [_, book] : marketplace.orderBooks) {
        book.bids.clear();
        book.asks.clear();
        book.fills.clear();
        book.tradedQuantity = 0.0;
        book.tradedValue = 0.0;
        book.unmatchedBidQuantity = 0.0;
        book.unmatchedAskQuantity = 0.0;
    }
}

inline long long AddOrder(Marketplace& marketplace, OrderSide side, int ownerId, const std::string& goodId,
                          double price, double quantity, int quality = 3) {
    if (price <= 0.0 || quantity <= 0.0) {
        return 0;
    }
    auto& book = BookFor(marketplace, goodId);
    const long long orderId = marketplace.nextOrderId++;
    Order order{side, orderId, marketplace.nextSequence++, ownerId, goodId, price, quantity, std::clamp(quality, 1, 5)};
    if (side == OrderSide::Bid) {
        book.bids.push_back(order);
        book.unmatchedBidQuantity += quantity;
    } else {
        book.asks.push_back(order);
        book.unmatchedAskQuantity += quantity;
    }
    return orderId;
}

inline void RecordCommodityFill(Marketplace& marketplace, const std::string& goodId,
                                long long buyOrderId, long long sellOrderId,
                                int buyerId, int sellerId, double price, double quantity,
                                int quality, long long transactionId) {
    if (price <= 0.0 || quantity <= 0.0) {
        return;
    }
    auto& book = BookFor(marketplace, goodId);
    book.fills.push_back(CommodityFill{buyOrderId, sellOrderId, buyerId, sellerId, goodId, price, quantity, std::clamp(quality, 1, 5), transactionId});
    book.lastTradePrice = price;
    book.tradedQuantity += quantity;
    book.tradedValue += price * quantity;
    book.unmatchedBidQuantity = std::max(0.0, book.unmatchedBidQuantity - quantity);
    book.unmatchedAskQuantity = std::max(0.0, book.unmatchedAskQuantity - quantity);
    ++marketplace.totalTrades;
}

inline std::vector<CommodityFill> MatchOrderBook(OrderBook& book) {
    std::sort(book.bids.begin(), book.bids.end(), [](const Order& a, const Order& b) {
        if (a.price != b.price) return a.price > b.price;
        if (a.quality != b.quality) return a.quality > b.quality;
        return a.sequence < b.sequence;
    });
    std::sort(book.asks.begin(), book.asks.end(), [](const Order& a, const Order& b) {
        if (a.price != b.price) return a.price < b.price;
        if (a.quality != b.quality) return a.quality > b.quality;
        return a.sequence < b.sequence;
    });

    std::vector<CommodityFill> fills;
    size_t bidIndex = 0;
    size_t askIndex = 0;
    while (bidIndex < book.bids.size() && askIndex < book.asks.size()) {
        Order& bid = book.bids[bidIndex];
        Order& ask = book.asks[askIndex];
        if (bid.price + 0.000001 < ask.price) {
            break;
        }
        const double quantity = std::min(bid.quantity, ask.quantity);
        const double price = ask.sequence < bid.sequence ? ask.price : bid.price;
        fills.push_back(CommodityFill{bid.orderId, ask.orderId, bid.ownerId, ask.ownerId,
                                      book.goodId, price, quantity, ask.quality, 0});
        bid.quantity -= quantity;
        ask.quantity -= quantity;
        if (bid.quantity <= 0.000001) ++bidIndex;
        if (ask.quantity <= 0.000001) ++askIndex;
    }
    book.unmatchedBidQuantity = 0.0;
    book.unmatchedAskQuantity = 0.0;
    for (const auto& bid : book.bids) book.unmatchedBidQuantity += std::max(0.0, bid.quantity);
    for (const auto& ask : book.asks) book.unmatchedAskQuantity += std::max(0.0, ask.quantity);
    return fills;
}

inline void TrimEvolutionLog(Marketplace& marketplace) {
    if (marketplace.evolutionLog.size() > 1200) {
        marketplace.evolutionLog.erase(marketplace.evolutionLog.begin(), marketplace.evolutionLog.begin() + 240);
    }
}

inline void RecordEvolution(Marketplace& marketplace, int day, const std::string& id, double price,
                            double supply, double demand, double baseValue) {
    const double profitSignal = baseValue > 0.01 ? price / baseValue - 1.0 : 0.0;
    std::string note = "balanced";
    if (profitSignal > 0.28) {
        note = "high profit: more producers enter";
    } else if (profitSignal < -0.18) {
        note = "low margin: producers exit";
    } else if (demand > supply * 1.35) {
        note = "shortage";
    } else if (supply > demand * 1.8) {
        note = "surplus";
    }
    marketplace.evolutionLog.push_back(MarketEvolutionEntry{day, id, price, supply, demand, profitSignal, note});
    TrimEvolutionLog(marketplace);
}

} // namespace town

